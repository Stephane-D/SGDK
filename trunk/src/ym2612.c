#include "config.h"
#include "types.h"

#include "ym2612.h"

#include "vdp.h"
#include "z80_ctrl.h"


void YM2612_reset()
{
    u16 i;
    u16 bus_taken;

    bus_taken = Z80_isBusTaken();
    if (!bus_taken)
        Z80_requestBus(1);

    // enable left and right output for all channel
    for(i = 0; i < 3; i++)
    {
        YM2612_write(0, 0xB4 | i);
        YM2612_write(1, 0xC0);
        YM2612_write(2, 0xB4 | i);
        YM2612_write(3, 0xC0);
    }

    // disable LFO
    YM2612_write(0, 0x22);
    YM2612_write(1, 0x00);

    // disable timer & set channel 6 to normal mode
    YM2612_write(0, 0x27);
    YM2612_write(1, 0x00);

    // ALL KEY OFF
    YM2612_write(0, 0x28);
    for (i = 0; i < 3; i++)
    {
        YM2612_write(1, 0x00 | i);
        YM2612_write(1, 0x04 | i);
    }

    // disable DAC
    YM2612_write(0, 0x2B);
    YM2612_write(1, 0x00);

    if (!bus_taken)
        Z80_releaseBus();
}


u8 YM2612_read(const u16 port)
{
    vu8 *pb;

    pb = (u8*) YM2612_BASEPORT;

    return pb[port & 3];
}

void YM2612_write(const u16 port, const u8 data)
{
    vs8 *pb;

    pb = (u8*) YM2612_BASEPORT;

    // wait while YM2612 busy
    while (*pb < 0);
    // write data
    pb[port & 3] = data;
}

void YM2612_writeSafe(const u16 port, const u8 data)
{
    YM2612_write(port, data);
}

void YM2612_writeReg(const u16 part, const u8 reg, const u8 data)
{
    vs8 *pb;
    u16 port;

    pb = (u8*) YM2612_BASEPORT;
    port = (part << 1) & 2;

    // wait while YM2612 busy
    while (*pb < 0);
    // set reg
    pb[port + 0] = reg;

    // busy flag is not updated immediatly, force wait (needed on MD2)
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");

    // wait while YM2612 busy
    while (*pb < 0);
    // set data
    pb[port + 1] = data;
}

void YM2612_writeRegSafe(const u16 part, const u8 reg, const u8 data)
{
    YM2612_writeReg(part, reg, data);
}


void YM2612_enableDAC()
{
    // enable DAC
    YM2612_write(0, 0x2B);
    YM2612_write(1, 0x80);
}

void YM2612_disableDAC()
{
    // disable DAC
    YM2612_write(0, 0x2B);
    YM2612_write(1, 0x00);
}
