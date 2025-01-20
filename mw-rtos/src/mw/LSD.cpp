
/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#include "mw/LSD.h"
#define LOG_LOCAL_LEVEL CORE_DEBUG_LEVEL

void LSD::LsdInit(QueueHandle_t q, QueueHandle_t qtx) {
	// Set variables to default values
	memset(&lsdData, 0, sizeof(LsdData));
	lsdData.rxs = LSD_ST_STX_WAIT;
	// Create semaphore used to handle receive buffers
	lsdData.sem = xSemaphoreCreateCounting(LSD_BUF_FRAMES, LSD_BUF_FRAMES);
	this->q = q;
	this->qTx = qtx;
}

int LSD::LsdChEnable(uint8_t ch) {
	if (ch >= LSD_MAX_CH) return LSD_ERROR;

	lsdData.en[ch] = TRUE;
	return LSD_OK;
}

int LSD::LsdChDisable(uint8_t ch) {
	if (ch >= LSD_MAX_CH) return LSD_ERROR;

	lsdData.en[ch] = FALSE;

	return LSD_OK;
}

size_t LSD::LsdSend(const uint8_t *data, uint16_t len, uint8_t ch) {

	char scratch[3];

	if (len > MW_MSG_MAX_BUFLEN || ch >= LSD_MAX_CH) {
		ESP_LOGE(LSD_TAG,"Invalid length (%d) or channel (%d).", len, ch);
		return -1;
	}
	if (!lsdData.en[ch]) {
		ESP_LOGE(LSD_TAG,"LsdSend: Channel %d not enabled.", ch);
		return 0;
	}

	ESP_LOGI(LSD_TAG,"sending %d bytes", len);
	scratch[0] = LSD_STX_ETX;
	scratch[1] = (ch<<4) | (len>>8);
	scratch[2] = len & 0xFF;
	
	size_t data_out_len = sizeof(scratch) + len + 1;
	
    // Crear mensaje
    MegaDeviceX7TxMsg m = {
        .size = data_out_len
    };
	TXB.len = data_out_len;

    // Copiar datos al buffer de salida
    memcpy(TXB.data, scratch, sizeof(scratch));
    memcpy((TXB.data + sizeof(scratch)), data, len);
    TXB.data[sizeof(scratch) + len] = scratch[0]; 

	m.buffer = TXB.data;
    ESP_LOGI(LSD_TAG, "Sending...");
    if (xQueueSend(qTx, &m, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(LSD_TAG, "Failed to send to queue.");
        return -1;
    }
    return data_out_len; 
}

size_t LSD::LsdSplitStart(uint8_t *data, uint16_t len,
		              uint16_t total, uint8_t ch) {
						
	char scratch[3];

	if (total > MW_MSG_MAX_BUFLEN || ch >= LSD_MAX_CH) return -1;
	if (!lsdData.en[ch]) return 0;

	scratch[0] = LSD_STX_ETX;
	scratch[1] = (ch<<4) | (total>>8);
	scratch[2] = total & 0xFF;
	// Send STX, channel and length
	ESP_LOGD(LSD_TAG,"sending header");
	size_t data_out_len = sizeof(scratch) + len;
	
    // Crear mensaje
    MegaDeviceX7TxMsg m = {
        .size = data_out_len
    };
	TXB.len = data_out_len;
	memcpy((char *)TXB.data, (const char *)scratch, sizeof(scratch));
	// Send data payload
	ESP_LOGD(LSD_TAG,"sending %d bytes", len);
	if (len) {
    	memcpy((char *)(TXB.data + sizeof(scratch)), (const char *)data, len);
	}

    ESP_LOGI(LSD_TAG, "Sending...");
	m.buffer = TXB.data;
    if (xQueueSend(qTx, &m, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(LSD_TAG, "Failed to send to queue.");
        return -1;
    }
	return data_out_len;
}

size_t LSD::LsdSplitNext(uint8_t *data, uint16_t len) {
	// send data
	ESP_LOGD(LSD_TAG,"Sending %d bytes", len);
	size_t data_out_len = len;
	
    // Crear mensaje
    MegaDeviceX7TxMsg m = {
        .size = data_out_len
    };
	TXB.len = data_out_len;

    memcpy((char *)TXB.data, (const char *)data, len);
    ESP_LOGI(LSD_TAG, "Sending...");
	m.buffer = TXB.data;
    if (xQueueSend(qTx, &m, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(LSD_TAG, "Failed to send to queue.");
        return -1;
    }
	return data_out_len;
}

size_t LSD::LsdSplitEnd(uint8_t *data, uint16_t len) {
	char scratch = LSD_STX_ETX;

	// Send data
	ESP_LOGD(LSD_TAG,"Sending %d bytes", len);
	size_t data_out_len = len + 1;
	
    // Crear mensaje
    MegaDeviceX7TxMsg m = {
        .size = data_out_len
    };
	TXB.len = data_out_len;

    memcpy((char *)TXB.data, (const char *)data, len);

	// Send ETX
	ESP_LOGD(LSD_TAG,"Sending ETX");
    TXB.data[len] = scratch; 

    ESP_LOGI(LSD_TAG, "Sending...");
	m.buffer = TXB.data;
    if (xQueueSend(qTx, &m, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(LSD_TAG, "Failed to send to queue.");
        return -1;
    }
	return data_out_len;
}

void LSD::LsdRxBufFree(void) {
	// Just increment the receiver semaphore count. Might cause problems
	// if not properly used!
	xSemaphoreGive(lsdData.sem);
}

void LSD::LsdRecv(const uint8_t *data, size_t data_len){
	MwFsmMsg m;
	static uint16_t pos = 0;
	uint8_t recv;
	bool error = TRUE;
	xSemaphoreTake(lsdData.sem, portMAX_DELAY);
	for (size_t i = 0; i<data_len; i++) {
		recv = data[i];		
	    ESP_LOGD(LSD_TAG,"pos: %u len: %u LsdRecv: %x", pos, RXB.len, recv);
		switch (lsdData.rxs) {
			case LSD_ST_IDLE:			// Do nothing!
				break;

			case LSD_ST_STX_WAIT:		// Wait for STX to arrive
				if (LSD_STX_ETX == recv) {
					pos = 0;
					lsdData.rxs = LSD_ST_CH_LENH_RECV;
				}
				break;

			case LSD_ST_CH_LENH_RECV:	// Receive CH and len high
				// Check special case: if we receive STX and pos == 0,
				// then this is the real STX (previous one was ETX from
				// previous frame!).
				if (!(LSD_STX_ETX == recv && 0 == pos)) {
					RXB.ch = recv>>4;
					RXB.len = (recv & 0x0F)<<8;
					// Sanity check (not exceding number of channels)
					if (RXB.ch >= LSD_MAX_CH) {
						lsdData.rxs = LSD_ST_STX_WAIT;
						ESP_LOGE(LSD_TAG,"invalid channel %" PRIu8, RXB.ch);
					}
					// Check channel is enabled
					else if (lsdData.en[RXB.ch]) {
						lsdData.rxs = LSD_ST_LEN_RECV;
					}
					else {
						lsdData.rxs = LSD_ST_STX_WAIT;
						ESP_LOGE(LSD_TAG,"Recv data on not enabled channel!");
					}
				}
				break;

			case LSD_ST_LEN_RECV:		// Receive len low
				RXB.len |= recv;
				// Sanity check (not exceeding maximum buffer length)
				if (RXB.len <= MW_MSG_MAX_BUFLEN) {
					lsdData.rxs = LSD_ST_DATA_RECV;
				} else {
					ESP_LOGE(LSD_TAG,"Recv length exceeds buffer length!");
					lsdData.rxs = LSD_ST_STX_WAIT;
				}
				break;

			case LSD_ST_DATA_RECV:		// Receive payload
				ESP_LOGD(LSD_TAG,"Receive payload");
				RXB.data[pos++] = recv;
	            ESP_LOGD(LSD_TAG,"len: %u pos: %u", RXB.len, pos);
				if (pos >= RXB.len) lsdData.rxs = LSD_ST_ETX_RECV;
				break;

			case LSD_ST_ETX_RECV:		// ETX should come here
				if (LSD_STX_ETX == recv) {
					// Send message to FSM and switch buffer
					m.e = MW_EV_SER_RX;
					m.d = lsdData.rx + lsdData.current;
					lsdData.current ^= 1;
					// Set receiving to false, to grab a new buffer
					ESP_LOGD(LSD_TAG,"Send Message to MW Thread");
					error = FALSE;
					xQueueSend(q, &m, portMAX_DELAY);
				} else {
					ESP_LOGE(LSD_TAG,"Expecting ETX but not received!");
				}
				pos = 0;
				lsdData.rxs = LSD_ST_STX_WAIT;
				break;

			default:
				// Code should never reach here!
				break;
		} // switch(lsdData.rxs)
	} // if (uart_read_bytes(...))
	if(error)LsdRxBufFree();
}