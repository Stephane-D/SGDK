/************************************************************************//**
 * \brief MeGaWiFi API implementation.
 *
 * \author Jesus Alonso (doragasu)
 * \date 2015
 *
 * \note Module is not reentrant.
 *
 * \todo Missing a lot of integrity checks, also module should track used
 *       channels, and is not currently doing it
 ****************************************************************************/

#include "config.h"
#include "types.h"

#include "string.h"
#include "memory.h"
#include "task.h"

#if (MODULE_MEGAWIFI != 0)

#include "ext/mw/megawifi.h"

/// Remove compiler warnings when not using a function parameter
#define UNUSED_PARAM(x)		(void)x

#if !defined(MAX)
/// Returns the maximum of two numbers
#define MAX(a, b)	((a)>(b)?(a):(b))
#endif
#if !defined(MIN)
/// Returns the minimum of two numbers
#define MIN(a, b)	((a)<(b)?(a):(b))
#endif

// Should consider if console is PAL or NTSC
#define MS_TO_FRAMES(ms)	(((ms)*60/500 + 1)/2)

#define MW_COMMAND_TOUT		MS_TO_FRAMES(MW_COMMAND_TOUT_MS)
#define MW_CONNECT_TOUT		MS_TO_FRAMES(MW_CONNECT_TOUT_MS)
#define MW_SCAN_TOUT		MS_TO_FRAMES(MW_SCAN_TOUT_MS)
#define MW_ASSOC_TOUT		MS_TO_FRAMES(MW_ASSOC_TOUT_MS)
#define MW_ASSOC_WAIT_SLEEP	MS_TO_FRAMES(MW_ASSOC_WAIT_SLEEP_MS)
#define MW_STAT_POLL_TOUT	MS_TO_FRAMES(MW_STAT_POLL_MS)
#define MW_HTTP_OPEN_TOUT	MS_TO_FRAMES(MW_HTTP_OPEN_TOUT_MS)
#define MW_UPGRADE_TOUT		MS_TO_FRAMES(MW_UPGRADE_TOUT_MS)

/*
 * The module assumes that once started, sending always succeeds, but uses
 * timers (when defined) for data reception.
 */

enum cmd_stat {
	CMD_ERR_PROTO = -1,
	CMD_ERR_TIMEOUT = 0,
	CMD_OK = 1
};

struct recv_metadata {
	uint16_t len;
	uint8_t ch;
};

/// Data required by the module
struct mw_data {
	mw_cmd *cmd;
	// TODO This callback is never set!
	lsd_recv_cb cmd_data_cb;
	uint16_t buf_len;
	union {
		uint8_t flags;
		struct {
			uint8_t mw_ready:1;
		};
	};
} d = {};

static uint16_t concat_strings(const char **str, uint8_t num_strs,
		char *output, uint16_t max_len)
{
	uint16_t pos = 0;
	int str_len;

	for (uint8_t i = 0; i < num_strs; i++) {
		if (!str[i]) {
			return 0;
		}
		str_len = strlen(str[i]) + 1;
		if ((pos + str_len) > max_len) {
			return 0;
		}
		memcpy(output + pos, str[i], str_len);
		pos += str_len;
	}

	return pos;
}

static uint16_t concat_kv_pairs(const char **key, const char **value,
		uint8_t num_pairs, char *output, uint16_t max_len)
{
	uint16_t pos = 0;
	int key_len, value_len;

	for (uint8_t i = 0; i < num_pairs; i++) {
		if (!key[i] || !value[i]) {
			return 0;
		}
		key_len = strlen(key[i]) + 1;
		value_len = strlen(value[i]) + 1;
		if ((pos + key_len + value_len) > max_len) {
			return 0;
		}
		memcpy(output + pos, key[i], key_len);
		pos += key_len;
		memcpy(output + pos, value[i], value_len);
		pos += value_len;
	}

	return pos;
}

int16_t mw_init(uint16_t *cmd_buf, uint16_t buf_len)
{
	if (!cmd_buf || buf_len < MW_CMD_MIN_BUFLEN) {
		return MW_ERR_BUFFER_TOO_SHORT;
	}

	memset(&d, 0, sizeof(struct mw_data));
	d.cmd = (mw_cmd*)cmd_buf;
	d.buf_len = buf_len;

	lsd_init();

	// Keep WiFi module in reset
	mw_module_reset();
	// Power down and Program not active (required for the module to boot)
	uart_clr_bits(MCR, MW__PRG | MW__PD);

	// Try accessing UART scratch pad register to see if it is installed
	UART_SPR = 0x55;
	if (UART_SPR != 0x55) return MW_ERR;
	UART_SPR = 0xAA;
	if (UART_SPR != 0xAA) return MW_ERR;

	// Enable control channel
	lsd_ch_enable(MW_CTRL_CH);

	d.mw_ready = TRUE;

	return MW_ERR_NONE;
}

static void cmd_send_cb(enum lsd_status err, void *ctx)
{
	UNUSED_PARAM(ctx);
	UNUSED_PARAM(err);

	// FIXME: Treat errors!
	if (!err) {
		// TODO: Do we always want to force a context switch
		TSK_superPost(TRUE);
	}
}

static void cmd_recv_cb(enum lsd_status err, uint8_t ch,
		char *data, uint16_t len, void *ctx)
{
	UNUSED_PARAM(data);
	struct recv_metadata *md = (struct recv_metadata*)ctx;

	// FIXME: Treat errors!
	if (!err) {
		md->ch = ch;
		md->len = len;
		TSK_superPost(TRUE);
	}
}

static enum mw_err mw_command(int16_t timeout_frames)
{
	struct recv_metadata md;
	bool tout;
	bool done = FALSE;

	// Optimization: we do not wait until the command is sent
	mw_cmd_send(d.cmd, NULL, NULL);

	while (!done) {
		mw_cmd_recv(d.cmd, &md, cmd_recv_cb);
		tout = TSK_superPend(timeout_frames);
		if (tout) {
			return MW_ERR_RECV;
		}
		// We might receive network data while waiting
		// for a command reply
		if (MW_CTRL_CH == md.ch) {
			if (d.cmd->cmd != MW_CMD_OK) {
				return MW_ERR_RECV;
			}
			done = TRUE;
		} else if (d.cmd_data_cb) {
			d.cmd_data_cb(LSD_STAT_COMPLETE, md.ch,
					(char*)d.cmd, md.len, NULL);
		}
	}

	return MW_ERR_NONE;
}

static enum mw_err string_based_cmd(enum mw_command cmd, const char *payload,
		int16_t timeout_frames)
{
	enum mw_err err;
	int16_t len;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	if (!payload || !(len = strlen(payload))) {
		return MW_ERR_PARAM;
	}

	d.cmd->cmd = cmd;
	d.cmd->data_len = len + 1;
	memcpy(d.cmd->data, payload, len + 1);
	err = mw_command(timeout_frames);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

enum mw_err mw_recv_sync(uint8_t *ch, char *buf, int16_t *buf_len,
		int16_t tout_frames)
{
	struct recv_metadata md;
	bool tout;

	lsd_recv(buf, *buf_len, &md, cmd_recv_cb);
	tout = TSK_superPend(tout_frames);
	if (tout) {
		return MW_ERR_RECV;
	}

	*ch = md.ch;
	*buf_len = md.len;

	return MW_ERR_NONE;
}

enum mw_err mw_send_sync(uint8_t ch, const char *data, uint16_t len,
		int16_t tout_frames)
{
	uint16_t to_send;
	int16_t tout;
	uint16_t sent = 0;

	while (sent < len) {
		to_send = MIN(len - sent, d.buf_len);
		lsd_send(ch, data + sent, to_send, NULL, cmd_send_cb);
		tout = TSK_superPend(tout_frames);
		if (tout) {
			return MW_ERR_SEND;
		}
		sent += to_send;
	}

	return MW_ERR_NONE;
}

enum mw_err mw_detect(uint8_t *major, uint8_t *minor, char **variant)
{
	int16_t retries = 5;
	enum mw_err err;
	uint8_t version[3];

	// Wait a bit and take module out of resest
	TSK_superPend(MS_TO_FRAMES(30));
	mw_module_start();
	TSK_superPend(MS_TO_FRAMES(1000));

	do {
		retries--;
		uart_reset_fifos();
		err = mw_version_get(version, variant);
	} while (err != MW_ERR_NONE && retries);

	if (MW_ERR_NONE == err) {
		if (major) {
			*major = version[0];
		}
		if (minor) {
			*minor = version[1];
		}
	}

	return err;
}

enum mw_err mw_version_get(uint8_t version[3], char **variant)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_VERSION;
	d.cmd->data_len = 0;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return err;
	}
	if (version) {
		version[0] = d.cmd->data[0];
		version[1] = d.cmd->data[1];
		version[2] = d.cmd->data[2];
	}
	if (variant) {
		// Version string is NULL terminated
		*variant = (char*)(d.cmd->data + 3);
	}

	return MW_ERR_NONE;
}

enum mw_err mw_default_cfg_set(void)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_DEF_CFG_SET;
	d.cmd->data_len = 4;
	d.cmd->dw_data[0] = 0xFEAA5501;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

enum mw_err mw_ap_cfg_set(uint8_t slot, const char *ssid, const char *pass,
		 enum mw_phy_type phy_type)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!ssid) {
		// At least SSID is required.
		return MW_ERR_PARAM;
	}
	if (slot >= MW_NUM_CFG_SLOTS) {
		return MW_ERR_PARAM;
	}

	d.cmd->cmd = MW_CMD_AP_CFG;
	d.cmd->data_len = sizeof(struct mw_msg_ap_cfg);

	memset(&d.cmd->ap_cfg, 0, sizeof(struct mw_msg_ap_cfg));
	d.cmd->ap_cfg.cfg_num = slot;
	d.cmd->ap_cfg.phy_type = phy_type;
	// Note: *NOT* NULL terminated strings are allowed on cmd.ap_cfg.ssid
	// and cmd.ap_cfg.pass
	memcpy(d.cmd->ap_cfg.ssid, ssid, strnlen(ssid, MW_SSID_MAXLEN));
	if (pass) {
		memcpy(d.cmd->ap_cfg.pass, pass, strnlen(pass, MW_PASS_MAXLEN));
	}

	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

enum mw_err mw_ap_cfg_get(uint8_t slot, char **ssid, char **pass,
		enum mw_phy_type *phy_type)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (slot >= MW_NUM_CFG_SLOTS) {
		return MW_ERR_PARAM;
	}

	d.cmd->cmd = MW_CMD_AP_CFG_GET;
	d.cmd->data_len = 1;
	d.cmd->data[0] = slot;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	if (ssid) {
		*ssid = d.cmd->ap_cfg.ssid;
	}
	if (pass) {
		*pass = d.cmd->ap_cfg.pass;
	}
	if (phy_type) {
		*phy_type = d.cmd->ap_cfg.phy_type;
	}

	return MW_ERR_NONE;
}

enum mw_err mw_ip_cfg_set(uint8_t slot, const struct mw_ip_cfg *ip)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (slot >= MW_NUM_CFG_SLOTS) {
		return MW_ERR_PARAM;
	}

	d.cmd->cmd = MW_CMD_IP_CFG;
	d.cmd->data_len = sizeof(struct mw_msg_ip_cfg);
	d.cmd->ip_cfg.cfg_slot = slot;
	d.cmd->ip_cfg.reserved[0] = 0;
	d.cmd->ip_cfg.reserved[1] = 0;
	d.cmd->ip_cfg.reserved[2] = 0;
	d.cmd->ip_cfg.ip = *ip;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

enum mw_err mw_ip_cfg_get(uint8_t slot, struct mw_ip_cfg **ip)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_IP_CFG_GET;
	d.cmd->data_len = 1;
	d.cmd->data[0] = slot;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	*ip = &d.cmd->ip_cfg.ip;

	return MW_ERR_NONE;
}

enum mw_err mw_ip_current(struct mw_ip_cfg **ip)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_IP_CURRENT;
	d.cmd->data_len = 0;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	*ip = &d.cmd->ip_cfg.ip;

	return MW_ERR_NONE;
}

int16_t mw_ap_scan(enum mw_phy_type phy_type, char **ap_data, uint8_t *aps)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return -1;
	}

	d.cmd->cmd = MW_CMD_AP_SCAN;
	d.cmd->data[0] = phy_type;
	d.cmd->data_len = 1;
	err = mw_command(MW_SCAN_TOUT);
	if (err) {
		return -1;
	}

	// Fill number of APs and skip it for the apData array
	*aps = *d.cmd->data;
	*ap_data = ((char*)d.cmd->data) + 1;

	return d.cmd->data_len - 1;
}

int16_t mw_ap_fill_next(const char *ap_data, uint16_t pos,
		struct mw_ap_data *apd, uint16_t data_len)
{
	if (pos >= data_len) {
		return 0;	// End reached
	}
	if ((pos + ap_data[pos + 3] + 4) > data_len) {
		return -1;
	}
	apd->auth = ap_data[pos++];
	apd->channel = ap_data[pos++];
	apd->rssi = ap_data[pos++];
	apd->ssid_len = ap_data[pos++];
	apd->ssid = (char*)ap_data + pos;

	// Return updated position
	return pos + apd->ssid_len;
}

enum mw_err mw_ap_assoc(uint8_t slot)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_AP_JOIN;
	d.cmd->data_len = 1;
	d.cmd->data[0] = slot;
	err = mw_command(MW_ASSOC_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

enum mw_err mw_ap_assoc_wait(int16_t tout_frames)
{
	union mw_msg_sys_stat *stat;

	// Workaround: When the MW_CMD_AP_JOIN is run, module seems to freeze
	// communications for ~3 seconds. This can cause mw_sys_stat_get() to
	// timeout and this function to fail. Workaround this by sleeping
	// before trying to poll module for status
	mw_sleep(MW_ASSOC_WAIT_SLEEP);
	while (tout_frames > 0) {
		// FIXME: timing is not accurate because of the time this
		// command gets to complete
		stat = mw_sys_stat_get();
		if (!stat) {
			// Command failed
			return MW_ERR_NOT_READY;
		}
		if (stat->sys_stat >= MW_ST_READY) {
			return MW_ERR_NONE;
		}
		// Sleep for MW_STAT_POLL_TOUT frames
		TSK_superPend(MW_STAT_POLL_TOUT);
		tout_frames -= MW_STAT_POLL_TOUT;
	}

	return MW_ERR_NOT_READY;
}

enum mw_err mw_ap_disassoc(void)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_AP_LEAVE;
	d.cmd->data_len = 0;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

enum mw_err mw_def_ap_cfg(uint8_t slot)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->data_len = 1;
	d.cmd->cmd = MW_CMD_DEF_AP_CFG;
	d.cmd->data[0] = slot;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

int16_t mw_def_ap_cfg_get(void)
{
	enum mw_err err;

	d.cmd->data_len = 0;
	d.cmd->cmd = MW_CMD_DEF_AP_CFG_GET;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return -1;
	}

	return d.cmd->data[0];
}

static int16_t fill_addr(const char *dst_addr, const char *dst_port,
		const char *src_port, struct mw_msg_in_addr *in_addr)
{
	// Zero structure data
	memset(in_addr, 0, sizeof(struct mw_msg_in_addr));
	in_addr->dst_addr[0] = '\0';
	strcpy(in_addr->dst_port, dst_port);
	if (src_port) {
		strcpy(in_addr->src_port, src_port);
	}
	if (dst_addr && dst_port) {
		strcpy(in_addr->dst_addr, dst_addr);
		strcpy(in_addr->dst_port, dst_port);
	}

	// Length is the length of both ports, the channel and the address.
	return 6 + 6 + 1 + strlen(in_addr->dst_addr) + 1;
}

enum mw_err mw_tcp_connect(uint8_t ch, const char *dst_addr,
		const char *dst_port, const char *src_port)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (ch > MW_MAX_SOCK) {
		return MW_ERR_PARAM;
	}

	// Configure TCP socket
	d.cmd->cmd = MW_CMD_TCP_CON;
	d.cmd->data_len = fill_addr(dst_addr, dst_port, src_port,
			&d.cmd->in_addr);
	d.cmd->in_addr.channel = ch;
	err = mw_command(MW_CONNECT_TOUT);
	if (err) {
		return MW_ERR;
	}

	// Enable channel
	lsd_ch_enable(ch);

	return MW_ERR_NONE;
}

enum mw_err mw_close(uint8_t ch)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_CLOSE;
	d.cmd->data_len = 1;
	d.cmd->data[0] = ch;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	// Disable channel
	lsd_ch_disable(ch);

	return MW_ERR_NONE;
}

enum mw_err mw_tcp_bind(uint8_t ch, uint16_t port)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_TCP_BIND;
	d.cmd->data_len = 7;
	d.cmd->bind.reserved = 0;
	d.cmd->bind.port = port;
	d.cmd->bind.channel = ch;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	lsd_ch_enable(ch);

	return MW_ERR_NONE;
}

enum mw_err mw_udp_set(uint8_t ch, const char *dst_addr, const char *dst_port,
		const char *src_port)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (ch > MW_MAX_SOCK) {
		return MW_ERR_PARAM;
	}

	// Configure UDP socket
	d.cmd->cmd = MW_CMD_UDP_SET;
	d.cmd->data_len = fill_addr(dst_addr, dst_port, src_port,
			&d.cmd->in_addr);
	d.cmd->in_addr.channel = ch;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	// Enable channel
	lsd_ch_enable(ch);

	return MW_ERR_NONE;
}

enum mw_err mw_sock_conn_wait(uint8_t ch, int16_t tout_frames)
{
	enum mw_sock_stat stat;

	while (tout_frames > 0) {
		// FIXME: timing is not accurate because of the time this
		// command gets to complete
		stat = mw_sock_stat_get(ch);
		if (stat < 0) {
			// Command failed
			return MW_ERR_NOT_READY;
		}
		if (stat >= MW_SOCK_TCP_EST) {
			return MW_ERR_NONE;
		}
		// Sleep for MW_STAT_POLL_TOUT frames
		TSK_superPend(MW_STAT_POLL_TOUT);
		tout_frames -= MW_STAT_POLL_MS;
	}

	return MW_ERR_NOT_READY;
}

union mw_msg_sys_stat *mw_sys_stat_get(void)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return NULL;
	}

	d.cmd->cmd = MW_CMD_SYS_STAT;
	d.cmd->data_len = 0;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return NULL;
	}

	return &d.cmd->sys_stat;
}

enum mw_sock_stat mw_sock_stat_get(uint8_t ch)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return -1;
	}

	d.cmd->cmd = MW_CMD_SOCK_STAT;
	d.cmd->data_len = 1;
	d.cmd->data[0] = ch;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return -1;
	}

	return d.cmd->data[0];
}

// TODO Check for overflows when copying server data.
enum mw_err mw_sntp_cfg_set(const char *tz_str, const char *server[3])
{
	enum mw_err err;
	int16_t offset;
	int16_t len;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_SNTP_CFG;
	offset = 1 + strlen(tz_str);
	memcpy(d.cmd->data, tz_str, offset);

	for (int16_t i = 0; i < 3 && server[i] && *server[i]; i++) {
		len = 1 + strlen(server[i]);
		memcpy(&d.cmd->data[offset], server[i], len);
		offset += len;
	}
	d.cmd->data[offset++] = '\0';
	d.cmd->data_len = offset;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

static int16_t tokens_get(const char *in, char *token[], int16_t token_max)
{
	int16_t i;
	int16_t len;

	token[0] = (char*)in;
	for (i = 0; i < (token_max - 1) && *token[i]; i++) {
		len = strlen(token[i]);
		token[i + 1] = token[i] + len + 1;
	}

	if (*token[i]) {
		i++;
	}

	return i;
}

enum mw_err mw_sntp_cfg_get(char **tz_str, char *server[3])
{
	enum mw_err err;
	char *token[4] = {0};

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_SNTP_CFG_GET;
	d.cmd->data_len = 0;

	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	tokens_get((char*)d.cmd->data, token, 4);
	*tz_str = token[0];
	for (int16_t i = 0; i < 3; i++) {
		server[i] = token[i + 1];
	}

	return MW_ERR_NONE;
}

char *mw_date_time_get(uint32_t dt_bin[2])
{
	enum mw_err err;

	if (!d.mw_ready) {
		return NULL;
	}

	d.cmd->cmd = MW_CMD_DATETIME;
	d.cmd->data_len = 0;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return NULL;
	}

	if (dt_bin) {
		dt_bin[0] = d.cmd->date_time.dt_bin[0];
		dt_bin[1] = d.cmd->date_time.dt_bin[1];
	}
	// Set NULL termination of the string
	d.cmd->data[d.cmd->data_len] = '\0';

	return d.cmd->date_time.dt_str;
}

enum mw_err mw_flash_id_get(uint8_t *man_id, uint16_t *dev_id)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_FLASH_ID;
	d.cmd->data_len = 0;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	if (man_id) {
		*man_id = d.cmd->flash_id.manufacturer;
	}
	if (dev_id) {
		*dev_id = d.cmd->flash_id.device;
	}

	return MW_ERR_NONE;
}

// sect = 0 corresponds to flash sector 0x80
enum mw_err mw_flash_sector_erase(uint16_t sect)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_FLASH_ERASE;
	d.cmd->data_len = sizeof(uint16_t);
	d.cmd->fl_sect = sect;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

// Address 0 corresponds to flash address 0x80000
enum mw_err mw_flash_write(uint32_t addr, uint8_t *data, uint16_t data_len)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_FLASH_WRITE;
	d.cmd->data_len = data_len + sizeof(uint32_t);
	d.cmd->fl_data.addr = addr;
	memcpy(d.cmd->fl_data.data, data, data_len);
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

// Address 0 corresponds to flash address 0x80000
uint8_t *mw_flash_read(uint32_t addr, uint16_t data_len)
{
	enum mw_err err;

	if (!d.mw_ready || data_len > d.buf_len) {
		return NULL;
	}

	d.cmd->cmd = MW_CMD_FLASH_READ;
	d.cmd->fl_range.addr = addr;
	d.cmd->fl_range.len = data_len;
	d.cmd->data_len = sizeof(struct mw_msg_flash_range);
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return NULL;
	}

	return d.cmd->data;
}

uint8_t *mw_hrng_get(uint16_t rnd_len) {
	enum mw_err err;

	if (!d.mw_ready) {
		return NULL;
	}

	d.cmd->cmd = MW_CMD_HRNG_GET;
	d.cmd->data_len = sizeof(uint16_t);
	d.cmd->rnd_len = rnd_len;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return NULL;
	}

	return d.cmd->data;
}

uint8_t *mw_bssid_get(enum mw_if_type interface_type)
{
	enum mw_err err;

	if (!d.mw_ready || interface_type >= MW_IF_MAX) {
		return NULL;
	}

	d.cmd->cmd = MW_CMD_BSSID_GET;
	d.cmd->data_len = 1;
	d.cmd->data[0] = interface_type;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return NULL;
	}

	return d.cmd->data;
}

enum mw_err mw_gamertag_set(uint8_t slot, const struct mw_gamertag *gamertag)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_GAMERTAG_SET;
	d.cmd->gamertag_set.slot = slot;
	d.cmd->gamertag_set.reserved[0] = 0;
	d.cmd->gamertag_set.reserved[1] = 0;
	d.cmd->gamertag_set.reserved[2] = 0;
	d.cmd->data_len = sizeof(struct mw_gamertag_set_msg);
	memcpy(&d.cmd->gamertag_set.gamertag, gamertag,
			sizeof(struct mw_gamertag));
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

struct mw_gamertag *mw_gamertag_get(uint8_t slot)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return NULL;
	}

	d.cmd->cmd = MW_CMD_GAMERTAG_GET;
	d.cmd->data_len = 1;
	d.cmd->data[0] = slot;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return NULL;
	}

	return &d.cmd->gamertag_get;
}

enum mw_err mw_http_url_set(const char *url)
{
	return string_based_cmd(MW_CMD_HTTP_URL_SET, url, MW_COMMAND_TOUT);
}

enum mw_err mw_http_method_set(enum mw_http_method method)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	if (method >= MW_HTTP_METHOD_MAX) {
		return MW_ERR_PARAM;
	}

	d.cmd->cmd = MW_CMD_HTTP_METHOD_SET;
	d.cmd->data_len = 1;
	d.cmd->data[0] = method;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

enum mw_err mw_http_header_add(const char *key, const char *value)
{
	int16_t key_len;
	int16_t value_len;

	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	if (!key || !value || !(key_len = strlen(key)) ||
			!(value_len = strlen(value))) {
		return MW_ERR_PARAM;
	}

	key_len++;
	value_len++;
	d.cmd->cmd = MW_CMD_HTTP_HDR_ADD;
	d.cmd->data_len = key_len + value_len;
	memcpy(d.cmd->data, key, key_len);
	memcpy(d.cmd->data + key_len, value, value_len);
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

enum mw_err mw_http_header_del(const char *key)
{
	return string_based_cmd(MW_CMD_HTTP_HDR_DEL, key, MW_COMMAND_TOUT);
}

enum mw_err mw_http_open(uint32_t content_len)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_HTTP_OPEN;
	d.cmd->data_len = 4;
	d.cmd->dw_data[0] = content_len;
	err = mw_command(MW_HTTP_OPEN_TOUT);
	if (err) {
		return MW_ERR;
	}

	lsd_ch_enable(MW_HTTP_CH);
	return MW_ERR_NONE;
}

int16_t mw_http_finish(uint32_t *content_len, int16_t tout_frames)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	if (!content_len) {
		return MW_ERR_PARAM;
	}

	d.cmd->cmd = MW_CMD_HTTP_FINISH;
	d.cmd->data_len = 0;
	err = mw_command(tout_frames);
	if (err) {
		return MW_ERR;
	}

	*content_len = d.cmd->dw_data[0];
	return d.cmd->w_data[2];
}

uint32_t mw_http_cert_query(void)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return 0xFFFFFFFF;
	}

	d.cmd->cmd = MW_CMD_HTTP_CERT_QUERY;
	d.cmd->data_len = 0;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return 0xFFFFFFF;
	}

	return d.cmd->dw_data[0];
}

enum mw_err mw_http_cert_set(uint32_t cert_hash, const char *cert,
		uint16_t cert_len)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	if (cert_len && !cert) {
		return MW_ERR_PARAM;
	}
	d.cmd->cmd = MW_CMD_HTTP_CERT_SET;
	d.cmd->data_len = 6;
	d.cmd->dw_data[0] = cert_hash;
	d.cmd->w_data[2] = cert_len;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	lsd_ch_enable(MW_HTTP_CH);
	// Command succeeded, now send the certificate using MW_CH_HTTP
	err = mw_send_sync(MW_HTTP_CH, cert, cert_len, TSK_PEND_FOREVER);
	lsd_ch_disable(MW_HTTP_CH);

	return MW_ERR_NONE;
}

int16_t mw_http_cleanup(void)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_HTTP_FINISH;
	d.cmd->data_len = 0;
	err = mw_command(MW_COMMAND_TOUT);
	lsd_ch_disable(MW_HTTP_CH);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

char *mw_def_server_get(void)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return NULL;
	}

	d.cmd->cmd = MW_CMD_SERVER_URL_GET;
	d.cmd->data_len = 0;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return NULL;
	}

	return (char*)d.cmd->data;
}

enum mw_err mw_def_server_set(const char *server_url)
{
	return string_based_cmd(MW_CMD_SERVER_URL_SET, server_url,
			MW_COMMAND_TOUT);
}

enum mw_err mw_log(const char *msg)
{
	return string_based_cmd(MW_CMD_LOG, msg, MW_COMMAND_TOUT);
}

enum mw_err mw_factory_settings(void)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_FACTORY_RESET;
	d.cmd->data_len = 0;

	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

void mw_power_off(void)
{
	d.cmd->cmd = MW_CMD_SLEEP;
	d.cmd->data_len = 0;

	mw_cmd_send(d.cmd, NULL, NULL);
}

void mw_sleep(int16_t frames)
{
	TSK_superPend(frames);
}

enum mw_err mw_cfg_save(void)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_NV_CFG_SAVE;
	d.cmd->data_len = 0;

	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

struct mw_wifi_adv_cfg *mw_wifi_adv_cfg_get(void)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return NULL;
	}

	d.cmd->cmd = MW_CMD_WIFI_ADV_GET;
	d.cmd->data_len = 0;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return NULL;
	}

	return &d.cmd->wifi_adv_cfg;
}

enum mw_err mw_wifi_adv_cfg_set(const struct mw_wifi_adv_cfg *wifi)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_WIFI_ADV_SET;
	d.cmd->data_len = sizeof(struct mw_wifi_adv_cfg);
	d.cmd->wifi_adv_cfg = *wifi;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

enum mw_err mw_ga_endpoint_set(const char *endpoint, const char *priv_key)
{
	const char * strings[2] = {endpoint, priv_key};
	enum mw_err err;
	uint16_t pos;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	pos = concat_strings(strings, 2, (char*)d.cmd->data,
			MW_CMD_MAX_BUFLEN - 1);

	if (!pos) {
		return MW_ERR_PARAM;
	}

	d.cmd->data[pos++] = '\0';
	d.cmd->cmd = MW_CMD_GAME_ENDPOINT_SET;
	d.cmd->data_len = pos;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

enum mw_err mw_ga_key_value_add(const char **key, const char **value,
		uint16_t num_pairs)
{
	uint16_t pos;
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	pos = concat_kv_pairs(key, value, num_pairs, (char*)d.cmd->data,
			MW_CMD_MAX_BUFLEN - 1);
	if (!pos && num_pairs) {
		return MW_ERR_PARAM;
	}

	d.cmd->data[pos++] = '\0';
	d.cmd->cmd = MW_CMD_GAME_KEYVAL_ADD;
	d.cmd->data_len = pos;
	err = mw_command(MW_COMMAND_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

int16_t mw_ga_request(enum mw_http_method method, const char **path,
		uint8_t num_paths, const char **key, const char **value,
		uint8_t num_kv_pairs, uint32_t *content_len,
		int16_t tout_frames)
{
	enum mw_err err;
	uint16_t pos;
	uint16_t added;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	added = concat_strings(path, num_paths, d.cmd->ga_request.req,
			MW_CMD_MAX_BUFLEN - 4);
	if (!added) {
		return MW_ERR_PARAM;
	}

	pos = added;
	added = concat_kv_pairs(key, value, num_kv_pairs, d.cmd->ga_request.req + pos,
			MW_CMD_MAX_BUFLEN - 4 - pos);
	if (!added && num_kv_pairs) {
		return MW_ERR_PARAM;
	}
	pos += added;
	d.cmd->ga_request.req[pos++] = '\0';

	d.cmd->ga_request.method = method;
	d.cmd->ga_request.num_paths = num_paths;
	d.cmd->ga_request.num_kv_pairs = num_kv_pairs;
	d.cmd->cmd = MW_CMD_GAME_REQUEST;
	d.cmd->data_len = pos + 3;
	err = mw_command(tout_frames);
	if (err) {
		return MW_ERR;
	}

	lsd_ch_enable(MW_HTTP_CH);

	*content_len = d.cmd->dw_data[0];
	return d.cmd->w_data[2];
}

enum mw_err mw_fw_upgrade(const char *name)
{
	enum mw_err err;

	if (!d.mw_ready) {
		return MW_ERR_NOT_READY;
	}

	d.cmd->cmd = MW_CMD_UPGRADE_PERFORM;
	d.cmd->data_len = strlen(name) + 1;
	memcpy(d.cmd->data, name, d.cmd->data_len);
	err = mw_command(MW_UPGRADE_TOUT);
	if (err) {
		return MW_ERR;
	}

	return MW_ERR_NONE;
}

#endif // MODULE_MEGAWIFI