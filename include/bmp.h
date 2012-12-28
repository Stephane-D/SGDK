/**
 * \file bmp.h
 * \brief Software bitmap engine
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides methods to simulate bitmap mode on SEGA genesis.<br>
 *<br>
 * The software bitmap engine permits to simulate a 128x160 pixels bitmap screen with doubled X resolution.<br>
 * It uses a double buffer so you can draw to buffer while other buffer is being sent to video memory.<br>
 * Bitmap buffer requires ~41 KB of memory which is dynamically allocated.<br>
 * These buffers are transfered to VRAM during blank area, by default on NTSC system the blanking period<br>
 * is very short so it takes approximately 10 frames to blit an entire buffer.<br>
 * To improve transfer performance the blank area is extended to fit bitmap resolution:<br>
 * 0-31 = blank<br>
 * 32-191 = active<br>
 * 192-262/312 = blank<br>
 * <br>
 * With extended blank bitmap can be transfered to VRAM 20 times per second in NTSC<br>
 * and 25 time per second in PAL.
 */

#ifndef _BMP_H_
#define _BMP_H_

#include "bmp_cmn.h"


/**
 * \brief
 *      Initialize the software bitmap engine.
 *
 * The software bitmap engine permit to simulate a 128x160 pixels bitmap screen with doubled X resolution.<br>
 * It uses a double buffer so you can draw to buffer while other buffer is being sent to video memory.<br>
 * Requires ~41 KB of memory which is dynamically allocated.
 */
void BMP_init();
/**
 * \brief
 *      End the software bitmap engine.
 *
 * Release memory used by software bitmap engine (~41 KB).
 */
void BMP_end();
/**
 * \brief
 *      Reset the software bitmap engine.
 *
 * Rebuild tilemap for bitmap engine and clear buffers.
 */
void BMP_reset();

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
u16 BMP_flip();

/**
 * \brief
 *      Clear bitmap buffer.
 */
void BMP_clear();

/**
 * \brief
 *      Get write pointer for specified pixel.
 *
 * \param x
 *      X pixel coordinate.
 * \param y
 *      Y pixel coordinate.
 */
u8*  BMP_getWritePointer(u16 x, u16 y);
/**
 * \brief
 *      Get read pointer for specified pixel.
 *
 * \param x
 *      X pixel coordinate.
 * \param y
 *      Y pixel coordinate.
 */
u8*  BMP_getReadPointer(u16 x, u16 y);

/**
 * \brief
 *      Get pixel.
 *
 * \param x
 *      X coordinate.
 * \param y
 *      Y coordinate.
 */
u8   BMP_getPixel(u16 x, u16 y);
/**
 * \brief
 *      Set pixel.
 *
 * \param x
 *      X pixel coordinate.
 * \param y
 *      Y pixel coordinate.
 * \param col
 *      pixel color.
 */
void BMP_setPixel(u16 x, u16 y, u8 col);
/**
 * \brief
 *      Set pixels.
 *
 * \param crd
 *      Coordinates buffer.
 * \param col
 *      pixels color.
 * \param num
 *      number of pixel to draw (lenght of coordinates buffer).
 */
void BMP_setPixels_V2D(const Vect2D_u16 *crd, u8 col, u16 num);
/**
 * \brief
 *      Set pixels.
 *
 * \param pixels
 *      Pixels buffer.
 * \param num
 *      number of pixel to draw (lenght of pixels buffer).
 */
void BMP_setPixels(const Pixel *pixels, u16 num);

/**
 * \brief
 *      Draw a line.
 *
 * \param l
 *      Line to draw.
 */
void BMP_drawLine(Line *l);
/**
 * \brief
 *      Draw a polygon.
 *
 * \param pts
 *      Polygon points buffer.
 * \param num
 *      number of point (lenght of points buffer).
 * \param col
 *      fill color.
 * \param culling
 *      Enabled backface culling (back faced polygon are not drawn).
 */
void BMP_drawPolygon(const Vect2D_s16 *pts, u16 num, u8 col, u8 culling);

/**
 * \brief
 *      Load and draw the specified bitmap (should be 4 BPP).
 *
 * \param data
 *      bitmap data buffer.
 * \param x
 *      X coordinate.
 * \param y
 *      y coordinate.
 * \param w
 *      width (expected to be relative to bitmap resolution : 1 pixel = 1 byte).
 * \param h
 *      height.
 * \param pitch
 *      bitmap pitch (number of bytes per bitmap scanline).
 */
void BMP_loadBitmap(const u8 *data, u16 x, u16 y, u16 w, u16 h, u32 pitch);
/**
 * \brief
 *      Load and draw a Genesis 4 BPP bitmap.<br>
 *
 *      A Genesis bitmap is a 4 bpp bitmap which has been converted via the bintos tool.<br>
 *      The resulting file contains bitmap size info and 16 colors palette.
 *
 * \param bitmap
 *      Genesis bitmap.
 * \param x
 *      X coordinate.
 * \param y
 *      y coordinate.
 * \param numpal
 *      Palette (index) to use to load the bitmap palette information.
 */
void BMP_loadGenBitmap(const Bitmap *bitmap, u16 x, u16 y, u16 numpal);
/**
 * \brief
 *      Load and draw a Genesis 4 BPP bitmap with specified dimension.<br>
 *
 *      A Genesis bitmap is a 4 bpp bitmap which has been converted via the bintos tool.<br>
 *      The resulting file contains bitmap size info and 16 colors palette.<br>
 *      The bitmap is scaled to the specified dimension.
 *
 * \param bitmap
 *      Genesis bitmap.
 * \param x
 *      X coordinate.
 * \param y
 *      y coordinate.
 * \param w
 *      width (expected to be relative to bitmap resolution : 1 pixel = 1 byte).
 * \param h
 *      height.
 * \param numpal
 *      Palette (index) to use to load the bitmap palette information.
 */
void BMP_loadAndScaleGenBitmap(const Bitmap *bitmap, u16 x, u16 y, u16 w, u16 h, u16 numpal);
/**
 * \deprecated
 *      Uses bitmap->palette instead.
 */
void BMP_getGenBitmapPalette(const Bitmap *bitmap, u16 *pal);

/**
 * \brief
 *      Scale the specified source bitmap to specified dimension.<br>
 *
 *      Take a source bitmap with its specified dimension and scale it in the<br>
 *      destination buffer with specified dimension.<br>
 *
 * \param src_buf
 *      source bitmap buffer.
 * \param src_w
 *      source width (expected to be relative to bitmap resolution : 1 pixel = 1 byte).
 * \param src_h
 *      source height.
 * \param src_pitch
 *      source pitch (number of bytes per scanline).
 * \param dst_buf
 *      destination bitmap buffer.
 * \param dst_w
 *      destination width (expected to be relative to bitmap resolution : 1 pixel = 1 byte).
 * \param dst_h
 *      destination height.
 * \param dst_pitch
 *      destination pitch (number of bytes per scanline).
 */
void BMP_scale(const u8 *src_buf, u16 src_w, u16 src_h, u16 src_pitch, u8 *dst_buf, u16 dst_w, u16 dst_h, u16 dst_pitch);


#endif // _BMP_H_

