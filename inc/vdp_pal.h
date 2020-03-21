/**
 *  \deprecated Use pal.h unit instead
 */

#ifndef _VDP_PAL_H_
#define _VDP_PAL_H_


/**
 *  \deprecated Use #PAL_getColor(..) instead
 */
u16  VDP_getPaletteColor(u16 index);
/**
 *  \deprecated Use #PAL_getColors(..) instead
 */
void  VDP_getPaletteColors(u16 index, u16* dest, u16 count);
/**
 *  \deprecated Use #PAL_getPalette(..) instead
 */
void VDP_getPalette(u16 num, u16 *pal);

/**
 *  \deprecated Use #PAL_setColor(..) instead
 */
void VDP_setPaletteColor(u16 index, u16 value);
/**
 *  \deprecated Use #PAL_setColors(..) instead
 */
void VDP_setPaletteColors(u16 index, const u16* values, u16 count);
/**
 *  \deprecated Use #PAL_setPalette(..) instead
 */
void VDP_setPalette(u16 num, const u16 *pal);


// these functions should be private as they are called by VDP_fadeXXX functions internally
// but they can be useful sometime for better control on the fading processus
bool VDP_doFadingStep();
bool VDP_initFading(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe);


/**
 *  \deprecated Use #PAL_fade(..) instead
 */
void VDP_fade(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async);
/**
 *  \deprecated Use #PAL_fadeTo(..) instead
 */
void VDP_fadeTo(u16 fromcol, u16 tocol, const u16 *pal, u16 numframe, u8 async);
/**
 *  \deprecated Use #PAL_fadeOut(..) instead
 */
void VDP_fadeOut(u16 fromcol, u16 tocol, u16 numframe, u8 async);
/**
 *  \deprecated Use #PAL_fadeIn(..) instead
 */
void VDP_fadeIn(u16 fromcol, u16 tocol, const u16 *pal, u16 numframe, u8 async);

/**
 *  \deprecated Use #PAL_fadePalette(..) instead
 */
void VDP_fadePal(u16 numpal, const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async);
/**
 *  \deprecated Use #PAL_fadeToPalette(..) instead
 */
void VDP_fadeToPal(u16 numpal, const u16 *pal, u16 numframe, u8 async);
/**
 *  \deprecated Use #PAL_fadeOutPalette(..) instead
 */
void VDP_fadeOutPal(u16 numpal, u16 numframe, u8 async);
/**
 *  \deprecated Use #PAL_fadeInPalette(..) instead
 */
void VDP_fadeInPal(u16 numpal, const u16 *pal, u16 numframe, u8 async);

/**
 *  \deprecated Use #PAL_fadeAll(..) instead
 */
void VDP_fadeAll(const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async);
/**
 *  \deprecated Use #PAL_fadeToAll(..) instead
 */
void VDP_fadeToAll(const u16 *pal, u16 numframe, u8 async);
/**
 *  \deprecated Use #PAL_fadeOutAll(..) instead
 */
void VDP_fadeOutAll(u16 numframe, u8 async);
/**
 *  \deprecated Use #PAL_fadeInAll(..) instead
 */
void VDP_fadeInAll(const u16 *pal, u16 numframe, u8 async);

/**
 *  \deprecated Use #PAL_isDoingFade(..) instead
 */
u16 VDP_isDoingFade();
/**
 *  \deprecated Use #PAL_waitFadeCompletion(..) instead
 */
void VDP_waitFadeCompletion();
/**
 *  \deprecated Use #PAL_interruptFade() instead
 */
void VDP_interruptFade();


#endif // _VDP_PAL_H_
