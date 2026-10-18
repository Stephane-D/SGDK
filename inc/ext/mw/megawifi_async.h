/************************************************************************//**
 * \file
 *
 * \brief MegaWiFi Async API implementation.
 *
 * \defgroup megawifi megawifi
 * \{
 *
 * \brief MegaWiFi API implementation.
 *
 * \author Juan Antonio (PaCHoN)
 * \date 2025
 * 
 * \note This module requires setting MODULE_MEGAWIFI to 1 in config.h and
 * rebuilding the library (if you had to change them).
 ****************************************************************************/

#ifndef _MEGAWIFI_ASYNC_H_
#define _MEGAWIFI_ASYNC_H_

#define MW_PROCESS_OVERRIDE
#include "ext/mw/megawifi.h"

typedef void (*mw_async_cb)(enum mw_err err);

typedef void (*mw_detect_async_cb)(enum mw_err err, uint8_t *major, uint8_t *minor,
		char **variant);
typedef void (*mw_ip_cfg_get_async_cb)(enum mw_err err,
		struct mw_ip_cfg *ip);
typedef void (*mw_ip_current_async_cb)(enum mw_err err,
		struct mw_ip_cfg *ip);
typedef void (*mw_sys_stat_get_async_cb)(enum mw_err err,
		union mw_msg_sys_stat *status);
typedef void (*mw_bssid_get_async_cb)(enum mw_err err, uint8_t *bssid);
typedef void (*mw_flash_read_async_cb)(enum mw_err err, uint8_t *data);
typedef void (*mw_hrng_get_async_cb)(enum mw_err err, uint8_t *data);
typedef void (*mw_gamertag_get_async_cb)(enum mw_err err,
		struct mw_gamertag *gamertag);
typedef void (*mw_def_server_get_async_cb)(enum mw_err err, char *server_url);
typedef void (*mw_wifi_adv_cfg_get_async_cb)(enum mw_err err,
		struct mw_wifi_adv_cfg *wifi);
typedef void (*mw_ap_cfg_get_async_cb)(enum mw_err err,
		struct mw_msg_ap_cfg *ap_cfg);
typedef void (*mw_sntp_cfg_get_async_cb)(enum mw_err err,
		struct mw_msg_sntp_cfg *sntp_cfg);
typedef void (*mw_date_time_get_async_cb)(enum mw_err err,
		struct mw_msg_date_time *date_time);
typedef void (*mw_flash_id_get_async_cb)(enum mw_err err,
		struct mw_flash_id *flash_id);
typedef void (*mw_ap_scan_async_cb)(enum mw_err err, uint8_t *ap_data,
		uint16_t data_len);
typedef void (*mw_ping_async_cb)(enum mw_err err,
		struct mw_ping_response *response);
typedef void (*mw_fw_list_upgrades_async_cb)(enum mw_err err,
		struct mw_upgrade_list_response *upgrades);
typedef void (*mw_http_finish_async_cb)(enum mw_err err, uint8_t *data);
typedef void (*mw_ga_request_async_cb)(enum mw_err err, uint8_t *data);
typedef void (*mw_def_ap_cfg_get_async_cb)(enum mw_err err, u8 slot);
typedef void (*mw_sock_stat_get_async_cb)(enum mw_err err, u8 sock);
typedef void (*mw_http_cert_query_async_cb)(enum mw_err err, int32_t certSize);

/************************************************************************//**
 * \brief Starts an asynchronous MegaWiFi version detection.
 *
 * Call mw_process() regularly until the callback is invoked.
 * Only one asynchronous detection can be in progress at a
 * time.
 *
 * \param[in] callback Callback called with the detected version, or NULL
 *                      arguments when detection fails.
 *
 * \return MW_ERR_NONE when detection was started, other code on failure.
 ****************************************************************************/
enum mw_err mw_detect_async(mw_detect_async_cb callback);
enum mw_err mw_default_cfg_set_async(mw_async_cb callback);
enum mw_err mw_ap_cfg_set_async(uint8_t slot, const char *ssid,
		const char *pass, enum mw_phy_type phy_type, mw_async_cb callback);
enum mw_err mw_ip_cfg_set_async(uint8_t slot, const struct mw_ip_cfg *ip,
		mw_async_cb callback);
enum mw_err mw_ap_assoc_async(uint8_t slot, mw_async_cb callback);
enum mw_err mw_ap_disassoc_async(mw_async_cb callback);
enum mw_err mw_def_ap_cfg_async(uint8_t slot, mw_async_cb callback);
enum mw_err mw_sntp_cfg_set_async(const char *tz_str,
		const char *server[3], mw_async_cb callback);
enum mw_err mw_flash_sector_erase_async(uint16_t sect, mw_async_cb callback);
enum mw_err mw_flash_write_async(uint32_t addr, uint8_t *data,
		uint16_t data_len, mw_async_cb callback);
enum mw_err mw_gamertag_set_async(uint8_t slot,
		const struct mw_gamertag *gamertag, mw_async_cb callback);
enum mw_err mw_http_url_set_async(const char *url, mw_async_cb callback);
enum mw_err mw_http_method_set_async(enum mw_http_method method,
		mw_async_cb callback);
enum mw_err mw_http_header_add_async(const char *key, const char *value,
		mw_async_cb callback);
enum mw_err mw_http_header_del_async(const char *key, mw_async_cb callback);
enum mw_err mw_def_server_set_async(const char *server_url,
		mw_async_cb callback);
enum mw_err mw_log_async(const char *msg, mw_async_cb callback);
enum mw_err mw_factory_settings_async(mw_async_cb callback);
enum mw_err mw_cfg_save_async(mw_async_cb callback);
enum mw_err mw_wifi_adv_cfg_set_async(const struct mw_wifi_adv_cfg *wifi,
		mw_async_cb callback);
enum mw_err mw_ga_endpoint_set_async(const char *endpoint,
		const char *priv_key, mw_async_cb callback);
enum mw_err mw_ga_key_value_add_async(const char **key, const char **value,
		uint16_t num_pairs, mw_async_cb callback);
enum mw_err mw_fw_upgrade_async(const char *name, mw_async_cb callback);
enum mw_err mw_tcp_connect_async(uint8_t ch, const char *dst_addr,
		const char *dst_port, const char *src_port, mw_async_cb callback);
enum mw_err mw_close_async(uint8_t ch, mw_async_cb callback);
enum mw_err mw_tcp_bind_async(uint8_t ch, uint16_t port,
		mw_async_cb callback);
enum mw_err mw_udp_set_async(uint8_t ch, const char *dst_addr,
		const char *dst_port, const char *src_port, mw_async_cb callback);
enum mw_err mw_http_open_async(uint32_t content_len, mw_async_cb callback);
enum mw_err mw_ip_cfg_get_async(uint8_t slot, mw_ip_cfg_get_async_cb callback);
enum mw_err mw_ip_current_async(mw_ip_current_async_cb callback);
enum mw_err mw_sys_stat_get_async(mw_sys_stat_get_async_cb callback);
enum mw_err mw_bssid_get_async(enum mw_if_type interface_type,
		mw_bssid_get_async_cb callback);
enum mw_err mw_flash_read_async(uint32_t addr, uint16_t data_len,
		mw_flash_read_async_cb callback);
enum mw_err mw_hrng_get_async(uint16_t rnd_len, mw_hrng_get_async_cb callback);
enum mw_err mw_gamertag_get_async(uint8_t slot, mw_gamertag_get_async_cb callback);
enum mw_err mw_def_server_get_async(mw_def_server_get_async_cb callback);
enum mw_err mw_wifi_adv_cfg_get_async(mw_wifi_adv_cfg_get_async_cb callback);
enum mw_err mw_ap_cfg_get_async(uint8_t slot, mw_ap_cfg_get_async_cb callback);
enum mw_err mw_sntp_cfg_get_async(mw_sntp_cfg_get_async_cb callback);
enum mw_err mw_date_time_get_async(mw_date_time_get_async_cb callback);
enum mw_err mw_flash_id_get_async(mw_flash_id_get_async_cb callback);
enum mw_err mw_ap_scan_async(enum mw_phy_type phy_type,
		mw_ap_scan_async_cb callback);
enum mw_err mw_def_ap_cfg_get_async(mw_def_ap_cfg_get_async_cb callback);
enum mw_err mw_sock_stat_get_async(uint8_t ch, mw_sock_stat_get_async_cb callback);
enum mw_err mw_http_cert_query_async(mw_http_cert_query_async_cb callback);
enum mw_err mw_ping_async(const char *domain, u8 retries,
		mw_ping_async_cb callback);
enum mw_err mw_fw_list_upgrades_async(uint8_t page, uint8_t size,
		uint8_t offset, mw_fw_list_upgrades_async_cb callback);
enum mw_err mw_http_finish_async(mw_http_finish_async_cb callback);
enum mw_err mw_ga_request_async(enum mw_http_method method,
		const char **path, uint8_t num_paths, const char **key,
		const char **value, uint8_t num_kv_pairs, mw_ga_request_async_cb callback);
enum mw_err mw_http_cleanup_async(mw_async_cb callback);
enum mw_err mw_http_cert_set_async(uint32_t cert_hash, const char *cert,
		uint16_t cert_len, mw_async_cb callback);

void mw_process_async(void);

#define mw_process mw_process_async

#endif /*_MEGAWIFI_ASYNC_H_*/

/** \} */

