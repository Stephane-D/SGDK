/************************************************************************
 * \brief Simple SSF driver.
 *
 * \author Juan Antonio Ruiz (PaCHoN)
 * \date   2025
 * \defgroup SSF SSF
 * \brief
 *      SSF Implementation.
*************************************** */
#include "config.h"

#if (MODULE_MEGAWIFI == 1)

#include "ext/mw/ssf_ed_pro.h"

static u8 bi_fifo_busy(void)
{
    return (REG_FIFO_STAT & FIFO_CPU_RXF) ? 1 : 0;
}

static void bi_fifo_rd(void* data, u16 len)
{
    u8* data8 = data;
    u16 block = 0;

    while (len) {

        block = REG_FIFO_STAT & FIFO_RXF_MSK;
        if (block > len)
            block = len;
        len -= block;

        while (block >= 4) {
            *data8++ = REG_FIFO_DATA;
            *data8++ = REG_FIFO_DATA;
            *data8++ = REG_FIFO_DATA;
            *data8++ = REG_FIFO_DATA;
            block -= 4;
        }

        while (block--)
            *data8++ = REG_FIFO_DATA;
    }
}

static void bi_fifo_wr(void* data, u16 len)
{
    u8* data8 = data;

    while (len--) {
        REG_FIFO_DATA = *data8++;
    }
}

static void bi_cmd_tx(u8 cmd)
{
    u8 buff[4];
    buff[0] = '+';
    buff[1] = '+' ^ 0xff;
    buff[2] = cmd;
    buff[3] = cmd ^ 0xff;
    bi_fifo_wr(buff, sizeof(buff));
}

static void bi_cmd_usb_wr(void* data, u16 len)
{
    bi_cmd_tx(CMD_USB_WR);
    bi_fifo_wr(&len, 2);
    bi_fifo_wr(data, len);
}

bool ssf_ed_pro_is_present(void){
     return (REG_SYS_STAT & 0xFFF0) == STAT_PRO_PRESENT;
}

void ssf_ed_pro_write(u8 data){
    bi_cmd_usb_wr(&data, 1);
    (void)data;
}

bool ssf_ed_pro_read_ready(void){
    return !bi_fifo_busy();
}

bool ssf_ed_pro_write_ready(void){
    return TRUE;
}

u8 ssf_ed_pro_read(void){
    u8 data;
    bi_fifo_rd(&data, 1);
    return data;
}

void ssf_ed_pro_init(void) {
}

u16 ssf_ed_pro_get_buff_length(void){
    return MW_EDPRO_BUFLEN;
}

u16 ssf_ed_pro_get_tx_fifo_length(void){
    return MW_EDPRO_TXFIFO_LEN;
}

#endif // MODULE_MEGAWIFI SSF IMPL
