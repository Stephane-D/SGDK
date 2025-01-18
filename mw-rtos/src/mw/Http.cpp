#include "mw/Http.h"
#define LOG_LOCAL_LEVEL 5 //CORE_DEBUG_LEVEL

Http::Http(LSD* lsd){
    this->lsd = lsd;
};

int Http::http_module_init(char *data_buf)
{
    memset(&d, 0, sizeof(struct http_data));

    d.p = esp_partition_find_first(MW_DATA_PART_TYPE, MW_CERT_PART_SUBTYPE,
            MW_CERT_PART_LABEL);

    if (!d.p) {
        return 1;
    }

    d.buf = data_buf;

    return 0;
}

esp_http_client_handle_t Http::http_init(const char *url,
            http_event_handle_cb event_cb)
    {
        // No fancy esp_partition_mmap support, so access directly to data
        uint32_t cert_len = *CERT_P_PTR(uint32_t, CERT_LEN_OFF);
        const char *cert = CERT_P_PTR(const char, CERT_OFF);
        if (!cert_len || cert_len > MW_CERT_MAXLEN) {
            ESP_LOGE(HTTP_TAG,"no valid certificate found, len %u", cert_len);
            cert = NULL;
        }else if(!cert[0] || 0xFF == (unsigned char)cert[0]){
            ESP_LOGE(HTTP_TAG,"no valid certificate found, len %u, start: %d",
                    cert_len, cert[0]);     
            cert = NULL;       
        }

        return http_init_int(url, cert, event_cb);
    }

bool Http::http_url_set(const char *url)
{
    ESP_LOGD(HTTP_TAG,"set url %s", url);
    if (!d.h) {
        ESP_LOGD(HTTP_TAG,"init, HTTP URL: %s", url);
        d.h = http_init(url, NULL);
        return false;
    }

    if (((MW_HTTP_ST_IDLE != d.s) && (MW_HTTP_ST_ERROR != d.s)) ||
        http_url_set_int(d.h, url)) {
        ESP_LOGE(HTTP_TAG,"HTTP failed to set URL %s", url);
        return true;
    }
    ESP_LOGD(HTTP_TAG,"HTTP URL: %s", url);

    return false;
}

bool Http::http_method_set(esp_http_client_method_t method)
{
    bool err = 0;

    ESP_LOGD(HTTP_TAG,"set method %d", method);
    if (!d.h) {
        d.h = http_init("", NULL);
    }

    if (((MW_HTTP_ST_IDLE != d.s) && (MW_HTTP_ST_ERROR != d.s)) ||
            http_method_set_int(d.h, method)) {
        ESP_LOGE(HTTP_TAG,"HTTP failed to set method %d", method);
        err = 1;
    } else {
        ESP_LOGD(HTTP_TAG,"HTTP method: %d", method);
    }

    return err;
}

bool Http::http_header_add(const char *data)
{
    const char *item[2] = {0};
    int n_items;
    bool err = 1;

    if (!d.h) {
        d.h = http_init("", NULL);
    }

    if (((MW_HTTP_ST_IDLE != d.s) && (MW_HTTP_ST_ERROR != d.s))) {
        ESP_LOGE(HTTP_TAG,"not allowed in HTTP state %d", d.s);
        goto out;
    }

    n_items = itemizer(data, item, 2);
    ESP_LOGD(HTTP_TAG,"HTTP header: %s: %s", item[0], item[1]);

    if ((n_items != 2) || http_header_add_int(d.h, item[0], item[1])) {
        ESP_LOGE(HTTP_TAG,"invalid header data");
        goto out;
    }
    err = 0;

out:
    return err;
}

bool Http::http_header_del(const char *key)
{
    bool err = false;

    if (((MW_HTTP_ST_IDLE != d.s) && (MW_HTTP_ST_ERROR != d.s)) ||
        http_header_del_int(d.h, key)) {
        ESP_LOGE(HTTP_TAG,"HTTP failed to del header %s", key);
        err = true;
    } else {
        ESP_LOGD(HTTP_TAG,"HTTP del header: %s", key);
    }

    return err;
}

bool Http::http_open(uint32_t write_len)
{
    bool err = false;

    ESP_LOGD(HTTP_TAG,"opening ");
    if (((MW_HTTP_ST_IDLE != d.s) && (MW_HTTP_ST_ERROR != d.s)) ||
            http_open_int(d.h, write_len)) {
        ESP_LOGE(HTTP_TAG,"HTTP open failed");
        err = true;
    } else {
        lsd->LsdChEnable(MW_HTTP_CH);
        ESP_LOGD(HTTP_TAG,"HTTP open OK, %u bytes", write_len);
        if (write_len) {
            d.remaining = write_len;
            d.s = MW_HTTP_ST_OPEN_CONTENT_WAIT;
        } else {
            d.s = MW_HTTP_ST_FINISH_WAIT;
        }
    }

    return err;
}

bool Http::http_finish(uint16_t *status, int32_t *body_len)
{
    int status_code;
    int32_t len = 0;
    bool err = false;

    if ((MW_HTTP_ST_FINISH_WAIT != d.s) ||
            ((status_code = http_finish_int(d.h, &len)) < 0)) {
        ESP_LOGE(HTTP_TAG,"HTTP finish failed");
        err = true;
    } else {
        ESP_LOGD(HTTP_TAG,"HTTP finish: %d: %u bytes", status_code, len);
        if (len) {
            d.remaining = len;
            d.s = MW_HTTP_ST_FINISH_CONTENT_WAIT;
        } else {
            d.s = MW_HTTP_ST_IDLE;
            lsd->LsdChDisable(MW_HTTP_CH);
        }
        if (status) {
            *status = status_code;
        }
        if (body_len) {
            *body_len = len;
        }
    }

    return err;
}

bool Http::http_cleanup(void)
{
    d.s = MW_HTTP_ST_IDLE;
    bool err = false;

    if (!d.h) {
        goto out;
    }

    lsd->LsdChDisable(MW_HTTP_CH);
    if (http_cleanup_int(d.h)) {
        ESP_LOGE(HTTP_TAG,"HTTP cleanup failed");
        err = true;
    } else {
        ESP_LOGD(HTTP_TAG,"HTTP cleanup OK");
    }
    d.h = NULL;

out:
    return err;
}

void Http::http_cert_flash_write(const char *data, uint16_t len)
{
    static uint16_t written;
    uint16_t to_write;
    esp_err_t err;

    // Special condition: if no data payload, reset written counter
    if (!data && !len) {
        ESP_LOGD(HTTP_TAG,"reset data counter");
        written = 0;
        return;
    }

    ESP_LOGI(HTTP_TAG,"write %" PRIu16 " cert bytes. writen %d remaining %d bytes", len, written, d.remaining);
    // Note we are using d.remaining as total (it is not decremented
    // each time we write data)
    to_write = MIN(d.remaining - written, len);
    if (to_write) {
        err = esp_partition_write(d.p, CERT_OFF + written,
                data, to_write);
        if (err) {
            ESP_LOGE(HTTP_TAG,"flash write failed");
            d.s = MW_HTTP_ST_IDLE;
            lsd->LsdChDisable(MW_HTTP_CH);
            return;
        }
        written += to_write;
    }

    if (written >= d.remaining) {
        // Write certificate hash
        err = esp_partition_write(d.p, 0, &d.hash_tmp, sizeof(uint32_t));
        d.s = MW_HTTP_ST_IDLE;
        if(err){
            ESP_LOGE(HTTP_TAG,"flash store failed");
        }else{
            ESP_LOGI(HTTP_TAG,"certificate %08"PRIx32" stored", d.hash_tmp);
            if (to_write < len) {
                ESP_LOGW(HTTP_TAG,"ignoring %d certificate bytes", len - to_write);
            }
        }
        lsd->LsdChDisable(MW_HTTP_CH);
    }
}

uint32_t Http::http_cert_query(void)
{
    uint32_t cert = 0xFFFFFFFF;

    esp_partition_read(d.p, 0, &cert, sizeof(uint32_t));
    ESP_LOGD(HTTP_TAG,"cert hash: %08"PRIx32, cert);

    return cert;
}

bool Http::http_cert_erase(void)
{
    return esp_partition_erase_range(d.p, 0, d.p->size);
}

bool Http::http_cert_set(uint32_t x509_hash, uint16_t cert_len)
{
    bool err = true;
    uint32_t  installed = *CERT_P_PTR(uint32_t, CERT_HASH_OFF);

    if (d.s != MW_HTTP_ST_IDLE && d.s != MW_HTTP_ST_ERROR) {
        ESP_LOGE(HTTP_TAG,"not allowed in HTTP state %d", d.s);
        goto out;
    }
    // Check if erase request
    if (installed != 0xFFFFFFFF && !cert_len) {
        ESP_LOGD(HTTP_TAG,"erasing cert as per request");
        // Erase cert
        err = http_cert_erase();
        goto out;
    }
    if (x509_hash == installed) {
        ESP_LOGW(HTTP_TAG,"cert %08"PRIx32" is already installed", x509_hash);
        err = false;
        goto out;
    }
    if (cert_len > MW_CERT_MAXLEN) {
        ESP_LOGE(HTTP_TAG,"cert is %"PRIu16" bytes, maximum allowed is "
                STR(MW_CERT_MAXLEN) " bytes",
                cert_len);
        goto out;
    }
    // Erase the required sectors (round up the division between sect len)
    ESP_LOGD(HTTP_TAG,"erasing previous cert");
    err = http_cert_erase();
    if (!err) {
        // Write certificate length, and store for later the hash
        uint32_t dw_len = cert_len;
        ESP_LOGI(HTTP_TAG,"write cert hash %08"PRIx32", len %" PRIu32, x509_hash, dw_len);
        err = esp_partition_write(d.p, CERT_LEN_OFF, &dw_len, sizeof(uint32_t));
        d.hash_tmp = x509_hash;
    }
    if (err) {
        ESP_LOGE(HTTP_TAG,"failed to erase certificate store");
        goto out;
    }

    ESP_LOGI(HTTP_TAG,"waiting certificate data");
    // Reset written data counter
    lsd->LsdChEnable(MW_HTTP_CH);
    http_cert_flash_write(NULL, 0);
    d.s = MW_HTTP_ST_CERT_SET;
    d.remaining = cert_len;

out:
    return err;
}
void Http::http_recv(void)
{
    int readed;

    if (d.s != MW_HTTP_ST_FINISH_CONTENT_WAIT) {
        http_err_set("ignoring unexpected HTTP data on state %d",
                d.s);
        return;
    }

    while (d.remaining > 0) {
        readed = esp_http_client_read(d.h, d.buf, MW_MSG_MAX_BUFLEN);
        if (-1 == readed) {
            http_err_set("HTTP read error, %d remaining",
                    d.remaining);
            return;
        } else if (0 == readed) {
            ESP_LOGI(HTTP_TAG,"server closed the connection");
            if (INT32_MAX == d.remaining) {
                d.remaining = 0;
            }
        }
        lsd->LsdSend((uint8_t*)d.buf, readed, MW_HTTP_CH);
        // Only decrement if not on chunked transfer mode
        if (d.remaining != INT32_MAX) {
            d.remaining -= readed;
        }
    }

    if (d.remaining < 0) {
        ESP_LOGW(HTTP_TAG,"HTTP ignoring extra %d bytes", -d.remaining);
    }
    ESP_LOGD(HTTP_TAG,"HTTP request complete");
    d.s = MW_HTTP_ST_IDLE;
    lsd->LsdChDisable(MW_HTTP_CH);
}

void Http::http_send(const char *data, uint16_t len)
{
    uint16_t to_write;

    ESP_LOGD(HTTP_TAG,"HTTP data %" PRIu16 " bytes", len);
    // Writes should only be performed by client during the
    // OPEN_CONTENT_WAIT or CERT_SET states
    switch (d.s) {
    case MW_HTTP_ST_OPEN_CONTENT_WAIT:
        to_write = MIN(d.remaining, len);
        esp_http_client_write(d.h, data, to_write);
        d.remaining -= to_write;
        if (!d.remaining) {
            if (len != to_write) {
                ESP_LOGW(HTTP_TAG,"ignoring %" PRIu16 " extra bytes",
                        len - to_write);
            }
            d.s = MW_HTTP_ST_FINISH_WAIT;
        }
        break;

    case MW_HTTP_ST_CERT_SET:
        // Save cert to allocated slot in flash
        http_cert_flash_write(data, len);
        break;

    default:
        ESP_LOGE(HTTP_TAG,"unexpected HTTP write attempt at state %d", d.s);
        break;
    }
}

esp_http_client_handle_t Http::http_init_int(const char *url, const char *cert_pem,
            http_event_handle_cb event_cb)
    {
        esp_http_client_config_t config = {
            .url = url,
            .cert_pem = cert_pem,
            .timeout_ms = 60000//,	// 1 minute default timeout
           // .transport_type = HTTP_TRANSPORT_OVER_SSL
        };

        if (event_cb) {
            config.event_handler = event_cb;
        }

        return esp_http_client_init(&config);
    }

    int Http::http_url_set_int(esp_http_client_handle_t client,
            const char *url)
    {
        
        ESP_LOGI(HTTP_TAG,"init, HTTP URL: %s", url);
        return esp_http_client_set_url(client, url);
    }

    int Http::http_method_set_int(esp_http_client_handle_t client,
            esp_http_client_method_t method)
    {
        return esp_http_client_set_method(client, method);
    }

    int Http::http_header_add_int(esp_http_client_handle_t client,
            const char *key, const char *value)
    {
        return esp_http_client_set_header(client, key, value);
    }

    int Http::http_header_del_int(esp_http_client_handle_t client,
            const char *key)
    {
        return esp_http_client_delete_header(client, key);
    }

    int Http::http_open_int(esp_http_client_handle_t client, int write_len)
    {
        return esp_http_client_open(client, write_len);
    }

    int Http::http_finish_int(esp_http_client_handle_t client, int32_t *data_len)
    {
        int len;

        len = esp_http_client_fetch_headers(client);
        if (ESP_FAIL == len) {
            return ESP_FAIL;
        }
        if (!len && esp_http_client_is_chunked_response(client)) {
            len = INT32_MAX;
        }
        if (data_len) {
            *data_len = len;
        }
        return esp_http_client_get_status_code(client);
    }

    int Http::http_cleanup_int(esp_http_client_handle_t client)
    {
        return esp_http_client_cleanup(client);
    }