#include "genesis.h"

#include "psg_state.h"


static void PSGState_writeLow(PSG* psg, u16 value);
static void PSGState_writeHigh(PSG* psg, u16 value);


void PSGState_init(PSG* psg)
{
    u16 i;

    for (i = 0; i < 4; i++)
    {
        psg->env[i] = 0x0F;
        psg->tone[i] = 0x00;
    }

    psg->index = 0;
    psg->type = 0;
}


void PSGState_XGMWrites(PSG *psg, u8* data, u16 num)
{
    u8 *src;

    src = data;
    while(num--)
        PSGState_write(psg, *src++);
}

void PSGState_write(PSG* psg, u16 value)
{
    if (value & 0x80)
        PSGState_writeLow(psg, value & 0x7F);
    else
        PSGState_writeHigh(psg, value & 0x7F);
}


static void PSGState_writeLow(PSG* psg, u16 value)
{
    u16 index = (value >> 5) & 0x03;
    u16 type = (value >> 4) & 0X01;

    if (type == 0)
        psg->tone[index] = (psg->tone[index] & ~0xF) | (value & 0xF);
    else
        psg->env[index] = (psg->env[index] & ~0xF) | (value & 0xF);

    psg->index = index;
    psg->type = type;
}

static void PSGState_writeHigh(PSG* psg, u16 value)
{
    u16 index = psg->index;

    if (psg->type == 0)
        psg->tone[index] = (psg->tone[index] & ~0x3F0) | ((value & 0x3F) << 4);
    else
        psg->env[index] = (psg->env[index] & ~0xF) | (value & 0xF);
}
