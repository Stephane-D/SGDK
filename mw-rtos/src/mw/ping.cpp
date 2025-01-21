/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#include "mw/ping.h"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE //CORE_DEBUG_LEVEL

void Ping::init(void *args, mw_ping_callback_t cb){
    ping_config = ESP_PING_DEFAULT_CONFIG();
    //ping_config.count = ESP_PING_COUNT_INFINITE;    // ping in infinite mode, esp_ping_stop can stop it

    /* set callback functions */        
    cbs.on_ping_success = test_on_ping_success;
    cbs.on_ping_timeout = test_on_ping_timeout;
    cbs.on_ping_end = test_on_ping_end;
    cbs.cb_args = args;  
    mw_cb = cb;
}

esp_err_t Ping::ping(const char *url, uint32_t count){        
    ip_addr_t target_addr;
    struct addrinfo *res = NULL;
    memset(&target_addr, 0, sizeof(target_addr));
    net_dns_lookup(url, NULL, &res);
    struct in_addr addr4 = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
    inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    freeaddrinfo(res);

    ping_config.count = count;
    ping_config.target_addr = target_addr;          // target IP address  
    char ip_str[16];
    ipv4_to_str(target_addr.u_addr.ip4.addr, ip_str);
	ESP_LOGI(PING_TAG,"sending ping to %s(%s) retries: %u", url, ip_str, count);
    esp_ping_handle_t ping;
    esp_err_t err = esp_ping_new_session(&ping_config, &cbs, &ping);
    if(err) goto err;     
    err = esp_ping_start(ping); 
    if(err) goto err;     
    return ESP_OK;
err:
    ESP_LOGE(PING_TAG,"Error on Ping request: %u",err);
    return err;
}

void Ping::test_on_ping_success(esp_ping_handle_t hdl, void *args)
{
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    ESP_LOGI(PING_TAG,"%d bytes from %s icmp_seq=%d ttl=%d time=%d ms\n",
        recv_len, inet_ntoa(target_addr.u_addr.ip4), seqno, ttl, elapsed_time);
    mw_cb(MW_PING_RESULT_SUCCESS, args);
}

void Ping::test_on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    uint16_t seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    ESP_LOGI(PING_TAG,"From %s icmp_seq=%d timeout\n", inet_ntoa(target_addr.u_addr.ip4), seqno);
    mw_cb(MW_PING_RESULT_TIMEOUT, args);
}

void Ping::test_on_ping_end(esp_ping_handle_t hdl, void *args)
{
    MwMsgPingStat stat;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &(stat.transmitted), sizeof(stat.transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &(stat.received), sizeof(stat.received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &(stat.total_time_ms), sizeof(stat.total_time_ms));
    ESP_LOGI(PING_TAG,"%d packets transmitted, %d received, time %dms\n", stat.transmitted, stat.received, stat.total_time_ms);
    mw_cb(MW_PING_RESULT_END, &stat);
}