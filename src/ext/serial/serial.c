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

static IoPort io_port = IoPort_Ctrl2;

static const struct {
    size_t ctrl;
    size_t sctrl;
    size_t tx;
    size_t rx;
} regs[] = {
    { EXT_CTRL, EXT_SCTRL, EXT_TX, EXT_RX },
    { PORT2_CTRL, PORT2_SCTRL, PORT2_TX, PORT2_RX },
};

typedef u8 (*serial_read_func)(void);
typedef bool (*serial_read_ready_func)(void);



#include "ext/serial/buffer.h"

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

bool serial_read_ready_sync(void)
{
    return *((vu8*)(regs[io_port].sctrl)) & SCTRL_RRDY;
}

bool serial_read_ready_async(void)
{
    return buffer_canRead();
}

u8 serial_read_async(void)
{
    return buffer_read();
}

u8 serial_read_sync(void)
{
    return *((vu8*)(regs[io_port].rx));
}

static void serial_async_callback(void)
{
    buffer_write(serial_read_sync());
}

static serial_read_ready_func serial_read_ready_impl = serial_read_ready_sync;
static serial_read_func serial_read_impl = serial_read_sync;

u8 serial_read(void)
{
   return serial_read_impl();
}

bool serial_read_ready(void)
{
    return serial_read_ready_impl();
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
    u8 sctrlFlags = SCTRL_4800_BPS | SCTRL_SIN | SCTRL_SOUT | SCTRL_RINT;
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

u16 serial_set_mode(u16 value){
    if(value){
        serial_read_ready_impl = serial_read_ready_async;
        serial_read_impl = serial_read_async;
        SYS_setExtIntCallback(&serial_async_callback);
        return buffer_init(value);
    }else{
        serial_read_ready_impl = serial_read_ready_sync;
        serial_read_impl = serial_read_sync;
        SYS_setExtIntCallback(NULL);
        buffer_free();
        return 0;
    }
}

void serial_set_port(IoPort port){
    io_port = port;
}

IoPort serial_get_port(){
    return io_port;
}

#endif // MODULE_SERIAL