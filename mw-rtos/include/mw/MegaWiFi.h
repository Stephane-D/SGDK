#pragma once

#include <stdint.h>

// Newlib
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/timers.h>

// lwIP
#include <lwip/err.h>
#include <WiFiUdp.h> //WOKAROUND PARA LOS DATOS s16
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>
#include <lwip/ip_addr.h>
#include <lwip/apps/sntp.h>

// TLS
#include <mbedtls/md5.h>

// Time keeping
#include <sntp.h>
#include <time.h>

// WiFi
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_sleep.h>

// Flash manipulation
//#include <nvs.h>
//#include <nvs_flash.h>
#include <esp_partition.h>
//#include "flash.h"

#include <esp_log.h>

#define MW_TAG "MEGAWIFI"

#include "mw/util.h"
#include "mw/mw-msg.h"
#include "mw/LSD.h"
#include "mw/led.h"
#include "mw/globals.h"
#include "mw/Http.h"
#include "mw/net_util.h"
#include "mw/game_api.h"

/// Major firmware version
#define MW_FW_VERSION_MAJOR	1
/// Minor firmware version
#define MW_FW_VERSION_MINOR	5
/// Minor firmware version
#define MW_FW_VERSION_MICRO	1
/// Firmware variant, "std" for standard version
#define MW_FW_VARIANT	"es3"

/// Maximum length of an NTP pool (TZ string + up to 3 servers)
#define MW_NTP_POOL_MAXLEN	(64 + 80)
/// Number of AP configurations stored to nvflash.
#define MW_NUM_AP_CFGS		3
/// Number of DSN servers supported per AP configuration.
#define MW_NUM_DNS_SERVERS	2
/// Number of gamertags that can be stored in the module
#define MW_NUM_GAMERTAGS	3
/// Length of the FSM queue
#define MW_FSM_QUEUE_LEN	8
/// Maximum number of simultaneous TCP connections
#define MW_MAX_SOCK		2
/// Maximum length of the default server
#define MW_SERVER_DEFAULT_MAXLEN	64

/// Length of the flash chip (4 megabytes for the ESP-12 modules.
#define FLASH_LENGTH		(4*1024*1024)

/// Maximum number of supported SNTP servers
//#define SNTP_MAX_SERVERS	3

/// Default timezone
#define MW_TZ_DEF		"GMT"
/// Default NTP server 0
#define MW_SNTP_SERV_0		"0.pool.ntp.org"
/// Default NTP server 1
#define MW_SNTP_SERV_1		"1.pool.ntp.org"
/// Default NTP server 2
#define MW_SNTP_SERV_2		"2.pool.ntp.org"

#define MW_TXTASK_STACK_LEN       8192

/// Stack size (in elements) for FSM task
#define MW_FSM_STACK_LEN	8192

/// Stack size (in elements) for SOCK task
#define MW_SOCK_STACK_LEN	2048

/// Priority for the FSM task, higher than the reception tasks, to make sure
/// we do not receive data if there is data pending processing
#define MW_FSM_PRIO		3

/// Priority for the SOCK task
#define MW_SOCK_PRIO		2

#define MW_TXTASK_PRIO     3

/// Priority for the WPOLL task
#define MW_WPOLL_PRIO		1

/** \addtogroup MwApi RetCodes Return values for functions of this module.
 *  \{ */
/// Operation completed successfully
#define MW_OK			0
/// Generic error code
#define MW_ERROR		-1
/// Command format error
#define MW_CMD_FMT_ERROR	-2
/// Unknown command code
#define MW_CMD_UNKNOWN		-3
/** \} */

/** \addtogroup MwApi Cmds Supported commands.
 *  \{ */
#define MW_CMD_OK			 	  0	///< OK command reply
#define MW_CMD_VERSION     	 	  1	///< Get firmware version
#define MW_CMD_ECHO			 	  2	///< Echo data
#define MW_CMD_AP_SCAN			  3	///< Scan for access points
#define MW_CMD_AP_CFG			  4	///< Configure access point
#define MW_CMD_AP_CFG_GET		  5	///< Get access point configuration
#define MW_CMD_IP_CURRENT		  6	///< Get current IPv4 configuration
// Reserved slot
#define MW_CMD_IP_CFG			  8	///< Configure IPv4
#define MW_CMD_IP_CFG_GET		  9	///< Get IPv4 configuration
#define MW_CMD_DEF_AP_CFG		 10	///< Set default AP configuration
#define MW_CMD_DEF_AP_CFG_GET	 11	///< Get default AP configuration
#define MW_CMD_AP_JOIN			 12	///< Join access point
#define MW_CMD_AP_LEAVE			 13	///< Leave previously joined AP
#define MW_CMD_TCP_CON			 14	///< Connect TCP socket
#define MW_CMD_TCP_BIND			 15	///< Bind TCP socket to port
// Reserved slot
#define MW_CMD_CLOSE			 17	///< Disconnect and free TCP socket
#define MW_CMD_UDP_SET			 18	///< Configure UDP socket
// Reserved slot (for setting socket options)
#define MW_CMD_SOCK_STAT		 20	///< Get socket status
#define MW_CMD_PING				 21	///< Ping host
#define MW_CMD_SNTP_CFG			 22	///< Configure SNTP service
#define MW_CMD_SNTP_CFG_GET		 23 ///< Get SNTP configuration
#define MW_CMD_DATETIME			 24	///< Get date and time
#define MW_CMD_DT_SET			 25	///< Set date and time
//#define MW_CMD_FLASH_WRITE	 26	///< Write to WiFi module flash
//#define MW_CMD_FLASH_READ		 27	///< Read from WiFi module flash
//#define MW_CMD_FLASH_ERASE     28	///< Erase sector from WiFi flash
//#define MW_CMD_FLASH_ID 		 29	///< Get WiFi flash chip identifiers
#define MW_CMD_SYS_STAT			 30	///< Get system status
#define MW_CMD_DEF_CFG_SET		 31	///< Set default configuration
#define MW_CMD_HRNG_GET			 32	///< Gets random numbers
#define MW_CMD_BSSID_GET		 33	///< Gets the WiFi BSSID
#define MW_CMD_GAMERTAG_SET		 34	///< Configures a gamertag
#define MW_CMD_GAMERTAG_GET		 35	///< Gets a stored gamertag
#define MW_CMD_LOG				 36	///< Write LOG information
#define MW_CMD_FACTORY_RESET		 37	///< Set all settings to default
//#define MW_CMD_SLEEP				 38	///< Set the module to sleep mode
#define MW_CMD_HTTP_URL_SET			 39	///< Set HTTP URL for request
#define MW_CMD_HTTP_METHOD_SET		 40	///< Set HTTP request method
#define MW_CMD_HTTP_CERT_QUERY		 41	///< Query the X.509 hash of cert
#define MW_CMD_HTTP_CERT_SET		 42	///< Set HTTPS certificate
#define MW_CMD_HTTP_HDR_ADD			 43	///< Add HTTP request header
#define MW_CMD_HTTP_HDR_DEL			 44	///< Delete HTTP request header
#define MW_CMD_HTTP_OPEN			 45	///< Open HTTP request
#define MW_CMD_HTTP_FINISH			 46	///< Finish HTTP request
#define MW_CMD_HTTP_CLEANUP			 47	///< Clean request data
// Reserved slot
#define MW_CMD_SERVER_URL_GET		 49	///< Get the main server URL
#define MW_CMD_SERVER_URL_SET		 50	///< Set the main server URL
#define MW_CMD_WIFI_ADV_GET			 51	///< Get advanced WiFi parameters
#define MW_CMD_WIFI_ADV_SET			 52	///< Set advanced WiFi parameters
#define MW_CMD_NV_CFG_SAVE		 	 53	///< Save non-volatile config
//#define MW_CMD_UPGRADE_LIST		     54	///< Get firmware upgrade versions
//#define MW_CMD_UPGRADE_PERFORM		 55	///< Start firmware upgrade
#define MW_CMD_GAME_ENDPOINT_SET	 56	///< Set game API endpoint
#define MW_CMD_GAME_KEYVAL_ADD		 57	///< Add key/value appended to requests
#define MW_CMD_GAME_REQUEST		     58	///< Perform a game API request
#define MW_CMD_ERROR			     255	///< Error command reply
/** \} */

/// Length of a command header (command and datalen fields)
#define MW_CMD_HEADLEN	(2 * sizeof(uint16_t))

/// Pointer to the command data
#define MW_CMD_DATA(pCmd)		(((uint8_t*)pCmd)+sizeof(MwCmd))

#define MW_SERVER_DEFAULT		"doragasu.com"

/// Maximum number of reassociation attempts
#define MW_REASSOC_MAX		5

/// Sleep timer period in ms
#define MW_SLEEP_TIMER_MS	30000

/// Default PHY protocol bitmap
#define MW_PHY_PROTO_DEF	WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | \
	WIFI_PROTOCOL_11N

/// Commands allowed while in IDLE state
const static uint32_t idleCmdMask[2] = {(uint32_t)(
    (1<<MW_CMD_VERSION)               | (1<<MW_CMD_ECHO)                |
    (1<<MW_CMD_AP_SCAN)               | (1<<MW_CMD_AP_CFG)              |
    (1<<MW_CMD_AP_CFG_GET)            | (1<<MW_CMD_IP_CFG)              |
    (1<<MW_CMD_IP_CFG_GET)            | (1<<MW_CMD_DEF_AP_CFG)          |
    (1<<MW_CMD_DEF_AP_CFG_GET)        | (1<<MW_CMD_AP_JOIN)             |
    (1<<MW_CMD_SNTP_CFG)              | (1<<MW_CMD_SNTP_CFG_GET)        |
    (1<<MW_CMD_DATETIME)              | (1<<MW_CMD_DT_SET)              |
    (1<<MW_CMD_SYS_STAT)              | (1<<MW_CMD_DEF_CFG_SET)),(uint32_t)(
    (1<<(MW_CMD_HRNG_GET - 32))       | (1<<(MW_CMD_BSSID_GET - 32))    |
    (1<<(MW_CMD_GAMERTAG_SET - 32))   | (1<<(MW_CMD_GAMERTAG_GET - 32)) |
    (1<<(MW_CMD_LOG - 32))            | (1<<(MW_CMD_FACTORY_RESET - 32))|
    (1<<(MW_CMD_HTTP_URL_SET - 32)) |
    (1<<(MW_CMD_HTTP_METHOD_SET - 32))| (1<<(MW_CMD_HTTP_CERT_QUERY - 32))|
    (1<<(MW_CMD_HTTP_CERT_SET - 32))  | (1<<(MW_CMD_HTTP_HDR_ADD - 32)) |
    (1<<(MW_CMD_HTTP_HDR_DEL - 32))   | (1<<(MW_CMD_HTTP_CLEANUP - 32)) |
    (1<<(MW_CMD_SERVER_URL_GET - 32)) | (1<<(MW_CMD_SERVER_URL_SET - 32)) |
    (1<<(MW_CMD_WIFI_ADV_GET - 32))   | (1<<(MW_CMD_WIFI_ADV_SET - 32)) |
    (1<<(MW_CMD_NV_CFG_SAVE - 32))    | (1<<(MW_CMD_GAME_ENDPOINT_SET - 32)) |
    (1<<(MW_CMD_GAME_KEYVAL_ADD - 32)))
};

/// Commands allowed while in READY state
const static uint32_t readyCmdMask[2] = {(uint32_t)(
    (1<<MW_CMD_VERSION)              | (1<<MW_CMD_ECHO)                  |
    (1<<MW_CMD_AP_CFG)               | (1<<MW_CMD_AP_CFG_GET)            |
    (1<<MW_CMD_IP_CURRENT)           | (1<<MW_CMD_IP_CFG)                |
    (1<<MW_CMD_IP_CFG_GET)           | (1<<MW_CMD_DEF_AP_CFG)            |
    (1<<MW_CMD_DEF_AP_CFG_GET)       | (1<<MW_CMD_AP_LEAVE)              |
    (1<<MW_CMD_TCP_CON)              | (1<<MW_CMD_TCP_BIND)              |
    (1<<MW_CMD_CLOSE)                | (1<<MW_CMD_UDP_SET)               |
    (1<<MW_CMD_SOCK_STAT)            | (1<<MW_CMD_PING)                  |
    (1<<MW_CMD_SNTP_CFG)             | (1<<MW_CMD_SNTP_CFG_GET)          |
    (1<<MW_CMD_DATETIME)             | (1<<MW_CMD_DT_SET)                |
    (1<<MW_CMD_SYS_STAT)             | (1<<MW_CMD_DEF_CFG_SET)),(uint32_t)(
    (1<<(MW_CMD_HRNG_GET - 32))      | (1<<(MW_CMD_BSSID_GET - 32))      |
    (1<<(MW_CMD_GAMERTAG_SET - 32))  | (1<<(MW_CMD_GAMERTAG_GET - 32))   |
    (1<<(MW_CMD_LOG - 32))           | 
    (1<<(MW_CMD_HTTP_URL_SET - 32))  | (1<<(MW_CMD_HTTP_METHOD_SET - 32))|
    (1<<(MW_CMD_HTTP_CERT_QUERY - 32))|(1<<(MW_CMD_HTTP_CERT_SET - 32))  |
    (1<<(MW_CMD_HTTP_HDR_ADD - 32))  | (1<<(MW_CMD_HTTP_HDR_DEL - 32))   |
    (1<<(MW_CMD_HTTP_OPEN - 32))     | (1<<(MW_CMD_HTTP_FINISH - 32))    |
    (1<<(MW_CMD_HTTP_CLEANUP - 32))  | (1<<(MW_CMD_SERVER_URL_GET - 32)) |
    (1<<(MW_CMD_SERVER_URL_SET - 32))| (1<<(MW_CMD_WIFI_ADV_GET - 32))   |
    (1<<(MW_CMD_WIFI_ADV_SET - 32))  | (1<<(MW_CMD_NV_CFG_SAVE - 32))    |
    (1<<(MW_CMD_GAME_ENDPOINT_SET - 32))| (1<<(MW_CMD_GAME_KEYVAL_ADD - 32))|
    (1<<(MW_CMD_GAME_REQUEST - 32)))
};

class MegaWiFi {
public:
    
    /** \addtogroup MwApi MwFdOps FD set operations (add/remove)
 *  \{ */
    typedef enum {
        MW_FD_NONE = 0,		///< Do nothing
        MW_FD_ADD,			///< Add socket to the FD set
        MW_FD_REM			///< Remove socket from the FD set
    } MwFdOps;

    /** \addtogroup MwApi ApCfg Configuration needed to connect to an AP
         *  \{ */
    typedef struct {
        /// SSID
        char ssid[MW_SSID_MAXLEN];
        /// Password
        char pass[MW_PASS_MAXLEN];
        /// Connection PHY protocol
        uint8_t phy;
        /// Reserved, set to 0
        uint8_t reserved[3];
    } ApCfg;
    /** \} */

    /** \addtogroup MwApi MwNvCfg Configuration saved to non-volatile memory.
     *  \{ */
    typedef struct {
        /// Advanced WiFi config
        struct mw_wifi_adv_cfg wifi;
        /// Access point configuration (SSID, password).
        ApCfg ap[MW_NUM_AP_CFGS];
        /// IPv4 (IP addr, mask, gateway). If IP=0.0.0.0, use DHCP.
        tcpip_adapter_ip_info_t ip[MW_NUM_AP_CFGS];
        /// DNS configuration (when not using DHCP). 2 servers per AP config.
        ip_addr_t dns[MW_NUM_AP_CFGS][MW_NUM_DNS_SERVERS];
        /// Pool length for SNTP configuration
        uint16_t ntpPoolLen;
        /// SNTP configuration. The TZ string and up to 3 servers are
        /// concatenated and null separated. Two NULL characters mark
        /// the end of the pool
        char ntpPool[MW_NTP_POOL_MAXLEN];
        /// Index of the configuration used on last connection (-1 for none).
        char defaultAp;
        /// Gamertag
        struct mw_gamertag gamertag[MW_NUM_GAMERTAGS];
        /// URL of the main server
        char serverUrl[MW_SERVER_DEFAULT_MAXLEN];
        /// Checksum
        uint8_t md5[16];
    } MwNvCfg;
    /** \} */

    struct net_event {
        esp_event_base_t event_base;
        int32_t event_id;
        union {
            ip_event_got_ip_t got_ip;
            wifi_event_sta_connected_t sta_connected;
            wifi_event_sta_disconnected_t sta_disconnected;
        };
    };


    #define MW_CFG_SECT_LEN		MW_FLASH_SECT_ROUND(sizeof(MwNvCfg))

    /** \addtogroup MwApi MwData Module data needed to handle module status
     *  \todo Maybe we should add a semaphore to access data in this struct.
     *  \{ */
    typedef struct {
        /// System status
        MwMsgSysStat s;
        /// Sockets associated with each channel. NOTE: the index to this array
        /// must be the channel number minus 1 (as channel 0 is the control
        /// channel and has no socket associated).
        int8_t sock[MW_MAX_SOCK];
        /// Socket status. As with sock[], index must be channel number - 1.
        MwSockStat ss[MW_MAX_SOCK];
        /// Channel associated with each socket (like sock[] but reversed). NOTE:
        /// An extra socket placeholder is reserved because of server sockets that
        /// might use a temporary additional socket during the accept() stage.
        int8_t chan[MW_MAX_SOCK + 1];
        /// FSM queue for event reception
        QueueHandle_t q;
        QueueHandle_t qtx;
        /// Sleep inactivity timer
        TimerHandle_t tim;
        /// File descriptor set for select()
        fd_set fds;
        /// Maximum socket identifier value
        int fdMax;
        /// Address of the remote end, used in UDP sockets
        struct sockaddr_in raddr[MW_MAX_SOCK];
        /// Association retries
        uint8_t n_reassoc;
        /// Current PHY type
        uint8_t phy;
        /// Configuration partition handle
        const esp_partition_t *p_cfg;
        /// Flash chip device id
        uint16_t flash_dev;
        /// Flash chip manufacturer id
        uint8_t flash_man;
    } MwData;
    /** \} */

    MwData d;

    LSD* lsd = new LSD();
    Http* http = NULL;
    GameApi* ga = NULL;
    Led* led = NULL;
    MegaWiFi(){};

    int MwInit();
    int MwCfgLoad(void);
    void MwSetDefaultCfg(void);
    size_t MwFsm(MwFsmMsg *msg);
    size_t MwFsmCmdProc(MwCmd *c, uint16_t totalLen);
    void MwSysStatFill(MwCmd *rep);

    int MwRecv(int ch, char *buf, int len);
    int MwUdpRecv(int idx, char *buf);
    int MwAccept(int sock, int ch);
    void MwSockClose(int ch);
    void MwFsmRaiseChEvent(int ch);
    void MwApJoin(uint8_t n);
    int MwFsmTcpCon(MwMsgInAddr* addr);
    int MwFsmTcpBind(MwMsgBind *b);
    int MwChannelCheck(int ch);
    int MwUdpSet(MwMsgInAddr* addr);
    void MwFsmClearChEvent(int ch);
    void MwFsmReady(MwFsmMsg *msg);
    int MwSend(int ch, const void *data, int len);
    int MwUdpSend(int idx, const void *data, int len);

private:
    /*
    * Private local variables
    */
    /// Configuration data
    MwNvCfg cfg;
    /// Module static data
    /// Temporal data buffer for data forwarding
    /// \todo FIXME This buffer is used by MwFsmSckTsk, and by the HTTP module.
    /// Access should be synchronized, and it is not.
    uint8_t buf[LSD_MAX_LEN];

    esp_err_t wifi_init();
    void reply_set_ok_empty(MwCmd *reply);
    void sntp_set_config(void);
    int tokens_get(const char *in, const char *token[], int token_max, uint16_t *len_total);
    void wifi_cfg_log(void);
    int wifi_scan(uint8_t phy_type, uint8_t *data);
    void ap_cfg_set(uint8_t num, uint8_t phy_type, const char *ssid, const char *pass, MwCmd *reply);
    void ap_print(const wifi_ap_record_t *ap, int n_aps);
    void ap_join_ev_handler(struct net_event *net);
    int build_scan_reply(const wifi_ap_record_t *ap, uint8_t n_aps, uint8_t *data);
    void log_ip_cfg(MwMsgIpCfg *ip);
    void rand_fill(uint8_t *buf, uint16_t len);
    void sntp_config_set(const char *data, uint16_t len, MwCmd *reply);
    int mw_nv_cfg_save(void);
    int http_parse_finish(MwCmd *reply);
    int parse_server_url_get(MwCmd *reply);
    void parse_server_url_set(const char *url, MwCmd *reply);
    int parse_wifi_adv_get(MwCmd *reply);
    void parse_wifi_adv_set(struct mw_wifi_adv_cfg *wifi, MwCmd *reply);
    void parse_game_endpoint_set(const char *data, MwCmd *reply);
    int parse_game_request(struct mw_ga_request *req, MwCmd *reply);
    void parse_game_add_keyval(const char *data, MwCmd *reply);
    void SetIpCfg(int slot);
    // void deep_sleep(void);
    void disconnect(void);
    void close_all(void);

    static void MwFsmTsk(void *pvParameters);
    static void MwFsmSockTsk(void *pvParameters);
    static void txTask(void *pvParameters);
    static int MwCmdInList(uint8_t cmd, const uint32_t list[2]);
    static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    static void time_sync_cb(struct timeval *tv);
};

static MegaWiFi *instance_p = NULL;