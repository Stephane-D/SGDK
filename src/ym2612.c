#include "config.h"
#include "types.h"

#include "ym2612.h"

#include "vdp.h"
#include "z80_ctrl.h"


void YM2612_reset()
{
    u16 i;

    Z80_requestBus(1);

    // enable left and right output for all channel
    for(i = 0; i < 3; i++)
    {
        YM2612_writeSafe(0, 0xB4 | i);
        YM2612_writeSafe(1, 0xC0);
        YM2612_writeSafe(2, 0xB4 | i);
        YM2612_writeSafe(3, 0xC0);
    }

    // disable LFO
    YM2612_writeSafe(0, 0x22);
    YM2612_writeSafe(1, 0x00);

    // disable timer & set channel 6 to normal mode
    YM2612_writeSafe(0, 0x27);
    YM2612_writeSafe(1, 0x00);

    // ALL KEY OFF
    YM2612_writeSafe(0, 0x28);
    for (i = 0; i < 3; i++)
    {
        YM2612_writeSafe(1, 0x00 | i);
        YM2612_writeSafe(1, 0x04 | i);
    }

    // disable DAC
    YM2612_writeSafe(0, 0x2B);
    YM2612_writeSafe(1, 0x00);

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
    vu8 *pb;

    pb = (u8*) YM2612_BASEPORT;

    pb[port & 3] = data;
}

void YM2612_writeSafe(const u16 port, const u8 data)
{
    vu8 *pb;

    pb = (u8*) YM2612_BASEPORT;

    // wait while YM2612 busy
    while (*pb & 0x80);
    // write data
    pb[port & 3] = data;
}

void YM2612_writeReg(const u16 part, const u8 reg, const u8 data)
{
    vu8 *pb;
    u16 port;

    pb = (u8*) YM2612_BASEPORT;
    port = (part << 1) & 2;

    // set reg and write data
    pb[port + 0] = reg;
    pb[port + 1] = data;
}

void YM2612_writeRegSafe(const u16 part, const u8 reg, const u8 data)
{
    vu8 *pb;
    u16 port;

    pb = (u8*) YM2612_BASEPORT;
    port = (part << 1) & 2;

    // wait while YM2612 busy
    while (*pb & 0x80);
    // set reg
    pb[port + 0] = reg;
    // wait while YM2612 busy
    while (*pb & 0x80);
    // set data
    pb[port + 1] = data;
}


void YM2612_enableDAC()
{
    // enable DAC
    YM2612_writeSafe(0, 0x2B);
    YM2612_writeSafe(1, 0x80);
}

void YM2612_disableDAC()
{
    // disable DAC
    YM2612_writeSafe(0, 0x2B);
    YM2612_writeSafe(1, 0x00);
}
