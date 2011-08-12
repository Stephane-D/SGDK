/**
 * \file vdp_pal.c
 * \brief VDP Palette support
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides methods to manipulate the VDP Color Palettes.
 * The Sega Genesis VDP has 4 palettes of 16 colors.
 * Color is defined with 3 bits for each component : 0RRR00GGG00BBB00
 */

#include "config.h"
#include "types.h"

#include "vdp.h"
#include "vdp_pal.h"

#include "base.h"


#define PALETTEFADE_FRACBITS    8


// we don't want to share them
extern u32 VBlankProcess;
extern u32 HBlankProcess;


const u16 palette_black[16] =
{
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,

    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
};

const u16 palette_grey[16] =
{
    0x0000,
    0x0222,
    0x0444,
    0x0666,
    0x0888,
    0x0AAA,
    0x0CCC,
    0x0EEE,

    0x0EEE,
    0x0EEE,
    0x0EEE,
    0x0EEE,
    0x0EEE,
    0x0EEE,
    0x0EEE,
    0x0EEE
};

const u16 palette_red[16] =
{
    0x0000,
    0x0002,
    0x0004,
    0x0006,
    0x0008,
    0x000A,
    0x000C,
    0x000E,

    0x000E,
    0x000E,
    0x000E,
    0x000E,
    0x000E,
    0x000E,
    0x000E,
    0x000E
};

const u16 palette_green[16] =
{
    0x0000,
    0x0020,
    0x0040,
    0x0060,
    0x0080,
    0x00A0,
    0x00C0,
    0x00E0,

    0x00E0,
    0x00E0,
    0x00E0,
    0x00E0,
    0x00E0,
    0x00E0,
    0x00E0,
    0x00E0
};

const u16 palette_blue[16] =
{
    0x0000,
    0x0200,
    0x0400,
    0x0600,
    0x0800,
    0x0A00,
    0x0C00,
    0x0E00,

    0x0E00,
    0x0E00,
    0x0E00,
    0x0E00,
    0x0E00,
    0x0E00,
    0x0E00,
    0x0E00
};


// used for palette fading (need 774 bytes of memory)
static s16 fading_palR[64];
static s16 fading_palG[64];
static s16 fading_palB[64];
static s16 fading_stepR[64];
static s16 fading_stepG[64];
static s16 fading_stepB[64];
static u16 fading_from;
static u16 fading_to;
static u16 fading_cnt;


u16 VDP_getPaletteColor(u16 numpal, u16 numcol)
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = (numpal * 32) + (numcol * 2);
    *pl = GFX_READ_CRAM_ADDR(addr);

    return *pw;
}

void VDP_getPalette(u16 num, u16 *pal)
{
    vu16 *pw;
    vu32 *pl;
    u16 *dest;
    u16 i;
    u16 addr;

    VDP_setAutoInc(2);

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    dest = pal;
    addr = num * 32;
    *pl = GFX_READ_CRAM_ADDR(addr);

    i = 16;
    while(i--) *dest++ = *pw;
}


void VDP_setPaletteColor(u16 numpal, u16 numcol, u16 value)
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = (numpal * 32) + (numcol * 2);
    *pl = GFX_WRITE_CRAM_ADDR(addr);

    *pw = value;
}

void VDP_setPalette(u16 num, const u16 *pal)
{
    vu16 *pw;
    vu32 *pl;
    const u16 *src;
    u16 i;
    u16 addr;

    VDP_setAutoInc(2);

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    src = pal;
    addr = num * 32;
    *pl = GFX_WRITE_CRAM_ADDR(addr);

    i = 16;
    while(i--) *pw = *src++;
}


u16 VDP_doStepFading()
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;
    u16 i;

    // end fading ?
    if (fading_cnt == 0) return 0;

    VDP_setAutoInc(2);

    /* point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = fading_from * 2;
    *pl = GFX_WRITE_CRAM_ADDR(addr);

    for(i = fading_from; i <= fading_to; i++)
    {
        u16 col;

        fading_palR[i] += fading_stepR[i];
        fading_palG[i] += fading_stepG[i];
        fading_palB[i] += fading_stepB[i];

        col = ((fading_palR[i] >> PALETTEFADE_FRACBITS) << VDPPALETTE_REDSFT) & VDPPALETTE_REDMASK;
        col |= ((fading_palG[i] >> PALETTEFADE_FRACBITS) << VDPPALETTE_GREENSFT) & VDPPALETTE_GREENMASK;
        col |= ((fading_palB[i] >> PALETTEFADE_FRACBITS) << VDPPALETTE_BLUESFT) & VDPPALETTE_BLUEMASK;
        *pw = col;
    }

    fading_cnt--;

    return 1;
}

u16 VDP_initFading(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe)
{
    u16 i;
    const u16 *src;
    const u16 *dst;

    // can't do a fade on 0 frame !
    if (numframe == 0) return 0;

    fading_from = fromcol;
    fading_to = tocol;
    fading_cnt = numframe;

    src = palsrc;
    dst = paldst;
    for(i = fading_from; i <= fading_to; i++)
    {
        fading_palR[i] = ((*src & VDPPALETTE_REDMASK) >> VDPPALETTE_REDSFT) << PALETTEFADE_FRACBITS;
        fading_palG[i] = ((*src & VDPPALETTE_GREENMASK) >> VDPPALETTE_GREENSFT) << PALETTEFADE_FRACBITS;
        fading_palB[i] = ((*src & VDPPALETTE_BLUEMASK) >> VDPPALETTE_BLUESFT) << PALETTEFADE_FRACBITS;
        src++;

        fading_stepR[i] = ((((*dst & VDPPALETTE_REDMASK) >> VDPPALETTE_REDSFT) << PALETTEFADE_FRACBITS) - fading_palR[i]) / numframe;
        fading_stepG[i] = ((((*dst & VDPPALETTE_GREENMASK) >> VDPPALETTE_GREENSFT) << PALETTEFADE_FRACBITS) - fading_palG[i]) / numframe;
        fading_stepB[i] = ((((*dst & VDPPALETTE_BLUEMASK) >> VDPPALETTE_BLUESFT) << PALETTEFADE_FRACBITS) - fading_palB[i]) / numframe;
        dst++;
    }

    return 1;
}


void VDP_fade(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async)
{
    // error during fading initialisation, exit !
    if (!VDP_initFading(fromcol, tocol, palsrc, paldst, numframe)) return;

    // process asynchrone fading
    if (async) VBlankProcess |= PROCESS_PALETTE_FADING;
    // process fading immediatly
    else while (VDP_doStepFading()) VDP_waitVSync();
}


void VDP_fadeTo(u16 fromcol, u16 tocol, const u16 *pal, u16 numframe, u8 async)
{
    u16 tmp_pal[64];
    u16 i;

    for (i = 0; i < 4; i++) VDP_getPalette(i, &tmp_pal[i << 4]);

    // do the fade
    VDP_fade(fromcol, tocol, tmp_pal, pal, numframe, async);
}

void VDP_fadeOut(u16 fromcol, u16 tocol, u16 numframe, u8 async)
{
    VDP_fadeTo(fromcol, tocol, palette_black, numframe, async);
}

void VDP_fadeIn(u16 fromcol, u16 tocol, const u16 *pal, u16 numframe, u8 async)
{
    VDP_fade(fromcol, tocol, palette_black, pal, numframe, async);
}


void VDP_fadePal(u16 numpal, const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async)
{
    VDP_fade(numpal << 4, (numpal << 4) + 15, palsrc, paldst, numframe, async);
}

void VDP_fadePalTo(u16 numpal, const u16 *pal, u16 numframe, u8 async)
{
    VDP_fadeTo(numpal << 4, (numpal << 4) + 15, pal, numframe, async);
}

void VDP_fadePalOut(u16 numpal, u16 numframe, u8 async)
{
    VDP_fadeTo(numpal << 4, (numpal << 4) + 15, palette_black, numframe, async);
}

void VDP_fadePalIn(u16 numpal, const u16 *pal, u16 numframe, u8 async)
{
    VDP_fade(numpal << 4, (numpal << 4) + 15, palette_black, pal, numframe, async);
}


void VDP_fadeAll(const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async)
{
    VDP_fade(0, 63, palsrc, paldst, numframe, async);
}

void VDP_fadeAllTo(const u16 *pal, u16 numframe, u8 async)
{
    VDP_fadeTo(0, 63, pal, numframe, async);
}

void VDP_fadeOutAll(u16 numframe, u8 async)
{
    VDP_fadeTo(0, 63, palette_black, numframe, async);
}

void VDP_fadeInAll(const u16 *pal, u16 numframe, u8 async)
{
    VDP_fade(0, 63, palette_black, pal, numframe, async);
}


void VDP_waitFadeCompletion()
{
    vu32 *processing;

    // temporary reference VBlankProcess as volatile
    // to avoid dead lock compiler optimisation
    processing = &VBlankProcess;

    while (*processing & PROCESS_PALETTE_FADING);
}
