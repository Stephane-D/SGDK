#include "ext/mw/serial.h"

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

static bool recvData = false;

static volatile u16 tail = 0;
static volatile u16 head = 0;
static volatile u8 ring_buf[SERIAL_BUFLEN];

ring_buf_status_t ring_buf_read(u8* data)
{
    if (data == NULL) {
        return RING_BUF_ERROR;
    }

    if (tail == head) {
        return RING_BUF_EMPTY;
    }

    *data = ring_buf[tail];
    tail = (tail + 1) % SERIAL_BUFLEN;
    return RING_BUF_OK;
}

ring_buf_status_t ring_buf_write(u8 data)
{
    u16 nextHead = (head + 1) % SERIAL_BUFLEN;
    if (nextHead == tail) {
        return RING_BUF_FULL;
    }

    ring_buf[head] = data;
    head = nextHead;
    return RING_BUF_OK;
}

u16 ring_buf_available(void)
{
    if (tail == head) {
        return SERIAL_BUF_CAPACITY;
    } else if (tail < head) {
        return SERIAL_BUF_CAPACITY - (head - tail);
    } else {
        return tail - head - 1;
    }
}

bool ring_buf_can_write(void)
{
    u16 nextHead = (head + 1) % SERIAL_BUFLEN;
    return nextHead != tail;
}

static void _serial_set_sctrl(u8 value)
{
    *((vu8*)(regs[io_port].sctrl)) = value;
}

static void _serial_set_ctrl(u8 value)
{
    *((vu8*)(regs[io_port].ctrl)) = value;
}

u8 _serial_sctrl(void)
{
    return *((vu8*)(regs[io_port].sctrl));
}

bool _serial_readyToReceive(void)
{
    return *((vu8*)(regs[io_port].sctrl)) & SCTRL_RRDY;
}

u8 _serial_receive(void)
{
    return *((vu8*)(regs[io_port].rx));
}

static void recv_ready_callback(void)
{
    while (_serial_readyToReceive()) {
        recvData = true;

        ring_buf_status_t status = ring_buf_write(_serial_receive());
        if (status == RING_BUF_FULL) {
            // log_warn("Serial: Buffer overflow!");
            break;
        }
    }
}

u16 baud_rate(void)
{
    switch (_serial_sctrl() & 0xC0) {
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
    tail = 0;
    head = 0;
    // 0 o 1
    IoPort port = IoPort_Ctrl2;
    u8 sctrlFlags = SCTRL_4800_BPS | SCTRL_SIN | SCTRL_SOUT | SCTRL_RINT;
    io_port = port;
    _serial_set_sctrl(sctrlFlags);
    _serial_set_ctrl(CTRL_PCS_OUT);
    if (sctrlFlags & SCTRL_RINT) {
        SYS_setInterruptMaskLevel(INT_MASK_LEVEL_ENABLE_ALL);
        VDP_setReg(VDP_MODE_REG_3, VDP_getReg(VDP_MODE_REG_3) | VDP_IE2);
        SYS_setExtIntCallback(&recv_ready_callback);
    }
    serial_reset_fifos();
}

bool serial_is_present(void)
{
    return true;
}

bool serial_read_ready(void)
{
    if (!recvData)
        return false;
    return tail != head;
}

u8 serial_read(void)
{
    u8 data = 0;
    ring_buf_status_t status = ring_buf_read(&data);
    if (status == RING_BUF_OK) {
        // u16 bufferAvailable = ring_buf_available();
        // if (bufferAvailable < 32) {
        //    log_warn("Serial: Buffer free = %d bytes", bufferAvailable);
        // }
        return data;
    }

    // if (status == RING_BUF_EMPTY) {
    //     log_warn("Serial: Attempted read from empty buffer");
    // }

    return 0;
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
    while (_serial_readyToReceive()) {
        _serial_receive();
    }
}

u16 serial_get_buff_length(void){
    return SERIAL_BUFLEN;
}

u16 serial_get_tx_fifo_length(void){
    return SERIAL_TXFIFO_LEN;
}