/************************************************************************
 * \brief Simple SSF driver to Everdrive PRO.
 *
 * \author Juan Antonio Ruiz (PaCHoN)
 * \date   2024-2025
 * \defgroup SSF SSF
 * \brief
 *      https://github.com/krikzz/mega-ed-pub
*************************************** */
#ifndef _SSF_ED_PRO_H_
#define _SSF_ED_PRO_H_

#include "types.h"

#define REG_FIFO_DATA (*((volatile u16*)(0xA130D0)))
#define REG_FIFO_STAT (*((volatile u16*)(0xA130D2)))
#define REG_SYS_STAT (*((volatile u16*)(0xA130D4)))

#define FIFO_CPU_RXF 0x8000 // fifo flags. system cpu can read
#define FIFO_RXF_MSK 0x7FF
#define STAT_PRO_PRESENT 0x55A0
#define CMD_USB_WR 0x22

#define MW_EDPRO_BUFLEN	1436
#define MW_EDPRO_TXFIFO_LEN 512

bool ssf_ed_pro_is_present(void);
void ssf_ed_pro_write(u8 data);
bool ssf_ed_pro_read_ready(void);
bool ssf_ed_pro_write_ready(void);
u8 ssf_ed_pro_read(void);
u16 ssf_ed_pro_get_buff_length(void);
void ssf_ed_pro_reset(void);
void ssf_ed_pro_reset_fifos(void);
void ssf_ed_pro_start(void);
u16 ssf_ed_pro_get_tx_fifo_length(void);

/************************************************************************//**
 * \brief Initializes the driver on Everdrive PRO.
 ****************************************************************************/
void ssf_ed_pro_init(void);

#endif /*_SSF_ED_PRO_H_*/
