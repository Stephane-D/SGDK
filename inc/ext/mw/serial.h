/************************************************************************
 * \brief Simple SSF driver to Everdrive PRO.
 *
 * \author Juan Antonio Ruiz (PaCHoN)
 * \date   2024-2025
 * \defgroup SSF SSF
 * \brief
 *      https://github.com/krikzz/mega-ed-pub
*************************************** */
#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "genesis.h"
#include "types.h"

#define PORT2_CTRL 0xA1000B
#define PORT2_SCTRL 0xA10019
#define PORT2_TX 0xA10015
#define PORT2_RX 0xA10017

#define EXT_CTRL 0xA1000D
#define EXT_SCTRL 0xA1001F
#define EXT_TX 0xA1001B
#define EXT_RX 0xA1001D

#define SCTRL_SIN 0x20
#define SCTRL_SOUT 0x10
#define SCTRL_300_BPS 0xC0
#define SCTRL_1200_BPS 0x80
#define SCTRL_2400_BPS 0x40
#define SCTRL_4800_BPS 0x00

#define SCTRL_TFUL 0x1
#define SCTRL_RRDY 0x2
#define SCTRL_RERR 0x4
#define SCTRL_RINT 0x8

#define CTRL_PCS_OUT 0x7F

#define VDP_MODE_REG_3 0xB
#define VDP_IE2 0x08
#define INT_MASK_LEVEL_ENABLE_ALL 1

#define SERIAL_BUFLEN	4096
#define SERIAL_BUF_CAPACITY (SERIAL_BUFLEN - 1)
#define SERIAL_TXFIFO_LEN 512

typedef enum IoPort {
    IoPort_Ext,
    IoPort_Ctrl2
} IoPort;

typedef enum { RING_BUF_OK = 0, RING_BUF_EMPTY, RING_BUF_FULL, RING_BUF_ERROR } ring_buf_status_t;

bool serial_is_present(void);
void serial_write(u8 data);
bool serial_read_ready(void);
bool serial_write_ready(void);
u8 serial_read(void);
u16 serial_get_buff_length(void);
u16 serial_get_tx_fifo_length(void);

/************************************************************************//**
 * \brief Initializes the driver on Everdrive PRO.
 ****************************************************************************/
void serial_init(void);

void serial_reset_fifos(void);
u16 serial_baud_rate(void);

#endif /*_SERIAL_H_*/
