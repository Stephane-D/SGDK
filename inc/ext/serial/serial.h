/************************************************************************
 * \brief Simple Serial Driver to comunicate by game port.
 *
 * \author Juan Antonio Ruiz (PaCHoN)
 * \date   2024-2025
 * \defgroup MW Serial
 * \brief
 *      ## Hardware
 * 
 *      ### Controller Port Pinout
 *      
 *      | Pin | Parallel Mode \*    | Serial Mode | I/O Reg Bit |
 *      | --- | ------------------- | ----------- | ----------- |
 *      | 1   | Up                  |             | UP (D0)     |
 *      | 2   | Down                |             | DOWN (D1)   |
 *      | 3   | Gnd / Left          |             | LEFT (D2)   |
 *      | 4   | Gnd / Right         |             | RIGHT (D3)  |
 *      | 5   | +5VDC               | +5VDC       |
 *      | 6   | Button A / Button B | Tx          | TL (D4)     |
 *      | 7   | Select              |             |
 *      | 8   | Gnd                 | Gnd         |
 *      | 9   | Start / Button C    | Rx          | TR (D5)     |
 *      
 *      \* Pins in parallel mode can have two interpretations, depending on if `Select` has been set low or high (L / H) from the console.
 *      
 *      ### Controller Plug
 *      
 *      Looking directly at plug (Female 9-pin Type D)
 *      
 *      ```
 *      -------------
 *      \ 5 4 3 2 1 /
 *       \ 9 8 7 6 /
 *        ---------
 *      ```
 *      
 *      ### FTDI USB TTL Serial Cable
 *      
 *      From the [datasheet](https://www.ftdichip.com/Support/Documents/DataSheets/Cables/DS_TTL-232RG_CABLES.pdf):
 *      
 *      | Colour | Name | Type            | Description                                                                                                                        |
 *      | ------ | ---- | --------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
 *      | Black  | GND  | GND             | Device ground supply pin.                                                                                                          |
 *      | Brown  | CTS# | Input           | Clear to Send Control input / Handshake signal.                                                                                    |
 *      | Red    | VCC  | Output or input | Power Supply Output except for the TTL-232RG-VIPWE were this is an input and power is supplied by the application interface logic. |
 *      | Orange | TXD  | Output          | Transmit Asynchronous Data output.                                                                                                 |
 *      | Yellow | RXD  | Input           | Receive Asynchronous Data input.                                                                                                   |
 *      | Green  | RTS# | Output          | Request To Send Control Output / Handshake signal.                                                                                 |
 *      
 *      ### Mappings
 *      
 *      | FTDI Cable   | Mega Drive Port Pin |
 *      | ------------ | ------------------- |
 *      | Red (VCC)    | 5 (5v)              |
 *      | Orange (TXD) | 9 (Rx)              |
 *      | Yellow (RXD) | 6 (Tx)              |
 *      | Black (GND)  | 8 (Gnd)             |
 *      *************************************** */
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

#define SERIAL_BUFLEN	1
#define SERIAL_TXFIFO_LEN 1

typedef enum IoPort {
    IoPort_Ext,
    IoPort_Ctrl2
} IoPort;

bool serial_is_present(void);
void serial_write(u8 data);
bool serial_read_ready(void);
bool serial_write_ready(void);
u8 serial_read(void);
void serial_init(void);

void serial_reset_fifos(void);
u16 serial_baud_rate(void);
u8 serial_get_sctrl(void);
void serial_set_sctrl(u8 value);

#endif /*_SERIAL_H_*/
