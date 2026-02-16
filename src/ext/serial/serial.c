/************************************************************************
 * \brief Simple Serial driver.
 *
 * \author Juan Antonio Ruiz (PaCHoN)
 * \date   2025
 * \defgroup SERIAL SERIAL
 * \brief
 *      Serial Implementation. Base on https://github.com/rhargreaves/mega-drive-serial-port.
*************************************** */
#include "config.h"

#if (MODULE_SERIAL)
#include "ext/serial/serial.h"
static IoPort io_port;

static const struct {
    size_t ctrl;
    size_t sctrl;
    size_t tx;
    size_t rx;
} regs[] = {
    { EXT_CTRL, EXT_SCTRL, EXT_TX, EXT_RX },
    { PORT2_CTRL, PORT2_SCTRL, PORT2_TX, PORT2_RX },
};


void serial_set_sctrl(u8 value)
{
    *((vu8*)(regs[io_port].sctrl)) = value;
}

void _serial_set_ctrl(u8 value)
{
    *((vu8*)(regs[io_port].ctrl)) = value;
}

u8 serial_get_sctrl(void)
{
    return *((vu8*)(regs[io_port].sctrl));
}

bool serial_read_ready(void)
{
    return *((vu8*)(regs[io_port].sctrl)) & SCTRL_RRDY;
}

u8 serial_read(void)
{
    return *((vu8*)(regs[io_port].rx));
}

u16 serial_baud_rate(void)
{
    switch (serial_get_sctrl() & 0xC0) {
    case SCTRL_300_BPS:
        return 300;
    case SCTRL_1200_BPS:
        return 1200;
    case SCTRL_2400_BPS:
        return 2400;
    default:
        return 4800;
    }
}

void serial_init(void)
{   
    IoPort port = IoPort_Ctrl2;
    u8 sctrlFlags = SCTRL_4800_BPS | SCTRL_SIN | SCTRL_SOUT | SCTRL_RINT;
    io_port = port;
    serial_set_sctrl(sctrlFlags);
    _serial_set_ctrl(CTRL_PCS_OUT);
    if (sctrlFlags & SCTRL_RINT) {
        SYS_setInterruptMaskLevel(INT_MASK_LEVEL_ENABLE_ALL);
        VDP_setReg(VDP_MODE_REG_3, VDP_getReg(VDP_MODE_REG_3) | VDP_IE2);
    }
    serial_reset_fifos();
}

bool serial_is_present(void)
{
    return true;
}

bool serial_write_ready(void)
{
    return !((*((vu8*)(regs[io_port].sctrl))) & SCTRL_TFUL);
}

void serial_write(u8 data)
{
    *((vu8*)(regs[io_port].tx)) = data;
}

void serial_reset_fifos(void)
{
    while (serial_read_ready()) {
        serial_read();
    }
}

#endif // MODULE_SERIAL