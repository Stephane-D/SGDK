/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#pragma once

#include <esp_wifi.h>

/// Maximum SSID length (including '\0').
#define MW_SSID_MAXLEN		32
/// Maximum password length (including '\0').
#define MW_PASS_MAXLEN		64

#define MW_FACT_RESET_MAGIC	0xFEAA5501	

/// Gamertag nickname maximum length
#define MW_GT_NICKNAME_MAX		32
/// Gamertag security maximum length
#define MW_GT_SECURITY_MAX		32
/// Gamertag tagline maximum length
#define MW_GT_TAGLINE_MAX		32
/// Gamertag avatar graphick width in pixels
#define MW_GT_AVATAR_WIDTH		32
/// Gamertag avatar graphick height in pixels
#define MW_GT_AVATAR_HEIGHT		48
/// Telegram token maximum length
#define MW_GT_TG_TOKEN_MAX		64
/// Control channel used for command interpreter
#define MW_CTRL_CH			0
/// Channel used for HTTP requests and cert sets
#define MW_HTTP_CH			LSD_MAX_CH - 1
/// Maximum length of the default server
#define MW_SERVER_DEFAULT_MAXLEN	64

/** \addtogroup MwApi MwEvent Events parsed by the system FSM.
 *  \{ */
typedef enum {
	MW_EV_NONE = 0,		///< No event.
	MW_EV_INIT_DONE,	///< Initialization complete.
	MW_EV_NET,		///< WiFi events.
	MW_EV_SER_RX,		///< Data reception from serial line.
	MW_EV_MAX		///< Number of total events.
} MwEvent;
/** \} */

/// Maximum buffer length (bytes)
#define MW_MSG_MAX_BUFLEN	CONFIG_TCP_MSS

#define MW_CMD_MAX_BUFLEN	(MW_MSG_MAX_BUFLEN - 4)

/** \addtogroup MwApi MwState Possible states of the system state machine.
 *  \{ */
typedef enum {
	MW_ST_INIT = 0,		///< Initialization state.
	MW_ST_IDLE,			///< Idle state, until connected to an AP.
	MW_ST_AP_JOIN,		///< Trying to join an access point.
	MW_ST_RESERVED,
	MW_ST_READY,		///< Connected to The Internet.
	MW_ST_TRANSPARENT,	///< Transparent communication state.
	MW_ST_MAX			///< Limit number for state machine.
} MwState;
/** \} */

/// TCP/UDP address message
typedef struct {
	char dst_port[6];
	char src_port[6];
	uint8_t channel;
	char data[MW_CMD_MAX_BUFLEN - 6 - 6 - 1];
} MwMsgInAddr;

/// AP configuration message
typedef struct {
	uint8_t cfgNum;
	uint8_t phy_type;
	char ssid[MW_SSID_MAXLEN];
	char pass[MW_PASS_MAXLEN];
} MwMsgApCfg;

/// IP configuration message
typedef struct {
	uint8_t cfgNum;
	uint8_t reserved[3];
	tcpip_adapter_ip_info_t cfg;
	ip_addr_t dns1;
	ip_addr_t dns2;
} MwMsgIpCfg;

/// Date and time message
typedef struct {
	uint32_t dtBin[2];
	char dtStr[MW_CMD_MAX_BUFLEN - sizeof(uint64_t)];
} MwMsgDateTime;

typedef struct {
	uint32_t addr;
	uint8_t data[MW_CMD_MAX_BUFLEN - sizeof(uint32_t)];
} MwMsgFlashData;

typedef struct {
	uint32_t addr;
	uint16_t len;
} MwMsgFlashRange;

typedef struct {
	uint32_t reserved;
	uint16_t port;
	uint8_t  channel;
} MwMsgBind;

/// Gamertag data
struct mw_gamertag {
	/// Unique gamertag id
	int id;
	/// User nickname
	char nickname[MW_GT_NICKNAME_MAX];
	/// User security string
	char security[MW_GT_SECURITY_MAX];
	/// User defined text tag
	char tagline[MW_GT_TAGLINE_MAX];
	/// Telegram token
	char tg_token[MW_GT_TG_TOKEN_MAX];
	/// Avatar image tiles
	uint8_t avatar_tiles[MW_GT_AVATAR_WIDTH * MW_GT_AVATAR_HEIGHT / 2];
	/// Avatar image palette
	uint8_t avatar_pal[32];
};

/// Gamertag set message data
struct mw_gamertag_set_msg {
	uint8_t slot;			///< Slot to store gamertag (0 to 2)
	uint8_t reserved[3];		///< Reserved, set to 0
	struct mw_gamertag gamertag;	///< Gamertag to set
};

/// Advanced WiFi configuration. Handle with care!
struct mw_wifi_adv_cfg {
	uint8_t qos_enable;			///< WiFi QOS feature enable flag
	uint8_t ampdu_rx_enable;		///< WiFi AMPDU RX feature enable flag
	uint8_t rx_ba_win;			///< WiFi Block Ack RX window size
	uint8_t rx_ampdu_buf_num;		///< WiFi AMPDU RX buffer number
	uint32_t rx_ampdu_buf_len;		///< WiFi AMPDU RX buffer length
	uint32_t rx_max_single_pkt_len;		///< WiFi RX max single packet size
	uint32_t rx_buf_len;			///< WiFi RX buffer size
	uint8_t amsdu_rx_enable;		///< WiFi AMSDU RX feature enable flag
	uint8_t rx_buf_num;			///< WiFi RX buffer number
	uint8_t rx_pkt_num;			///< WiFi RX packet number
	uint8_t left_continuous_rx_buf_num;	///< WiFi Rx left continuous rx buffer number
	uint8_t tx_buf_num;			///< WiFi TX buffer number
	uint8_t reserved[3];			///< Unused, set to 0
};

/// Flash chip identifiers
struct mw_flash_id {
	uint16_t device;
	uint8_t manufacturer;
};

/// Game API request
struct mw_ga_request {
	uint8_t method;		///< Request method
	uint8_t num_paths;	///< Number of paths
	uint8_t num_kv_pairs;	///< Number of key/value pairs
	char req[];		///< Request data
};

/** \addtogroup MwApi MwSockStat Socket status.
 *  \{ */
typedef enum {
	MW_SOCK_NONE = 0,	///< Unused socket.
	MW_SOCK_TCP_LISTEN,	///< Socket bound and listening.
	MW_SOCK_TCP_EST,	///< TCP socket, connection established.
	MW_SOCK_UDP_READY	///< UDP socket ready for sending/receiving
} MwSockStat;
/** \} */

/** \addtogroup MwApi MwMsgSysStat System status
 *  \{ */
typedef union {
	uint32_t st_flags;
	struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		MwState sys_stat:8;	///< System status
		uint8_t online:1;	///< Module is connected to the Internet
		uint8_t cfg_ok:1;	///< Configuration OK
		uint8_t dt_ok:1;	///< Date and time synchronized at least once
		uint8_t cfg:2;		///< Default network configuration
		uint16_t reserved:3;	///< Reserved flags
		uint16_t ch_ev:16;	///< Channel flags with the pending event
#else
		uint16_t ch_ev:16;	///< Channel flags with the pending event
		uint16_t reserved:3;	///< Reserved flags
		uint8_t cfg:2;		///< Default network configuration
		uint8_t dt_ok:1;	///< Date and time synchronized at least once
		uint8_t cfg_ok:1;	///< Configuration OK
		uint8_t online:1;	///< Module is connected to the Internet
		MwState sys_stat:8;	///< System status
#endif
	};
} MwMsgSysStat;
/** \} */

typedef struct {
	uint8_t finish;
	uint8_t ok;
	uint8_t fail;
	char domain[MW_SERVER_DEFAULT_MAXLEN];
}MwMsgPingStat;

enum mw_http_req_type {
		MW_HTTP_NONE = 0,
		MW_HTTP_GET,
		MW_HTTP_HEAD,
		MW_HTTP_POST,
		MW_HTTP_PUT,
		MW_HTTP_DELETE,
		MW_HTTP_MAX
} __attribute__((packed));

typedef struct {
	char dst_port[6];
	char src_port[6];
	uint32_t payload_len;
	enum mw_http_req_type req_type;
	// both address and URI, MW_CMD_MAX_BUFLEN - 6 - 6 - 4 - 1
	char addr_plus_uri[];
} MwMsgHttpReq;

typedef struct {
	uint8_t retries;
	char domain[MW_SERVER_DEFAULT_MAXLEN];
} MwMsgPing;

/** \} *//** \addtogroup MwApi MwCmd Command sent to system FSM
 *  \{ */
typedef struct {
	uint16_t cmd;		///< Command code
	uint16_t datalen;	///< Data length
	// If datalen is nonzero, additional command data goes here until
	// filling datalen bytes.
	union {
		uint8_t ch;		// Channel number for channel related requests
		uint8_t data[MW_CMD_MAX_BUFLEN];
		uint16_t wData[MW_CMD_MAX_BUFLEN / sizeof(uint16_t)];
		uint32_t dwData[MW_CMD_MAX_BUFLEN / sizeof(uint32_t)];
		MwMsgInAddr inAddr;
		MwMsgApCfg apCfg;
		MwMsgIpCfg ipCfg;
		MwMsgDateTime datetime;
		MwMsgFlashData flData;
		MwMsgFlashRange flRange;
		MwMsgBind bind;
		MwMsgSysStat sysStat;
		MwMsgPing ping;
		struct mw_gamertag_set_msg gamertag_set;///< Gamertag set
		struct mw_gamertag gamertag_get;	///< Gamertag get
		struct mw_wifi_adv_cfg wifi_adv_cfg;
		struct mw_flash_id flash_id;
		struct mw_ga_request ga_request;	///< Game API request
		uint16_t flSect;	// Flash sector
		uint32_t flId;		// Flash IDs
		uint16_t rndLen;	// Length of the random buffer to fill
	};
} MwCmd;
/** \} */

typedef struct {
	union {
		uint8_t data[MW_MSG_MAX_BUFLEN];	///< Buffer raw data
		MwCmd cmd;							///< Command
	};
	uint16_t len;							///< Length of buffer contents
	uint8_t ch;								///< Channel associated with buffer
} MwMsgBuf;

/** \addtogroup MwApi MwFsmMsg Message parsed by the FSM
 *  \{ */
typedef struct {
	MwEvent e;			///< Message event.
	void *d;			///< Pointer to data related to event.
} MwFsmMsg;
/** \} */


typedef struct {
	uint8_t* buffer;
	size_t size;
}MegaDeviceX7TxMsg;
