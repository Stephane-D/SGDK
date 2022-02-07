/************************************************************************//**
 * \brief  Local Symmetric Data-link. Implements an extremely simple
 *         protocol to link two full-duplex devices, multiplexing the
 *         data link.
 *
 * The multiplexing facility allows having up to LSD_MAX_CH simultaneous
 * channels on the serial link.
 *
 * The module has synchronous functions to send/receive data (easy to use, but
 * due to polling hang the console until transfer is complete) and their
 * asyncronous counterparts. The asynchronous functions return immediately,
 * but require calling frequently lsd_process() to actually send/receive data.
 * Once the asynchronous functions complete sending/receiving data, the
 * specified callback is run.
 *
 * \author Jesus Alonso (doragasu)
 * \date   2019
 * \note   Unfortunately the Megadrive does have neither an interrupt pin nor
 *         DMA threshold pins in the cartridge slot, so polling is the only
 *         way. So you have
 *         Megadrive does not have an interrupt pin on the cart, implementing
 *         more efficient data transmission techniques will be tricky.
 * \warning The syncrhonous API is easier to use, but a lot less reliable:
 * * It polls, using all the CPU until the send/recv operation completes.
 * * A lsd_recv_sync() can freeze the machine if no frame is received. USE IT
 *   WITH CARE!
 *
 * \defgroup lsd lsd
 * \{
 ****************************************************************************/

/*
 * Frame format is:
 *
 * STX : CH-LENH : LENL : DATA : ETX
 *
 * - STX and ETX are the start/end of transmission characters (1 byte each).
 * - CH-LENH is the channel number (first 4 bits) and the 4 high bits of the
 *   data length.
 * - LENL is the low 8 bits of the data length.
 * - DATA is the payload, of the previously specified length.
 */
#ifndef _LSD_H_
#define _LSD_H_

#include "16c550.h"
#include "mw-msg.h"

#if (MODULE_MEGAWIFI != 0)

/// LSD frame overhead in bytes
#define LSD_OVERHEAD		4

/// Maximum number of available simultaneous channels
#define LSD_MAX_CH		4

/// Maximum data payload length
#define LSD_MAX_LEN		 4095

/// Number of buffer frames available
#define LSD_BUF_FRAMES		2

/// Return status codes for LSD functions
enum lsd_status {
	LSD_STAT_ERR_FRAMING = -5,		///< Frame format error
	LSD_STAT_ERR_INVALID_CH = -4,		///< Invalid channel
	LSD_STAT_ERR_FRAME_TOO_LONG = -3,	///< Frame is too long
	LSD_STAT_ERR_IN_PROGRESS = -2,		///< Operation in progress
	LSD_STAT_ERROR = -1,			///< General error
	LSD_STAT_COMPLETE = 0,			///< No error
	LSD_STAT_BUSY = 1			///< Doing requested operation
};

/// Callback for the asynchronous lsd_send() function.
typedef void (*lsd_send_cb)(enum lsd_status stat, void *ctx);
/// Callback for the asynchronous lsd_recv() function.
typedef void (*lsd_recv_cb)(enum lsd_status stat, uint8_t ch,
		char *data, uint16_t len, void *ctx);

/************************************************************************//**
 * \brief Module initialization.
 ****************************************************************************/
void lsd_init(void);

/************************************************************************//**
 * \brief Enables a channel to start reception and be able to send data.
 *
 * \param[in] ch Channel number.
 *
 * \return LSD_OK on success, LSD_ERROR otherwise.
 ****************************************************************************/
int lsd_ch_enable(uint8_t ch);

/************************************************************************//**
 * \brief Disables a channel to stop reception and prohibit sending data.
 *
 * \param[in] ch Channel number.
 *
 * \return LSD_OK on success, LSD_ERROR otherwise.
 ****************************************************************************/
int lsd_ch_disable(uint8_t ch);


/************************************************************************//**
 * \brief Asynchronously sends data through a previously enabled channel.
 *
 * \param[in] ch      Channel number to use.
 * \param[in] data    Buffer to send.
 * \param[in] len     Length of the buffer to send.
 * \param[in] ctx     Context for the send callback function.
 * \param[in] send_cb Callback to run when send completes or errors.
 *
 * \return Status of the send procedure. Usually LSD_STAT_BUSY is returned,
 * and the send procedure is then performed in background.
 * \note Calling this function while there is a send procedure in progress,
 * will cause the function call to fail with LSD_STAT_SEND_ERR_IN_PROGRESS.
 ****************************************************************************/
enum lsd_status lsd_send(uint8_t ch, const char *data, int16_t len,
		void *ctx, lsd_send_cb send_cb);

/************************************************************************//**
 * \brief Synchronously sends data through a previously enabled channel.
 *
 * \param[in] ch   Channel number to use.
 * \param[in] data Buffer to send.
 * \param[in] len  Length of the buffer to send.
 *
 * \return Status of the send procedure.
 * \warning This function polls until the procedure is complete (or errors).
 ****************************************************************************/
enum lsd_status lsd_send_sync(uint8_t ch, const char *data, int16_t len);

/************************************************************************//**
 * \brief Asyncrhonously Receives a frame using LSD protocol.
 *
 * \param[in] buf     Buffer for reception.
 * \param[in] len     Buffer length.
 * \param[in] ctx     Context for the receive callback function.
 * \param[in] recv_cb Callback to run when receive completes or errors.
 *
 * \return Status of the receive procedure.
 ****************************************************************************/
enum lsd_status lsd_recv(char *buf, int16_t len, void *ctx,
		lsd_recv_cb recv_cb);

/************************************************************************//**
 * \brief Syncrhonously Receives a frame using LSD protocol.
 *
 * \param[out]   buf Buffer for received data.
 * \param[inout] len On input: buffer length. On output: received frame length.
 * \param[out]   ch  Channel on which the data has been received.
 *
 * \warning This function polls until the reception is complete, or a reception
 * error occurs.
 * \warning If no frame is received when this function is called, the machine
 * will lock.
 ****************************************************************************/
enum lsd_status lsd_recv_sync(char *buf, uint16_t *len, uint8_t *ch);

/************************************************************************//**
 * \brief Processes sends/receives pending data.
 *
 * Call this function as much as possible when using the asynchronous
 * lsd_send() and lsd_receive() functions.
 ****************************************************************************/
void lsd_process(void);

/************************************************************************//**
 * \brief Sends syncrhonization frame.
 *
 * This function sends a chunk of 0x55 bytes to help physical layer to
 * synchronize. It is usually not necessary to use this function, but might
 * help some UART chips to compute an accurate clock.
 ****************************************************************************/
void lsd_line_sync(void);

#endif // MODULE_MEGAWIFI

#endif //_LSD_H_

/** \} */
