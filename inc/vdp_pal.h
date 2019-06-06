/**
 *  \deprecated Uses pal.h unit instead
 */

#ifndef _VDP_PAL_H_
#define _VDP_PAL_H_


/**
 *  \brief
 *      Returns RGB color of specified palette entry.
 *
 *  \param index
 *      Color index (0-63).
 *  \return RGB intensity for the specified color index.
 */
u16  VDP_getPaletteColor(u16 index);
/**
 *  \brief
 *      Read count RGB colors from specified index and store them in specified palette.
 *
 *  \param index
 *      Color index where start to read (0-63).
 *  \param dest
 *      Destination palette where to write read RGB intensities.
 *  \param count
 *      Number of color to get.
 */
void  VDP_getPaletteColors(u16 index, u16* dest, u16 count);
/**
 *  \brief
 *      Get a complete palette (16 colors).
 *
 *  \param num
 *      Palette number: PAL0, PAL1, PAL2 or PAL3
 *  \param pal
 *      Destination where to copy palette colors (should be 16 words long at least)
 */
void VDP_getPalette(u16 num, u16 *pal);

/**
 *  \brief
 *      Set RGB color to specified palette entry.
 *
 *  \param index
 *      Color index to set (0-63).
 *  \param value
 *      RGB intensity to set in the specified color index.
 */
void VDP_setPaletteColor(u16 index, u16 value);
/**
 *  \brief
 *      Set RGB colors to specified palette entries.
 *
 *  \param index
 *      Color index where to start to write (0-63).
 *  \param values
 *      RGB intensities to set.
 *  \param count
 *      Number of color to set.
 */
void VDP_setPaletteColors(u16 index, const u16* values, u16 count);
/**
 *  \brief
 *      Set a complete palette (16 colors).
 *
 *  \param num
 *      Palette number: PAL0, PAL1, PAL2 or PAL3
 *  \param pal
 *      Source palette.
 */
void VDP_setPalette(u16 num, const u16 *pal);


// these functions should be private as they are called by VDP_fadeXXX functions internally
// but they can be useful sometime for better control on the fading processus
bool VDP_doFadingStep();
bool VDP_initFading(u16 fromcol, u16 tocol, const u16 *palsrc, const u16 *paldst, u16 numframe);


/**
 *  \brief
 *      Interrupt any asynchronous palette fading effect.
 */
void VDP_interruptFade();

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
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  This function does general palette fading effect.<br>
 *  The fade operation is done to all palette entries between 'fromcol' and 'tocol'.<br>
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
 *      Async process.<br>
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
 *      Async process.<br>
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
 *      Async process.<br>
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
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all specified palette entries.<br>
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
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See VDP_fadePal() for more informations.
 */
void VDP_fadeToPal(u16 numpal, const u16 *pal, u16 numframe, u8 async);
/**
 *  \brief
 *      Fade out (current color to black) effect.
 *
 *  \param numpal
 *      Palette to fade.
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See VDP_fadePal() for more informations.
 */
void VDP_fadeOutPal(u16 numpal, u16 numframe, u8 async);
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
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See VDP_fadePal() for more informations.
 */
void VDP_fadeInPal(u16 numpal, const u16 *pal, u16 numframe, u8 async);

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
 *      Async process.<br>
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
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all palette entries.<br>
 *  See VDP_fadeAll().
 */
void VDP_fadeToAll(const u16 *pal, u16 numframe, u8 async);
/**
 *  \brief
 *      Fade out (current color to black) effect.
 *
 *  \param numframe
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all palette entries.<br>
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
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all palette entries.<br>
 *  See VDP_fadeAll().
 */
void VDP_fadeInAll(const u16 *pal, u16 numframe, u8 async);

/**
 *  \brief
 *      Returns TRUE if currently doing a asynchronous fade operation.
 */
u16 VDP_isDoingFade();
/**
 *  \brief
 *      Wait for palette fading operation to complete (for asynchrone fading).
 */
void VDP_waitFadeCompletion();


#endif // _VDP_PAL_H_
