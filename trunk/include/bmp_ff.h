/**
 * \file bmp_ff.h
 * \brief Software bitmap engine (Fast Fill optimized)
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides methods to simulate a bitmap mode on SEGA genesis.
 * This engine is optimized for Fast Fill (uses plains tiles when possible)
 */

#ifndef _BMP_FF_H_
#define _BMP_FF_H_

#include "bmp_cmn.h"


#if ((BMP_TABLES != 0) && (VRAM_TABLE != 0))

/**
 * \brief
 *      Initialize the Fast Fill optimized software bitmap engine.<br>
 *
 * \param flags
 *      Bitmap engine flags, see BMP_setFlags() for description.
 *
 * The Fast Fill optimized software bitmap engine permit to simulate a 128x160 pixels bitmap screen with doubled X resolution.<br>
 * It uses a double buffer so you can draw to buffer while other buffer is currently blitting in video memory.<br>
 * It uses the hardware tile to perform fast filling when possible (fast clear, fast polygon fill...).<br>
 * Take note that this mode is not optimized for no fill processing (line or pixel draw) and will perform slowly in this case.<br>
 * Requires ~44 KB of memory which is dynamically allocated.
 */
void BMP_FF_init(u16 flags);
/**
 * \brief
 *      End the Fast Fill optimized software bitmap engine.
 *
 * Release memory used by Fast Fill optimized software bitmap engine (~44 KB).
 */
void BMP_FF_end();
/**
 * \brief
 *      Reset the Fast Fill optimized software bitmap engine.
 *
 * Rebuild tilemap for Fast Fill optimized bitmap engine and clear buffers.
 */
void BMP_FF_reset();

/**
 * \brief
 *      Set Fast Fill bitmap engine flags.
 *
 * \param value
 *      Bitmap engine flags, see BMP_setFlags() for description.
 */
void BMP_FF_setFlags(u16 value);

/**
 * \brief
 *      Enable <b>BMP_ENABLE_WAITVSYNC</b> flag (see BMP_setFlags() method).
 */
void BMP_FF_enableWaitVSync();
/**
 * \brief
 *      Disable <b>BMP_ENABLE_WAITVSYNC</b> flag (see BMP_setFlags() method).
 */
void BMP_FF_disableWaitVSync();
/**
 * \brief
 *      Enable <b>BMP_ENABLE_ASYNCFLIP</b> flag (see BMP_setFlags() method).
 */
void BMP_FF_enableASyncFlip();
/**
 * \brief
 *      Disable <b>BMP_ENABLE_ASYNCFLIP</b> flag (see BMP_setFlags() method).
 */
void BMP_FF_disableASyncFlip();
/**
 * \brief
 *      Enable <b>BMP_ENABLE_FPSDISPLAY</b> flag (see BMP_setFlags() method).
 */
void BMP_FF_enableFPSDisplay();
/**
 * \brief
 *      Disable <b>BMP_ENABLE_FPSDISPLAY</b> flag (see BMP_setFlags() method).
 */
void BMP_FF_disableFPSDisplay();
/**
 * \brief
 *      Enable <b>BMP_ENABLE_BLITONBLANK</b> flag (see BMP_setFlags() method).
 */
void BMP_FF_enableBlitOnBlank();
/**
 * \brief
 *      Disable <b>BMP_ENABLE_BLITONBLANK</b> flag (see BMP_setFlags() method).
 */
void BMP_FF_disableBlitOnBlank();
/**
 * \brief
 *      Enable <b>BMP_ENABLE_EXTENDEDBLANK</b> flag (see BMP_setFlags() method).
 */
void BMP_FF_enableExtendedBlank();
/**
 * \brief
 *      Disable <b>BMP_ENABLE_EXTENDEDBLANK</b> flag (see BMP_setFlags() method).
 */
void BMP_FF_disableExtendedBlank();

/**
 * \brief
 *      Flip bitmap buffer to screen.
 *
 * Blit the current bitmap back buffer to the screen then flip buffers<br>
 * so back buffer becomes front buffer and vice versa.
 */
void BMP_FF_flip();
//void BMP_FF_internalBufferFlip();

/**
 * \brief
 *      Clear bitmap buffer (uses optimized Fast Fill method).
 */
void BMP_FF_clear();

/**
 * \brief
 *      Get pixel (not optimized in Fast Fill mode).
 *
 * \param x
 *      X coordinate.
 * \param y
 *      Y coordinate.
 */
u8   BMP_FF_getPixel(u16 x, u16 y);
/**
 * \brief
 *      Set pixel (not optimized in Fast Fill mode).
 *
 * \param x
 *      X pixel coordinate.
 * \param y
 *      Y pixel coordinate.
 * \param col
 *      pixel color.
 */
void BMP_FF_setPixel(u16 x, u16 y, u8 col);
/**
 * \brief
 *      Set pixels (not optimized in Fast Fill mode).
 *
 * \param crd
 *      Coordinates buffer.
 * \param col
 *      pixels color.
 * \param num
 *      number of pixel to draw (lenght of coordinates buffer).
 */
void BMP_FF_setPixels_V2D(const Vect2D_u16 *crd, u8 col, u16 num);
/**
 * \brief
 *      Set pixels (not optimized in Fast Fill mode).
 *
 * \param pixels
 *      Pixels buffer.
 * \param num
 *      number of pixel to draw (lenght of pixels buffer).
 */
void BMP_FF_setPixels(const Pixel *pixels, u16 num);

/**
 * \brief
 *      Draw a line (not optimized in Fast Fill mode).
 *
 * \param l
 *      Line to draw.
 */
void BMP_FF_drawLine(Line *l);

#endif


#endif // _BMP_FF_H_

