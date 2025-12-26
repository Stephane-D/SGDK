/************************************************************************
 * \brief COMM driver.
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

u16 comm_get_buffer_length(void);
u16 comm_get_tx_fifo_length(void);

void comm_init(void);
void comm_write(u8 data);
bool comm_read_ready(void);
bool comm_write_ready(void);
u8 comm_read(void);
char* comm_mode(void);
#endif /*_COMM_H_*/

