#pragma once 

#include "mw/Http.h"

class GameApi {

public:
    GameApi(){}

    GameApi(Http *http){
        this->http = http;
    };

    Http *http = NULL;
    
    void ga_init();

    void ga_deinit(void);

    bool ga_endpoint_set(const char *endpoint, const char *private_key);

    bool ga_private_key_set(const char *private_key);

    bool ga_key_value_add(const char *key, const char *value);

    uint16_t ga_request(esp_http_client_method_t method, uint8_t num_paths,
		uint8_t num_kv_pairs, const char *data, int32_t *body_len);

};