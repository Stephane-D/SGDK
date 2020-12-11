/**
 *  \file pal.h
 *  \brief Palette support (herited from vdp_pal.h unit)
 *  \author Stephane Dallongeville
 *  \date 06/2019
 *
 * This unit provides methods to manipulate the VDP Color Palette.<br>
 * The Sega Genesis VDP has 4 palettes of 16 colors.<br>
 * Color is defined with 3 bits for each component : xxxxBBBxGGGxRRRx
 */

#ifndef _PAL_H_
#define _PAL_H_


#define VDPPALETTE_REDSFT           1
#define VDPPALETTE_GREENSFT         5
#define VDPPALETTE_BLUESFT          9

#define VDPPALETTE_REDMASK          0x000E
#define VDPPALETTE_GREENMASK        0x00E0
#define VDPPALETTE_BLUEMASK         0x0E00
#define VDPPALETTE_COLORMASK        0x0EEE

/**
 *  \brief
 *      Convert a RGB 24 bits color to VDP color
 *
 *  \param color
 *      RGB 24 bits color
 */
#define RGB24_TO_VDPCOLOR(color)    (((color >> ((2 * 8) + 4)) & VDPPALETTE_REDMASK) | ((color >> ((1 * 4) + 4)) & VDPPALETTE_GREENMASK) | ((color << 4) & VDPPALETTE_BLUEMASK))


/**
 *  \brief
 *      Palette structure contains color data.
 *
 *  \param length
 *      Size of this palette.
 *  \param dat
 *      Color data.
 */
typedef struct
{
    u16 length;
    u16* data;
} Palette;


/**
 *  \brief
 *      Default black palette.
 */
extern const u16* const palette_black;
/**
 *  \brief
 *      Default grey palette.
 */
extern const u16 palette_grey[16];
/**
 *  \brief
 *      Default red palette.
 */
extern const u16 palette_red[16];
/**
 *  \brief
 *      Default green palette.
 */
extern const u16 palette_green[16];
/**
 *  \brief
 *      Default blue palette.
 */
extern const u16 palette_blue[16];


/**
 *  \brief
 *      Returns RGB color value from CRAM for the specified palette entry.
 *
 *  \param index
 *      Color index (0-63).
 *  \return RGB intensity for the specified color index.
 */
u16  PAL_getColor(u16 index);
/**
 *  \brief
 *      Read count RGB colors from CRAM starting at specified index and store them in specified destination palette.
 *
 *  \param index
 *      Color index where start to read (0-63).
 *  \param dest
 *      Destination palette where to write read the RGB color values (should be large enough to store count colors).
 *  \param count
 *      Number of color to get.
 */
void  PAL_getColors(u16 index, u16* dest, u16 count);
/**
 *  \brief
 *      Get a complete palette (16 colors) from CRAM.
 *
 *  \param numPal
 *      Palette number: PAL0, PAL1, PAL2 or PAL3
 *  \param dest
 *      Destination where to write palette colors (should be 16 words long at least)
 */
void PAL_getPalette(u16 numPal, u16* dest);

/**
 *  \brief
 *      Set RGB color into CRAM for the specified palette entry.
 *
 *  \param index
 *      Color index to set (0-63).
 *  \param value
 *      RGB intensity to set at the specified color index.
 */
void PAL_setColor(u16 index, u16 value);
/**
 *  \brief
 *      Write RGB colors into CRAM for the specified palette entries.
 *
 *  \param index
 *      Color index where to start to write (0-63).
 *  \param pal
 *      RGB intensities to set.
 *  \param count
 *      Number of color to set.
 */
void PAL_setColors(u16 index, const u16* pal, u16 count);
/**
 *  \brief
 *      Write the given Palette RGB colors into CRAM for the specified palette entries.
 *
 *  \param index
 *      Color index where to start to write (0-63).
 *  \param pal
 *      Source Palette.
 */
void PAL_setPaletteColors(u16 index, const Palette* pal);
/**
 *  \brief
 *      Set a complete palette (16 colors) into CRAM.
 *
 *  \param numPal
 *      Palette number: PAL0, PAL1, PAL2 or PAL3
 *  \param pal
 *      Source palette.
 */
void PAL_setPalette(u16 numPal, const u16* pal);

/**
 *  \brief
 *      Write RGB colors into CRAM for the specified palette entries (DMA queue version).
 *
 *  \param index
 *      Color index where to start to write (0-63).
 *  \param pal
 *      RGB intensities to set.
 *  \param count
 *      Number of color to set.
 */
void PAL_setColorsDMA(u16 index, const u16* pal, u16 count);
/**
 *  \brief
 *      Write the given Palette RGB colors into CRAM for the specified palette entries (DMA queue version).
 *
 *  \param index
 *      Color index where to start to write (0-63).
 *  \param pal
 *      Source Palette.
 */
void PAL_setPaletteColorsDMA(u16 index, const Palette* pal);
/**
 *  \brief
 *      Set a complete palette (16 colors) into CRAM (DMA queue version).
 *
 *  \param numPal
 *      Palette number: PAL0, PAL1, PAL2 or PAL3
 *  \param pal
 *      Source palette.
 */
void PAL_setPaletteDMA(u16 numPal, const u16* pal);


// these functions should be private as they are called by PAL_fadeXXX functions internally
// but they can be useful sometime for better control on the fading processus
bool PAL_initFade(u16 fromCol, u16 toCol, const u16* palSrc, const u16* palDst, u16 numFrame);
bool PAL_doFadeStep();


/**
 *  \brief
 *      General palette fading effect.
 *
 *  \param fromCol
 *      Start color index for the fade effect (0-63).
 *  \param toCol
 *      End color index for the fade effect (0-63 and >= fromCol).
 *  \param palSrc
 *      Fade departure palette.
 *  \param palDst
 *      Fade arrival palette.
 *  \param numFrame
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  This function does general palette fading effect.<br>
 *  The fade operation is done to all palette entries between 'fromCol' and 'toCol'.<br>
 *  Example: fading to all palette entries --> fromCol = 0  and  toCol = 63
 */
void PAL_fade(u16 fromCol, u16 toCol, const u16* palSrc, const u16* palDst, u16 numFrame, bool async);
/**
 *  \brief
 *      Fade current color palette to specified one.
 *
 *  \param fromCol
 *      Start color index for the fade operation (0-63).
 *  \param toCol
 *      End color index for the fade operation (0-63 and >= fromCol).
 *  \param pal
 *      Fade arrival palette.
 *  \param numFrame
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See PAL_fade() for more informations.
 */
void PAL_fadeTo(u16 fromCol, u16 toCol, const u16* pal, u16 numFrame, bool async);
/**
 *  \brief
 *      Fade out (current color to black) effect.
 *
 *  \param fromCol
 *      Start color index for the fade operation (0-63).
 *  \param toCol
 *      End color index for the fade operation (0-63 and >= fromCol).
 *  \param numFrame
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See PAL_fade() for more informations.
 */
void PAL_fadeOut(u16 fromCol, u16 toCol, u16 numFrame, bool async);
/**
 *  \brief
 *      Fade in (black to specified color) effect.
 *
 *  \param fromCol
 *      Start color index for the fade operation (0-63).
 *  \param toCol
 *      End color index for the fade operation (0-63 and >= fromCol).
 *  \param pal
 *      Fade arrival palette.
 *  \param numFrame
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See PAL_fade() for more informations.
 */
void PAL_fadeIn(u16 fromCol, u16 toCol, const u16* pal, u16 numFrame, bool async);

/**
 *  \brief
 *      Do palette fade effect.
 *
 *  \param numPal
 *      Palette number to use for fade effect.
 *  \param palSrc
 *      Fade departure palette.
 *  \param palDst
 *      Fade arrival palette.
 *  \param numFrame
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all specified palette entries.<br>
 *  See PAL_fade() for more informations.
 */
void PAL_fadePalette(u16 numPal, const u16* palSrc, const u16* palDst, u16 numFrame, bool async);
/**
 *  \brief
 *      Fade current palette to specified one.
 *
 *  \param numPal
 *      Palette to fade.
 *  \param pal
 *      Fade arrival palette.
 *  \param numFrame
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See PAL_fadePal() for more informations.
 */
void PAL_fadeToPalette(u16 numPal, const u16* pal, u16 numFrame, bool async);
/**
 *  \brief
 *      Fade out (current color to black) effect.
 *
 *  \param numPal
 *      Palette to fade.
 *  \param numFrame
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See PAL_fadePal() for more informations.
 */
void PAL_fadeOutPalette(u16 numPal, u16 numFrame, bool async);
/**
 *  \brief
 *      Fade in (black to specified color) effect.
 *
 *  \param numPal
 *      Palette to fade.
 *  \param pal
 *      Fade arrival palette.
 *  \param numFrame
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  See PAL_fadePal() for more informations.
 */
void PAL_fadeInPalette(u16 numPal, const u16* pal, u16 numFrame, bool async);

/**
 *  \brief
 *      Global palette fading effect.
 *
 *  \param palSrc
 *      Fade departure palette (should contains 64 colors entries).
 *  \param palDst
 *      Fade arrival palette (should contains 64 colors entries).
 *  \param numFrame
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all palette entries.
 */
void PAL_fadeAll(const u16* palSrc, const u16* palDst, u16 numFrame, bool async);
/**
 *  \brief
 *      Palettes fade to specified one.
 *
 *  \param pal
 *      Fade arrival palette (should contains 64 entries).
 *  \param numFrame
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all palette entries.<br>
 *  See PAL_fadeAll().
 */
void PAL_fadeToAll(const u16* pal, u16 numFrame, bool async);
/**
 *  \brief
 *      Fade out (current color to black) effect.
 *
 *  \param numFrame
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all palette entries.<br>
 *  See PAL_fadeAll().
 */
void PAL_fadeOutAll(u16 numFrame, bool async);
/**
 *  \brief
 *      Fade in (black to specified color) effect.
 *
 *  \param pal
 *      Fade arrival palette (should contains 64 entries).
 *  \param numFrame
 *      Duration of palette fading in number of frame.
 *  \param async
 *      Async process.<br>
 *      If set the function return immediatly else the function wait for fading to complete.
 *
 *  The fade operation is done to all palette entries.<br>
 *  See PAL_fadeAll().
 */
void PAL_fadeInAll(const u16* pal, u16 numFrame, bool async);

/**
 *  \brief
 *      Returns TRUE if currently doing a asynchronous fade operation.
 */
bool PAL_isDoingFade();
/**
 *  \brief
 *      Wait for palette fading operation to complete (for asynchrone fading).
 */
void PAL_waitFadeCompletion();
/**
 *  \brief
 *      Interrupt any asynchronous palette fading effect.
 */
void PAL_interruptFade();


#endif // _VDP_PAL_H_
