/************************************************************************
 * \brief COMM driver Interface.
 *
 * \author Juan Antonio Ruiz (PaCHoN)
 * \date   2024-2025
 * \defgroup COMM COMM
 * \brief
 *      Interface for COMM communication over Everdrive and EverdrivePro.
*************************************** */
#ifndef _COMM_H_
#define _COMM_H_

#include "types.h"
/**************************************** */

/**
 * \brief Get the current receive buffer length.
 * \return Number of bytes available to read in the receive buffer.
 */
u16 comm_get_buffer_length(void);

/**
 * \brief Get the current transmit FIFO length.
 * \return Number of bytes currently in the transmit FIFO.
 */
u16 comm_get_tx_fifo_length(void);

/**
 * \brief Initialize the COMM communication interface.
 * 
 * Must be called before using any other COMM functions.
 */
void comm_init(void);

/**
 * \brief Write a single byte to the COMM interface.
 * \param data The byte to transmit.
 */
void comm_write(u8 data);

/**
 * \brief Check if data is available to read.
 * \return TRUE if data is available in the receive buffer, FALSE otherwise.
 */
bool comm_read_ready(void);

/**
 * \brief Check if the COMM interface is ready to accept data for transmission.
 * \return TRUE if write buffer has space available, FALSE otherwise.
 */
bool comm_write_ready(void);

/**
 * \brief Read a single byte from the COMM interface.
 * \return The byte read from the receive buffer.
 * \note Call comm_read_ready() first to ensure data is available.
 */
u8 comm_read(void);

/**
 * \brief Get the current COMM operation mode.
 * \return A string describing the current communication mode.
 */
char* comm_mode(void);
#endif /*_COMM_H_*/

