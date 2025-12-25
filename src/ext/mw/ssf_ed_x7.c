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

#if (MODULE_MEGAWIFI == 1)

#include "ext/mw/ssf_ed_x7.h"

#define IO_STATUS_HI_SD 0x00
#define IO_STATUS_HI_SDHC 0x40

bool ssf_ed_x7_is_present(void){
    /* REG_STE values:
    0x3F00 with OpenEmu v2.4.1
    0x3F00 with Exodus
    0x3F00 with Regen v0.97d
    0xFFFF with Fusion 3.6.4
    0x3F00 with BlastEm nightly (0.6.3-pre-4c418ee9a9d8) Win
    0x3015 with BlastEm nightly (0.6.3-pre) (also 0x3014)
    0x4003 when ME X7 idle with USB in
    0x4009 when ME X7 idle loaded via SD without USB cable connected
    0x4003 when ME X7 idle loaded via SD with USB cable connected
    0x3F00 with ME PRO
    */

    u8 status = X7_UART_STE >> 8;
    return status == IO_STATUS_HI_SD || status == IO_STATUS_HI_SDHC;
}

void ssf_ed_x7_write(u8 data){
    X7_UART_DATA = data;
}
bool ssf_ed_x7_read_ready(void){
    return (X7_UART_STE & X7_UART_STE_RD_RDY);
}
bool ssf_ed_x7_write_ready(void){
    return (X7_UART_STE & X7_UART_STE_WR_RDY);
}
u8 ssf_ed_x7_read(void){
    return X7_UART_DATA;
}

volatile u16 ctrl;
volatile u16 cfg_io;

void ssf_set_rom_bank(u8 bank, u8 val) {
	*((volatile u16 *)(X7_UART_BASE + X7_UART_REG_SSF_CTRL + bank * 2u)) = val;
}

void ssf_ed_x7_init(void) {
    ctrl = SSF_CTRL_P;
    cfg_io = CFG_SPI_SS;

    X7_UART_REG_SSF_CTRL = ctrl;
    X7_UART_REG_CFG = cfg_io;

    for (u8 i = 1; i < 8; i++)ssf_set_rom_bank(i, i);
}

u16 ssf_ed_x7_get_buff_length(void) {
    return MW_EDX7_BUFLEN;
}

u16 ssf_ed_x7_get_tx_fifo_length(void) {
    return MW_EDX7_TXFIFO_LEN;
}

#endif // MODULE_MEGAWIFI SSF IMPL
