#include <string.h>
#include <mbedtls/sha1.h>
#include "mw/game_api.h"
#include "mw/util.h"

/// \todo Module is slow because of te continuous realloc and copy. Optimize.

#define GAMEAPI_TAG "GAMEAPI"
#define STR_LEN(string)	(sizeof(string) - 1)

#define SHA1_BYTES 20
#define SHA1_DIGITS (SHA1_BYTES * 2)
#define SIGNATURE_STR "signature="
// Separator plus signature string plush SHA signature digits
#define SIGNATURE_EXTRA_LEN (1 + STR_LEN(SIGNATURE_STR) + SHA1_DIGITS)

static struct {
	char *endpoint;
	char *keyval;
	char *priv_key;
	int endpoint_len;
	int keyval_len;
	int priv_key_len;
} ga = {};

void GameApi::ga_init()
{
	ga_deinit();
}

void GameApi::ga_deinit(void)
{
	free(ga.endpoint);
	free(ga.keyval);
	free(ga.priv_key);
	memset(&ga, 0, sizeof(ga));
}

bool GameApi::ga_endpoint_set(const char *endpoint, const char *private_key)
{
	free(ga.endpoint);
	ga.endpoint_len = strlen(endpoint);
	ga.endpoint = (char*)malloc(ga.endpoint_len + 1);
	ga.priv_key_len = strlen(private_key);
	ga.priv_key = (char*)malloc(ga.priv_key_len + 1);
	if (!ga.endpoint || !ga.priv_key) {
		free(ga.endpoint);
		free(ga.priv_key);
		ga.endpoint = NULL;
		ga.priv_key = NULL;
		ga.endpoint_len = 0;
		ga.priv_key_len = 0;
		return true;
	}

	memcpy(ga.endpoint, endpoint, ga.endpoint_len + 1);
	memcpy(ga.priv_key, private_key, ga.priv_key_len + 1);

	return false;
}

bool GameApi::ga_private_key_set(const char *private_key)
{
	free(ga.priv_key);

	ga.priv_key_len = strlen(private_key);
	ga.priv_key = (char*)malloc(ga.priv_key_len + 1);
	if (!ga.priv_key) {
		return true;
	}

	memcpy(ga.priv_key, private_key, ga.priv_key_len + 1);

	return false;
}

static int path_urlencode_add(char **dest, const char *path) {
	const int len = strlen(path);
	char *str_urlenc = (char *)malloc(3 * len + 1);
	char *aux = NULL;
	int result = -1;
	int aux_len;

	if (str_urlenc) {
        const int enc_len = urlencode(str_urlenc, path);

        if (*dest) {
            aux_len = asprintf(&aux, "%s%s/", *dest ?  *dest : "",
                    str_urlenc);
        } else {
            aux_len = enc_len;
            aux = (char*)malloc(enc_len + 1);
            memcpy(aux, str_urlenc, enc_len + 1);
        }
	}
    
    if (-1 != aux_len && aux) {        
        free(*dest);
        *dest = aux;
        result = aux_len;
    }

	free(str_urlenc);
	return result;
}

static int key_val_urlencode_add(char **dest, const char *key, const char *value)
{
	const int key_len = strlen(key);
	const int val_len = strlen(value);
	char *aux = NULL;
	char *key_urlenc = (char*)malloc(3 * key_len + 1);
	char *val_urlenc = (char*)malloc(3 * val_len + 1);
	int result = -1;
    int aux_len = -1;
	if (key_urlenc && val_urlenc) {
		urlencode(key_urlenc, key);
        urlencode(val_urlenc, value);

        aux_len = asprintf(&aux, "%s&%s=%s", *dest ? *dest : "", key_urlenc, val_urlenc);
	}
    

    if (-1 != aux_len && aux) {
        free(*dest);
        *dest = aux;
        result = aux_len;
    }
	free(key_urlenc);
	free(val_urlenc);
	return result;
}

bool GameApi::ga_key_value_add(const char *key, const char *value)
{
	bool err = false;
	const int len = key_val_urlencode_add(&ga.keyval, key, value);

	if (len >= 0) {
		ga.keyval_len = len;
		ESP_LOGD(GAMEAPI_TAG,"%s, %d bytes", ga.keyval, ga.keyval_len);
	} else {
		ESP_LOGE(GAMEAPI_TAG,"failed");
		err = true;
	}

	return err;
}

static int url_alloc_build(char **url, uint8_t num_paths, uint8_t num_kv_pairs,
		const char *data)
{
	int pos = 0;
	int url_len = ga.endpoint_len;

	*url = (char *)malloc(ga.endpoint_len + 1);
	memcpy(*url, ga.endpoint, ga.endpoint_len + 1);
	for (int i = 0; i < num_paths; i++) {
		if (!data[pos]) {
			return -1;
		}
		url_len = path_urlencode_add(url, data + pos);
		if (url_len < 0) {
			return url_len;
		}
		pos += strlen(data + pos) + 1;
	}

	// Offset of key/value pairs starts here
	const int kv_start_off = url_len;

	for (int i = 0; i < num_kv_pairs; i++) {
		const char *key = data + pos;
		if (!*key) {
			return -1;
		}
		pos += strlen(key) + 1;
		const char *value = data + pos;
		if (!*value) {
			return -1;
		}
		pos += strlen(value) + 1;
		url_len = key_val_urlencode_add(url, key, value);
		if (url_len < 0) {
			return url_len;
		}
	}

	if (ga.keyval) {
		char *aux = NULL;
		url_len = asprintf(&aux, "%s%s", *url, ga.keyval);
		if (url_len < 0 || !aux) {
			return -1;
		}
		free(*url);
		*url = aux;
	}
	// Replace '&' with '?' at the beginning of the key/val pairs
	if (num_kv_pairs || ga.keyval) {
		(*url)[kv_start_off] = '?';
	}

	return url_len;
}

static void signature_append(char **url, int url_len)
{
	mbedtls_sha1_context sha1_handle;
	uint8_t sha1[SHA1_BYTES] = {0};
	char sha1_str[SHA1_DIGITS + 1] = {0};
	char *aux = NULL;
	int aux_len;

	mbedtls_sha1_init(&sha1_handle);
	mbedtls_sha1_update(&sha1_handle, (unsigned char*)*url, url_len);
	mbedtls_sha1_update(&sha1_handle, (unsigned char*)ga.priv_key, ga.priv_key_len);
	mbedtls_sha1_finish(&sha1_handle, sha1);

	sha1_to_str(sha1, sha1_str);
	// FIXME assumes URL has at least 1 parameter. Separator should be
	// '?' if no parameters present
	aux_len = asprintf(&aux, "%s&" SIGNATURE_STR "%s", *url, sha1_str);
	if (aux_len < 0 || !aux) {
		free(aux);
		return;
	}
	free(*url);
	*url = aux;
}

uint16_t GameApi::ga_request(esp_http_client_method_t method, uint8_t num_paths,
		uint8_t num_kv_pairs, const char *data, int32_t *body_len)
{
	char *url = NULL;
	const int url_len = url_alloc_build(&url, num_paths, num_kv_pairs,
			data);
	uint16_t status = 0;

	if (url_len < 0 || !url) {
		free(url);
		ESP_LOGE(GAMEAPI_TAG,"failed to build URL");
		return 0;
	}

	signature_append(&url, url_len);

	http->http_url_set(url);
	http->http_method_set(method);
	http->http_open(0);
	http->http_finish(&status, body_len);
	ESP_LOGD(GAMEAPI_TAG,"finished with code=%"PRIu16", length=%"PRId32, status, *body_len);

	free(url);

	return status;
}

