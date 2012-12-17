#include "config.h"
#include "types.h"

#include "psg.h"

#include "vdp.h"


void PSG_init()
{
    vu8 *pb;
    u16 i;

    pb = (u8*) PSG_PORT;

    for (i = 0; i < 4; i++)
    {
        // set tone to 0
        *pb = 0x80 | (i << 5) | 0x00;
        *pb = 0x00;

        // set envelope to silent
        *pb = 0x90 | (i << 5) | 0x0F;
    }
}


void PSG_write(u8 data)
{
    vu8 *pb;

    pb = (u8*) PSG_PORT;
    *pb = data;
}


void PSG_setEnvelope(u8 channel, u8 value)
{
    vu8 *pb;

    pb = (u8*) PSG_PORT;
    *pb = 0x90 | ((channel & 3) << 5) | (value & 0xF);
}

void PSG_setTone(u8 channel, u16 value)
{
    vu8 *pb;

    pb = (u8*) PSG_PORT;
    *pb = 0x80 | ((channel & 3) << 5) | (value & 0xF);
    *pb = (value >> 4) & 0x3F;
}

void PSG_setFrequency(u8 channel, u16 value)
{
    u16 data;

    if (value)
    {
        // frequency to tone conversion
        if (IS_PALSYSTEM) data = 3546893 / (value * 32);
        else data = 3579545 / (value * 32);
    }
    else data = 0;

    PSG_setTone(channel, data);
}

void PSG_setNoise(u8 type, u8 frequency)
{
    vu8 *pb;

    pb = (u8 *) PSG_PORT;
    *pb = 0xE0 | ((type & 1) << 2) | (frequency & 0x3);
}
