/************************************************************************
 * \brief Simple SSF driver.
 *
 * \author Juan Antonio Ruiz (PaCHoN)
 * \date   2024
 * \defgroup SSF SSF
 * \brief
 *      SSF Implementation.
*************************************** */
#include "config.h"

#if (MODULE_MEGAWIFI == 1 && MODULE_EVERDRIVE == 1)

#include "ext/mw/ssf.h"

volatile u16 ctrl;
volatile u16 cfg_io;

void uart_init(void) {
    ctrl = SSF_CTRL_P;
    cfg_io = CFG_SPI_SS;

    UART_REG_SSF_CTRL = ctrl;
    UART_REG_CFG = cfg_io;

    for (u8 i = 1; i < 8; i++)ssf_set_rom_bank(i, i);
}

void ssf_set_rom_bank(u8 bank, u8 val) {
	*((volatile u16 *)(UART_BASE + UART_REG_SSF_CTRL + bank * 2u)) = val;
}

#endif // MODULE_MEGAWIFI SSF IMPL