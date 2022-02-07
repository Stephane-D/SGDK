/************************************************************************//**
 * \brief Simple 16C550 UART chip driver.
 *
 * \author Jesus Alonso (doragasu)
 * \date   2016
 * \defgroup 16C550 16c550
 * \{
 ****************************************************************************/

#ifndef _16C550_H_
#define _16C550_H_

#include "types.h"

#if (MODULE_MEGAWIFI != 0)

/// 16C550 UART base address
#define UART_BASE		0xA130C1

/// Clock applied to 16C550 chip. Currently using 24 MHz crystal
#define UART_CLK		24000000LU

/// Desired baud rate. Maximum achievable baudrate with 24  MHz crystal
/// is 24000000/16 = 1.5 Mbps
#define UART_BR			1500000LU
//#define UART_BR			500000LU
//#define UART_BR			750000LU
//#define UART_BR			115200

/// Length of the TX FIFO in bytes
#define UART_TX_FIFO_LEN		16

/// Division with one bit rounding, useful for divisor calculations.
#define DivWithRounding(dividend, divisor)	((((dividend)*2/(divisor))+1)/2)
/// Value to load on the UART divisor, high byte
#define UART_DLM_VAL	(DivWithRounding(UART_CLK, 16 * UART_BR)>>8)
//#define UART_DLM_VAL	((UART_CLK/16/UART_BR)>>8)
/// Value to load on the UART divisor, low byte
#define UART_DLL_VAL	(DivWithRounding(UART_CLK, 16 * UART_BR) & 0xFF)
//#define UART_DLL_VAL	((UART_CLK/16/UART_BR)&0xFF)

/** \addtogroup UartRegs UartRegs
 *  \brief 16C550 UART registers
 *  \note Do NOT access IER, FCR, LCR and MCR directly, use Set/Get functions.
 *        Remaining registers can be directly accessed, but meeting the
 *        read only/write only restrictions.
 *  \{
 */
/// Receiver holding register. Read only.
#define UART_RHR	(*((volatile uint8_t*)(UART_BASE +  0)))
/// Transmit holding register. Write only.
#define UART_THR	(*((volatile uint8_t*)(UART_BASE +  0)))
/// Interrupt enable register. Write only.
#define UART_IER	(*((volatile uint8_t*)(UART_BASE +  2)))
/// FIFO control register. Write only.
#define UART_FCR	(*((volatile uint8_t*)(UART_BASE +  4)))
/// Interrupt status register. Read only.
#define UART_ISR	(*((volatile uint8_t*)(UART_BASE +  4)))
/// Line control register. Write only.
#define UART_LCR	(*((volatile uint8_t*)(UART_BASE +  6)))
/// Modem control register. Write only.
#define UART_MCR	(*((volatile uint8_t*)(UART_BASE +  8)))
/// Line status register. Read only.
#define UART_LSR	(*((volatile uint8_t*)(UART_BASE + 10)))
/// Modem status register. Read only.
#define UART_MSR	(*((volatile uint8_t*)(UART_BASE + 12)))
/// Scratchpad register.
#define UART_SPR	(*((volatile uint8_t*)(UART_BASE + 14)))
/// Divisor latch LSB. Acessed only when LCR[7] = 1.
#define UART_DLL	(*((volatile uint8_t*)(UART_BASE +  0)))
/// Divisor latch MSB. Acessed only when LCR[7] = 1.
#define UART_DLM	(*((volatile uint8_t*)(UART_BASE +  2)))
/** \} */

/// Structure with the shadow registers.
typedef struct {
	uint8_t IER;	///< Interrupt Enable Register
	uint8_t FCR;	///< FIFO Control Register
	uint8_t LCR;	///< Line Control Register
	uint8_t MCR;	///< Modem Control Register
} UartShadow;

/// Uart shadow registers. Do NOT access directly!
extern UartShadow sh;

/** \addtogroup UartOuts UartOuts
 *  \brief Output pins controlled by the MCR UART
 *  register.
 *  \{ */
#define UART_MCR__DTR		0x01	///< Data Terminal Ready.
#define UART_MCR__RTS		0x02	///< Request To Send.
#define UART_MCR__OUT1		0x04	///< GPIO pin 1.
#define UART_MCR__OUT2		0x08	///< GPIO pin 2.
/** \} */

/** \addtogroup UartIns UartIns
 *  \brief Input pins readed in the MSR UART register.
 *  \{ */
#define UART_MSR__DSR		0x20	///< Data Set Ready
/** \} */

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
#define uart_tx_ready()	(UART_LSR & 0x20)

/************************************************************************//**
 * \brief Checks if UART receive register/FIFO has data available.
 *
 * \return TRUE if at least 1 byte is available, FALSE otherwise.
 ****************************************************************************/
#define uart_rx_ready()	(UART_LSR & 0x01)

/************************************************************************//**
 * \brief Sends a character. Please make sure there is room in the transmit
 *        register/FIFO by calling uart_rx_ready() before using this function.
 *
 * \return Received character.
 ****************************************************************************/
#define uart_putc(c)		do{UART_RHR = (c);}while(0);

/************************************************************************//**
 * \brief Returns a received character. Please make sure data is available by
 *        calling uart_rx_ready() before using this function.
 *
 * \return Received character.
 ****************************************************************************/
#define uart_getc()		(UART_RHR)

/************************************************************************//**
 * \brief Sets a value in IER, FCR, LCR or MCR register.
 *
 * \param[in] reg Register to modify (IER, FCR, LCR or MCR).
 * \param[in] val Value to set in IER, FCR, LCR or MCR register.
 ****************************************************************************/
#define uart_set(reg, val)	do{sh.reg = (val);UART_##reg = (val);}while(0)

/************************************************************************//**
 * \brief Gets value of IER, FCR, LCR or MCR register.
 *
 * \param[in] reg Register to read (IER, FCR, LCR or MCR).
 * \return The value of the requested register.
 ****************************************************************************/
#define uart_get(reg)		(sh.reg)

/************************************************************************//**
 * \brief Sets bits in IER, FCR, LCR or MCR register.
 *
 * \param[in] reg Register to modify (IER, FCR, LCR or MCR).
 * \param[in] val Bits set in val, will be set in reg register.
 ****************************************************************************/
#define uart_set_bits(reg, val)	do{sh.reg |= (val);			\
	UART_##reg = sh.reg;}while(0)

/************************************************************************//**
 * \brief Clears bits in IER, FCR, LCR or MCR register.
 *
 * \param[in] reg Register to modify (IER, FCR, LCR or MCR).
 * \param[in] val Bits set in val, will be cleared in reg register.
 ****************************************************************************/
#define uart_clr_bits(reg, val)	do{sh.reg &= ~(val);			\
	UART_##reg = sh.reg;}while(0)

/************************************************************************//**
 * \brief Reset TX and RX FIFOs.
 ****************************************************************************/
#define uart_reset_fifos()	uart_set_bits(FCR, 0x07)

#endif // MODULE_MEGAWIFI

#endif /*_16C550_H_*/

/** \} */

