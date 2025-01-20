/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#pragma once
#include "mw/net_util.h"
#include "ping/ping_sock.h"

#define PING_TAG "PING"

class Ping {
public:


    /** \addtogroup MwApi MwPingResult Possible results of the ping state machine.
     *  \{ */
    typedef enum {
        MW_PING_RESULT_END = 0,		///< Initialization state and finish.
        MW_PING_RESULT_SUCCESS,		///< Success Result on Ping.
        MW_PING_RESULT_TIMEOUT,		///< Timeout Result on Ping.
    } MwPingResult;
    /** \} */

    typedef void (*mw_ping_callback_t)(MwPingResult result, void *args);

	Ping(){};

    esp_ping_callbacks_t cbs;
    esp_ping_config_t ping_config;

    void init(void *args, mw_ping_callback_t cb);

    esp_err_t ping(const char *url, uint32_t count);

	static void test_on_ping_success(esp_ping_handle_t hdl, void *args);

    static void test_on_ping_timeout(esp_ping_handle_t hdl, void *args);

    static void test_on_ping_end(esp_ping_handle_t hdl, void *args);

private:
};

static Ping::mw_ping_callback_t mw_cb;