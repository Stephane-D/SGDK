/**
 *  \deprecated Use pal.h unit instead
 */

#ifndef _VDP_PAL_H_
#define _VDP_PAL_H_


/**
 *  \deprecated Use #PAL_getColor(..) instead
 */
#define VDP_getPaletteColor(index)     _Pragma("This definition is deprecated, use PAL_getColor(..) instead.")

/**
 *  \deprecated Use #PAL_getColors(..) instead
 */
#define VDP_getPaletteColors(index, dest, count)     _Pragma("This definition is deprecated, use PAL_getColors(..) instead.")
/**
 *  \deprecated Use #PAL_getPalette(..) instead
 */
#define VDP_getPalette(num, pal)     _Pragma("This definition is deprecated, use PAL_getPalette(..) instead.")

/**
 *  \deprecated Use #PAL_setColor(..) instead
 */
#define VDP_setPaletteColor(index, value)     _Pragma("This definition is deprecated, use PAL_setColor(..) instead.")
/**
 *  \deprecated Use #PAL_setColors(..) instead
 */
#define VDP_setPaletteColors(index, values, count)     _Pragma("This definition is deprecated, use PAL_setColors(..) instead.")
/**
 *  \deprecated Use #PAL_setPalette(..) instead
 */
#define VDP_setPalette(num, pal)     _Pragma("This definition is deprecated, use PAL_setPalette(..) instead.")

/**
 *  \deprecated Use #PAL_fade(..) instead
 */
#define VDP_fade(fromcol, tocol, palsrc, paldst, numframe, async)     _Pragma("This definition is deprecated, use PAL_fade(..) instead.")
/**
 *  \deprecated Use #PAL_fadeTo(..) instead
 */
#define VDP_fadeTo(fromcol, tocol, pal, numframe, async)     _Pragma("This definition is deprecated, use PAL_fadeTo(..) instead.")
/**
 *  \deprecated Use #PAL_fadeOut(..) instead
 */
#define VDP_fadeOut(fromcol, tocol, numframe, async)     _Pragma("This definition is deprecated, use PAL_fadeOut(..) instead.")
/**
 *  \deprecated Use #PAL_fadeIn(..) instead
 */
#define VDP_fadeIn(fromcol, tocol, pal, numframe, async)     _Pragma("This definition is deprecated, use PAL_fadeIn(..) instead.")

/**
 *  \deprecated Use #PAL_fadePalette(..) instead
 */
#define VDP_fadePal(numpal, palsrc, paldst, numframe, async)     _Pragma("This definition is deprecated, use PAL_fadePalette(..) instead.")
/**
 *  \deprecated Use #PAL_fadeToPalette(..) instead
 */
#define VDP_fadeToPal(numpal, pal, numframe, async)     _Pragma("This definition is deprecated, use PAL_fadeToPalette(..) instead.")
/**
 *  \deprecated Use #PAL_fadeOutPalette(..) instead
 */
#define VDP_fadeOutPal(numpal, numframe, async)     _Pragma("This definition is deprecated, use PAL_fadeOutPalette(..) instead.")
/**
 *  \deprecated Use #PAL_fadeInPalette(..) instead
 */
#define VDP_fadeInPal(numpal, pal, numframe, async)     _Pragma("This definition is deprecated, use PAL_fadeInPalette(..) instead.")

/**
 *  \deprecated Use #PAL_fadeAll(..) instead
 */
#define VDP_fadeAll(palsrc, paldst, numframe, async)     _Pragma("This definition is deprecated, use PAL_fadeAll(..) instead.")
/**
 *  \deprecated Use #PAL_fadeToAll(..) instead
 */
#define VDP_fadeToAll(pal, numframe, async)     _Pragma("This definition is deprecated, use PAL_fadeToAll(..) instead.")
/**
 *  \deprecated Use #PAL_fadeOutAll(..) instead
 */
#define VDP_fadeOutAll(numframe, async)     _Pragma("This definition is deprecated, use PAL_fadeOutAll(..) instead.")
/**
 *  \deprecated Use #PAL_fadeInAll(..) instead
 */
#define VDP_fadeInAll(pal, numframe, async)     _Pragma("This definition is deprecated, use PAL_fadeInAll(..) instead.")

/**
 *  \deprecated Use #PAL_isDoingFade(..) instead
 */
#define VDP_isDoingFade()     _Pragma("This definition is deprecated, use PAL_isDoingFade(..) instead.")
/**
 *  \deprecated Use #PAL_waitFadeCompletion(..) instead
 */
#define VDP_waitFadeCompletion()     _Pragma("This definition is deprecated, use PAL_waitFadeCompletion(..) instead.")
/**
 *  \deprecated Use #PAL_interruptFade() instead
 */
#define VDP_interruptFade()     _Pragma("This definition is deprecated, use PAL_interruptFade(..) instead.")


#endif // _VDP_PAL_H_
