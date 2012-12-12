#include "genesis.h"

#include "sram.h"


void SRAM_enable()
{
    *(vu8*)SRAM_CONTROL = 1;
}

void SRAM_enableRO()
{
    *(vu8*)SRAM_CONTROL = 3;
}

void SRAM_disable()
{
    *(vu8*)SRAM_CONTROL = 0;
}


u8 SRAM_readByte(u32 offset)
{
    return *(vu8*)(SRAM_BASE + (offset * 2));
}

void SRAM_writeByte(u32 offset, u8 val)
{
    *(vu8*)(SRAM_BASE + (offset * 2)) = val;
}
