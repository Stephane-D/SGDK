/**
 * \file vdp_dma.c
 * \brief VDP DMA support
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides methods to use the VDP DMA capabilities.
 */

#include "config.h"
#include "types.h"

#include "vdp.h"
#include "vdp_dma.h"

#include "z80_ctrl.h"


void VDP_doDMA(u8 location, u32 from, u16 to, u16 len)
{
    vu16 *pw;
    vu32 *pl;
    u32 newlen;
    u32 banklimit;

    // DMA works on 128 KB bank
    banklimit = 0x20000 - (from & 0x1FFFF);
    // bank limit exceded
    if (len > banklimit)
    {
        // we first do the second bank transfert
        VDP_doDMA(location, from + banklimit, to + banklimit, len - banklimit);
        newlen = banklimit;
    }
    // ok, use normal len
    else newlen = len;

    VDP_setAutoInc(2);

    pw = (u16 *) GFX_CTRL_PORT;

    // Setup DMA length (in word here)
    newlen >>= 1;
    *pw = 0x9300 + (newlen & 0xff);
    newlen >>= 8;
    *pw = 0x9400 + (newlen & 0xff);

    // Setup DMA address
    from >>= 1;
    *pw = 0x9500 + (from & 0xff);
    from >>= 8;
    *pw = 0x9600 + (from & 0xff);
    from >>= 8;
    *pw = 0x9700 + (from & 0x7f);

    // Halt the Z80 for DMA
//    Z80_RequestBus(0);

    // Enable DMA
    pl = (u32 *) GFX_CTRL_PORT;
    switch(location)
    {
        case VDP_DMA_VRAM:
            *pl = GFX_DMA_VRAM_ADDR(to);
            break;

        case VDP_DMA_CRAM:
            *pl = GFX_DMA_CRAM_ADDR(to);
            break;

        case VDP_DMA_VSRAM:
            *pl = GFX_DMA_VSRAM_ADDR(to);
            break;
    }

    // Enable Z80
//    Z80_ReleaseBus();
}

void VDP_doDMAFill(u8 location, u16 to, u16 len, u8 value)
{
    vu16 *pw;
    vu32 *pl;

    pw = (u16 *) GFX_CTRL_PORT;

    // Setup DMA length
    *pw = 0x9300 + (len & 0xff);
    len >>= 8;
    *pw = 0x9400 + (len & 0xff);

    // Setup DMA operation (VRAM FILL)
    *pw = 0x9780;

    // Enable DMA
    pl = (u32 *) GFX_CTRL_PORT;
    switch(location)
    {
        case VDP_DMA_VRAM:
            VDP_setAutoInc(1);
            *pl = GFX_DMA_VRAM_ADDR(to);
            break;

        case VDP_DMA_CRAM:
            VDP_setAutoInc(2);
            *pl = GFX_DMA_CRAM_ADDR(to);
            break;

        case VDP_DMA_VSRAM:
            VDP_setAutoInc(2);
            *pl = GFX_DMA_VSRAM_ADDR(to);
            break;
    }

    // set up value to fill (need to be 16 bits extended)
    pw = (u16 *) GFX_DATA_PORT;
    *pw = value | (value << 8);
}

void VDP_doDMACopy(u8 location, u16 from, u16 to, u16 len)
{
    vu16 *pw;
    vu32 *pl;

    pw = (u16 *) GFX_CTRL_PORT;

    // Setup DMA length
    *pw = 0x9300 + (len & 0xff);
    len >>= 8;
    *pw = 0x9400 + (len & 0xff);

    // Setup DMA address
    *pw = 0x9500 + (from & 0xff);
    *pw = 0x9600 + ((from >> 8) & 0xff);

    // Setup DMA operation (VRAM COPY)
    *pw = 0x97C0;

    // Enable DMA
    pl = (u32 *) GFX_CTRL_PORT;
    switch(location)
    {
        case VDP_DMA_VRAM:
            VDP_setAutoInc(1);
            *pl = GFX_DMA_VRAM_ADDR(to);
            break;

        case VDP_DMA_CRAM:
            VDP_setAutoInc(2);
            *pl = GFX_DMA_CRAM_ADDR(to);
            break;

        case VDP_DMA_VSRAM:
            VDP_setAutoInc(2);
            *pl = GFX_DMA_VSRAM_ADDR(to);
            break;
    }
}
