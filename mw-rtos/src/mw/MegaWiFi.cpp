#include "mw/MegaWiFi.h"
#define LOG_LOCAL_LEVEL CORE_DEBUG_LEVEL //CORE_DEBUG_LEVEL

void MegaWiFi::time_sync_cb(struct timeval *tv)
{
        UNUSED_PARAM(tv);

        if (instance_p) {
			instance_p->d.s.dt_ok = true;
			ESP_LOGI(MW_TAG, "date/time set");
		}
}

int MegaWiFi::MwInit() {
	MwFsmMsg m;
	int i;
	
	instance_p = this;

	memset(&d, 0, sizeof(d));

	http = new Http(this->lsd);
	ga = new GameApi(this->http);
    led = new Led(&(this->d.s));

	if (flash->flash_init()) {
		ESP_LOGE(MW_TAG,"could not initialize user data partition");
		return 1;
	}

	if (http->http_module_init((char*)buf)) {
		ESP_LOGE(MW_TAG,"http module initialization failed");
		return 1;
	}

	if (!(d.p_cfg = esp_partition_find_first(MW_DATA_PART_TYPE,
					MW_CFG_PART_SUBTYPE,
					MW_CFG_PART_LABEL))) {
		ESP_LOGE(MW_TAG,"config partition initialization failed");
		return 1;
	}
	if (sizeof(MwNvCfg) > d.p_cfg->size) {
		ESP_LOGE(MW_TAG,"config length too big (%d)", sizeof(MwNvCfg));
		return 1;
	}
	print_flash_id();

	// Load configuration from flash
	MwCfgLoad();
	wifi_cfg_log();
	// Set default values for global variables
	d.phy = MW_PHY_PROTO_DEF;
	for (i = 0; i < MW_MAX_SOCK; i++) {
		d.sock[i] = -1;
		d.chan[i] = -1;
	}
    
	// Create system queue
	if (!(d.q = xQueueCreate(MW_FSM_QUEUE_LEN, sizeof(MwFsmMsg)))) {
		ESP_LOGE(MW_TAG,"could not create system queue!");
        ESP_LOGE(MW_TAG,"fatal error during initialization");
        return 1;
	};
    // Create system queue
	if (!(d.qtx = xQueueCreate(MW_FSM_QUEUE_LEN, sizeof(MegaDeviceX7TxMsg)))) {
		ESP_LOGE(MW_TAG,"could not create system queue!");
        ESP_LOGE(MW_TAG,"fatal error during initialization");
        return 1;
	};

	// Init WiFi subsystem
	wifi_init();
	d.s.sys_stat = MW_ST_IDLE;

	if(d.s.cfg_ok && cfg.defaultAp > -1){
		MwApJoin(cfg.defaultAp);
	}

  	// Create FSM task
    ESP_LOGD(MW_TAG,"Init MwFsmTsk");
	if (pdPASS != xTaskCreate(MegaWiFi::MwFsmTsk, "FSM", MW_FSM_STACK_LEN, this,
			MW_FSM_PRIO, NULL)) {
		ESP_LOGE(MW_TAG,"could not create Fsm task!");		
        ESP_LOGE(MW_TAG,"fatal error during initialization");
        return 1;
	}
	// Create task for receiving data from sockets
    ESP_LOGD(MW_TAG,"Init MwFsmSockTsk");
	if (pdPASS != xTaskCreate(MegaWiFi::MwFsmSockTsk, "SCK", MW_SOCK_STACK_LEN, this,
			MW_SOCK_PRIO, NULL)) {
		ESP_LOGE(MW_TAG,"could not create FsmSock task!");
        ESP_LOGE(MW_TAG,"fatal error during initialization");
        return 1;
	}
	//Pre Initialize SNTP 
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_set_time_sync_notification_cb(MegaWiFi::time_sync_cb);
	// Initialize LSD layer (will create receive task among other stuff).
    ESP_LOGD(MW_TAG,"Init LSD");
	lsd->LsdInit(d.q, d.qtx);
	lsd->LsdChEnable(MW_CTRL_CH);
	ga->ga_init();
    led->led_init();
	// Send the init done message
	m.e = MW_EV_INIT_DONE;
	xQueueSend(d.q, &m, portMAX_DELAY);
	// Start the one-shot inactivity sleep timer
	/*
	d.tim = xTimerCreate("SLEEP", MW_SLEEP_TIMER_MS / portTICK_PERIOD_MS,
			pdFALSE, (void*) 0, sleep_timer_cb);
	xTimerStart(d.tim, MW_SLEEP_TIMER_MS / portTICK_PERIOD_MS);
	*/
	return 0;
}

#define NET_EVENT_BASE_LEN (sizeof(esp_event_base_t) + sizeof(int32_t))
void MegaWiFi::event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)

{
	MegaWiFi* mw = (MegaWiFi*)arg;
	MwFsmMsg msg;
	uint8_t data_len;

	if (!arg) {
		ESP_LOGE(MW_TAG,"ctx");
		return;
	}

	if (WIFI_EVENT == event_base) {
		switch (event_id) {
		case WIFI_EVENT_STA_START:
		case WIFI_EVENT_STA_STOP:
			data_len = 0;
			break;

		case WIFI_EVENT_STA_CONNECTED:
			data_len = sizeof(wifi_event_sta_connected_t);
			break;

		case WIFI_EVENT_STA_DISCONNECTED:
			data_len = sizeof(wifi_event_sta_connected_t);
			break;

		default:
			ESP_LOGE(MW_TAG,"unsupported wifi event: %"PRId32, event_id);
			return;

		}
	} else if (IP_EVENT == event_base) {
		switch (event_id) {
		case IP_EVENT_STA_GOT_IP:
			data_len = sizeof(ip_event_got_ip_t);
			break;

		default:
			ESP_LOGE(MW_TAG,"unsuported IP event %"PRId32, event_id);
			return;
		}
	} else {
		ESP_LOGE(MW_TAG,"unsupported event_base %p", event_base);
		return;
	}

	struct net_event *evt =(net_event *) malloc(NET_EVENT_BASE_LEN + data_len);
	if (data_len) {
		memcpy(&evt->got_ip, event_data, data_len);
	}
	evt->event_base = event_base;
	evt->event_id = event_id;

	// Forward event to sysfsm
	msg.e = MW_EV_NET;
	msg.d = evt;

	xQueueSend(mw->d.q, &msg, portMAX_DELAY);
}

size_t MegaWiFi::MwFsm(MwFsmMsg *msg) {
	MwMsgBuf *b = (MwMsgBuf *)(msg->d);
	MwCmd *rep;

	size_t data_out_len = 0;

	switch (d.s.sys_stat) {
		case MW_ST_INIT:
			// Ignore all events excepting the INIT DONE one
			if (msg->e == MW_EV_INIT_DONE) {
				ESP_LOGI(MW_TAG,"INIT DONE!");				
			}
			break;

		case MW_ST_AP_JOIN:
			if (MW_EV_NET == msg->e) {
				ap_join_ev_handler((struct net_event *)(msg->d));
				
			} else if (MW_EV_SER_RX == msg->e) {
				// The only rx events supported during AP_JOIN are AP_LEAVE,
				// VERSION_GET and SYS_STAT
				if (MW_CMD_AP_LEAVE == (b->cmd.cmd>>8)) {
					data_out_len = MwFsmCmdProc((MwCmd*)b, b->len);
				} else if (MW_CMD_VERSION == (b->cmd.cmd>>8)) {
					rep = (MwCmd*)msg->d;
					rep->cmd = MW_CMD_OK;
					rep->datalen = ByteSwapWord(2 + sizeof(MW_FW_VARIANT) - 1);
					rep->data[0] = MW_FW_VERSION_MAJOR;
					rep->data[1] = MW_FW_VERSION_MINOR;
					memcpy(rep->data + 2, MW_FW_VARIANT,
							sizeof(MW_FW_VARIANT) - 1);
					data_out_len = lsd->LsdSend((uint8_t*)rep, ByteSwapWord(rep->datalen) +
							MW_CMD_HEADLEN, 0);
				} else if (MW_CMD_SYS_STAT == (b->cmd.cmd>>8)) {
					rep = (MwCmd*)msg->d;
					rep->cmd = MW_CMD_OK;
					MwSysStatFill(rep);
					ESP_LOGD(MW_TAG,"%02X %02X %02X %02X", rep->data[0],
							rep->data[1], rep->data[2], rep->data[3]);
					data_out_len = lsd->LsdSend((uint8_t*)rep, sizeof(MwMsgSysStat) + MW_CMD_HEADLEN,
							0);
				} else {
					ESP_LOGE(MW_TAG,"Command %d not allowed on AP_JOIN state",
							b->cmd.cmd>>8);
				}
			}
			break;

		case MW_ST_IDLE:
			// IDLE state is abandoned once connected to an AP
			if (MW_EV_SER_RX == msg->e) {
				// Parse commands on channel 0 only
				ESP_LOGD(MW_TAG,"Serial recvd %d bytes.", b->len);
				if (MW_CTRL_CH == b->ch) {
					// Check command is allowed on IDLE state
					if (MwCmdInList(b->cmd.cmd>>8, idleCmdMask)) {
						data_out_len = MwFsmCmdProc((MwCmd*)b, b->len);
					} else {
						ESP_LOGE(MW_TAG,"Command %d not allowed on IDLE state",
								b->cmd.cmd>>8);
						// TODO: Throw error event?
						rep = (MwCmd*)msg->d;
						rep->datalen = 0;
						rep->cmd = ByteSwapWord(MW_CMD_ERROR);
						data_out_len = lsd->LsdSend((uint8_t*)rep, MW_CMD_HEADLEN, 0);
					}
				} else {
					ESP_LOGE(MW_TAG,"IDLE received data on non ctrl channel!");
				}
			}
			break;

		case MW_ST_READY:
			// Module ready. Here we will have most of the activity
			MwFsmReady(msg);
			// NOTE: LSD channel 0 is treated as a special control channel.
			// All other channels are socket-bridged.
			
			break;

		case MW_ST_TRANSPARENT:
			ESP_LOGI(MW_TAG,"TRANSPARENT!");
			break;
		default:
			break;
	}
	// Free WiFi event
	if (MW_EV_NET == msg->e) {
		free(msg->d);
	}
	return data_out_len;
}

// Process messages during ready stage
void MegaWiFi::MwFsmReady(MwFsmMsg *msg) {
	// Pointer to the message buffer (from RX line).
	MwMsgBuf *b = (MwMsgBuf *)msg->d;
	MwCmd *rep;
	struct net_event *net = (struct net_event *)msg->d;

	switch (msg->e) {
	case MW_EV_NET:		///< WiFi events, excluding scan related.
		ESP_LOGI(MW_TAG,"WIFI_EVENT %"PRId32" (not parsed)", net->event_id);
		break;

	case MW_EV_SER_RX:		///< Data reception from serial line.
		ESP_LOGD("Serial recvd %"PRIu16" bytes.", b->len);
		// If using channel 0, process command. Else forward message
		// to the appropiate socket.
		if (MW_CTRL_CH == b->ch) {
			// Check command is allowed on READY state
			if (MwCmdInList(b->cmd.cmd>>8, readyCmdMask)) {
				MwFsmCmdProc((MwCmd*)b, b->len);
			} else {
				ESP_LOGE(MW_TAG,"Command %"PRIu16" not allowed on READY state",
						b->cmd.cmd>>8);
				// TODO: Throw error event?
				rep = (MwCmd*)msg->d;
				rep->datalen = 0;
				rep->cmd = ByteSwapWord(MW_CMD_ERROR);
				lsd->LsdSend((uint8_t*)rep, MW_CMD_HEADLEN, 0);
			}
		} else if (MW_HTTP_CH == b->ch) {
			// Process channel using HTTP state machine
			http->http_send((char*)b->data, b->len);
		} else {
			// Forward message if channel is enabled.
			if (b->ch < LSD_MAX_CH && d.ss[b->ch - 1]) {
				if (MwSend(b->ch, b->data, b->len) != b->len) {
					ESP_LOGE(MW_TAG,"ch %"PRIu8" socket send error!", b->ch);
					// TODO throw error event?
					rep = (MwCmd*)msg->d;
					rep->datalen = 0;
					rep->cmd = ByteSwapWord(MW_CMD_ERROR);
					lsd->LsdSend((uint8_t*)rep, MW_CMD_HEADLEN, 0);
				}
			} else {
				ESP_LOGE(MW_TAG,"Requested to forward data on wrong channel: %"
						PRIu8, b->ch);
			}
		}
		break;

	default:
		ESP_LOGI(MW_TAG,"UNKNOKWN EVENT %d", msg->e);
		break;
	}
}

int MegaWiFi::MwSend(int ch, const void *data, int len) {
	int idx = ch - 1;
	int s = d.sock[idx];

	switch (d.ss[idx]) {
	case MW_SOCK_TCP_EST:
		return lwip_send(s, data, len, 0);

	case MW_SOCK_UDP_READY:
		return MwUdpSend(idx, data, len);
		break;

	default:
		return -1;
	}
}

int MegaWiFi::MwUdpSend(int idx, const void *data, int len) {
	struct sockaddr_in remote;
	int s = d.sock[idx];
	int sent;

	if (d.raddr[idx].sin_addr.s_addr != lwip_htonl(INADDR_ANY)) {
		sent = lwip_sendto(s, data, len, 0, (struct sockaddr*)
				&d.raddr[idx], sizeof(struct sockaddr_in));
	} else {
		// Reuse mode, extract address from leading bytes
		// NOTE: d.raddr[idx].sin_addr.s_addr == INADDR_ANY
		remote.sin_addr.s_addr = *((int32_t*)data);
		remote.sin_port = *((int16_t*)((int16_t*)data + 4));
		remote.sin_family = AF_INET;
		remote.sin_len = sizeof(struct sockaddr_in);
		memset(remote.sin_zero, 0, sizeof(remote.sin_zero));
		sent = lwip_sendto(s, (int16_t*)data + 6, len - 6, 0, (struct sockaddr*)
				&remote, sizeof(struct sockaddr_in)) + 6;
	}

	return sent;
}

void MegaWiFi::ap_join_ev_handler(struct net_event *net)
{
	ip4_addr_t addr;
	ESP_LOGD(MW_TAG,"WiFi event: %d", net->event_id);
	switch(net->event_id) {
		case WIFI_EVENT_STA_START:
			ESP_LOGI(MW_TAG,"setting mode %x", d.phy);
			esp_wifi_set_protocol((wifi_interface_t)ESP_IF_WIFI_STA, d.phy);
			esp_wifi_connect();
			break;

		case WIFI_EVENT_STA_STOP:
			break;

		case IP_EVENT_STA_GOT_IP:
			addr.addr = net->got_ip.ip_info.ip.addr;
			ESP_LOGI(MW_TAG,"got IP: %s, READY!", ip4addr_ntoa(&addr));
			d.s.sys_stat = MW_ST_READY;			
			sntp_set_config();
			sntp_init();
			d.s.online = TRUE;
			break;

		case WIFI_EVENT_STA_CONNECTED:
			ESP_LOGD(MW_TAG,"station:"MACSTR" join",
					MAC2STR(net->event_info.connected.bssid));
			break;

		case WIFI_EVENT_STA_DISCONNECTED:
			d.n_reassoc++;
			ESP_LOGE(MW_TAG,"Disconnect %d, reason : %d", d.n_reassoc,
					net->sta_disconnected.reason);
			sntp_stop();
			if (d.n_reassoc < MW_REASSOC_MAX) {
				esp_wifi_connect();
			} else {
				ESP_LOGE(MW_TAG,"Too many assoc attempts, dessisting");
				esp_wifi_disconnect();
				d.s.sys_stat = MW_ST_IDLE;
			}
			break;

		default:
			ESP_LOGE(MW_TAG,"unhandled event %d, connect failed, IDLE!",
					net->event_id);
			esp_wifi_disconnect();
			d.s.sys_stat = MW_ST_IDLE;
	}
}

int MegaWiFi::MwCfgLoad(void) {
	uint8_t md5[16];
	// Load configuration from flash
    ESP_LOGD(MW_TAG,"Load configuration from flash %x", d.p_cfg);
	esp_partition_read(d.p_cfg, 0, &cfg, sizeof(MwNvCfg));
	// Check MD5
	mbedtls_md5((const unsigned char*)&cfg, ((uint32_t)&cfg.md5) - ((uint32_t)&cfg), md5);
	if (!memcmp(cfg.md5, md5, 16)) {
		// MD5 test passed, return with loaded configuration
		d.s.cfg_ok = TRUE;
		ESP_LOGI(MW_TAG,"configuration loaded from flash.");
		return 0;
	}
#ifdef _DEBUG_MSGS
	unsigned char md5_str[33];
	mbedtls_md5((const unsigned char*)cfg.md5, 16, md5_str);
	ESP_LOGI(MW_TAG,"loaded MD5:   %s", md5_str);
	mbedtls_md5((const unsigned char*)md5, 16, md5_str);
	ESP_LOGI(MW_TAG,"computed MD5: %s", md5_str);
#endif
	// MD5 did not pass, load default configuration
	MwSetDefaultCfg();
	ESP_LOGI(MW_TAG,"loaded default configuration.");
	return 1;
}

void MegaWiFi::MwSetDefaultCfg(void) {
	memset(&cfg, 0, sizeof(cfg));
	cfg.defaultAp = -1;
	// Default WiFi config, from menuconfig parameters
	cfg.wifi.ampdu_rx_enable = WIFI_AMPDU_RX_ENABLED;

	// Copy the default timezone and NTP servers
	*(1 + StrCpyDst(1 + StrCpyDst(1 + StrCpyDst(1 + StrCpyDst(cfg.ntpPool,
		MW_TZ_DEF), MW_SNTP_SERV_0), MW_SNTP_SERV_1), MW_SNTP_SERV_2)) =
		'\0';
	cfg.ntpPoolLen = sizeof(MW_TZ_DEF) + sizeof(MW_SNTP_SERV_0) +
		sizeof(MW_SNTP_SERV_1) + sizeof(MW_SNTP_SERV_2) + 1;
	strcpy(cfg.serverUrl, MW_SERVER_DEFAULT);
	cfg.ap[0].phy = cfg.ap[1].phy = cfg.ap[2].phy = MW_PHY_PROTO_DEF;
}

#define WIFI_CFG_FROM_NVS(param)	wifi_cfg.param = cfg.wifi.param
esp_err_t MegaWiFi::wifi_init(){
    esp_err_t err;
	wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
	WIFI_CFG_FROM_NVS(ampdu_rx_enable);	
	//WIFI_CFG_FROM_NVS(rx_ba_win);
	esp_netif_init();
	esp_event_loop_create_default();		
	err = esp_wifi_init(&wifi_cfg);
	if (err) {
		ESP_LOGE(MW_TAG,"wifi init failed: %s", esp_err_to_name(err));
	}else{
		//WIFI_STORAGE_FLASH
		esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, this);
		esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, this);

		esp_netif_create_default_wifi_sta();

		esp_wifi_set_storage(WIFI_STORAGE_RAM);
		esp_wifi_set_mode(WIFI_MODE_STA);
	}
	return err;
}

void MegaWiFi::MwFsmTsk(void *pvParameters) {
	MegaWiFi *mw = (MegaWiFi *)pvParameters;
	MwFsmMsg m;

	while(1) {
		if (xQueueReceive(mw->d.q, &m, 1000)) {
			ESP_LOGD(MW_TAG,"Recv msg, evt=%d", m.e);
			mw->MwFsm(&m);
			// If event was MW_EV_SER_RX, free the buffer
			mw->lsd->LsdRxBufFree();
		} else {
			// Timeout
			ESP_LOGD(MW_TAG,".");
		}
	}
}

/// Polls sockets for data or incoming connections using select()
void MegaWiFi::MwFsmSockTsk(void *pvParameters) {
	fd_set readset;
	int i, ch, retval;
	int max;
	ssize_t recvd;
	struct timeval tv = {
		.tv_sec = 1,
		.tv_usec = 0
	};
	MegaWiFi *mw = (MegaWiFi *)pvParameters;
	FD_ZERO(&readset);
	mw->d.fdMax = -1;

	while (1) {
		mw->led->led_toggle();
		// Update list of active sockets
		readset = mw->d.fds;

		// Wait until event or timeout
		// TODO: mw->d.fdMax is initialized to -1. How does select() behave if
		// nfds = 0?
		ESP_LOGD(MW_TAG,".");
		if ((retval = select(mw->d.fdMax + 1, &readset, NULL, NULL, &tv)) < 0) {
			// Error.
			ESP_LOGE(MW_TAG,"select() completed with error!");
			vTaskDelayMs(1000);
			continue;
		}
		// If select returned 0, there was a timeout
		if (0 == retval) continue;
		// Poll the socket for data, and forward through the associated
		// channel.
		max = mw->d.fdMax;
		for (i = LWIP_SOCKET_OFFSET; i <= max; i++) {
			if (FD_ISSET(i, &readset)) {
				// Check if new connection or data received
				ch = mw->d.chan[i - LWIP_SOCKET_OFFSET];
				if (mw->d.ss[ch - 1] != MW_SOCK_TCP_LISTEN) {
					ESP_LOGD(MW_TAG,"Rx: sock=%d, ch=%d", i, ch);
					if ((recvd = mw->MwRecv(ch, (char*)mw->buf, LSD_MAX_LEN)) < 0) {
						// Error!
						mw->MwSockClose(ch);
						mw->lsd->LsdChDisable(ch);
						ESP_LOGE(MW_TAG,"Error %d receiving from socket!", recvd);
					} else if (0 == recvd) {
						// Socket closed
						// A listen on a socket closed, should trigger
						// a 0-byte reception, for the client to be able to
						// check server state and close the connection.
						ESP_LOGD(MW_TAG,"Received 0!");
						mw->MwSockClose(ch);
						ESP_LOGE(MW_TAG,"Socket closed!");
						mw->MwFsmRaiseChEvent(ch);
						// Send a 0-byte frame for the receiver to wake up and
						// notice the socket close
						mw->lsd->LsdSend(NULL, 0, ch);
						mw->lsd->LsdChDisable(ch);
					} else {
						//ESP_LOGD(MW_TAG,"%02X %02X %02X %02X: WF->MD %d bytes",
						//		buf[0], buf[1], buf[2], buf[3],
						//		recvd);
						mw->lsd->LsdSend(mw->buf, (uint16_t)recvd, ch);
					}
				} else {
					// Incoming connection. Accept it.
					mw->MwAccept(i, ch);
					mw->MwFsmRaiseChEvent(ch);
				}
			}
		}
	} // while (1)
}

int MegaWiFi::MwRecv(int ch, char *buf, int len) {
	int idx = ch - 1;
	int s = d.sock[idx];
	// No IPv6 support yet
	ssize_t recvd;
	UNUSED_PARAM(len);

	switch(d.ss[idx]) {
		case MW_SOCK_TCP_EST:
			return lwip_recv(s, buf, LSD_MAX_LEN, 0);

		case MW_SOCK_UDP_READY:
			recvd = MwUdpRecv(idx, buf);
			return recvd;

		default:
			return -1;
	}
}

int MegaWiFi::MwUdpRecv(int idx, char *buf) {
	ssize_t recvd;
	int s = d.sock[idx];
	struct sockaddr_in remote;
	socklen_t addr_len = sizeof(remote);

	if (d.raddr[idx].sin_addr.s_addr != lwip_htonl(INADDR_ANY)) {
		// Receive only from specified address
		recvd = lwip_recvfrom(s, buf, LSD_MAX_LEN, 0,
				(struct sockaddr*)&remote, &addr_len);
		if (recvd > 0) {
			if (remote.sin_addr.s_addr != d.raddr[idx].sin_addr.s_addr) {
				ESP_LOGE(MW_TAG,"Discarding UDP packet from unknown addr");
				recvd = -1;
			}
		}
	} else {
		// Reuse mode, data is preceded by remote IPv4 and port
		recvd = lwip_recvfrom(s, buf + 6, LSD_MAX_LEN - 6, 0,
				(struct sockaddr*)&remote, &addr_len);
		if (recvd > 0) {
			*((uint32_t*)buf) = remote.sin_addr.s_addr;
			*((uint16_t*)(buf + 4)) = remote.sin_port;
			recvd += 6;
		}
	}

	return recvd;
}

/// Process command requests (coming from the serial line)
size_t MegaWiFi::MwFsmCmdProc(MwCmd *c, uint16_t totalLen) {
	MwCmd reply;
	uint16_t len = ByteSwapWord(c->datalen);
	time_t ts;	// For datetime replies
	uint16_t tmp, replen;

	size_t data_out_len = 0;
	
	// Sanity check: total Lengt - header length = data length
	if ((totalLen - MW_CMD_HEADLEN) != len) {
		ESP_LOGE(MW_TAG,"ERROR: Length inconsistent");
		ESP_LOGE(MW_TAG,"totalLen=%d, dataLen=%d", totalLen, len);
		return 0;
		//return MW_CMD_FMT_ERROR;
	}

	// parse command
	ESP_LOGI(MW_TAG,"CmdRequest: %d", ByteSwapWord(c->cmd));
	reply_set_ok_empty(&reply);
	switch (ByteSwapWord(c->cmd)) {
		case MW_CMD_VERSION:
			ESP_LOGI(MW_TAG,"CHEKING VERSION!");
			// Cancel sleep timer
            if (d.tim) {
				xTimerStop(d.tim, 0);
				xTimerDelete(d.tim, 0);
				d.tim = NULL;
			}
			reply.cmd = MW_CMD_OK;
			reply.datalen = ByteSwapWord(3 + sizeof(MW_FW_VARIANT));
			reply.data[0] = MW_FW_VERSION_MAJOR;
			reply.data[1] = MW_FW_VERSION_MINOR;
			reply.data[2] = MW_FW_VERSION_MICRO;
			memcpy(reply.data + 3, MW_FW_VARIANT, sizeof(MW_FW_VARIANT));
			data_out_len = lsd->LsdSend((uint8_t*)&reply, ByteSwapWord(reply.datalen) +
					MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_ECHO:		// Echo request
			reply.datalen = c->datalen;
			ESP_LOGI(MW_TAG,"SENDING ECHO!");
			// Send the command response
			if ((lsd->LsdSplitStart((uint8_t*)&reply, MW_CMD_HEADLEN,
					len + MW_CMD_HEADLEN, 0) == MW_CMD_HEADLEN) &&
					len) {
				// Send echoed data
				lsd->LsdSplitEnd(c->data, len);
			}
			break;
            
		case MW_CMD_AP_SCAN:{
	    	ESP_LOGI(MW_TAG,"SCAN!");
			int scan_len = wifi_scan(c->data[0], reply.data);
			if (scan_len <= 0) {
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			} else {
				reply.datalen = scan_len;
				reply.datalen = ByteSwapWord(reply.datalen);
			}
			lsd->LsdSend((uint8_t*)&reply, scan_len + MW_CMD_HEADLEN, 0);
			break;
        }

		case MW_CMD_AP_CFG:
	    	ESP_LOGI(MW_TAG,"AP_CONFIG!: %d %s %s", c->apCfg.cfgNum, c->apCfg.ssid, c->apCfg.pass);
			ap_cfg_set(c->apCfg.cfgNum, c->apCfg.phy_type, c->apCfg.ssid,
					c->apCfg.pass, &reply);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;
		case MW_CMD_AP_CFG_GET:
			tmp = c->apCfg.cfgNum;
			if (tmp >= MW_NUM_AP_CFGS) {
				ESP_LOGE(MW_TAG,"Requested AP for cfg %d!", tmp);
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
				replen = 0;
			} else {
				ESP_LOGI(MW_TAG,"Getting AP configuration %d...", tmp);
				replen = sizeof(MwMsgApCfg);
				reply.datalen = ByteSwapWord(sizeof(MwMsgApCfg));
				reply.apCfg.cfgNum = c->apCfg.cfgNum;
				strncpy(reply.apCfg.ssid, cfg.ap[tmp].ssid, MW_SSID_MAXLEN);
				strncpy(reply.apCfg.pass, cfg.ap[tmp].pass, MW_PASS_MAXLEN);
				reply.apCfg.phy_type = cfg.ap[tmp].phy;
				ESP_LOGI(MW_TAG,"phy: 0x%X, ssid: %s, pass: %s", reply.apCfg.phy_type,
						reply.apCfg.ssid, reply.apCfg.pass);
			} 
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + replen, 0);
			break;
		case MW_CMD_IP_CURRENT:
			replen = 0;
			ESP_LOGI(MW_TAG,"Getting current IP configuration...");
			replen = sizeof(MwMsgIpCfg);
			reply.datalen = ByteSwapWord(sizeof(MwMsgIpCfg));
			reply.ipCfg.cfgNum = 0;
			tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &reply.ipCfg.cfg);
			reply.ipCfg.dns1 = *dns_getserver(0);
			reply.ipCfg.dns2 = *dns_getserver(1);
			log_ip_cfg(&reply.ipCfg);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + replen, 0);
			break;

		case MW_CMD_IP_CFG:
			tmp = (uint8_t)c->ipCfg.cfgNum;
			if (tmp >= MW_NUM_AP_CFGS) {
				ESP_LOGE(MW_TAG,"Tried to set IP for cfg %d!", tmp);
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			} else {
				ESP_LOGI(MW_TAG,"Setting IP configuration %d...", tmp);
				cfg.ip[tmp] = c->ipCfg.cfg;
				cfg.dns[tmp][0] = c->ipCfg.dns1;
				cfg.dns[tmp][1] = c->ipCfg.dns2;
				log_ip_cfg(&c->ipCfg);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_IP_CFG_GET:
			tmp = c->ipCfg.cfgNum;
			replen = 0;
			if (tmp >= MW_NUM_AP_CFGS) {
				ESP_LOGE(MW_TAG,"Requested IP for cfg %d!", tmp);
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			} else {
				ESP_LOGI(MW_TAG,"Getting IP configuration %d...", tmp);
				replen = sizeof(MwMsgIpCfg);
				reply.datalen = ByteSwapWord(sizeof(MwMsgIpCfg));
				reply.ipCfg.cfgNum = c->ipCfg.cfgNum;
				reply.ipCfg.cfg = cfg.ip[tmp];
				reply.ipCfg.dns1 = cfg.dns[tmp][0];
				reply.ipCfg.dns2 = cfg.dns[tmp][1];
				log_ip_cfg(&reply.ipCfg);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + replen, 0);
			break;
		case MW_CMD_DEF_AP_CFG:
			tmp = c->data[0];
			if (tmp < MW_NUM_AP_CFGS) {
				cfg.defaultAp = tmp;
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_DEF_AP_CFG_GET:
			reply.datalen = ByteSwapWord(1);
			reply.data[0] = cfg.defaultAp;
			ESP_LOGI(MW_TAG,"Sending default AP: %d", cfg.defaultAp);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + 1, 0);
			break;
        
		case MW_CMD_AP_JOIN:
			// Start connecting to AP and jump to AP_JOIN state
			if ((c->data[0] >= MW_NUM_AP_CFGS) ||
					!(cfg.ap[c->data[0]].ssid[0])) {
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
				ESP_LOGE(MW_TAG,"Invalid AP_JOIN on config %d", c->data[0]);
			} else {
				MwApJoin(c->data[0]);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_AP_LEAVE:	// Leave access point
			ESP_LOGI(MW_TAG,"Disconnecting from AP");
			disconnect();
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;
		case MW_CMD_TCP_CON:
			ESP_LOGI(MW_TAG,"TRYING TO CONNECT TCP SOCKET...");
			if (MwFsmTcpCon(&c->inAddr) < 0) {
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_TCP_BIND:
			if (MwFsmTcpBind(&c->bind)) {
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_CLOSE:
			// If channel number OK, disconnect the socket on requested channel
			if ((c->data[0] > 0) && (c->data[0] <= LSD_MAX_CH) &&
					d.ss[c->data[0] - 1]) {
				ESP_LOGI(MW_TAG,"Closing socket %d from channel %d",
						d.sock[c->data[0] - 1], c->data[0]);
				MwSockClose(c->data[0]);
				lsd->LsdChDisable(c->data[0]);
			} else {
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
				ESP_LOGE(MW_TAG,"Requested disconnect of not opened channel %d.",
						c->data[0]);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;
            
		case MW_CMD_UDP_SET:
			ESP_LOGI(MW_TAG,"Configuring UDP socket...");
			if (MwUdpSet(&c->inAddr) < 0) {
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_SOCK_STAT:
			if ((c->data[0] > 0) && (c->data[0] < LSD_MAX_CH)) {
				// Send channel status and clear channel event flag
				replen = 1;
				reply.datalen = ByteSwapWord(1);
				reply.data[0] = (uint8_t)d.ss[c->data[0] - 1];
				MwFsmClearChEvent(c->data[0]);
			} else {
				replen = 0;
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
				ESP_LOGE(MW_TAG,"Requested unavailable channel!");
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + replen, 0);
			break;

		case MW_CMD_PING:
			ESP_LOGE(MW_TAG,"PING unimplemented");
			break;

		case MW_CMD_SNTP_CFG:
			ESP_LOGI(MW_TAG,"setting SNTP cfg for zone %s", c->data);
			sntp_config_set((char*)c->data, len, &reply);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_SNTP_CFG_GET:
			replen = cfg.ntpPoolLen;
			ESP_LOGI(MW_TAG,"sending SNTP cfg (%d bytes)", replen);
			memcpy(reply.data, cfg.ntpPool, replen);
			reply.datalen = htons(replen);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + replen, 0);
			break;

		case MW_CMD_DATETIME:
			ts = time(NULL);
			reply.datetime.dtBin[0] = 0;
			reply.datetime.dtBin[1] = ByteSwapDWord((uint32_t)ts);
			strcpy(reply.datetime.dtStr, ctime(&ts));
			ESP_LOGI(MW_TAG,"sending datetime %s", reply.datetime.dtStr);
			tmp = 2*sizeof(uint32_t) + strlen(reply.datetime.dtStr);
			reply.datalen = ByteSwapWord(tmp);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + tmp, 0);
			break;

		case MW_CMD_DT_SET:
			ESP_LOGE(MW_TAG,"DT_SET unimplemented");
			break;

		case MW_CMD_FLASH_WRITE:
			if (flash->flash_write(ntohl(c->flData.addr),
						len - sizeof(uint32_t),
						(char*)c->flData.data)) {
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_FLASH_READ:
			c->flRange.addr = ntohl(c->flRange.addr);
			c->flRange.len = ntohs(c->flRange.len);
			if (flash->flash_read(c->flRange.addr, c->flRange.len,
						(char*)reply.data)) {
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
				c->flRange.len = 0;
			}
			lsd->LsdSend((uint8_t*)&reply, c->flRange.len + MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_FLASH_ERASE:
			if (flash->flash_erase(ntohs(c->flSect))) {
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_FLASH_ID:
			reply.flash_id.device = htons(d.flash_dev);
			reply.flash_id.manufacturer = d.flash_man;
			reply.datalen = htons(3);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + 3, 0);
			break;

		case MW_CMD_SYS_STAT:
			MwSysStatFill(&reply);
			ESP_LOGI(MW_TAG,"%02X %02X %02X %02X", reply.data[0], reply.data[1],
					reply.data[2], reply.data[3]);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + sizeof(MwMsgSysStat),
				0);
			break;
		case MW_CMD_DEF_CFG_SET:
			// Check lengt and magic value
			if ((len != 4) || (c->dwData[0] !=
						ByteSwapDWord(MW_FACT_RESET_MAGIC))) {
				ESP_LOGE(MW_TAG,"Wrong DEF_CFG_SET command invocation!");
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			} else if (esp_partition_erase_range(d.p_cfg, 0,
						MW_CFG_SECT_LEN) != ESP_OK) {
				ESP_LOGE(MW_TAG,"Config flash sector erase failed!");
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			} else {
				ESP_LOGI(MW_TAG,"Configuration set to default.");
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;
		case MW_CMD_HRNG_GET:
			replen = ByteSwapWord(c->rndLen);
			if (replen > MW_CMD_MAX_BUFLEN) {
				replen = 0;
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			} else {
				reply.datalen = c->rndLen;
				rand_fill(reply.data, replen);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + replen, 0);
			break;
		case MW_CMD_BSSID_GET:
			reply.datalen = ByteSwapWord(6);
			esp_wifi_get_mac((wifi_interface_t )(c->data[0]), reply.data);
			ESP_LOGI(MW_TAG,"Got BSSID(%d) %02X:%02X:%02X:%02X:%02X:%02X",
					c->data[0], reply.data[0], reply.data[1],
					reply.data[2], reply.data[3],
					reply.data[4], reply.data[5]);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + 6, 0);
			break;
		case MW_CMD_GAMERTAG_SET:
			if (c->gamertag_set.slot >= MW_NUM_GAMERTAGS ||
					len != sizeof(struct mw_gamertag_set_msg)) {
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			} else {
				// Copy gamertag and save to flash
				memcpy(&cfg.gamertag[c->gamertag_set.slot],
						&c->gamertag_set.gamertag,
						sizeof(struct mw_gamertag));
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_GAMERTAG_GET:
			if (c->data[0] >= MW_NUM_GAMERTAGS) {
				replen = 0;
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			} else {
				replen = sizeof(struct mw_gamertag);
				memcpy(&reply.gamertag_get, &cfg.gamertag[c->data[0]],
						replen);
			}
			reply.datalen = ByteSwapWord(replen);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + replen, 0);
			break;

		case MW_CMD_LOG:
			puts((char*)c->data);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;
		case MW_CMD_FACTORY_RESET:
			MwSetDefaultCfg();
			// Fallthrough
		case MW_CMD_NV_CFG_SAVE:
			if (mw_nv_cfg_save() < 0) {
				reply.cmd = ByteSwapWord(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;
		case MW_CMD_SLEEP:
			// No reply, wakeup continues from user_init()
			deep_sleep();
			ESP_LOGI(MW_TAG,"Entering deep sleep");
			esp_deep_sleep(0);
			// As it takes a little for the module to enter deep
			// sleep, stay here for a while
			vTaskDelayMs(60000);
			// fallthrough
		case MW_CMD_HTTP_URL_SET:
			if (http->http_url_set((const char*)c->data)) {
				reply.cmd = htons(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_HTTP_METHOD_SET:
			if (http->http_method_set((esp_http_client_method_t)c->data[0])) {
				reply.cmd = htons(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_HTTP_HDR_ADD:
			if (http->http_header_add((char*)c->data)) {
				reply.cmd = htons(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_HTTP_HDR_DEL:
			if (http->http_header_del((char*)c->data)) {
				reply.cmd = htons(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_HTTP_OPEN:
			if (http->http_open(ntohl(c->dwData[0]))) {
				reply.cmd = htons(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_HTTP_FINISH:
			replen = http_parse_finish(&reply);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + replen, 0);
			/// TODO: Thread this
			http->http_recv();
			break;

		case MW_CMD_HTTP_CLEANUP:
			if (http->http_cleanup()) {
				reply.cmd = htons(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_HTTP_CERT_QUERY:
			reply.dwData[0] = htonl(http->http_cert_query());
			if (0xFFFFFFFF == reply.dwData[0]) {
				reply.cmd = htons(MW_CMD_ERROR);
			} else {
				replen = 4;
				reply.datalen = htons(4);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + replen, 0);
			break;

		case MW_CMD_HTTP_CERT_SET:
			if (http->http_cert_set(ntohl(c->dwData[0]),
					ntohs(c->wData[2]))) {
				reply.cmd = htons(MW_CMD_ERROR);
			}
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;
		case MW_CMD_SERVER_URL_GET:
			replen = parse_server_url_get(&reply);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + replen, 0);
			break;

		case MW_CMD_SERVER_URL_SET:
			parse_server_url_set((char*)c->data, &reply);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_WIFI_ADV_GET:
			replen = parse_wifi_adv_get(&reply);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + replen, 0);
			break;

		case MW_CMD_WIFI_ADV_SET:
			parse_wifi_adv_set(&c->wifi_adv_cfg, &reply);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;		

		case MW_CMD_UPGRADE_LIST:
			// TODO
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_UPGRADE_PERFORM:
			parse_upgrade((char*)c->data, &reply);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_GAME_ENDPOINT_SET:
			parse_game_endpoint_set((char*)c->data, &reply);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;

		case MW_CMD_GAME_KEYVAL_ADD:
			parse_game_add_keyval((char*)c->data, &reply);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN, 0);
			break;
		case MW_CMD_GAME_REQUEST:
			replen = parse_game_request(&c->ga_request, &reply);
			lsd->LsdSend((uint8_t*)&reply, MW_CMD_HEADLEN + replen, 0);
			/// TODO: Thread this
			http->http_recv();
			break;
		default:
			ESP_LOGE(MW_TAG,"UNKNOWN REQUEST!");
			break;
	}
	//*data_out = (uint8_t*)(&reply);
	return data_out_len;
}
void MegaWiFi::MwFsmClearChEvent(int ch) {
	if ((ch < 1) || (ch >= LSD_MAX_CH)) return;

	d.s.ch_ev &= ~(1<<ch);
}

void MegaWiFi::reply_set_ok_empty(MwCmd *reply)
{
	reply->datalen = 0;
	reply->cmd = MW_CMD_OK;
}

void MegaWiFi::sntp_set_config(void)
{
	const char *token[4];
	int num_tokens;

	num_tokens = tokens_get(cfg.ntpPool, token, 1 + SNTP_MAX_SERVERS, NULL);

	setenv("TZ", token[0], 1);
	tzset();

	for (int i = 1; i < num_tokens; i++) {
		sntp_setservername(i - 1, (char*)token[i]);
		ESP_LOGI(MW_TAG,"SNTP server: %s", token[i]);
	}
}

int MegaWiFi::tokens_get(const char *in, const char *token[],
		int token_max, uint16_t *len_total)
{
	int i;
	size_t len;

	token[0] = in;
	for (i = 0; i < (token_max - 1) && *token[i]; i++) {
		len = strlen(token[i]);
		token[i + 1] = token[i] + len + 1;
	}

	if (*token[i]) {
		i++;
	}

	if (len_total) {
		// ending address minus initial address plus an extra null
		// termination plus 1 (because if something uses bytes from
		// x pos to y pos, length is (x - y + 1).
		// note: this assumes that last token always has an extra
		// null termination.
		*len_total = ((token[i - 1] + strlen(token[i - 1]) + 2) - in);
	}

	return i;
}

#define PRINT_WIFI_CFG(param) ESP_LOGI(MW_TAG,#param " = %d", cfg.wifi.param)
void MegaWiFi::wifi_cfg_log(void)
{
	PRINT_WIFI_CFG(ampdu_rx_enable);
	PRINT_WIFI_CFG(amsdu_rx_enable);
	PRINT_WIFI_CFG(left_continuous_rx_buf_num);
	PRINT_WIFI_CFG(qos_enable);
	PRINT_WIFI_CFG(rx_ampdu_buf_len);
	PRINT_WIFI_CFG(rx_ampdu_buf_num);
	PRINT_WIFI_CFG(rx_ba_win);
	PRINT_WIFI_CFG(rx_buf_len);
	PRINT_WIFI_CFG(rx_buf_num);
	PRINT_WIFI_CFG(rx_max_single_pkt_len);
	PRINT_WIFI_CFG(rx_pkt_num);
	PRINT_WIFI_CFG(tx_buf_num);
}

void MegaWiFi::MwSysStatFill(MwCmd *rep) {
	rep->datalen = htons(sizeof(MwMsgSysStat));
	rep->sysStat.st_flags = htonl(d.s.st_flags);
	ESP_LOGD(MW_TAG,"stat flags: 0x%04X, len: %d", d.s.st_flags,
			sizeof(MwMsgSysStat));
}

int MegaWiFi::MwAccept(int sock, int ch) {
	// Client address
	struct sockaddr_in caddr;
	socklen_t addrlen = sizeof(caddr);
	int newsock;

	if ((newsock = lwip_accept(sock, (struct sockaddr*)&caddr,
					&addrlen)) < 0) {
		ESP_LOGE(MW_TAG,"Accept failed for socket %d, channel %d", sock, ch);
		return -1;
	}
	// Connection accepted, add to the FD set
	ESP_LOGI(MW_TAG,"Socket %d, channel %d: established connection from %s.", newsock, ch, inet_ntoa(caddr.sin_addr));
	FD_SET(newsock, &d.fds);
	d.fdMax = MAX(newsock, d.fdMax);
	// Close server socket and remove it from the set
	lwip_close(sock);
	FD_CLR(sock, &d.fds);
	// Update channel data
	d.chan[newsock - LWIP_SOCKET_OFFSET] = ch;
	d.sock[ch - 1] = newsock;
	d.ss[ch - 1] = MW_SOCK_TCP_EST;

	// Enable channel to send/receive data
	lsd->LsdChEnable(ch);

	return 0;
}

void MegaWiFi::MwSockClose(int ch) {
	// Close socket, remove from file descriptor set and mark as unused
	int idx = ch - 1;

	FD_CLR(d.sock[idx], &d.fds);
	lwip_close(d.sock[idx]);
	// No channel associated with this socket
	d.chan[d.sock[idx] - LWIP_SOCKET_OFFSET] = -1;
	d.sock[idx] = -1; // No socket on this channel
	d.ss[idx] = MW_SOCK_NONE;
}

int MegaWiFi::MwUdpSet(MwMsgInAddr* addr) {
	int err;
	int s;
	int idx;
	unsigned int local_port;
	unsigned int remote_port;
	struct addrinfo *raddr;
	struct sockaddr_in local;

	err = MwChannelCheck(addr->channel);
	if (err) {
		return err;
	}
	idx = addr->channel - 1;

	local_port = atoi(addr->src_port);
	remote_port = atoi(addr->dst_port);

	if ((s = lwip_socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		ESP_LOGE(MW_TAG,"Failed to create UDP socket");
		return -1;
	}

	memset(local.sin_zero, 0, sizeof(local.sin_zero));
	local.sin_len = sizeof(struct sockaddr_in);
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = lwip_htonl(INADDR_ANY);
	local.sin_port = lwip_htons(local_port);
	if (remote_port && addr->data[0]) {
		// Communication with remote peer
		ESP_LOGE(MW_TAG,"UDP ch %d, port %d to addr %s:%d.", addr->channel,
				local_port, addr->data, remote_port);
        // DNS lookup
		err = net_dns_lookup(addr->data, addr->dst_port, &raddr);
		if (err) {
			lwip_close(s);
			return -1;
		}
		d.raddr[idx] = *((struct sockaddr_in*)raddr->ai_addr);
		freeaddrinfo(raddr);
	} else if (local_port) {
		// Server in reuse mode
		ESP_LOGI(MW_TAG,"UDP ch %d, src port %d.", addr->channel, local_port);
		d.raddr[idx] = local;
	} else {
		ESP_LOGE(MW_TAG,"Invalid UDP socket data");
		return -1;
	}

	if (lwip_bind(s, (struct sockaddr*)&local,
				sizeof(struct sockaddr_in)) < 0) {
		ESP_LOGE(MW_TAG,"bind() failed. Is UDP port in use?");
		lwip_close(s);
		return -1;
	}

	ESP_LOGI(MW_TAG,"UDP socket %d bound", s);
	// Record socket number and mark channel as in use.
	d.sock[idx] = s;
	d.chan[s - LWIP_SOCKET_OFFSET] = addr->channel;
	d.ss[idx] = MW_SOCK_UDP_READY;
	// Enable LSD channel
	lsd->LsdChEnable(addr->channel);

	// Add listener to the FD set
	FD_SET(s, &d.fds);
	d.fdMax = MAX(s, d.fdMax);

	return s;
}

void MegaWiFi::MwFsmRaiseChEvent(int ch) {
	if ((ch < 1) || (ch >= LSD_MAX_CH)) return;
	d.s.ch_ev |= 1<<ch;
}

void MegaWiFi::log_ip_cfg(MwMsgIpCfg *ip)
{
	char ip_str[16];

	ipv4_to_str(ip->cfg.ip.addr, ip_str);
	ESP_LOGI(MW_TAG,"IP:   %s", ip_str);
	ipv4_to_str(ip->cfg.netmask.addr, ip_str);
	ESP_LOGI(MW_TAG,"MASK: %s", ip_str);
	ipv4_to_str(ip->cfg.gw.addr, ip_str);
	ESP_LOGI(MW_TAG,"GW:   %s", ip_str);
	ipv4_to_str(ip->dns1.u_addr.ip4.addr, ip_str);
	ESP_LOGI(MW_TAG,"DNS1: %s", ip_str);
	ipv4_to_str(ip->dns2.u_addr.ip4.addr, ip_str);
	ESP_LOGI(MW_TAG,"DNS2: %s\n", ip_str);
}

void MegaWiFi::rand_fill(uint8_t *buf, uint16_t len)
{
	uint32_t *data = (uint32_t*)buf;
	uint8_t last[4];
	uint16_t dwords = len>>2;
	uint16_t i;

	// Generate random dwords
	for (i = 0; i < dwords; i++) {
		data[i] = esp_random();
	}

	// Generate remaining random bytes
	*((uint32_t*)last) = esp_random();
	buf = (uint8_t*)(data + i);
	for (i = 0; i < (len & 3); i++) {
		buf[i] = last[i];
	}
}

void MegaWiFi::sntp_config_set(const char *data, uint16_t len, MwCmd *reply)
{
	// We should have from 2 to 4 null terminated strings. First one
	// is the TZ string. Remaining ones are SNTP server names.
	const char *token[4];
	int num_tokens;
	uint16_t len_total;

	num_tokens = tokens_get(data, token, 4, &len_total);
	if (num_tokens < 2 || len_total != len || len_total > MW_NTP_POOL_MAXLEN || (token[1] - token[0]) < 4) {
        ESP_LOGE(MW_TAG,"SNTP configuration failed");
        reply->cmd = htons(MW_CMD_ERROR);
        return;
	}

	memcpy(cfg.ntpPool, data, len);
	memset(cfg.ntpPool + len, 0, MW_NTP_POOL_MAXLEN - len);
	cfg.ntpPoolLen = len;

	sntp_set_config();
}

int MegaWiFi::http_parse_finish(MwCmd *reply)
{
	int32_t body_len = 0;
	uint16_t status = 0;

	if (http->http_finish(&status, &body_len)) {
		reply->cmd = htons(MW_CMD_ERROR);
		return 0;
	}

	reply->dwData[0] = htonl(body_len);
	reply->wData[2] = htons(status);
	reply->datalen = htons(6);

	return 6;
}


int MegaWiFi::wifi_scan(uint8_t phy_type, uint8_t *data)
{
	int length = -1;
	uint16_t n_aps = 0;
	wifi_ap_record_t *ap = NULL;
	wifi_scan_config_t scan_cfg = {};
	esp_err_t err;

	d.phy = phy_type;
	err = esp_wifi_start();
	if (ESP_OK != err) {
		ESP_LOGE(MW_TAG,"wifi start failed: %s!", esp_err_to_name(err));
	}
	err = esp_wifi_scan_start(&scan_cfg, true);
	if (ESP_OK != err) {
		ESP_LOGE(MW_TAG,"scan failed: %s!", esp_err_to_name(err));
	}

	esp_wifi_scan_get_ap_num(&n_aps);
	ESP_LOGI(MW_TAG,"found %d APs", n_aps);
	ap = (wifi_ap_record_t *)calloc(n_aps, sizeof(wifi_ap_record_t));
 	if (!ap) {
		ESP_LOGE(MW_TAG,"out of memory!");
	}
	err = esp_wifi_scan_get_ap_records(&n_aps, ap);
	if (ESP_OK != err) {
		ESP_LOGE(MW_TAG,"get AP records failed: %s", esp_err_to_name(err));
	}
    if (ESP_OK == err && ap) {
        ap_print(ap, n_aps);
        length = build_scan_reply(ap, n_aps, data);
		free(ap);
    }
	esp_wifi_stop();

	return length;
}

int MegaWiFi::build_scan_reply(const wifi_ap_record_t *ap, uint8_t n_aps, uint8_t *data)
{
	int i;
	uint8_t *pos = data;
	uint8_t ssid_len;
	int data_len = 1;

	// Skip number of aps
	pos = data + 1;
	for (i = 0; i < n_aps; i++) {
		ssid_len = strnlen((char*)ap[i].ssid, 32);
		// Check if next entry fits along with end
		if ((ssid_len + 5) >= LSD_MAX_LEN) {
			ESP_LOGI(MW_TAG,"discarding %d entries", n_aps - i);
			break;
		}
		pos[0] = ap[i].authmode;
		pos[1] = ap[i].primary;
		pos[2] = ap[i].rssi;
		pos[3] = ssid_len;
		memcpy(pos + 4, ap[i].ssid, data[3]);
		pos += 4 + ssid_len;
		data_len += 4 + ssid_len;
	}
	// Write number of APs in report
	*data = i;

	return data_len;
}

void MegaWiFi::ap_print(const wifi_ap_record_t *ap, int n_aps) {
	wifi_auth_mode_t auth;
	const char * const auth_str[WIFI_AUTH_MAX + 1] = {
		"OPEN", "WEP", "WPA_PSK", "WPA2_PSK", "WPA_WPA2_PSK",
		"WPA_WPA2_ENTERPRISE", "UNKNOWN"
	};
	int i;

	for (i = 0; i < n_aps; i++) {
		auth = MIN(ap[i].authmode, WIFI_AUTH_MAX);
		ESP_LOGI(MW_TAG,"%s, %s, ch=%d, str=%d", ap[i].ssid, auth_str[auth],
				ap[i].primary, ap[i].rssi);
	}

	ESP_LOGI(MW_TAG,"That's all!");
}

int MegaWiFi::mw_nv_cfg_save(void) {
	// Compute MD5 of the configuration data
	mbedtls_md5((const unsigned char*)&cfg, ((uint32_t)&cfg.md5) - 
			((uint32_t)&cfg), cfg.md5);
#ifdef _DEBUG_MSGS
	char md5_str[33];
	md5_to_str(cfg.md5, md5_str);
	ESP_LOGI(MW_TAG,"saved MD5: %s", md5_str);
#endif
	// Erase configuration sector
	if (esp_partition_erase_range(d.p_cfg, 0, MW_CFG_SECT_LEN) != ESP_OK) {
		ESP_LOGE(MW_TAG,"config flash erase failed");
		return -1;
	}
	// Write configuration to flash
	if (esp_partition_write(d.p_cfg, 0, &cfg, sizeof(MwNvCfg)) != ESP_OK) {
		ESP_LOGE(MW_TAG,"config flash write failed");
		return -1;
	}
	ESP_LOGI(MW_TAG,"configuration saved to flash.");
	return 0;
}

void MegaWiFi::ap_cfg_set(uint8_t num, uint8_t phy_type, const char *ssid,
		const char *pass, MwCmd *reply)
{
	if (num >= MW_NUM_AP_CFGS) {
		ESP_LOGE(MW_TAG,"tried to set AP for cfg %d", num);
		reply->cmd = htons(MW_CMD_ERROR);
	} else if (phy_type != WIFI_PROTOCOL_11B &&
			phy_type != (WIFI_PROTOCOL_11B + WIFI_PROTOCOL_11G) &&
			phy_type != (WIFI_PROTOCOL_11B + WIFI_PROTOCOL_11G +
				WIFI_PROTOCOL_11N)) {
		ESP_LOGE(MW_TAG,"PHY type 0x%X not supported", phy_type);
		reply->cmd = htons(MW_CMD_ERROR);
	} else {
		// Copy configuration and save it to flash
		ESP_LOGI(MW_TAG,"setting AP configuration %d...", num);
		strncpy(cfg.ap[num].ssid, ssid, MW_SSID_MAXLEN);
		strncpy(cfg.ap[num].pass, pass, MW_PASS_MAXLEN);
		cfg.ap[num].phy = phy_type;
		cfg.ap[num].reserved[0] = cfg.ap[num].reserved[1] =
			 cfg.ap[num].reserved[2] = 0;
		ESP_LOGI(MW_TAG,"phy %d, ssid: %s, pass: %s", phy_type, ssid, pass);
		cfg.defaultAp = num;
	}
}

int MegaWiFi::parse_server_url_get(MwCmd *reply)
{
	int len = 1 + strlen(cfg.serverUrl);
	memcpy(reply->data, cfg.serverUrl, len);
	reply->datalen = htons(len);

	return len;
}

void MegaWiFi::parse_server_url_set(const char *url, MwCmd *reply)
{
	size_t len = 1 + strlen(url);

	if (len > MW_SERVER_DEFAULT_MAXLEN) {
		reply->cmd = htons(MW_CMD_ERROR);
	} else {
		memcpy(cfg.serverUrl, url, len);
	}
}

#define FILL_REP_FROM_CFG(param) do { \
	reply->wifi_adv_cfg.param = cfg.wifi.param; \
} while (0)
int MegaWiFi::parse_wifi_adv_get(MwCmd *reply)
{
	size_t len = sizeof(struct mw_wifi_adv_cfg);

	wifi_cfg_log();
	FILL_REP_FROM_CFG(ampdu_rx_enable);
	FILL_REP_FROM_CFG(amsdu_rx_enable);
	FILL_REP_FROM_CFG(left_continuous_rx_buf_num);
	FILL_REP_FROM_CFG(qos_enable);
	FILL_REP_FROM_CFG(rx_ampdu_buf_len);
	FILL_REP_FROM_CFG(rx_ampdu_buf_num);
	FILL_REP_FROM_CFG(rx_ba_win);
	FILL_REP_FROM_CFG(rx_buf_len);
	FILL_REP_FROM_CFG(rx_buf_num);
	FILL_REP_FROM_CFG(rx_max_single_pkt_len);
	FILL_REP_FROM_CFG(rx_pkt_num);
	FILL_REP_FROM_CFG(tx_buf_num);

	reply->wifi_adv_cfg.rx_max_single_pkt_len =
		htonl(reply->wifi_adv_cfg.rx_max_single_pkt_len);
	reply->wifi_adv_cfg.rx_buf_len = htonl(reply->wifi_adv_cfg.rx_buf_len);
	reply->wifi_adv_cfg.rx_ampdu_buf_len =
		htonl(reply->wifi_adv_cfg.rx_ampdu_buf_len);

	reply->datalen = htons(len);

	return len;
}

#define THRESHOLD_MAX_CHECK(val, max) if (wifi->val > (max)) { \
	ESP_LOGE(MW_TAG, " check failed: %d < %d", wifi->val, max); \
	return; \
}

#define THRESHOLD_MAX_MIN_CHECK(val, min, max) if (wifi->val < (min) || wifi->val > (max)) { \
	ESP_LOGE(MW_TAG, " check failed: %d [%d,%d]", wifi->val, min, max); \
	return; \
}

#define FILL_CFG_FROM_REQ(param) do { \
	cfg.wifi.param = wifi->param; \
} while (0)

// Changes required to be saved and a reboot to be effective
void MegaWiFi::parse_wifi_adv_set(struct mw_wifi_adv_cfg *wifi, MwCmd *reply)
{
	wifi->rx_max_single_pkt_len = ntohl(wifi->rx_max_single_pkt_len);
	wifi->rx_buf_len = ntohl(wifi->rx_buf_len);
	wifi->rx_ampdu_buf_len = ntohl(wifi->rx_ampdu_buf_len);

	THRESHOLD_MAX_CHECK(left_continuous_rx_buf_num, 16);
	THRESHOLD_MAX_CHECK(rx_ba_win, 16);
	THRESHOLD_MAX_MIN_CHECK(rx_buf_num, 14, 28);
	THRESHOLD_MAX_MIN_CHECK(rx_pkt_num, 4, 16);
	THRESHOLD_MAX_MIN_CHECK(tx_buf_num, 4, 16);

	if (!wifi->ampdu_rx_enable) {
		if (wifi->rx_ba_win) {
			ESP_LOGE(MW_TAG,"rx_ba_win %d not allowed with AMPDU", wifi->rx_ba_win);
            reply->cmd = htons(MW_CMD_ERROR);
            return;
		}
	}

	// Checks passed, copy configuration
	FILL_CFG_FROM_REQ(ampdu_rx_enable);
	FILL_CFG_FROM_REQ(amsdu_rx_enable);
	FILL_CFG_FROM_REQ(left_continuous_rx_buf_num);
	FILL_CFG_FROM_REQ(qos_enable);
	FILL_CFG_FROM_REQ(rx_ampdu_buf_len);
	FILL_CFG_FROM_REQ(rx_ampdu_buf_num);
	FILL_CFG_FROM_REQ(rx_ba_win);
	FILL_CFG_FROM_REQ(rx_buf_len);
	FILL_CFG_FROM_REQ(rx_buf_num);
	FILL_CFG_FROM_REQ(rx_max_single_pkt_len);
	FILL_CFG_FROM_REQ(rx_pkt_num);
	FILL_CFG_FROM_REQ(tx_buf_num);

	wifi_cfg_log();	
}

void MegaWiFi::parse_game_endpoint_set(const char *data, MwCmd *reply)
{
	const char *token[2] = {NULL, NULL};

	if (tokens_get(data, token, 2, NULL) != 2 ||
			ga->ga_endpoint_set(token[0], token[1])) {
		reply->cmd = htons(MW_CMD_ERROR);
	}
}

int MegaWiFi::parse_game_request(struct mw_ga_request *req, MwCmd *reply)
{
	int32_t body_len = 0;
	uint16_t status = 0;

	status = ga->ga_request((esp_http_client_method_t)req->method, req->num_paths, req->num_kv_pairs,
			req->req, &body_len);
	if (!status) {
		reply->cmd = htons(MW_CMD_ERROR);
		return 0;
	}

	reply->dwData[0] = htonl(body_len);
	reply->wData[2] = htons(status);
	reply->datalen = htons(6);

	return 6;
}

void MegaWiFi::parse_game_add_keyval(const char *data, MwCmd *reply)
{
	const char *token[2];
	const char *pos = data;

	while (2 == tokens_get(pos, token, 2, NULL)) {
		ESP_LOGD(MW_TAG,"add %s:%s", token[0], token[1]);
		if (ga->ga_key_value_add(token[0], token[1])) {
			reply->cmd = htons(MW_CMD_ERROR);
			return;
		}
		pos = token[1] + strlen(token[1]) + 1;
	}
}

void MegaWiFi::disconnect(void)
{
	// Close all opened sockets
	close_all();
	// Disconnect and switch to IDLE state
	esp_wifi_disconnect();
	esp_wifi_stop();
	d.s.sys_stat = MW_ST_IDLE;
	d.s.online = FALSE;
	ESP_LOGI(MW_TAG,"IDLE!");
}

void MegaWiFi::close_all(void) {
	int i;

	for (i = 0; i < MW_MAX_SOCK; i++) {
		if (d.ss[i] > 0) {
			ESP_LOGI(MW_TAG,"Closing sock %d on ch %d", d.sock[i], i + 1);
			MwSockClose(i + 1);
			lsd->LsdChDisable(i + 1);
		}
	}
}

int MegaWiFi::MwCmdInList(uint8_t cmd, const uint32_t list[2]) {

	if (cmd < 32) {
		return (((1<<cmd) & list[0]) != 0);
	} else if (cmd < 64) {
		return (((1<<(cmd - 32)) & list[1]) != 0);
	} else {
		return 0;
	}
}

void MegaWiFi::MwApJoin(uint8_t n) {
	wifi_config_t if_cfg = {};

	SetIpCfg(n);
	strncpy((char*)if_cfg.sta.ssid, cfg.ap[n].ssid, MW_SSID_MAXLEN);
	strncpy((char*)if_cfg.sta.password, cfg.ap[n].pass, MW_PASS_MAXLEN);
	d.phy = cfg.ap[n].phy;
	esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &if_cfg);
	esp_wifi_start();
	tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, "MegaWiFi-"
			STR(MW_FW_VERSION_MAJOR) "-"
			STR(MW_FW_VERSION_MINOR));
	ESP_LOGI(MW_TAG,"AP ASSOC %d", n);
	d.s.sys_stat = MW_ST_AP_JOIN;
	d.n_reassoc = 0;
}

void MegaWiFi::SetIpCfg(int slot) {
	esp_err_t err;

	if ((cfg.ip[slot].ip.addr) && (cfg.ip[slot].netmask.addr)
				&& (cfg.ip[slot].gw.addr)) {
		tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
		err = tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &cfg.ip[slot]);
		if (!err) {
			ESP_LOGI(MW_TAG,"static IP configuration %d set", slot);
			// Set DNS servers if available
			if (cfg.dns[slot][0].u_addr.ip4.addr) {
				dns_setserver(0, cfg.dns[slot] + 0);
				if (cfg.dns[slot][1].u_addr.ip4.addr) {
					dns_setserver(1, cfg.dns[slot] + 1);
				}
			}
		} else {
			ESP_LOGE(MW_TAG,"failed setting static IP configuration %d: %s",
					slot, esp_err_to_name(err));
			tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
		}
	} else {
		ESP_LOGI(MW_TAG,"setting DHCP IP configuration.");
		tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
	}
}

int MegaWiFi::MwFsmTcpCon(MwMsgInAddr* addr) {
	struct addrinfo *res;
	int err;
	int s;

	ESP_LOGI(MW_TAG,"Con. ch %d to %s:%s", addr->channel, addr->data,
			addr->dst_port);

	err = MwChannelCheck(addr->channel);
	if (err) {
		return err;
	}
	// DNS lookup
	err = net_dns_lookup(addr->data, addr->dst_port, &res);
	if (err) {
		return err;
	}
	
	s = lwip_socket(res->ai_family, res->ai_socktype, 0);
	if(s < 0) {
		ESP_LOGE(MW_TAG,"... Failed to allocate socket.");
		freeaddrinfo(res);
		return -1;
	}

	ESP_LOGI(MW_TAG,"... allocated socket");

	if(lwip_connect(s, res->ai_addr, res->ai_addrlen) != 0) {
		lwip_close(s);
		freeaddrinfo(res);
		ESP_LOGE(MW_TAG,"... socket connect failed.");
		return -1;
	}

	ESP_LOGI(MW_TAG,"... connected sock %d on ch %d", s, addr->channel);
	freeaddrinfo(res);
	// Record socket number, type and mark channel as in use.
	d.sock[addr->channel - 1] = s;
	d.ss[addr->channel - 1] = MW_SOCK_TCP_EST;
	// Record channel number associated with socket
	d.chan[s - LWIP_SOCKET_OFFSET] = addr->channel;
	// Add socket to the FD set and update maximum socket valud
	FD_SET(s, &d.fds);
	d.fdMax = MAX(s, d.fdMax);

	// Enable LSD channel
	lsd->LsdChEnable(addr->channel);
	return s;
}

int MegaWiFi::MwFsmTcpBind(MwMsgBind *b) {
	struct sockaddr_in saddr;
	socklen_t addrlen = sizeof(saddr);
	int serv;
	int optval = 1;
	uint16_t port;
	int err;

	err = MwChannelCheck(b->channel);
	if (err) {
		return err;
	}

	// Create socket, set options
	if ((serv = lwip_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		ESP_LOGE(MW_TAG,"Could not create server socket!");
		return -1;
	}

	if (lwip_setsockopt(serv, SOL_SOCKET, SO_REUSEADDR, &optval,
				sizeof(int)) < 0) {
		lwip_close(serv);
		ESP_LOGE(MW_TAG,"setsockopt failed!");
		return -1;
	}

	// Fill in address information
	port = ByteSwapWord(b->port);
	memset((char*)&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_len = addrlen;
	saddr.sin_addr.s_addr = lwip_htonl(INADDR_ANY);
	saddr.sin_port = lwip_htons(port);

	// Bind to address
	if (lwip_bind(serv, (struct sockaddr*)&saddr, sizeof(saddr)) < -1) {
		lwip_close(serv);
		ESP_LOGE(MW_TAG,"Bind to port %d failed!", port);
		return -1;
	}

	// Listen for incoming connections
	if (lwip_listen(serv, MW_MAX_SOCK) < 0) {
		lwip_close(serv);
		ESP_LOGE(MW_TAG,"Listen to port %d failed!", port);
		return -1;
	}
	ESP_LOGE(MW_TAG,"Listening to port %d.", port);

	// Fill in channel data
	d.sock[b->channel - 1] = serv;
	d.chan[serv - LWIP_SOCKET_OFFSET] = b->channel;
	d.ss[b->channel - 1] = MW_SOCK_TCP_LISTEN;

	// Add listener to the FD set
	FD_SET(serv, &d.fds);
	d.fdMax = MAX(serv, d.fdMax);

	return 0;
}

 int MegaWiFi::MwChannelCheck(int ch) {
	// Check channel is valid and not in use.
	if (ch >= LSD_MAX_CH) {
		ESP_LOGE(MW_TAG,"Requested unavailable channel %d", ch);
		return -1;
	}
	if (d.ss[ch - 1]) {
		ESP_LOGW(MW_TAG,"Requested already in-use channel %d", ch);
		return -1;
	}

	return 0;
}

void MegaWiFi::deep_sleep(void)
{
	ESP_LOGI(MW_TAG,"Entering deep sleep");
	disconnect();
	esp_deep_sleep_start();
}

void MegaWiFi::sleep_timer_cb(TimerHandle_t xTimer)
{
	UNUSED_PARAM(xTimer);

	if(instance_p)instance_p->deep_sleep();
}

void MegaWiFi::print_flash_id()
{
	uint32_t id, len;
	char *byte = (char*)&id;

	// Using NULL as spi chip, gets the default one
	esp_flash_read_id(NULL, &id);
	esp_flash_get_physical_size(NULL, &len);

	d.flash_man = byte[0];
	d.flash_dev = (byte[1]<<8) | byte[2];

	ESP_LOGI(MW_TAG,"flash manufacturer: %02"PRIX8", device %04"PRIX16,d.flash_man, d.flash_dev);
	ESP_LOGI(MW_TAG,"SPI chip length: %"PRIu32, len);
}

void MegaWiFi::parse_upgrade(const char *name, MwCmd *reply)
{
	esp_err_t err;

	err = upgrade->upgrade_firmware(cfg.serverUrl, name);
	if (err) {
		reply->cmd = htons(MW_CMD_ERROR);
	}
}
