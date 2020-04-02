#include "config.h"
#include "types.h"

#include "vdp.h"
#include "vdp_dma.h"

#include "dma.h"


void VDP_doDMAEx(u8 location, u32 from, u16 to, u16 len, s16 vramStep)
{
    DMA_doDma(location, (void*) from, to, len, vramStep);
}

void VDP_doDMA(u8 location, u32 from, u16 to, u16 len)
{
    DMA_doDma(location, (void*) from, to, len, 2);
}

void VDP_doVRamDMAFill(u16 to, u16 len, u8 value)
{
    DMA_doVRamFill(to, len, value, 1);
}

void VDP_doVRamDMACopy(u16 from, u16 to, u16 len)
{
    DMA_doVRamCopy(from, to, len, 1);
}
