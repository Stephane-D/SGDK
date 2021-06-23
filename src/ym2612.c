#include "config.h"
#include "types.h"

#include "ym2612.h"

#include "vdp.h"
#include "z80_ctrl.h"


static void writeSlotReg(u16 port, u8 ch, u8 sl, u8 reg, u8 value)
{
    YM2612_write((port * 2) + 0, reg | (sl * 4) | ch);
    YM2612_write((port * 2) + 1, value);
}

static void writeChannelReg(u16 port, u8 ch, u8 reg, u8 value)
{
    YM2612_write((port * 2) + 0, reg | ch);
    YM2612_write((port * 2) + 1, value);
}

void YM2612_reset()
{
    u16 p, ch, sl;
    u16 busTaken;

    busTaken = Z80_getAndRequestBus(TRUE);

    // disable LFO
    YM2612_write(0, 0x22);
    YM2612_write(1, 0x00);

    // disable timer & set channel 6 to normal mode
    YM2612_write(0, 0x27);
    YM2612_write(1, 0x00);

    // disable DAC
    YM2612_write(0, 0x2B);
    YM2612_write(1, 0x00);

    for(p = 0; p < 1; p++)
    {
        for(ch = 0; ch < 3; ch++)
        {
            for(sl = 0; sl < 4; sl++)
            {
                // DT1 - MUL
                writeSlotReg(p, ch, sl, 0x30, 0x00);
                // TL set to max (silent)
                writeSlotReg(p, ch, sl, 0x40, 0x7F);
                // RS - AR
                writeSlotReg(p, ch, sl, 0x50, 0x00);
                // AM - D1R
                writeSlotReg(p, ch, sl, 0x60, 0x00);
                // D2R
                writeSlotReg(p, ch, sl, 0x70, 0x00);
                // D1L - RR set to max
                writeSlotReg(p, ch, sl, 0x80, 0xFF);
                // SSG-EG
                writeSlotReg(p, ch, sl, 0x90, 0x00);
            }
        }
    }

    for(p = 0; p < 1; p++)
    {
        for(ch = 0; ch < 3; ch++)
        {
            // Freq LSB
            writeChannelReg(p, ch, 0xA0, 0x00);
            // Block - Freq MSB
            writeChannelReg(p, ch, 0xA4, 0x00);
            // Freq LSB - CH3 spe
            writeChannelReg(p, ch, 0xA8, 0x00);
            // Block - Freq MSB - CH3 spe
            writeChannelReg(p, ch, 0xAC, 0x00);
            // Feedback - Algo
            writeChannelReg(p, ch, 0xB0, 0x00);
            // enable LR output
            writeChannelReg(p, ch, 0xB4, 0xC0);
        }
    }

    // ALL KEY OFF
    YM2612_write(0, 0x28);
    for (ch = 0; ch < 3; ch++)
    {
        YM2612_write(1, 0x00 | ch);
        YM2612_write(1, 0x04 | ch);
    }

    if (!busTaken)
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

    pb = (s8*) YM2612_BASEPORT;

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

    pb = (s8*) YM2612_BASEPORT;
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
