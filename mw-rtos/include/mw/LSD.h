/****************************************************************************
 * \brief MegaWiFi RTOS.
 * 
 * \author Juan Antonio (PaCHoN) 
 * \author Jesus Alonso (doragasu)
 * 
 * \date 01/2025
 ****************************************************************************/

#pragma once

#include <stdint.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#define LSD_TAG "LSD"

/** \addtogroup lsd ReturnCodes OK/Error codes returned by several functions.
 *  \{ */
/// Function completed successfully
#define LSD_OK				0
/// Generic error code
#define LSD_ERROR			-1
/// A framing error occurred. Possible data loss.
#define LSD_FRAMING_ERROR	-2
/** \} */

/// LSD frame overhead in bytes
#define LSD_OVERHEAD		4

/// Uart used for LSD
#define LSD_UART			0

/// Start/end of transmission character
#define LSD_STX_ETX		0x7E

/// Maximum number of available simultaneous channels
#define LSD_MAX_CH			4

/// Receive task priority
#define LSD_RECV_PRIO		2

/// Maximum data payload length
#define LSD_MAX_LEN		 CONFIG_TCP_MSS

#include <string.h>
#include <unistd.h>

#include "mw/mw-msg.h"
#include "mw/util.h"

#include <driver/uart.h>
#include <semphr.h>

/// Number of buffer frames available
#define LSD_BUF_FRAMES			2

/// Start of data in the buffer (skips STX and LEN fields).
#define LSD_BUF_DATA_START 		3

/// Threshold of the RX FIFO to generate RTS signal
#define LSD_RX_RTS_THR	(128 - 16)

// Macro to ease access to current reception buffer
#define RXB 	lsdData.rx[lsdData.current]

// Macro to ease access to current transaction buffer
#define TXB 	lsdData.tx[lsdData.currenttx]

class LSD {
public:

	LSD(){};

	QueueHandle_t q;
	QueueHandle_t qTx = NULL;

	/** \addtogroup lsd LsdState Allowed states for reception state machine.
	 *  \{ */
	typedef enum {
		LSD_ST_IDLE = 0,		///< Currently inactive
		LSD_ST_STX_WAIT,		///< Waiting for STX
		LSD_ST_CH_LENH_RECV,		///< Receiving channel and length (high bits)
		LSD_ST_LEN_RECV,		///< Receiving frame length
		LSD_ST_DATA_RECV,		///< Receiving data length
		LSD_ST_ETX_RECV,		///< Receiving ETX
		LSD_ST_MAX			///< Number of states
	} LsdState;
	/** \} */

	/** \addtogroup lsd LsdData Local data required by the module.
	 *  \{ */
	typedef struct {
		MwMsgBuf rx[LSD_BUF_FRAMES];	///< Reception buffers.
		SemaphoreHandle_t sem;		///< Semaphore to control buffers
		LsdState rxs;			///< Reception state
		uint8_t en[LSD_MAX_CH];		///< Channel enable
		uint16_t pos;			///< Position in current buffer
		uint8_t current;		///< Current buffer in use

		MwMsgBuf tx[LSD_BUF_FRAMES];	///< transaction buffers.
		uint8_t currenttx;		///< Current buffer in use
	} LsdData;
	/** \} */

	/*
	* Private prototypes
	*/
	//static void LsdRecvTsk(void *pvParameters);
	void LsdRecv(const uint8_t *data, size_t data_len);
	/// Module data
	LsdData lsdData;

	/************************************************************************//**
	* Module initialization. Call this function before any other one in this
	* module.
	****************************************************************************/
	void LsdInit(QueueHandle_t q, QueueHandle_t qtx);

	/************************************************************************//**
	* Enables a channel to start reception and be able to send data.
	*
	* \param[in] ch Channel number.
	*
	* \return A pointer to an empty TX buffer, or NULL if no buffer is
	*         available.
	****************************************************************************/
	int LsdChEnable(uint8_t ch);

	/************************************************************************//**
	* Disables a channel to stop reception and prohibit sending data.
	*
	* \param[in] ch Channel number.
	*
	* \return A pointer to an empty TX buffer, or NULL if no buffer is
	*         available.
	****************************************************************************/
	int LsdChDisable(uint8_t ch);


	/************************************************************************//**
	* Sends data through a previously enabled channel.
	*
	* \param[in] data Buffer to send.
	* \param[in] len  Length of the buffer to send.
	* \param[in] ch   Channel number to use.
	*
	* \return -1 if there was an error, or the number of characterse sent
	* 		   otherwise.
	****************************************************************************/
	size_t LsdSend(const uint8_t *data, uint16_t len, uint8_t ch);

	/************************************************************************//**
	* Starts sending data through a previously enabled channel. Once started,
	* you can send more additional data inside of the frame by issuing as
	* many LsdSplitNext() calls as needed, and end the frame by calling
	* LsdSplitEnd().
	*
	* \param[in] data  Buffer to send.
	* \param[in] len   Length of the data buffer to send.
	* \param[in] total Total length of the data to send using a split frame.
	* \param[in] ch    Channel number to use for sending.
	*
	* \return -1 if there was an error, or the number of characterse sent
	* 		   otherwise.
	****************************************************************************/
	size_t LsdSplitStart(uint8_t *data, uint16_t len,
						uint16_t total, uint8_t ch);

	/************************************************************************//**
	* Appends (sends) additional data to a frame previously started by an
	* LsdSplitStart() call.
	*
	* \param[in] data  Buffer to send.
	* \param[in] len   Length of the data buffer to send.
	*
	* \return -1 if there was an error, or the number of characterse sent
	* 		   otherwise.
	****************************************************************************/
	size_t LsdSplitNext(uint8_t *data, uint16_t len);

	/************************************************************************//**
	* Appends (sends) additional data to a frame previously started by an
	* LsdSplitStart() call, and finally ends the frame.
	*
	* \param[in] data  Buffer to send.
	* \param[in] len   Length of the data buffer to send.
	*
	* \return -1 if there was an error, or the number of characterse sent
	* 		   otherwise.
	****************************************************************************/
	size_t LsdSplitEnd(uint8_t *data, uint16_t len);

	/************************************************************************//**
	* Frees the oldest receive buffer. This function must be called each time
	* a buffer is processed to allow receiving a new frame.
	*
	* \warning Calling this function more times than buffers have been received,
	* will likely cause overruns!
	****************************************************************************/
	void LsdRxBufFree(void);

};

