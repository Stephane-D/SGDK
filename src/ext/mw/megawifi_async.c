/************************************************************************//**
 * \brief MeGaWiFi ASYNC API implementation.
 *
 * \author Juan Antonio (PaCHoN)
 * \date 2026
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
#include "maths.h"
#include "task.h"

#if (MODULE_MEGAWIFI == 1)

#include "ext/mw/megawifi_async.h"
#include "timer.h"

typedef void (*mw_cmd_async_cb)(void);
typedef void (*mw_async_dispatch)(mw_cmd_async_cb callback, bool success);

static void command_async_recv_cb(enum lsd_status err, uint8_t ch,
		char *data, uint16_t len, void *ctx);
static enum mw_err command_async_start(int16_t timeout_frames);

/// Data required by the module
struct mw_async_data {

	mw_cmd_async_cb cmd_async_cb;
	mw_async_dispatch async_cmd;
	union {
		uint8_t channel;
		struct {
			const char *data;
			uint16_t len;
		} payload;
	} pending;
	uint32_t command_deadline;
	uint8_t command_pending:1;
} da = {};

void mw_process_async_timeout(void)
{
	if (da.command_pending &&
			(int32_t)(vtimer - da.command_deadline) >= 0) {
		command_async_recv_cb(LSD_STAT_ERROR, 0, NULL, 0, NULL);
	}
}

static void detect_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	mw_detect_async_cb detect_callback = (mw_detect_async_cb)callback;
	if (!success) {
		detect_callback(MW_ERR_RECV, NULL, NULL, NULL);
		return;
	}

	char *variant = (char*)(mw_get_data()->cmd->data + 3);
	detect_callback(MW_ERR_NONE, &mw_get_data()->cmd->data[0], &mw_get_data()->cmd->data[1], &variant);
}

static void result_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV);
}

static void ip_cfg_get_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_ip_cfg_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? &mw_get_data()->cmd->ip_cfg.ip : NULL);
}

static void ip_current_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_ip_current_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? &mw_get_data()->cmd->ip_cfg.ip : NULL);
}

static void sys_stat_get_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_sys_stat_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? &mw_get_data()->cmd->sys_stat : NULL);
}

static void bssid_get_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_bssid_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? mw_get_data()->cmd->data : NULL);
}

static void flash_read_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_flash_read_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? mw_get_data()->cmd->data : NULL);
}

static void hrng_get_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_hrng_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? mw_get_data()->cmd->data : NULL);
}

static void gamertag_get_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_gamertag_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? &mw_get_data()->cmd->gamertag_get : NULL);
}

static void def_server_get_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_def_server_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? (char*)mw_get_data()->cmd->data : NULL);
}

static void wifi_adv_cfg_get_async_dispatch(mw_cmd_async_cb callback,
		bool success)
{
	((mw_wifi_adv_cfg_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? &mw_get_data()->cmd->wifi_adv_cfg : NULL);
}

static void ap_cfg_get_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_ap_cfg_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? &mw_get_data()->cmd->ap_cfg : NULL);
}

static void sntp_cfg_get_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_sntp_cfg_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? &mw_get_data()->cmd->sntp_cfg : NULL);
}

static void date_time_get_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	if (success) {
		mw_get_data()->cmd->data[mw_get_data()->cmd->data_len] = '\0';
	}
	((mw_date_time_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? &mw_get_data()->cmd->date_time : NULL);
}

static void flash_id_get_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_flash_id_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? &mw_get_data()->cmd->flash_id : NULL);
}

static void ap_scan_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_ap_scan_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? mw_get_data()->cmd->data : NULL, success ? mw_get_data()->cmd->data_len : 0);
}

static void ping_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_ping_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? &mw_get_data()->cmd->ping_response : NULL);
}

static void fw_list_upgrades_async_dispatch(mw_cmd_async_cb callback,
		bool success)
{
	((mw_fw_list_upgrades_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? &mw_get_data()->cmd->ug_list_response : NULL);
}

static void http_finish_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_http_finish_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? mw_get_data()->cmd->data : NULL);
}

static void def_ap_cfg_get_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_def_ap_cfg_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? mw_get_data()->cmd->data[0] : 0xFFu);
}

static void sock_stat_get_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	((mw_sock_stat_get_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? mw_get_data()->cmd->data[0] : -1);
}

static void http_cert_query_async_dispatch(mw_cmd_async_cb callback,
		bool success)
{
	((mw_http_cert_query_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? mw_get_data()->cmd->dw_data[0] : 0xFFFFFFFFUL);
}

static void cert_send_async_cb(enum lsd_status status, void *ctx)
{
	mw_cmd_async_cb callback = da.cmd_async_cb;

	UNUSED_PARAM(ctx);
	da.cmd_async_cb = NULL;
	lsd_ch_disable(MW_HTTP_CH);
	if (callback) {
		result_async_dispatch(callback, status == LSD_STAT_COMPLETE);
	}
}

static void socket_open_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	if (success) {
		lsd_ch_enable(da.pending.channel);
	}
	result_async_dispatch(callback, success);
}

static void socket_close_async_dispatch(mw_cmd_async_cb callback,
		bool success)
{
	if (success) {
		lsd_ch_disable(da.pending.channel);
	}
	result_async_dispatch(callback, success);
}

static void http_open_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	if (success) {
		lsd_ch_enable(MW_HTTP_CH);
	}
	result_async_dispatch(callback, success);
}

static void http_cleanup_async_dispatch(mw_cmd_async_cb callback,
		bool success)
{
	lsd_ch_disable(MW_HTTP_CH);
	result_async_dispatch(callback, success);
}

static void http_cert_set_async_dispatch(mw_cmd_async_cb callback,
		bool success)
{
	enum lsd_status status;

	if (!success) {
		result_async_dispatch(callback, FALSE);
		return;
	}
	da.cmd_async_cb = callback;
	lsd_ch_enable(MW_HTTP_CH);
	status = lsd_send(MW_HTTP_CH, da.pending.payload.data,
			da.pending.payload.len, NULL, cert_send_async_cb);
	if (status != LSD_STAT_BUSY && status != LSD_STAT_COMPLETE) {
		cert_send_async_cb(LSD_STAT_ERROR, NULL);
	}
}

static void ga_request_async_dispatch(mw_cmd_async_cb callback, bool success)
{
	if (success) {
		lsd_ch_enable(MW_HTTP_CH);
	}
	((mw_ga_request_async_cb)callback)(success ? MW_ERR_NONE : MW_ERR_RECV,
			success ? mw_get_data()->cmd->data : NULL);
}

static void command_async_recv_cb(enum lsd_status err, uint8_t ch,
		char *data, uint16_t len, void *ctx)
{
	mw_cmd_async_cb callback = da.cmd_async_cb;
	mw_async_dispatch cmd = da.async_cmd;
	bool success;

	UNUSED_PARAM(data);
	UNUSED_PARAM(len);
	UNUSED_PARAM(ctx);
	da.command_pending = FALSE;

	if (!callback) {
		return;
	}

	success = err == LSD_STAT_COMPLETE && ch == MW_CTRL_CH &&
			mw_get_data()->cmd->cmd == MW_CMD_OK;
	da.cmd_async_cb = NULL;

	if (cmd) {
		cmd(callback, success);
	}
}

static enum mw_err command_async_start(int16_t timeout_frames)
{
	enum lsd_status status;

	status = mw_cmd_send(mw_get_data()->cmd, NULL, NULL);
	if (status != LSD_STAT_BUSY && status != LSD_STAT_COMPLETE) {
		return MW_ERR_SEND;
	}

	status = mw_cmd_recv(mw_get_data()->cmd, NULL, command_async_recv_cb);
	if (status != LSD_STAT_BUSY && status != LSD_STAT_COMPLETE) {
		return MW_ERR_RECV;
	}
	da.command_pending = TRUE;
	da.command_deadline = vtimer + timeout_frames;

	return MW_ERR_NONE;
}

static enum mw_err mw_command_async(enum mw_command cmd, mw_cmd_async_cb callback,
		int16_t timeout_frames, mw_async_dispatch async_cmd)
{
	enum mw_err err;

	if (da.cmd_async_cb) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->cmd = cmd;
	da.async_cmd = async_cmd;
	da.cmd_async_cb = callback;
	err = command_async_start(timeout_frames);
	if (err != MW_ERR_NONE) {
		da.cmd_async_cb = NULL;
	}

	return err;
}

static enum mw_err string_based_cmd_async(enum mw_command cmd,
		const char *payload, mw_async_cb callback, mw_async_dispatch async_cmd)
{
	uint16_t len;
	uint16_t capacity = min(MW_CMD_MAX_BUFLEN,
			mw_get_data()->buf_len - MW_CMD_HEADLEN);

	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!payload || !callback) {
		return MW_ERR_PARAM;
	}
	len = strnlen(payload, capacity);
	if (!len || len == capacity) {
		return MW_ERR_PARAM;
	}
	mw_get_data()->cmd->data_len = len + 1;
	memcpy(mw_get_data()->cmd->data, payload, len + 1);
	return mw_command_async(cmd, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, async_cmd);
}

// FUNCTIONS

enum mw_err mw_detect_async(mw_detect_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_VERSION, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, detect_async_dispatch);
}


enum mw_err mw_default_cfg_set_async(mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 4;
	mw_get_data()->cmd->dw_data[0] = 0xFEAA5501;
	return mw_command_async(MW_CMD_DEF_CFG_SET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_ap_cfg_set_async(uint8_t slot, const char *ssid,
		const char *pass, enum mw_phy_type phy_type, mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!ssid || !callback || slot >= MW_NUM_CFG_SLOTS) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = sizeof(struct mw_msg_ap_cfg);
	memset(&mw_get_data()->cmd->ap_cfg, 0, sizeof(struct mw_msg_ap_cfg));
	mw_get_data()->cmd->ap_cfg.cfg_num = slot;
	mw_get_data()->cmd->ap_cfg.phy_type = phy_type;
	memcpy(mw_get_data()->cmd->ap_cfg.ssid, ssid, strnlen(ssid, MW_SSID_MAXLEN));
	if (pass) {
		memcpy(mw_get_data()->cmd->ap_cfg.pass, pass, strnlen(pass, MW_PASS_MAXLEN));
	}

	return mw_command_async(MW_CMD_AP_CFG, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_ip_cfg_set_async(uint8_t slot, const struct mw_ip_cfg *ip,
		mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!ip || !callback || slot >= MW_NUM_CFG_SLOTS) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = sizeof(struct mw_msg_ip_cfg);
	mw_get_data()->cmd->ip_cfg.cfg_slot = slot;
	mw_get_data()->cmd->ip_cfg.reserved[0] = 0;
	mw_get_data()->cmd->ip_cfg.reserved[1] = 0;
	mw_get_data()->cmd->ip_cfg.reserved[2] = 0;
	mw_get_data()->cmd->ip_cfg.ip = *ip;
	return mw_command_async(MW_CMD_IP_CFG, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_ap_assoc_async(uint8_t slot, mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready || !callback || slot >= MW_NUM_CFG_SLOTS) {
		return !mw_get_data()->mw_ready ? MW_ERR_NOT_READY : MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 1;
	mw_get_data()->cmd->data[0] = slot;
	return mw_command_async(MW_CMD_AP_JOIN, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_ap_disassoc_async(mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_AP_LEAVE, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_def_ap_cfg_async(uint8_t slot, mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback || slot >= MW_NUM_CFG_SLOTS) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 1;
	mw_get_data()->cmd->data[0] = slot;
	return mw_command_async(MW_CMD_DEF_AP_CFG, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_sntp_cfg_set_async(const char *tz_str,
		const char *server[3], mw_async_cb callback)
{
	uint16_t offset;
	uint16_t capacity = min(MW_CMD_MAX_BUFLEN,
			mw_get_data()->buf_len - MW_CMD_HEADLEN);

	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!tz_str || !server || !callback) {
		return MW_ERR_PARAM;
	}

	offset = strnlen(tz_str, capacity - 1);
	if (offset == capacity - 1) {
		return MW_ERR_PARAM;
	}
	offset++;
	memcpy(mw_get_data()->cmd->data, tz_str, offset);
	for (uint16_t serverIndex = 0; serverIndex < 3 &&
			server[serverIndex] && *server[serverIndex]; serverIndex++) {
		uint16_t available = capacity - offset - 1;
		uint16_t len = strnlen(server[serverIndex], available);
		if (len == available) {
			return MW_ERR_PARAM;
		}
		len++;
		memcpy(&mw_get_data()->cmd->data[offset], server[serverIndex], len);
		offset += len;
	}
	mw_get_data()->cmd->data[offset++] = '\0';
	mw_get_data()->cmd->data_len = offset;
	return mw_command_async(MW_CMD_SNTP_CFG, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_flash_sector_erase_async(uint16_t sect, mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = sizeof(uint16_t);
	mw_get_data()->cmd->fl_sect = sect;
	return mw_command_async(MW_CMD_FLASH_ERASE, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_flash_write_async(uint32_t addr, uint8_t *data,
		uint16_t data_len, mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!data || !data_len || !callback ||
			data_len + sizeof(uint32_t) > mw_get_data()->buf_len) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = data_len + sizeof(uint32_t);
	mw_get_data()->cmd->fl_data.addr = addr;
	memcpy(mw_get_data()->cmd->fl_data.data, data, data_len);
	return mw_command_async(MW_CMD_FLASH_WRITE, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_gamertag_set_async(uint8_t slot,
		const struct mw_gamertag *gamertag, mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!gamertag || !callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->gamertag_set.slot = slot;
	mw_get_data()->cmd->gamertag_set.reserved[0] = 0;
	mw_get_data()->cmd->gamertag_set.reserved[1] = 0;
	mw_get_data()->cmd->gamertag_set.reserved[2] = 0;
	mw_get_data()->cmd->data_len = sizeof(struct mw_gamertag_set_msg);
	memcpy(&mw_get_data()->cmd->gamertag_set.gamertag, gamertag,
			sizeof(struct mw_gamertag));
	return mw_command_async(MW_CMD_GAMERTAG_SET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_http_url_set_async(const char *url, mw_async_cb callback)
{
	return string_based_cmd_async(MW_CMD_HTTP_URL_SET, url, callback, result_async_dispatch);
}

enum mw_err mw_http_method_set_async(enum mw_http_method method,
		mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback || method >= MW_HTTP_METHOD_MAX) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 1;
	mw_get_data()->cmd->data[0] = method;
	return mw_command_async(MW_CMD_HTTP_METHOD_SET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_http_header_add_async(const char *key, const char *value,
		mw_async_cb callback)
{
	uint16_t key_len;
	uint16_t value_len;
	uint16_t capacity = min(MW_CMD_MAX_BUFLEN,
			mw_get_data()->buf_len - MW_CMD_HEADLEN);

	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback || !key || !value) {
		return MW_ERR_PARAM;
	}
	key_len = strnlen(key, capacity - 1);
	if (!key_len || key_len == capacity - 1) {
		return MW_ERR_PARAM;
	}

	key_len++;
	value_len = strnlen(value, capacity - key_len);
	if (!value_len || value_len == capacity - key_len) {
		return MW_ERR_PARAM;
	}
	value_len++;
	mw_get_data()->cmd->data_len = key_len + value_len;
	memcpy(mw_get_data()->cmd->data, key, key_len);
	memcpy(mw_get_data()->cmd->data + key_len, value, value_len);
	return mw_command_async(MW_CMD_HTTP_HDR_ADD, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_http_header_del_async(const char *key, mw_async_cb callback)
{
	return string_based_cmd_async(MW_CMD_HTTP_HDR_DEL, key, callback, result_async_dispatch);
}

enum mw_err mw_def_server_set_async(const char *server_url,
		mw_async_cb callback)
{
	return string_based_cmd_async(MW_CMD_SERVER_URL_SET, server_url,
			callback, result_async_dispatch);
}

enum mw_err mw_log_async(const char *msg, mw_async_cb callback)
{
	return string_based_cmd_async(MW_CMD_LOG, msg, callback, result_async_dispatch);
}

enum mw_err mw_factory_settings_async(mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_FACTORY_RESET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_cfg_save_async(mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_NV_CFG_SAVE, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_wifi_adv_cfg_set_async(const struct mw_wifi_adv_cfg *wifi,
		mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!wifi || !callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = sizeof(struct mw_wifi_adv_cfg);
	mw_get_data()->cmd->wifi_adv_cfg = *wifi;
	return mw_command_async(MW_CMD_WIFI_ADV_SET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, wifi_adv_cfg_get_async_dispatch);
}

enum mw_err mw_ga_endpoint_set_async(const char *endpoint,
		const char *priv_key, mw_async_cb callback)
{
	const char *strings[2] = {endpoint, priv_key};
	uint16_t pos;

	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	pos = concat_strings(strings, 2, (char*)mw_get_data()->cmd->data,
			MW_CMD_MAX_BUFLEN - 1);
	if (!pos) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data[pos++] = '\0';
	mw_get_data()->cmd->data_len = pos;
	return mw_command_async(MW_CMD_GAME_ENDPOINT_SET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_ga_key_value_add_async(const char **key, const char **value,
		uint16_t num_pairs, mw_async_cb callback)
{
	uint16_t pos;

	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	pos = concat_kv_pairs(key, value, num_pairs, (char*)mw_get_data()->cmd->data,
			MW_CMD_MAX_BUFLEN - 1);
	if (!pos && num_pairs) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data[pos++] = '\0';
	mw_get_data()->cmd->data_len = pos;
	return mw_command_async(MW_CMD_GAME_KEYVAL_ADD, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, result_async_dispatch);
}

enum mw_err mw_fw_upgrade_async(const char *name, mw_async_cb callback)
{
	return string_based_cmd_async(MW_CMD_UPGRADE_PERFORM, name, callback, result_async_dispatch);
}

enum mw_err mw_tcp_connect_async(uint8_t ch, const char *dst_addr,
		const char *dst_port, const char *src_port, mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback || !dst_port || ch > MW_MAX_SOCK) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = fill_addr(dst_addr, dst_port, src_port,
			&mw_get_data()->cmd->in_addr);
	mw_get_data()->cmd->in_addr.channel = ch;
	da.pending.channel = ch;
	return mw_command_async(MW_CMD_TCP_CON, (mw_cmd_async_cb)callback, MW_CONNECT_TOUT, socket_open_async_dispatch);
}

enum mw_err mw_close_async(uint8_t ch, mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback || ch > MW_MAX_SOCK) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 1;
	mw_get_data()->cmd->data[0] = ch;
	da.pending.channel = ch;
	return mw_command_async(MW_CMD_CLOSE, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, socket_close_async_dispatch);
}

enum mw_err mw_tcp_bind_async(uint8_t ch, uint16_t port,
		mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback || ch > MW_MAX_SOCK) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 7;
	mw_get_data()->cmd->bind.reserved = 0;
	mw_get_data()->cmd->bind.port = port;
	mw_get_data()->cmd->bind.channel = ch;
	da.pending.channel = ch;
	return mw_command_async(MW_CMD_TCP_BIND, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, socket_open_async_dispatch);
}

enum mw_err mw_udp_set_async(uint8_t ch, const char *dst_addr,
		const char *dst_port, const char *src_port, mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback || !dst_port || ch > MW_MAX_SOCK) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = fill_addr(dst_addr, dst_port, src_port,
			&mw_get_data()->cmd->in_addr);
	mw_get_data()->cmd->in_addr.channel = ch;
	da.pending.channel = ch;
	return mw_command_async(MW_CMD_UDP_SET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, socket_open_async_dispatch);
}

enum mw_err mw_http_open_async(uint32_t content_len, mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 4;
	mw_get_data()->cmd->dw_data[0] = content_len;
	return mw_command_async(MW_CMD_HTTP_OPEN, (mw_cmd_async_cb)callback, MW_HTTP_OPEN_TOUT, http_open_async_dispatch);
}

enum mw_err mw_ip_cfg_get_async(uint8_t slot, mw_ip_cfg_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback || slot >= MW_NUM_CFG_SLOTS) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 1;
	mw_get_data()->cmd->data[0] = slot;
	return mw_command_async(MW_CMD_IP_CFG_GET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, ip_cfg_get_async_dispatch);
}

enum mw_err mw_ip_current_async(mw_ip_current_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_IP_CURRENT, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, ip_current_async_dispatch);
}

enum mw_err mw_sys_stat_get_async(mw_sys_stat_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_SYS_STAT, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, sys_stat_get_async_dispatch);
}

enum mw_err mw_bssid_get_async(enum mw_if_type interface_type,
		mw_bssid_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback || interface_type >= MW_IF_MAX) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 1;
	mw_get_data()->cmd->data[0] = interface_type;
	return mw_command_async(MW_CMD_BSSID_GET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, bssid_get_async_dispatch);
}

enum mw_err mw_flash_read_async(uint32_t addr, uint16_t data_len,
		mw_flash_read_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback || data_len > mw_get_data()->buf_len) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->fl_range.addr = addr;
	mw_get_data()->cmd->fl_range.len = data_len;
	mw_get_data()->cmd->data_len = sizeof(struct mw_msg_flash_range);
	return mw_command_async(MW_CMD_FLASH_READ, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, flash_read_async_dispatch);
}

enum mw_err mw_hrng_get_async(uint16_t rnd_len, mw_hrng_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = sizeof(uint16_t);
	mw_get_data()->cmd->rnd_len = rnd_len;
	return mw_command_async(MW_CMD_HRNG_GET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, hrng_get_async_dispatch	);
}

enum mw_err mw_gamertag_get_async(uint8_t slot,
		mw_gamertag_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 1;
	mw_get_data()->cmd->data[0] = slot;
	return mw_command_async(MW_CMD_GAMERTAG_GET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, gamertag_get_async_dispatch);
}

enum mw_err mw_def_server_get_async(mw_def_server_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_SERVER_URL_GET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, def_server_get_async_dispatch);
}

enum mw_err mw_wifi_adv_cfg_get_async(mw_wifi_adv_cfg_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback) {
		return MW_ERR_PARAM;
	}

	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_WIFI_ADV_GET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, wifi_adv_cfg_get_async_dispatch	);
}

enum mw_err mw_ap_cfg_get_async(uint8_t slot, mw_ap_cfg_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) {
		return MW_ERR_NOT_READY;
	}
	if (!callback || slot >= MW_NUM_CFG_SLOTS) {
		return MW_ERR_PARAM;
	}
	
	mw_get_data()->cmd->data_len = 1;
	mw_get_data()->cmd->data[0] = slot;
	return mw_command_async(MW_CMD_AP_CFG_GET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, ap_cfg_get_async_dispatch);
}

enum mw_err mw_sntp_cfg_get_async(mw_sntp_cfg_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!callback) return MW_ERR_PARAM;
	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_SNTP_CFG_GET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, sntp_cfg_get_async_dispatch);
}

enum mw_err mw_date_time_get_async(mw_date_time_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!callback) return MW_ERR_PARAM;
	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_DATETIME, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, date_time_get_async_dispatch);
}

enum mw_err mw_flash_id_get_async(mw_flash_id_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!callback) return MW_ERR_PARAM;
	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_FLASH_ID, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, flash_id_get_async_dispatch);
}

enum mw_err mw_ap_scan_async(enum mw_phy_type phy_type,
		mw_ap_scan_async_cb callback)
{
	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!callback) return MW_ERR_PARAM;
	mw_get_data()->cmd->data_len = 1;
	mw_get_data()->cmd->data[0] = phy_type;
	return mw_command_async(MW_CMD_AP_SCAN, (mw_cmd_async_cb)callback, MW_SCAN_TOUT, ap_scan_async_dispatch);
}

enum mw_err mw_def_ap_cfg_get_async(mw_def_ap_cfg_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!callback) return MW_ERR_PARAM;
	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_DEF_AP_CFG_GET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, def_ap_cfg_get_async_dispatch);
}

enum mw_err mw_sock_stat_get_async(uint8_t ch, mw_sock_stat_get_async_cb callback)
{
	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!callback || ch > MW_MAX_SOCK) return MW_ERR_PARAM;
	mw_get_data()->cmd->data_len = 1;
	mw_get_data()->cmd->data[0] = ch;
	return mw_command_async(MW_CMD_SOCK_STAT, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, sock_stat_get_async_dispatch);
}

enum mw_err mw_http_cert_query_async(mw_http_cert_query_async_cb callback)
{
	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!callback) return MW_ERR_PARAM;
	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_HTTP_CERT_QUERY, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, http_cert_query_async_dispatch);
}

enum mw_err mw_ping_async(const char *domain, u8 retries,
		mw_ping_async_cb callback)
{
	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!domain || !callback) return MW_ERR_PARAM;
	mw_get_data()->cmd->data_len = sizeof(struct mw_ping_request);
	strcpy(mw_get_data()->cmd->ping.domain, domain);
	mw_get_data()->cmd->ping.retries = retries;
	return mw_command_async(MW_CMD_PING, (mw_cmd_async_cb)callback, MW_PING_TOUT_MS, ping_async_dispatch);
}

enum mw_err mw_fw_list_upgrades_async(uint8_t page, uint8_t size,
		uint8_t offset, mw_fw_list_upgrades_async_cb callback)
{
	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!callback) return MW_ERR_PARAM;
	mw_get_data()->cmd->data[0] = page;
	mw_get_data()->cmd->data[1] = size;
	mw_get_data()->cmd->data[2] = offset;
	mw_get_data()->cmd->data_len = 3;
	return mw_command_async(MW_CMD_UPGRADE_LIST, (mw_cmd_async_cb)callback, MW_UPGRADE_TOUT, fw_list_upgrades_async_dispatch);
}

enum mw_err mw_http_finish_async(mw_http_finish_async_cb callback)
{
	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!callback) return MW_ERR_PARAM;
	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_HTTP_FINISH, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, http_finish_async_dispatch);
}

enum mw_err mw_ga_request_async(enum mw_http_method method,
		const char **path, uint8_t num_paths, const char **key,
		const char **value, uint8_t num_kv_pairs,
		mw_ga_request_async_cb callback)
{
	uint16_t pos;
	uint16_t added;

	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!callback) return MW_ERR_PARAM;
	added = concat_strings(path, num_paths, mw_get_data()->cmd->ga_request.req,
			MW_CMD_MAX_BUFLEN - 4);
	if (!added) return MW_ERR_PARAM;
	pos = added;
	added = concat_kv_pairs(key, value, num_kv_pairs,
			mw_get_data()->cmd->ga_request.req + pos, MW_CMD_MAX_BUFLEN - 4 - pos);
	if (!added && num_kv_pairs) return MW_ERR_PARAM;
	pos += added;
	mw_get_data()->cmd->ga_request.req[pos++] = '\0';
	mw_get_data()->cmd->ga_request.method = method;
	mw_get_data()->cmd->ga_request.num_paths = num_paths;
	mw_get_data()->cmd->ga_request.num_kv_pairs = num_kv_pairs;
	mw_get_data()->cmd->data_len = pos + 3;
	return mw_command_async(MW_CMD_GAME_REQUEST, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, ga_request_async_dispatch);
}

enum mw_err mw_http_cleanup_async(mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!callback) return MW_ERR_PARAM;
	mw_get_data()->cmd->data_len = 0;
	return mw_command_async(MW_CMD_HTTP_CLEANUP, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, http_cleanup_async_dispatch);
}

enum mw_err mw_http_cert_set_async(uint32_t cert_hash, const char *cert,
		uint16_t cert_len, mw_async_cb callback)
{
	if (!mw_get_data()->mw_ready) return MW_ERR_NOT_READY;
	if (!callback || (cert_len && !cert)) return MW_ERR_PARAM;
	mw_get_data()->cmd->data_len = 6;
	mw_get_data()->cmd->dw_data[0] = cert_hash;
	mw_get_data()->cmd->w_data[2] = cert_len;
	da.pending.payload.data = cert;
	da.pending.payload.len = cert_len;
	return mw_command_async(MW_CMD_HTTP_CERT_SET, (mw_cmd_async_cb)callback, MW_COMMAND_TOUT, http_cert_set_async_dispatch);
}
#endif // MODULE_MEGAWIFI
