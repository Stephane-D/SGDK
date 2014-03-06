#include "config.h"
#include "types.h"

#include "vdp.h"
#include "vdp_pal.h"

#include "sys.h"


#define PALETTEFADE_FRACBITS    8


// we don't want to share them
extern u32 VIntProcess;
extern u32 HIntProcess;


const u16 palette_black_all[64] =
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

const u16* const palette_black = palette_black_all;

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
static s16 fading_cnt;


// forward
static void setFadePalette(u16 waitVSync);


u16 VDP_getPaletteColor(u16 index)
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = index * 2;
    *pl = GFX_READ_CRAM_ADDR(addr);

    return *pw;
}

void VDP_setPaletteColor(u16 index, u16 value)
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = index * 2;
    *pl = GFX_WRITE_CRAM_ADDR(addr);

    *pw = value;
}


static void setFadePalette(u16 waitVSync)
{
    s16 *palR;
    s16 *palG;
    s16 *palB;
    vu16 *pw;
    vu32 *pl;
    u16 addr;
    u16 i;

    // lazy optimization
    if (VDP_getAutoInc() != 2)
        VDP_setAutoInc(2);

    /* point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = fading_from * 2;
    *pl = GFX_WRITE_CRAM_ADDR(addr);

    i = fading_from;

    palR = fading_palR + i;
    palG = fading_palG + i;
    palB = fading_palB + i;

    // wait for VSync
    if (waitVSync) VDP_waitVSync();

    i = (fading_to - fading_from) + 1;
    while(i--)
    {
        u16 col;

        col = ((*palR++ >> PALETTEFADE_FRACBITS) << VDPPALETTE_REDSFT) & VDPPALETTE_REDMASK;
        col |= ((*palG++ >> PALETTEFADE_FRACBITS) << VDPPALETTE_GREENSFT) & VDPPALETTE_GREENMASK;
        col |= ((*palB++ >> PALETTEFADE_FRACBITS) << VDPPALETTE_BLUESFT) & VDPPALETTE_BLUEMASK;

        *pw = col;
    }
}

u16 VDP_doStepFading(u16 waitVSync)
{
    s16 *palR;
    s16 *palG;
    s16 *palB;
    s16 *stepR;
    s16 *stepG;
    s16 *stepB;
    u16 i;

    i = fading_from;

    palR = fading_palR + i;
    palG = fading_palG + i;
    palB = fading_palB + i;
    stepR = fading_stepR + i;
    stepG = fading_stepG + i;
    stepB = fading_stepB + i;

    i = (fading_to - fading_from) + 1;
    while(i--)
    {
        *palR++ += *stepR++;
        *palG++ += *stepG++;
        *palB++ += *stepB++;
    }

    // set current fade palette
    setFadePalette(waitVSync);

    // one step less
    if (--fading_cnt <= 0) return 0;

    return 1;
}

u16 VDP_initFading(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe, u16 waitVSync)
{
    const u16 *src;
    const u16 *dst;
    s16 *palR;
    s16 *palG;
    s16 *palB;
    s16 *stepR;
    s16 *stepG;
    s16 *stepB;
    u16 i;

    // can't do a fade on 0 frame !
    if (numframe == 0) return 0;

    fading_from = fromcol;
    fading_to = tocol;
    fading_cnt = numframe;

    src = palsrc;
    dst = paldst;
    palR = fading_palR + fromcol;
    palG = fading_palG + fromcol;
    palB = fading_palB + fromcol;
    stepR = fading_stepR + fromcol;
    stepG = fading_stepG + fromcol;
    stepB = fading_stepB + fromcol;

    i = (tocol - fromcol) + 1;
    while(i--)
    {
        const u16 s = *src++;
        const u16 d = *dst++;

        const s16 R = ((s & VDPPALETTE_REDMASK) >> VDPPALETTE_REDSFT) << PALETTEFADE_FRACBITS;
        const s16 G = ((s & VDPPALETTE_GREENMASK) >> VDPPALETTE_GREENSFT) << PALETTEFADE_FRACBITS;
        const s16 B = ((s & VDPPALETTE_BLUEMASK) >> VDPPALETTE_BLUESFT) << PALETTEFADE_FRACBITS;

        *stepR++ = ((((d & VDPPALETTE_REDMASK) >> VDPPALETTE_REDSFT) << PALETTEFADE_FRACBITS) - R) / numframe;
        *stepG++ = ((((d & VDPPALETTE_GREENMASK) >> VDPPALETTE_GREENSFT) << PALETTEFADE_FRACBITS) - G) / numframe;
        *stepB++ = ((((d & VDPPALETTE_BLUEMASK) >> VDPPALETTE_BLUESFT) << PALETTEFADE_FRACBITS) - B) / numframe;

        *palR++ = R;
        *palG++ = G;
        *palB++ = B;
    }

    // set current fade palette
    setFadePalette(waitVSync);

    return 1;
}


void VDP_fade(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async)
{
    // error during fading initialisation, exit !
    if (!VDP_initFading(fromcol, tocol, palsrc, paldst, numframe, TRUE)) return;

    // process asynchrone fading
    if (async) VIntProcess |= PROCESS_PALETTE_FADING;
    // process fading immediatly
    else while (VDP_doStepFading(TRUE));
}


void VDP_fadeTo(u16 fromcol, u16 tocol, const u16 *pal, u16 numframe, u8 async)
{
    u16 tmp_pal[64];

    // read current palette
    VDP_getPaletteColors(fromcol, tmp_pal, (tocol - fromcol) + 1);
    // do the fade
    VDP_fade(fromcol, tocol, tmp_pal, pal, numframe, async);
}

void VDP_fadeOut(u16 fromcol, u16 tocol, u16 numframe, u8 async)
{
    VDP_fadeTo(fromcol, tocol, palette_black_all, numframe, async);
}

void VDP_fadeIn(u16 fromcol, u16 tocol, const u16 *pal, u16 numframe, u8 async)
{
    VDP_fade(fromcol, tocol, palette_black_all, pal, numframe, async);
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
    VDP_fadeTo(0, 63, palette_black_all, numframe, async);
}

void VDP_fadeInAll(const u16 *pal, u16 numframe, u8 async)
{
    VDP_fade(0, 63, palette_black_all, pal, numframe, async);
}


u16 VDP_isDoingFade()
{
    return (VIntProcess & PROCESS_PALETTE_FADING)?TRUE:FALSE;
}

void VDP_waitFadeCompletion()
{
    vu32 *processing;

    // temporary reference VIntProcess as volatile
    // to avoid dead lock compiler optimisation
    processing = &VIntProcess;

    while (*processing & PROCESS_PALETTE_FADING);
}
