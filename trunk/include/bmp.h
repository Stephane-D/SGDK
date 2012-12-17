/**
 * \file bmp.h
 * \brief Software bitmap engine
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides methods to simulate bitmap mode on SEGA genesis.
 */

#ifndef _BMP_H_
#define _BMP_H_

#include "bmp_cmn.h"


/**
 * \brief
 *      Initialize the software bitmap engine.<br>
 *
 * \param flags
 *      Bitmap engine flags, see BMP_setFlags() for description.
 *
 * The software bitmap engine permit to simulate a 128x160 pixels bitmap screen with doubled X resolution.<br>
 * It uses a double buffer so you can draw to buffer while other buffer is currently blitting in video memory.<br>
 * Requires ~41 KB of memory which is dynamically allocated.
 */
void BMP_init(u16 flags);
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
 *      Set bitmap engine flags.
 *
 * \param value
 *      BITMAP engine flags :<br>
 *      <b>BMP_ENABLE_WAITVSYNC</b> = wait VBlank before doing flip operation (see BMP_flip).<br>
 *      <b>BMP_ENABLE_ASYNCFLIP</b> = Asynch flip operation.<br>
 *         When this flag is enabled BMP_flip() will return immediatly even if flip wait for VBlank.<br>
 *         Note that this flag automatically enable BMP_ENABLE_WAITVSYNC.<br>
 *      <b>BMP_ENABLE_BLITONBLANK</b> = Process blit only during VDP blanking.<br>
 *         VRAM access is faster during blanking so this permit to optimize blit processing on best period<br>
 *         and keep the rest of time available for others processing.<br>
 *         By default on NTSC system the blanking period is very short so it takes approximately 15 frames<br>
 *         to blit the entire bitmap screen (blit is done in software).<br>
 *         Note that this flag automatically enable BMP_ENABLE_ASYNCFLIP.<br>
 *      <b>BMP_ENABLE_EXTENDEDBLANK</b> = Extend blanking period to fit bitmap height resolution (160 pixels).<br>
 *         This permit to improve blit process time (reduce 15 frames to approximately 4 frames on NTSC system).<br>
 *         Note that this flag automatically enable BMP_ENABLE_BLITONBLANK.<br>
 *      <b>BMP_ENABLE_FPSDISPLAY</b> = Display frame rate (number of bitmap flip per second).<br>
 *      <b>BMP_ENABLE_BFCULLING</b> = Enabled culling (used only for polygon drawing).<br>
 */
void BMP_setFlags(u16 value);

/**
 * \brief
 *      Enable <b>BMP_ENABLE_WAITVSYNC</b> flag (see BMP_setFlags() method).
 */
void BMP_enableWaitVSync();
/**
 * \brief
 *      Disable <b>BMP_ENABLE_WAITVSYNC</b> flag (see BMP_setFlags() method).
 */
void BMP_disableWaitVSync();
/**
 * \brief
 *      Enable <b>BMP_ENABLE_ASYNCFLIP</b> flag (see BMP_setFlags() method).
 */
void BMP_enableASyncFlip();
/**
 * \brief
 *      Disable <b>BMP_ENABLE_ASYNCFLIP</b> flag (see BMP_setFlags() method).
 */
void BMP_disableASyncFlip();
/**
 * \brief
 *      Enable <b>BMP_ENABLE_FPSDISPLAY</b> flag (see BMP_setFlags() method).
 */
void BMP_enableFPSDisplay();
/**
 * \brief
 *      Disable <b>BMP_ENABLE_FPSDISPLAY</b> flag (see BMP_setFlags() method).
 */
void BMP_disableFPSDisplay();
/**
 * \brief
 *      Enable <b>BMP_ENABLE_BLITONBLANK</b> flag (see BMP_setFlags() method).
 */
void BMP_enableBlitOnBlank();
/**
 * \brief
 *      Disable <b>BMP_ENABLE_BLITONBLANK</b> flag (see BMP_setFlags() method).
 */
void BMP_disableBlitOnBlank();
/**
 * \brief
 *      Enable <b>BMP_ENABLE_EXTENDEDBLANK</b> flag (see BMP_setFlags() method).
 */
void BMP_enableExtendedBlank();
/**
 * \brief
 *      Disable <b>BMP_ENABLE_EXTENDEDBLANK</b> flag (see BMP_setFlags() method).
 */
void BMP_disableExtendedBlank();

/**
 * \brief
 *      Flip bitmap buffer to screen.
 *
 * Blit the current bitmap back buffer to the screen then flip buffers<br>
 * so back buffer becomes front buffer and vice versa.
 */
void BMP_flip();

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
 */
void BMP_drawPolygon(const Vect2D_s16 *pts, u16 num, u8 col);

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

