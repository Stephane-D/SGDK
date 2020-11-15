#include "config.h"
#include "types.h"

#include "vdp.h"
#include "pal.h"

#include "sys.h"
#include "timer.h"
#include "memory.h"
#include "dma.h"

#if (ENABLE_NEWLIB == 1)
#include <string.h>	// For memcpy
#endif

#define PALETTEFADE_FRACBITS    8
#define PALETTEFADE_ROUND_VAL   ((1 << (PALETTEFADE_FRACBITS - 1)) - 1)


// we don't want to share them
extern vu16 VBlankProcess;


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


const u16* const palette_black = palette_black_all;
const u16* const palette_white = palette_grey;

// used for palette fading (consumes 1024 bytes of memory)
static u16 fadeCurrentPal[64];
static u16 fadeEndPal[64];

static s16 fadeR[64];
static s16 fadeG[64];
static s16 fadeB[64];
static s16 fadeSR[64];
static s16 fadeSG[64];
static s16 fadeSB[64];

static u16 fadeInd;
static u16 fadeSize;
static s16 fadeCounter;


// forward
static void setFadePalette(u16 ind, const u16 *src, u16 len);


u16 PAL_getColor(u16 index)
{
    const u16 addr = index * 2;

    *((vu32*) GFX_CTRL_PORT) = GFX_READ_CRAM_ADDR((u32)addr);

    return *((vu16*) GFX_DATA_PORT);
}

void PAL_getColors(u16 index, u16* dest, u16 count)
{
    VDP_setAutoInc(2);

    const u16 addr = index * 2;
    *((vu32*) GFX_CTRL_PORT) = GFX_READ_CRAM_ADDR((u32)addr);

    vu32* pl = (u32*) GFX_DATA_PORT;
    u32* dl = (u32*) dest;

    u16 il = count >> 4;
    while(il--)
    {
        *dl++ = *pl;
        *dl++ = *pl;
        *dl++ = *pl;
        *dl++ = *pl;
        *dl++ = *pl;
        *dl++ = *pl;
        *dl++ = *pl;
        *dl++ = *pl;
    }

    vu16* pw = (u16*) pl;
    u16* dw = (u16*) dl;

    u16 i = count & 0xF;
    while(i--) *dw++ = *pw;
}

void PAL_getPalette(u16 numPal, u16* dest)
{
    VDP_setAutoInc(2);

    const u16 addr = numPal * (16 * 2);
    *((vu32*) GFX_CTRL_PORT) = GFX_READ_CRAM_ADDR((u32)addr);

    vu32* pl = (u32*) GFX_DATA_PORT;
    u32* d = (u32*) dest;

    *d++ = *pl;
    *d++ = *pl;
    *d++ = *pl;
    *d++ = *pl;
    *d++ = *pl;
    *d++ = *pl;
    *d++ = *pl;
    *d++ = *pl;
}

void PAL_setColor(u16 index, u16 value)
{
    const u16 addr = index * 2;

    *((vu32*) GFX_CTRL_PORT) = GFX_WRITE_CRAM_ADDR((u32)addr);
    *((vu16*) GFX_DATA_PORT) = value;
}

void PAL_setColors(u16 index, const u16* pal, u16 count)
{
    VDP_setAutoInc(2);

    const u16 addr = index * 2;
    *((vu32*) GFX_CTRL_PORT) = GFX_WRITE_CRAM_ADDR((u32)addr);

    vu32* pl = (u32*) GFX_DATA_PORT;
    u32* sl = (u32*) pal;

    u16 il = count >> 4;
    while(il--)
    {
        *pl = *sl++;
        *pl = *sl++;
        *pl = *sl++;
        *pl = *sl++;
        *pl = *sl++;
        *pl = *sl++;
        *pl = *sl++;
        *pl = *sl++;
    }

    vu16* pw = (u16*) pl;
    u16* sw = (u16*) sl;

    u16 i = count & 0xF;
    while(i--) *pw = *sw++;
}

void PAL_setPaletteColors(u16 index, const Palette* pal)
{
    PAL_setColors(index, pal->data, pal->length);
}

void PAL_setPalette(u16 numPal, const u16* pal)
{
    VDP_setAutoInc(2);

    const u16 addr = numPal * (16 * 2);
    *((vu32*) GFX_CTRL_PORT) = GFX_WRITE_CRAM_ADDR((u32)addr);

    vu32* pl = (u32*) GFX_DATA_PORT;
    u32* s = (u32*) pal;

    *pl = *s++;
    *pl = *s++;
    *pl = *s++;
    *pl = *s++;
    *pl = *s++;
    *pl = *s++;
    *pl = *s++;
    *pl = *s++;
}

void PAL_setColorsDMA(u16 index, const u16* pal, u16 count)
{
    DMA_queueDma(DMA_CRAM, (void*) pal, index * 2, count, 2);
}

void PAL_setPaletteColorsDMA(u16 index, const Palette* pal)
{
    PAL_setColorsDMA(index, pal->data, pal->length);
}

void PAL_setPaletteDMA(u16 numPal, const u16* pal)
{
    PAL_setColorsDMA(numPal * 16, pal, 16);
}


static void setFadePalette(u16 ind, const u16 *src, u16 len)
{
    static u32 lastVTimer = 0;

    // be sure to wait at least 1 frame between set fade palette call
    if (lastVTimer == vtimer) VDP_waitVSync();

    // use DMA for long transfer
    if (len > 16) DMA_doDma(DMA_CRAM, (void*) src, ind * 2, len, 2);
    else PAL_setColors(ind, src, len);

    // keep track of last update
    lastVTimer = vtimer;
}

//static void setFadePalette(u16 ind, const u16 *src, u16 len)
//{
//    static u32 lastVTimer = 0;
//
//    // we are inside VInt callback --> just set palette colors immediately
//    if (SYS_isInVIntCallback())
//    {
//        // use DMA for long transfer
//        if (len > 16) DMA_doDma(DMA_CRAM, (u32) src, ind * 2, len, 2);
//        else PAL_setColors(ind, src, len);
//    }
//    else
//    {
//        // be sure to wait at least 1 frame between set fade palette call
//        if (lastVTimer == vtimer) VDP_waitVSync();
//
//        // disable interrupts to not conflict with VInt accesses
//        SYS_disableInts();
//        // use DMA for long transfer
//        if (len > 16) DMA_doDma(DMA_CRAM, (u32) src, ind * 2, len, 2);
//        else PAL_setColors(ind, src, len);
//        SYS_enableInts();
//
//        // keep track of last update
//        lastVTimer = vtimer;
//    }
//}


bool PAL_initFade(u16 fromCol, u16 toCol, const u16* palSrc, const u16* palDst, u16 numFrame)
{
    // can't do a fade on 0 frame !
    if (numFrame == 0) return FALSE;

    fadeInd = fromCol;
    fadeSize = (toCol - fromCol) + 1;
    fadeCounter = numFrame;

    const u16* src = palSrc;
    const u16* dst = palDst;
    u16 len = fadeSize;

    // set source palette as current fade palette
    memcpy(fadeCurrentPal, src, len * 2);
    // save end palette
    memcpy(fadeEndPal, dst, len * 2);

    s16 *palR = fadeR;
    s16 *palG = fadeG;
    s16 *palB = fadeB;
    s16 *stepR = fadeSR;
    s16 *stepG = fadeSG;
    s16 *stepB = fadeSB;

    while(len--)
    {
        const u16 s = *src++;
        const s16 RS = ((s & VDPPALETTE_REDMASK) >> VDPPALETTE_REDSFT) << PALETTEFADE_FRACBITS;
        const s16 GS = ((s & VDPPALETTE_GREENMASK) >> VDPPALETTE_GREENSFT) << PALETTEFADE_FRACBITS;
        const s16 BS = ((s & VDPPALETTE_BLUEMASK) >> VDPPALETTE_BLUESFT) << PALETTEFADE_FRACBITS;

        *palR++ = RS + PALETTEFADE_ROUND_VAL;
        *palG++ = GS + PALETTEFADE_ROUND_VAL;
        *palB++ = BS + PALETTEFADE_ROUND_VAL;

        const u16 d = *dst++;
        const s16 RD = ((d & VDPPALETTE_REDMASK) >> VDPPALETTE_REDSFT) << PALETTEFADE_FRACBITS;
        const s16 GD = ((d & VDPPALETTE_GREENMASK) >> VDPPALETTE_GREENSFT) << PALETTEFADE_FRACBITS;
        const s16 BD = ((d & VDPPALETTE_BLUEMASK) >> VDPPALETTE_BLUESFT) << PALETTEFADE_FRACBITS;

        *stepR++ = (RD - RS) / numFrame;
        *stepG++ = (GD - GS) / numFrame;
        *stepB++ = (BD - BS) / numFrame;
    }

    // do first fade step
    PAL_doFadeStep();

    return TRUE;
}

bool PAL_doFadeStep()
{
    // not yet done ?
    if (--fadeCounter >= 0)
    {
        // set current fade palette
        setFadePalette(fadeInd, fadeCurrentPal, fadeSize);

        // then prepare fade palette for next frame
        s16* palR = fadeR;
        s16* palG = fadeG;
        s16* palB = fadeB;
        s16* stepR = fadeSR;
        s16* stepG = fadeSG;
        s16* stepB = fadeSB;
        u16* dst = fadeCurrentPal;

        // compute the next fade palette
        u16 i = fadeSize;
        while(i--)
        {
            u16 col;

            const u16 R = *palR + *stepR++;
            const u16 G = *palG + *stepG++;
            const u16 B = *palB + *stepB++;

            *palR++ = R;
            *palG++ = G;
            *palB++ = B;

            col = ((R >> PALETTEFADE_FRACBITS) << VDPPALETTE_REDSFT) & VDPPALETTE_REDMASK;
            col |= ((G >> PALETTEFADE_FRACBITS) << VDPPALETTE_GREENSFT) & VDPPALETTE_GREENMASK;
            col |= ((B >> PALETTEFADE_FRACBITS) << VDPPALETTE_BLUESFT) & VDPPALETTE_BLUEMASK;

            *dst++ = col;
        }

        // not yet done
        return TRUE;
    }
    else
    {
        // last step --> we can just set the final fade palette
        setFadePalette(fadeInd, fadeEndPal, fadeSize);

        // done
        return FALSE;
    }
}


void PAL_interruptFade()
{
    VBlankProcess &= ~PROCESS_PALETTE_FADING;
}

void PAL_fade(u16 fromCol, u16 toCol, const u16* palSrc, const u16* palDst, u16 numFrame, bool async)
{
    // error during fading initialization --> exit
    if (!PAL_initFade(fromCol, toCol, palSrc, palDst, numFrame)) return;

    // process asynchrone fading
    if (async) VBlankProcess |= PROCESS_PALETTE_FADING;
    else
    {
        // process fading immediatly
        while (PAL_doFadeStep())
            SYS_doVBlankProcess();
    }
}


void PAL_fadeTo(u16 fromCol, u16 toCol, const u16* pal, u16 numFrame, bool async)
{
    u16 tmp_pal[64];

    // read current palette
    PAL_getColors(fromCol, tmp_pal, (toCol - fromCol) + 1);
    // do the fade
    PAL_fade(fromCol, toCol, tmp_pal, pal, numFrame, async);
}

void PAL_fadeOut(u16 fromCol, u16 toCol, u16 numFrame, bool async)
{
    PAL_fadeTo(fromCol, toCol, palette_black_all, numFrame, async);
}

void PAL_fadeIn(u16 fromCol, u16 toCol, const u16* pal, u16 numFrame, bool async)
{
    PAL_fade(fromCol, toCol, palette_black_all, pal, numFrame, async);
}


void PAL_fadePalette(u16 numpal, const u16* palSrc, const u16* palDst, u16 numFrame, bool async)
{
    PAL_fade(numpal << 4, (numpal << 4) + 15, palSrc, palDst, numFrame, async);
}

void PAL_fadeToPalette(u16 numpal, const u16* pal, u16 numFrame, bool async)
{
    PAL_fadeTo(numpal << 4, (numpal << 4) + 15, pal, numFrame, async);
}

void PAL_fadeOutPalette(u16 numpal, u16 numFrame, bool async)
{
    PAL_fadeTo(numpal << 4, (numpal << 4) + 15, palette_black, numFrame, async);
}

void PAL_fadeInPalette(u16 numpal, const u16* pal, u16 numFrame, bool async)
{
    PAL_fade(numpal << 4, (numpal << 4) + 15, palette_black, pal, numFrame, async);
}


void PAL_fadeAll(const u16* palSrc, const u16* palDst, u16 numFrame, bool async)
{
    PAL_fade(0, 63, palSrc, palDst, numFrame, async);
}

void PAL_fadeToAll(const u16* pal, u16 numFrame, bool async)
{
    PAL_fadeTo(0, 63, pal, numFrame, async);
}

void PAL_fadeOutAll(u16 numFrame, bool async)
{
    PAL_fadeTo(0, 63, palette_black_all, numFrame, async);
}

void PAL_fadeInAll(const u16* pal, u16 numFrame, bool async)
{
    PAL_fade(0, 63, palette_black_all, pal, numFrame, async);
}


u16 PAL_isDoingFade()
{
    return (VBlankProcess & PROCESS_PALETTE_FADING)?TRUE:FALSE;
}

void PAL_waitFadeCompletion()
{
    if (PAL_isDoingFade())
    {
        // need to do VBlank process otherwise we can wait a long time for completion ^^
        while (VBlankProcess & PROCESS_PALETTE_FADING)
            SYS_doVBlankProcess();
    }
}
