/**
 * \file vdp_pal.h
 * \brief VDP Palette support
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides methods to manipulate the VDP Color Palette.<br/>
 * The Sega Genesis VDP has 4 palettes of 16 colors.<br/>
 * Color is defined with 3 bits for each component : 0RRR00GGG00BBB00
 */

#ifndef _VDP_PAL_H_
#define _VDP_PAL_H_


#define VDPPALETTE_REDSFT           9
#define VDPPALETTE_GREENSFT         5
#define VDPPALETTE_BLUESFT          1

#define VDPPALETTE_REDMASK          0x0E00
#define VDPPALETTE_GREENMASK        0x00E0
#define VDPPALETTE_BLUEMASK         0x000E
#define VDPPALETTE_COLORMASK        0x0EEE

/**
 * \def RGB24_TO_VDPCOLOR
 *      Convert a RGB 24 bits color to VDP color
 *
 * \param color
 *      RGB 24 bits color
 */
#define RGB24_TO_VDPCOLOR(color)    (((color >> ((2 * 4) + 4)) & VDPPALETTE_REDMASK) | ((color >> ((1 * 4) + 4)) & VDPPALETTE_GREENMASK) | ((color >> ((0 * 4) + 4)) & VDPPALETTE_BLUEMASK))


/**
 * \brief
 *      Default black palette.
 */
extern const u16 palette_black[16];
/**
 * \brief
 *      Default grey palette.
 */
extern const u16 palette_grey[16];
/**
 * \brief
 *      Default red palette.
 */
extern const u16 palette_red[16];
/**
 * \brief
 *      Default green palette.
 */
extern const u16 palette_green[16];
/**
 * \brief
 *      Default blue palette.
 */
extern const u16 palette_blue[16];


/**
 *  \brief
 *      Returns RGB color intensity.
 *
 *  \param numpal
 *      Palette number (0-3).
 *  \param numcol
 *      Color index (0-15).
 *  \return RGB intensity for the specified palette and color index.
 */
u16  VDP_getPaletteColor(u16 numpal, u16 numcol);
/**
 *  \brief
 *      Get palette.
 *
 *  \param num
 *      Palette number (0-3).
 *  \param pal
 *      Destination where to copy palette (should be 16 words long at least)
 */
void VDP_getPalette(u16 num, u16 *pal);

/**
 *  \brief
 *      Set RGB color intensity.
 *
 *  \param numpal
 *      Palette number (0-3).
 *  \param numcol
 *      Color index (0-15).
 *  \param value
 *      RGB intensity to set in the specified palette and color index.
 */
void VDP_setPaletteColor(u16 numpal, u16 numcol, u16 value);
/**
 *  \brief
 *      Set palette.
 *
 *  \param num
 *      Palette number (0-3).
 *  \param pal
 *      Source palette.
 */
void VDP_setPalette(u16 num, const u16 *pal);


// these functions should be private as they are called by VDP_fadeXXX functions internally
// but they can be useful sometime for better control on the fading processus
u16  VDP_doStepFading();
u16  VDP_initFading(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe);


/**
 *  \brief
 *      General palette fading effect.
 *
 *  \param fromcol
 *      Start color index for the fade effect (0-63).
 *  \param tocol
 *      End color index for the fade effect (0-63 and >= fromcol).
 *  \param palsrc
 *      Fade departure palette.
 *  \param paldst
 *      Fade arrival palette.
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br/>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  This function does general palette fading effect.<br/>
 *  The fade operation is done to all palette entries between 'fromcol' and 'tocol'.<br/>
 *  Example: fading to all palette entries --> fromcol = 0  and  tocol = 63
 */
void VDP_fade(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async);
/**
 *  \brief
 *      Fade current color palette to specified one.
 *
 *  \param fromcol
 *      Start color index for the fade operation (0-63).
 *  \param tocol
 *      End color index for the fade operation (0-63 and >= fromcol).
 *  \param pal
 *      Fade arrival palette.
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br/>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See VDP_fade() for more informations.
 */
void VDP_fadeTo(u16 fromcol, u16 tocol, const u16 *pal, u16 numframe, u8 async);
/**
 *  \brief
 *      Fade out (current color to black) effect.
 *
 *  \param fromcol
 *      Start color index for the fade operation (0-63).
 *  \param tocol
 *      End color index for the fade operation (0-63 and >= fromcol).
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br/>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See VDP_fade() for more informations.
 */
void VDP_fadeOut(u16 fromcol, u16 tocol, u16 numframe, u8 async);
/**
 *  \brief
 *      Fade in (black to specified color) effect.
 *
 *  \param fromcol
 *      Start color index for the fade operation (0-63).
 *  \param tocol
 *      End color index for the fade operation (0-63 and >= fromcol).
 *  \param pal
 *      Fade arrival palette.
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br/>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See VDP_fade() for more informations.
 */
void VDP_fadeIn(u16 fromcol, u16 tocol, const u16 *pal, u16 numframe, u8 async);

/**
 *  \brief
 *      Do palette fade effect.
 *
 *  \param numpal
 *      Palette number to use for fade effect.
 *  \param palsrc
 *      Fade departure palette.
 *  \param paldst
 *      Fade arrival palette.
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br/>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all specified palette entries.<br/>
 *  See VDP_fade() for more informations.
 */
void VDP_fadePal(u16 numpal, const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async);
/**
 *  \brief
 *      Fade current palette to specified one.
 *
 *  \param numpal
 *      Palette to fade.
 *  \param pal
 *      Fade arrival palette.
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br/>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See VDP_fadePal() for more informations.
 */
void VDP_fadePalTo(u16 numpal, const u16 *pal, u16 numframe, u8 async);
/**
 *  \brief
 *      Fade out (current color to black) effect.
 *
 *  \param numpal
 *      Palette to fade.
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br/>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See VDP_fadePal() for more informations.
 */
void VDP_fadePalOut(u16 numpal, u16 numframe, u8 async);
/**
 *  \brief
 *      Fade in (black to specified color) effect.
 *
 *  \param numpal
 *      Palette to fade.
 *  \param pal
 *      Fade arrival palette.
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br/>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See VDP_fadePal() for more informations.
 */
void VDP_fadePalIn(u16 numpal, const u16 *pal, u16 numframe, u8 async);

/**
 *  \brief
 *      Global palette fading effect.
 *
 *  \param palsrc
 *      Fade departure palette (should contains 64 colors entries).
 *  \param paldst
 *      Fade arrival palette (should contains 64 colors entries).
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br/>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all palette entries.
 */
void VDP_fadeAll(const u16 *palsrc, const u16 *paldst, u16 numframe, u8 async);
/**
 *  \brief
 *      Palettes fade to specified one.
 *
 *  \param pal
 *      Fade arrival palette (should contains 64 entries).
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br/>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all palette entries.<br/>
 *  See VDP_fadeAll().
 */
void VDP_fadeAllTo(const u16 *pal, u16 numframe, u8 async);
/**
 *  \brief
 *      Fade out (current color to black) effect.
 *
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br/>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all palette entries.<br/>
 *  See VDP_fadeAll().
 */
void VDP_fadeOutAll(u16 numframe, u8 async);
/**
 *  \brief
 *      Fade in (black to specified color) effect.
 *
 *  \param pal
 *      Fade arrival palette (should contains 64 entries).
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br/>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all palette entries.<br/>
 *  See VDP_fadeAll().
 */
void VDP_fadeInAll(const u16 *pal, u16 numframe, u8 async);

/**
 *  \brief
 *      Wait for palette fading operation to complete (for asynchrone fading).
 */
void VDP_waitFadeCompletion();


#endif // _VDP_PAL_H_
