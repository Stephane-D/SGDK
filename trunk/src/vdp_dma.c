#include "config.h"
#include "types.h"

#include "vdp.h"
#include "vdp_dma.h"

#include "z80_ctrl.h"


void VDP_doDMAEx(u8 location, u32 from, u16 to, u16 len, s16 vramStep)
{
    vu16 *pw;
    vu32 *pl;
    u32 newlen;
    u32 banklimitb;
    u32 banklimitw;

    // DMA works on 64 KW bank
    banklimitb = 0x20000 - (from & 0x1FFFF);
    banklimitw = banklimitb >> 1;
    // bank limit exceded
    if (len > banklimitw)
    {
        // we first do the second bank transfert
        VDP_doDMA(location, from + banklimitb, to + banklimitb, len - banklimitw);
        newlen = banklimitw;
    }
    // ok, use normal len
    else newlen = len;

    if (vramStep != -1)
        VDP_setAutoInc(vramStep);

    pw = (u16 *) GFX_CTRL_PORT;

    // Setup DMA length (in word here)
    *pw = 0x9300 + (newlen & 0xff);
    *pw = 0x9400 + ((newlen >> 8) & 0xff);

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

void VDP_doDMA(u8 location, u32 from, u16 to, u16 len)
{
    VDP_doDMAEx(location, from, to, len, 2);
}

void VDP_doVRamDMAFill(u16 to, u16 len, u8 value)
{
    vu16 *pw;
    vu32 *pl;

    VDP_setAutoInc(1);

    pw = (u16 *) GFX_CTRL_PORT;

    // Setup DMA length
    *pw = 0x9300 + (len & 0xff);
    *pw = 0x9400 + ((len >> 8) & 0xff);

    // Setup DMA operation (VRAM FILL)
    *pw = 0x9780;

    // Write VRam DMA destination address
    pl = (u32 *) GFX_CTRL_PORT;
   *pl = GFX_DMA_VRAM_ADDR(to);

    // set up value to fill (need to be 16 bits extended)
    pw = (u16 *) GFX_DATA_PORT;
    *pw = value | (value << 8);
}

void VDP_doVRamDMACopy(u16 from, u16 to, u16 len)
{
    vu16 *pw;
    vu32 *pl;

    VDP_setAutoInc(1);

    pw = (u16 *) GFX_CTRL_PORT;

    // Setup DMA length
    *pw = 0x9300 + (len & 0xff);
    *pw = 0x9400 + ((len >> 8) & 0xff);

    // Setup DMA address
    *pw = 0x9500 + (from & 0xff);
    *pw = 0x9600 + ((from >> 8) & 0xff);

    // Setup DMA operation (VRAM COPY)
    *pw = 0x97C0;

    // Write VRam DMA destination address (start DMA copy operation)
    pl = (u32 *) GFX_CTRL_PORT;
    *pl = GFX_DMA_VRAM_ADDR(to);
}
