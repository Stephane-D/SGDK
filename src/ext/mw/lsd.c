/************************************************************************//**
 * \brief  Local Symmetric Data-link. Implements an extremely simple
 *         protocol to link two full-duplex devices, multiplexing the
 *         data link.
 *
 * \author Jesus Alonso (doragasu)
 * \date   2016
 * \todo   Implement UART RTS/CTS handshaking.
 * \todo   Current implementation uses polling. Unfortunately as the Genesis/
 *         Megadrive does not have an interrupt pin on the cart, implementing
 *         more efficient data transmission techniques will be tricky.
 * \todo   Proper implementation of error handling.
 ****************************************************************************/
#include "config.h"
#include "types.h"
#include "string.h"
#include "memory.h"


#if (MODULE_MEGAWIFI != 0)

#include "ext/mw/lsd.h"
/// Uart used for LSD
#define LSD_UART		0

/// Start/end of transmission character
#define LSD_STX_ETX		0x7E

/// Start of data in the buffer (skips STX and LEN fields).
#define LSD_BUF_DATA_START 		3

/// Allowed states for the reception state machine.
enum recv_state {
	LSD_RECV_ERROR = -1,	///< An error has occurred
	LSD_RECV_PARTIAL = 0,	///< Partial frame was received
	LSD_RECV_IDLE,		///< Currently inactive
	LSD_RECV_STX,		///< Waiting for STX
	LSD_RECV_CH_LENH,	///< Receiving channel and length (high bits)
	LSD_RECV_LEN,		///< Receiving frame length
	LSD_RECV_DATA,		///< Receiving data length
	LSD_RECV_ETX,		///< Receiving ETX
	LSD_RECV_MAX		///< Number of states
};

/// Allowed states for the sending state machine.
enum send_state {
	LSD_SEND_ERROR = -1,	///< An error has occurred
	LSD_SEND_IDLE = 0,	///< Currently inactive
	LSD_SEND_STX,           ///< Sending STX
	LSD_SEND_CH_LENH,       ///< Sending channel and length (high bits)
	LSD_SEND_LEN,           ///< Sending frame length
	LSD_SEND_DATA,          ///< Sending data length
	LSD_SEND_ETX,           ///< Sending ETX
	LSD_SEND_MAX            ///< Number of states
};

/// Data holding the send state
struct send_data {
	enum send_state stat;	///< Status of the send process
	const char *buf;	///< Send buffer
	int16_t pos;		///< Buffer position
	int16_t total; 		///< Total bytes to send
	void *ctx;		///< Send context
	lsd_send_cb cb;		///< Send completion callback
	uint8_t ch;		///< Send channel
};

/// Data holding the recv state
struct recv_data {
	enum recv_state stat;	///< Status of the recv process
	char *buf;		///< Receive buffer
	int16_t pos;		///< Buffer position
	int16_t frame_len;	///< Length of received frame
	int16_t max;		///< Buffer size
	void *ctx;		///< Receive context
	lsd_recv_cb cb;		///< Reception callback
	uint8_t ch;		///< Reception channel
};

/// Local data required by the module.
struct lsd_data {
	struct send_data tx;
	struct recv_data rx;
	uint8_t ch_enable[LSD_MAX_CH];
};

/// Module global data
static struct lsd_data d = {};

static void recv_error(enum lsd_status stat)
{
//	d.rx.stat = LSD_RECV_ERROR;
	d.rx.stat = LSD_RECV_IDLE;
	if (d.rx.cb) {
		d.rx.cb(stat, 0, NULL, 0, d.rx.ctx);
	}
}

static void recv_complete(void)
{
	if (d.rx.cb) {
		d.rx.cb(LSD_STAT_COMPLETE, d.rx.ch, d.rx.buf,
				d.rx.pos, d.rx.ctx);
	}
}

static void recv_add(uint8_t recv)
{
	d.rx.buf[d.rx.pos++] = recv;
	if (d.rx.pos >= d.rx.frame_len) {
		d.rx.stat = LSD_RECV_ETX;
	} else if (d.rx.pos >= d.rx.max) {
		// Filled the available buffer space, so force
		// a frame completion and flag partial reception
		d.rx.frame_len -= d.rx.pos;
		d.rx.stat = LSD_RECV_PARTIAL;
		recv_complete();
	}
}

static void process_recv(void)
{
	uint8_t recv = uart_getc();

	switch (d.rx.stat) {
	case LSD_RECV_STX:	// Wait for STX to arrive
		if (LSD_STX_ETX == recv) {
			d.rx.stat = LSD_RECV_CH_LENH;
		}
		break;

	case LSD_RECV_CH_LENH:	// Receive CH and len high
		// Check special case: if we receive ETX here,
		// then this is the real STX (previous one was ETX from
		// previous frame).
		if (!(LSD_STX_ETX == recv)) {
			d.rx.ch = recv>>4;
			d.rx.frame_len = (recv & 0x0F)<<8;
			// Sanity check (not exceding number of channels)
			if (d.rx.ch >= LSD_MAX_CH ||
					!d.ch_enable[d.rx.ch]) {
				recv_error(LSD_STAT_ERR_INVALID_CH);
			} else {
				d.rx.stat = LSD_RECV_LEN;
			}
		}
		break;

	case LSD_RECV_LEN:	// Receive len low
		d.rx.frame_len |= recv;
		d.rx.pos = 0;
		if (d.rx.frame_len) {
			// If there's payload, receive it. Else wait for ETX
			d.rx.stat = LSD_RECV_DATA;
		} else {
			d.rx.stat = LSD_RECV_ETX;
		}
		break;

	case LSD_RECV_DATA:	// Receive payload
		recv_add(recv);
		break;

	case LSD_RECV_ETX:	// ETX should come here
		if (LSD_STX_ETX == recv) {
			d.rx.stat = LSD_RECV_IDLE;
			recv_complete();
		} else {
			// Error, ETX not received.
			recv_error(LSD_STAT_ERR_FRAMING);
		}
		break;

	default:
		// Should not receive data in other states
		recv_error(LSD_STAT_ERROR);
		break;
	}
}

static void send_complete(void)
{
	d.tx.stat = LSD_SEND_IDLE;
	if (d.tx.cb) {
		d.tx.cb(LSD_STAT_COMPLETE, d.tx.ctx);
	}
}

static void process_send(void)
{
	switch (d.tx.stat) {
	case LSD_SEND_STX:
		uart_putc(LSD_STX_ETX);
		d.tx.stat = LSD_SEND_CH_LENH;
		break;

	case LSD_SEND_CH_LENH:
		uart_putc((d.tx.ch<<4) | (d.tx.total>>8));
		d.tx.stat = LSD_SEND_LEN;
		break;

	case LSD_SEND_LEN:
		uart_putc(d.tx.total & 0xFF);
		d.tx.stat = LSD_SEND_DATA;
		break;

	case LSD_SEND_DATA:
		uart_putc(d.tx.buf[d.tx.pos++]);
		if (d.tx.pos >= d.tx.total) {
			d.tx.stat = LSD_SEND_ETX;
		}
		break;

	case LSD_SEND_ETX:
		uart_putc(LSD_STX_ETX);
		send_complete();
		break;
	default:
		break;
	}
}

void lsd_process(void)
{
	int active;

	do {
		active = FALSE;
		if (d.rx.stat > LSD_RECV_IDLE && uart_rx_ready()) {
			active = TRUE;
			while (d.rx.stat > LSD_RECV_IDLE && uart_rx_ready()) {
				process_recv();
			}
		}
		if (d.tx.stat > LSD_SEND_IDLE && uart_tx_ready()) {
			active = TRUE;
			for (int i = 0; i < UART_TX_FIFO_LEN &&
					d.tx.stat > LSD_SEND_IDLE; i++) {
				process_send();
			}
		}
	} while(active);
}

void lsd_init(void)
{
	uart_init();
	memset(&d, 0, sizeof(struct lsd_data));
	d.rx.stat = LSD_RECV_IDLE;
	lsd_line_sync();
}

int lsd_ch_enable(uint8_t ch)
{
	if (ch >= LSD_MAX_CH) {
		return LSD_STAT_ERROR;
	}

	d.ch_enable[ch] = TRUE;
	return LSD_STAT_COMPLETE;
}

int lsd_ch_disable(uint8_t ch)
{
	if (ch >= LSD_MAX_CH) {
		return LSD_STAT_ERROR;
	}

	d.ch_enable[ch] = FALSE;
	return LSD_STAT_COMPLETE;
}

/// \todo Should we call the send callback on errors?
enum lsd_status lsd_send(uint8_t ch, const char *data, int16_t len,
		void *ctx, lsd_send_cb send_cb)
{
	if (d.tx.stat > LSD_SEND_IDLE) {
		return LSD_STAT_ERR_IN_PROGRESS;
	}
	if (ch >= LSD_MAX_CH || !d.ch_enable[ch]) {
		return LSD_STAT_ERR_INVALID_CH;
	}
	if (len > LSD_MAX_LEN) {
		return LSD_STAT_ERR_FRAME_TOO_LONG;
	}

	d.tx.ch = ch;
	d.tx.buf = data;
	d.tx.total = len;
	d.tx.cb = send_cb;
	d.tx.ctx = ctx;
	d.tx.pos = 0;
	d.tx.stat = LSD_SEND_STX;

	return LSD_STAT_BUSY;
}

enum lsd_status lsd_recv(char *buf, int16_t len, void *ctx, lsd_recv_cb recv_cb)
{
	if (len >= (LSD_MAX_LEN + 1)) {
		return LSD_STAT_ERR_FRAME_TOO_LONG;
	}

	if (LSD_RECV_IDLE == d.rx.stat) {
		d.rx.stat = LSD_RECV_STX;
	} else if (LSD_RECV_PARTIAL == d.rx.stat) {
		d.rx.pos = 0;
		d.rx.stat = LSD_RECV_DATA;
	}

	d.rx.buf = buf;
	d.rx.max = len;
	d.rx.cb = recv_cb;
	d.rx.ctx = ctx;

	return LSD_STAT_BUSY;
}

enum lsd_status lsd_send_sync(uint8_t ch, const char *data, int16_t len)
{
	enum lsd_status stat;

	stat = lsd_send(ch, data, len, NULL, NULL);

	if (stat <= LSD_STAT_COMPLETE) {
		return stat;
	}

	// Poll until sending is completed
	while (d.tx.stat > LSD_SEND_IDLE) {
		lsd_process();
	}

	if (LSD_SEND_IDLE == d.tx.stat) {
		return LSD_STAT_COMPLETE;
	} else {
		return LSD_STAT_ERROR;
	}
}

enum lsd_status lsd_recv_sync(char *buf, uint16_t *len, uint8_t *ch)
{
	enum lsd_status stat;

	stat = lsd_recv(buf, *len, NULL, NULL);

	if (stat <= LSD_STAT_COMPLETE) {
		return stat;
	}

	while (d.rx.stat > LSD_RECV_IDLE) {
		lsd_process();
	}

	if (LSD_RECV_IDLE == d.rx.stat) {
		*len = d.rx.pos;
		*ch = d.rx.ch;
		return LSD_STAT_COMPLETE;
	} else {
		return LSD_STAT_ERROR;
	}
}

void lsd_line_sync(void)
{
	for (int i = 0; i < 256; i++) {
		if (uart_tx_ready()) {
			uart_putc(0x55);
		}
	}
}

#endif // MODULE_MEGAWIFI