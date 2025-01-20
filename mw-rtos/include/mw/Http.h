/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#pragma once

#include <WiFiUdp.h> //WOKAROUND PARA LOS DATOS s16
#include <netdb.h>
#include <stdint.h>
#include <esp_http_client.h>

#include "mw/mw-msg.h"
#include <esp_partition.h>
#include <string.h>
#include "globals.h"
#include "util.h"
#include "LSD.h"

#define HTTP_TAG "HTTP"

#define CERT_HASH_OFF	((size_t)0)
#define CERT_LEN_OFF	sizeof(uint32_t)
#define CERT_OFF	(2 * sizeof(uint32_t))

#define CERT_P_PTR(type, off)	((type*)SPI_FLASH_ADDR((d.p->address + (off))))

class Http {
public:
    Http(){};
	Http(LSD* lsd);

    /// Status of the HTTP command
    enum http_stat {
        MW_HTTP_ST_IDLE = 0,
        MW_HTTP_ST_OPEN_CONTENT_WAIT,
        MW_HTTP_ST_FINISH_WAIT,
        MW_HTTP_ST_FINISH_CONTENT_WAIT,
        MW_HTTP_ST_CERT_SET,
        MW_HTTP_ST_ERROR,
        MW_HTTP_ST_STAT_MAX
    };

    struct http_data {
        /// HTTP client handle
        esp_http_client_handle_t h;
        /// HTTP machine state
        enum http_stat s;
        /// Remaining bytes to read/write
        int remaining;
        /// Certificate x509 hash, used during CERT_SET
        uint32_t hash_tmp;
        /// Partition with the certificate
        const esp_partition_t *p;
        /// Buffer used to send/recv HTTP data
        char *buf;
    };

    struct http_data d;

    LSD* lsd = NULL;

    esp_http_client_handle_t http_init(const char *url, http_event_handle_cb event_cb);

    int http_module_init(char *data_buf);
    
    bool http_url_set(const char *url);

    bool http_method_set(esp_http_client_method_t method);

    bool http_header_add(const char *data);

    bool http_header_del(const char *key);

    bool http_open(uint32_t write_len);

    bool http_finish(uint16_t *status, int32_t *body_len);

    bool http_cleanup(void);

    void http_cert_flash_write(const char *data, uint16_t len);

    uint32_t http_cert_query(void);

    bool http_cert_erase(void);

    bool http_cert_set(uint32_t x509_hash, uint16_t cert_len);

    #define http_err_set(...)	do {	\
        lsd->LsdChDisable(MW_HTTP_CH);	\
        ESP_LOGE(HTTP_TAG,__VA_ARGS__);		\
        d.s = MW_HTTP_ST_ERROR;	\
    } while(0)

    void http_recv(void);

    void http_send(const char *data, uint16_t len);

private:

    static esp_http_client_handle_t http_init_int(const char *url, const char *cert_pem, http_event_handle_cb event_cb);

    static int http_url_set_int(esp_http_client_handle_t client, const char *url);

    static int http_method_set_int(esp_http_client_handle_t client, esp_http_client_method_t method);

    static int http_header_add_int(esp_http_client_handle_t client, const char *key, const char *value);

    static int http_header_del_int(esp_http_client_handle_t client, const char *key);

    static int http_open_int(esp_http_client_handle_t client, int write_len);

    static int http_finish_int(esp_http_client_handle_t client, int32_t *data_len);

    static int http_cleanup_int(esp_http_client_handle_t client);
};
