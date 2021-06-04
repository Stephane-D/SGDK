#include "config.h"
#include "types.h"

#include "vdp.h"
#include "pal.h"


u16 VDP_getPaletteColor(u16 index)
{
    return PAL_getColor(index);
}

void  VDP_getPaletteColors(u16 index, u16* dest, u16 count)
{
    return PAL_getColors(index, dest, count);
}

void VDP_getPalette(u16 numPal, u16 *pal)
{
    return PAL_getPalette(numPal, pal);
}

void VDP_setPaletteColor(u16 index, u16 value)
{
    PAL_setColor(index, value);
}

void VDP_setPaletteColors(u16 index, const u16* values, u16 count)
{
    PAL_setColors(index, values, count, CPU);
}

void VDP_setPalette(u16 numPal, const u16 *pal)
{
    PAL_setPalette(numPal, pal, CPU);
}


bool VDP_doFadingStep()
{
    return PAL_doFadeStep();
}

bool VDP_initFading(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe)
{
    return PAL_initFade(fromcol, tocol, palsrc, paldst, numframe);
}

void VDP_interruptFade()
{
    PAL_interruptFade();
}

void VDP_fade(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async)
{
    PAL_fade(fromcol, tocol, palsrc, paldst, numframe, async);
}


void VDP_fadeTo(u16 fromcol, u16 tocol, const u16 *pal, u16 numframe, u8 async)
{
    PAL_fadeTo(fromcol, tocol, pal, numframe, async);
}

void VDP_fadeOut(u16 fromcol, u16 tocol, u16 numframe, u8 async)
{
    PAL_fadeOut(fromcol, tocol, numframe, async);
}

void VDP_fadeIn(u16 fromcol, u16 tocol, const u16 *pal, u16 numframe, u8 async)
{
    PAL_fadeIn(fromcol, tocol, pal, numframe, async);
}


void VDP_fadePal(u16 numpal, const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async)
{
    PAL_fadePalette(numpal, palsrc, paldst, numframe, async);
}

void VDP_fadeToPal(u16 numpal, const u16 *pal, u16 numframe, u8 async)
{
    PAL_fadeToPalette(numpal, pal, numframe, async);
}

void VDP_fadeOutPal(u16 numpal, u16 numframe, u8 async)
{
    PAL_fadeOutPalette(numpal, numframe, async);
}

void VDP_fadeInPal(u16 numpal, const u16 *pal, u16 numframe, u8 async)
{
    PAL_fadeInPalette(numpal, pal, numframe, async);
}


void VDP_fadeAll(const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async)
{
    PAL_fadeAll(palsrc, paldst, numframe, async);
}

void VDP_fadeToAll(const u16 *pal, u16 numframe, u8 async)
{
    PAL_fadeToAll(pal, numframe, async);
}

void VDP_fadeOutAll(u16 numframe, u8 async)
{
    PAL_fadeOutAll(numframe, async);
}

void VDP_fadeInAll(const u16 *pal, u16 numframe, u8 async)
{
    PAL_fadeInAll(pal, numframe, async);
}


u16 VDP_isDoingFade()
{
    return PAL_isDoingFade();
}

void VDP_waitFadeCompletion()
{
    PAL_waitFadeCompletion();
}