/************************************************************************
 * \brief Simple SSF driver.
 *
 * \author Juan Antonio Ruiz (PaCHoN)
 * \date   2024
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
#ifndef _SSF_H_
#define _SSF_H_

#include "config.h"
#include "types.h"

#if (MODULE_MEGAWIFI != 0 && MW_IMPLEMENTATION == MW_IMP_EVERDRIVE_X7)
//#define SSF_CTRL_P 0x8000 //register accesss protection bit. should be set, otherwise register will ignore any attempts to write
//#define SSF_CTRL_X 0x4000 //32x mode
//#define SSF_CTRL_W 0x2000 //ROM memory write protection
//#define SSF_CTRL_L 0x1000 //led

//#define USB_RD_BUSY while ((REG_STE & STE_USB_RD_RDY) == 0)
//#define USB_WR_BUSY while ((REG_STE & STE_USB_WR_RDY) == 0)
/**************************************** */


/// SSF UART base address
#define UART_BASE		0xA13000

/// Length of the TX FIFO in bytes
#define UART_TX_FIFO_LEN		1

/// Receiver holding register. Read only.
#define UART_DATA	          (*((volatile u16*)(UART_BASE +  226)))
#define UART_STE	          (*((volatile u16*)(UART_BASE +  228)))
#define UART_REG_CFG          (*((volatile u16*)(UART_BASE +  230)))
#define UART_REG_SSF_CTRL     (*((volatile u16*)(UART_BASE +  240)))

#define SSF_CTRL_P 0x8000 //register accesss protection bit. should be set, otherwise register will ignore any attempts to write
#define SSF_CTRL_X 0x4000 //32x mode
#define SSF_CTRL_W 0x2000 //ROM memory write protection
#define SSF_CTRL_L 0x1000 //led

#define UART_SPR UART_DATA

#define UART_STE_WR_RDY 2//usb write ready bit
#define UART_STE_RD_RDY 4//usb read ready bit

//spi chip select signal
#define CFG_SPI_SS 1
#define CFG_SPI_QRD 6
#define CFG_SPI_QWR 2

/************************************************************************//**
 * \brief Initializes the driver. The baud rate is set to UART_BR, and the
 *        UART FIFOs are enabled. This function must be called before using
 *        any other API call.
 ****************************************************************************/
void uart_init(void);

/************************************************************************//**
 * \brief Checks if UART transmit register/FIFO is ready. In FIFO mode, up to
 *        16 characters can be loaded each time transmitter is ready.
 *
 * \return TRUE if transmitter is ready, FALSE otherwise.
 ****************************************************************************/
#define uart_tx_ready()	(UART_STE & UART_STE_WR_RDY)

/************************************************************************//**
 * \brief Checks if UART receive register/FIFO has data available.
 *
 * \return TRUE if at least 1 byte is available, FALSE otherwise.
 ****************************************************************************/
#define uart_rx_ready()	(UART_STE & UART_STE_RD_RDY)

/************************************************************************//**
 * \brief Sends a character. Please make sure there is room in the transmit
 *        register/FIFO by calling uart_rx_ready() before using this function.
 *
 * \return Received character.
 ****************************************************************************/
#define uart_putc(c)		do{UART_DATA = (c);}while(0);

/************************************************************************//**
 * \brief Returns a received character. Please make sure data is available by
 *        calling uart_rx_ready() before using this function.
 *
 * \return Received character.
 ****************************************************************************/
#define uart_getc()		(UART_DATA)

/************************************************************************//**
 * \brief Sets a value in IER, FCR, LCR or MCR register.
 *
 * \param[in] reg Register to modify (IER, FCR, LCR or MCR).
 * \param[in] val Value to set in IER, FCR, LCR or MCR register.
 ****************************************************************************/
#define uart_set(reg, val)	while(0);

/************************************************************************//**
 * \brief Gets value of IER, FCR, LCR or MCR register.
 *
 * \param[in] reg Register to read (IER, FCR, LCR or MCR).
 * \return The value of the requested register.
 ****************************************************************************/
#define uart_get(reg)		while(0);

/************************************************************************//**
 * \brief Sets bits in IER, FCR, LCR or MCR register.
 *
 * \param[in] reg Register to modify (IER, FCR, LCR or MCR).
 * \param[in] val Bits set in val, will be set in reg register.
 ****************************************************************************/
#define uart_set_bits(reg, val)	while(0)

/************************************************************************//**
 * \brief Clears bits in IER, FCR, LCR or MCR register.
 *
 * \param[in] reg Register to modify (IER, FCR, LCR or MCR).
 * \param[in] val Bits set in val, will be cleared in reg register.
 ****************************************************************************/
#define uart_clr_bits(reg, val)	while(0)

/************************************************************************//**
 * \brief Reset TX and RX FIFOs.
 ****************************************************************************/
#define uart_reset_fifos()	while(0)

#endif 
#endif /*_SSF_H_*/

