#include "mw/upgrade.h"

static size_t append(char * dst, size_t pos, const char * org)
{
	size_t to_copy = MIN(strlen(org), URL_MAX_LEN - pos);

	memcpy(dst + pos, org, to_copy + 1);

	return pos + to_copy;
}

esp_err_t Upgrade::upgrade_firmware(const char *server, const char *name)
{
	char url[URL_MAX_LEN + 1] = "http://";
	int pos = 7;
	esp_err_t err;

	// Build URL. E.g.: "https://doragasu.com/mw_update/mw_rtos_std_v1.4.0"
	pos = append(url, pos, server);
	pos = append(url, pos, UPDATE_EP);
	pos = append(url, pos, name);

	ESP_LOGI(UPGRADE_TAG,"upgrading from %s", url);
	const esp_http_client_config_t http_cfg = {
		.url = url,
		.cert_pem = NULL, // Not secure!
		.keep_alive_enable = true
	};

	esp_https_ota_config_t ota_cfg = {
		.http_config = &http_cfg,
	};

	err = esp_https_ota_begin(&ota_cfg, handle);
    if(!err){        
        err = esp_https_ota_perform(handle);
    }

	if (err) {
		ESP_LOGE(UPGRADE_TAG,"upgrade failed with code %d", err);
	} else {
        while (!esp_https_ota_is_complete_data_received(handle)){
            ESP_LOGI(UPGRADE_TAG,"dowloading...");
            vTaskDelayMs(1000);            
        }
		ESP_LOGI(UPGRADE_TAG,"upgrade succeeded");
        err = esp_https_ota_finish(handle);
	}

	return err;
}

