/************************************************************************
 * \brief Simple SSF driver to Everdrive X7.
 *
 * \author Juan Antonio Ruiz (PaCHoN)
 * \date   2024-2025
 * \defgroup SSF SSF
 * \brief
 *      USB IO
 *        0xA130E2 [........ DDDDDDDD] read/write
 *        D data bits
 * 
 *      IO status
 *        0xA130E4 [.C...... .....RWS] read only
 *        S SPI controller ready. Not used on WIFI.
 *        W USB fifo ready to write
 *        R USB fifo ready to read
 *        C SD card type. 0=SD, 1=SDHC. Not used on WIFI.
*************************************** */
#ifndef _SSF_ED_X7_H_
#define _SSF_SSF_ED_X7_H_

#include "types.h"

//#define SSF_CTRL_P 0x8000 //register accesss protection bit. should be set, otherwise register will ignore any attempts to write
//#define SSF_CTRL_X 0x4000 //32x mode
//#define SSF_CTRL_W 0x2000 //ROM memory write protection
//#define SSF_CTRL_L 0x1000 //led

//#define USB_RD_BUSY while ((REG_STE & STE_USB_RD_RDY) == 0)
//#define USB_WR_BUSY while ((REG_STE & STE_USB_WR_RDY) == 0)
/**************************************** */


/// SSF UART base address
#define X7_UART_BASE		0xA13000

/// Receiver holding register. Read only.
#define X7_UART_DATA	          (*((volatile u16*)(X7_UART_BASE +  226)))
#define X7_UART_STE	          (*((volatile u16*)(X7_UART_BASE +  228)))
#define X7_UART_REG_CFG          (*((volatile u16*)(X7_UART_BASE +  230)))
#define X7_UART_REG_SSF_CTRL     (*((volatile u16*)(X7_UART_BASE +  240)))

#define SSF_CTRL_P 0x8000 //register accesss protection bit. should be set, otherwise register will ignore any attempts to write
#define SSF_CTRL_X 0x4000 //32x mode
#define SSF_CTRL_W 0x2000 //ROM memory write protection
#define SSF_CTRL_L 0x1000 //led

#define X7_UART_SPR X7_UART_DATA

#define X7_UART_STE_WR_RDY 2//usb write ready bit
#define X7_UART_STE_RD_RDY 4//usb read ready bit

//spi chip select signal
#define CFG_SPI_SS 1
#define CFG_SPI_QRD 6
#define CFG_SPI_QWR 2

#define MW_EDX7_BUFLEN	1436
#define MW_EDX7_TXFIFO_LEN 1

bool ssf_ed_x7_is_present(void);
void ssf_ed_x7_write(u8 data);
bool ssf_ed_x7_read_ready(void);
bool ssf_ed_x7_write_ready(void);
u8 ssf_ed_x7_read(void);

/************************************************************************//**
 * \brief Initializes the driver on Everdrive X7.
 ****************************************************************************/
void ssf_ed_x7_init(void);

#endif /*_SSF_ED_X7_H_*/

