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
 * It uses the hardware tile to perform fast filling when possible (fast clear, fast polygon fill...).<br>
 * Take note that this mode is not optimized for no fill processing (line or pixel draw) and will perform slowly in this case.<br>
 * <br>
 * It uses a double buffer so you can draw to buffer while other buffer is currently blitting in video memory.<br>
 * Bitmap buffer requires ~41 KB of memory which is dynamically allocated.<br>
 * These buffers are transfered to VRAM during blank area, by default on NTSC system the blanking period<br>
 * is very short so it takes approximately 10 frames to blit an entire buffer.<br>
 * To improve transfer performance the blank area is extended to fit bitmap resolution:<br>
 * 0-31 = blank<br>
 * 32-191 = active<br>
 * 192-262/312 = blank
 */
void BMP_FF_init();
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
 *      Enable frame rate display (usesul for profiling).
 */
void BMP_FF_enableFPSDisplay();
/**
 * \brief
 *      Disable frame rate display (usesul for profiling).
 */
void BMP_FF_disableFPSDisplay();

/**
 * \brief
 *      Flip bitmap buffer to screen.
 *
 * Blit the current bitmap back buffer to the screen then flip buffers
 * so back buffer becomes front buffer and vice versa.<br>
 * Bitmap buffer is sent to video memory asynchronously during blank period
 * so the function return immediatly.<br>
 * If a flip is already in process then flip request is marked as pending
 * and will be processed as soon the current one complete.<br>
 * Take care of that before writing to bitmap buffer, you can use the
 * #BMP_waitWhileFlipRequestPending() method to ensure no more flip request are pending.<br>
 * If a flip request is already pending the function wait until no more request are pending.
 *
 * \return
 *   0 if the flip request has be marked as pending as another flip is already in process.<br>
 *   1 if the flip request has be initiated.
 *
 * \see #BMP_hasFlipRequestPending()
 * \see #BMP_waitWhileFlipRequestPending()
 */
u16 BMP_FF_flip();

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

