/**
 *  \file bmp.h
 *  \brief Software bitmap engine
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides methods to simulate bitmap mode on SEGA genesis.<br>
 *<br>
 * The software bitmap engine permit to simulate a 256x160 pixels bitmap screen.<br>
 * Some methods as #BMP_setPixel and #BMP_drawLine use doubled X pixel so you can
 * consider resolution to be 128x160 in this case.<br>
 * Bitmap engine requires a large amount of memory (~41KB) which is dynamically allocated at BMP_init(..) time and released when BMP_end) is called.<br>
 * Bitmap engine uses a double buffer so you can draw to the write buffer while the read buffer is being sent to video memory.<br>
 * These buffers are transfered to VRAM during blank area, by default on NTSC system the blanking period is really short so it takes approximately 10 frames to blit an entire buffer.<br>
 * To improve transfer performance the blank area is extended to fit bitmap resolution:<br>
 * scanline 0-31 = blank<br>
 * scanline 32-191 = active<br>
 * scanline 192-262/312 = blank<br>
 * <br>
 * With extended blank bitmap buffer can be transfered to VRAM 20 times per second in NTSC<br>
 * and 25 time per second in PAL.
 */

#include "maths.h"
#include "vdp.h"
#include "vdp_pal.h"

#ifndef _BMP_H_
#define _BMP_H_


#define BMP_PLANWIDTH_SFT           planWidthSft
#define BMP_PLANHEIGHT_SFT          planHeightSft
/**
 *  \brief
 *          Bitmap plan width (in tile)
 */
#define BMP_PLANWIDTH               planWidth
/**
 *  \brief
 *          Bitmap plan height (in tile)
 */
#define BMP_PLANHEIGHT              planHeight

#define BMP_CELLWIDTH_SFT           5
/**
 *  \brief
 *          Bitmap width (in tile) : 32
 */
#define BMP_CELLWIDTH               (1 << BMP_CELLWIDTH_SFT)
/**
 *  \brief
 *          Bitmap height (in tile) : 20
 */
#define BMP_CELLHEIGHT              20
#define BMP_CELLWIDTHMASK           (BMP_CELLWIDTH - 1)

#define BMP_CELLXOFFSET             (((screenWidth >> 3) - BMP_CELLWIDTH) / 2)
#define BMP_CELLYOFFSET             (((screenHeight >> 3) - BMP_CELLHEIGHT) / 2)

#define BMP_XPIXPERTILE_SFT         3
#define BMP_YPIXPERTILE_SFT         3
/**
 *  \brief
 *          Number of X pixel per tile : 8 pixels per tile.
 */
#define BMP_XPIXPERTILE             (1 << BMP_XPIXPERTILE_SFT)
/**
 *  \brief
 *          Number of y pixel per tile : 8 pixels per tile.
 */
#define BMP_YPIXPERTILE             (1 << BMP_YPIXPERTILE_SFT)
#define BMP_XPIXPERTILEMASK         (BMP_XPIXPERTILE - 1)
#define BMP_YPIXPERTILEMASK         (BMP_YPIXPERTILE - 1)

#define BMP_WIDTH_SFT               (BMP_CELLWIDTH_SFT + BMP_XPIXPERTILE_SFT)
/**
 *  \brief
 *          Bitmap width (in pixel) : 256
 */
#define BMP_WIDTH                   (1 << BMP_WIDTH_SFT)
/**
 *  \brief
 *          Bitmap height (in pixel) : 160
 */
#define BMP_HEIGHT                  (BMP_CELLHEIGHT * BMP_YPIXPERTILE)
#define BMP_WIDTH_MASK              (BMP_WIDTH - 1)

#define BMP_PITCH_SFT               (BMP_CELLWIDTH_SFT + 2)
/**
 *  \brief
 *          Bitmap scanline pitch (number of bytes per scanline) : 128
 */
#define BMP_PITCH                   (1 << BMP_PITCH_SFT)
#define BMP_PITCH_MASK              (BMP_PITCH - 1)

/**
 *  \brief
 *          Get width of genesis bitmap 16 object.
 */
#define BMP_GENBMP16_WIDTH(genbmp16)    ((genbmp16)[0])
/**
 *  \brief
 *          Get height of genesis bitmap 16 object.
 */
#define BMP_GENBMP16_HEIGHT(genbmp16)   ((genbmp16)[1])
/**
 *  \brief
 *          Return pointer to palette of genesis bitmap 16 object.
 */
#define BMP_GENBMP16_PALETTE(genbmp16)  (&((genbmp16)[2]))
/**
 *  \brief
 *          Return pointer to image data of genesis bitmap 16 object.
 */
#define BMP_GENBMP16_IMAGE(genbmp16)    (&((genbmp16)[18]))

/**
 *  \brief
 *          Return pixel value at specified location.
 *      \param x
 *          X coordinate.
 *      \param y
 *          Y coordinate.
 *
 * Pixels are rendereded with X doubled resolution (byte operation) for performance reason.<br>
 * So BMP_GETPIXEL(0,0) will actually returns same value as BMP_GETPIXEL((1,0)<br>
 * Be careful this function does not check for retrieving pixel outside bitmap buffer.
 */
#define BMP_GETPIXEL(x, y)          bmp_buffer_write[((y) * BMP_PITCH) + ((x) >> 1)]

/**
 *  \brief
 *          Set pixel value at specified position.
 *
 *      \param x
 *          X pixel coordinate.
 *      \param y
 *          Y pixel coordinate.
 *      \param col
 *          pixel color.
 *
 * Pixels are rendereded with X doubled resolution (byte operation) for performance reason.<br>
 * So BMP_SETPIXEL(0,0,..) will actually set same pixel as BMP_SETPIXEL(1,0,..).
 * Be careful this function does not check for retrieving pixel outside bitmap buffer.
 */
#define BMP_SETPIXEL(x, y, col)     bmp_buffer_write[((y) * BMP_PITCH) + ((x) >> 1)] = col;


#define BMP_BASETILEINDEX       TILE_USERINDEX

#define BMP_FB0TILEINDEX        BMP_BASETILEINDEX
#define BMP_FB1TILEINDEX        (BMP_BASETILEINDEX + (BMP_CELLWIDTH * BMP_CELLHEIGHT))

#define BMP_FB0ENDTILEINDEX     (BMP_FB0TILEINDEX + (BMP_CELLWIDTH * BMP_CELLHEIGHT))
#define BMP_FB1ENDTILEINDEX     (BMP_FB1TILEINDEX + (BMP_CELLWIDTH * BMP_CELLHEIGHT))

#define BMP_BASETILE            (BMP_BASETILEINDEX * 32)
#define BMP_FB0TILE             (BMP_FB0TILEINDEX * 32)
#define BMP_FB1TILE             (BMP_FB1TILEINDEX * 32)


/**
 *  \brief
 *      Genesis 4bpp Bitmap structure definition.<br>
 *      Use the unpackBitmap() method to unpack if compression is enabled.
 *
 *  \param compression
 *      compression type for image data, accepted values:<br>
 *      <b>COMPRESSION_NONE</b><br>
 *      <b>COMPRESSION_APLIB</b><br>
 *      <b>COMPRESSION_LZ4W</b><br>
 *  \param w
 *      Width in pixel.
 *  \param h
 *      Height in pixel.
 *  \param palette
 *      Palette data.
 *  \param image
 *      Image data, array size = (w * h / 2).
 */
typedef struct
{
    u16 compression;
    u16 w;
    u16 h;
    const Palette *palette;
    const u8 *image;
} Bitmap;

/**
 *  \brief
 *          Pixel definition.
 *
 *  \param pt
 *          Coordinates.
 *  \param col
 *          Color.
 */
typedef struct
{
    Vect2D_s16 pt;
    u8 col;
} Pixel;

/**
 *  \brief
 *          Line definition.
 *
 *  \param pt1
 *          Start point.
 *  \param pt2
 *          End point.
 *  \param col
 *          Color.
 */
typedef struct
{
    Vect2D_s16 pt1;
    Vect2D_s16 pt2;
    u8 col;
} Line;

/**
 *  \brief
 *          Triangle definition.
 *
 *  \param pt1
 *          Start point.
 *  \param pt2
 *          Second point.
 *  \param pt3
 *          End point.
 *  \param col
 *          Color.
 */
typedef struct
{
    Vect2D_s16 pt1;
    Vect2D_s16 pt2;
    Vect2D_s16 pt3;
    u8 col;
} Triangle;


/**
 *      Current bitmap read buffer.
 */
extern u8 *bmp_buffer_read;
/**
 *      Current bitmap write buffer.
 */
extern u8 *bmp_buffer_write;


/**
 *  \brief
 *      Initialize the software bitmap engine.
 *
 *  \param double_buffer
 *      Enabled VRAM double buffer.<br>
 *      VRAM Double buffer permit to avoid image tearing because of partial screen refresh.<br>
 *      It requires almost all VRAM tiles space (~41 KB) so enable it only if you don't need other plan or sprites.
 *  \param plan
 *      Plan to use to display the bitmap.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *  \param palette
 *      Palette index to use to render the bitmap plan.<br>
 *      Set it to 0 if unsure.
 *  \param priority
 *      Set the priority of bitmap plan.<br>
 *      0 = low priority (default).<br>
 *      1 = high priority.
 *
 * Requires ~41 KB of memory which is dynamically allocated.
 */
void BMP_init(u16 double_buffer, VDPPlan plan, u16 palette, u16 priority);
/**
 *  \brief
 *      End the software bitmap engine.
 *
 * Release memory used by software bitmap engine (~41 KB).
 */
void BMP_end();
/**
 *  \brief
 *      Reset the software bitmap engine.
 *
 * Rebuild tilemap for bitmap engine and clear buffers.
 */
void BMP_reset();
/**
 *  \brief
 *      Enable back buffer preservation.
 *
 * The bitmap engine is always using double buffering in maim memory so you can continue to write your bitmap
 * while the previous bitmap is being transfered to video memory.<br>
 * The problem with double buffer is that your content is not preserved on a frame basis as you have 2 differents buffers,
 * by enabling "buffer copy" you can preserve your bitmap but this has an important CPU cost as we need to copy bitmap buffer
 * at each flip operation.<br>
 * By default buffer copy is disabled for obvious performance reason.
 */
void BMP_setBufferCopy(u16 value);
/**
 *  \brief
 *      Flip bitmap buffer to screen.
 *
 * Blit the current bitmap back buffer to the screen then flip buffers
 * so back buffer becomes front buffer and vice versa.<br>
 * If you use async flag the Bitmap buffer will be sent to video memory asynchronously
 * during blank period and the function return immediatly.<br>
 * If a flip is already in process then flip request is marked as pending
 * and will be processed as soon the current one complete.<br>
 * You can use #BMP_waitWhileFlipRequestPending() method to ensure no more flip request are pending
 * before writing to bitmap buffer.<br>
 * If a flip request is already pending the function wait until no more request are pending.
 *
 *  \param async
 *      Asynchronous flip operation flag.
 *  \return
 *   Only meaningful for async operation:<br>
 *   0 if the flip request has be marked as pending as another flip is already in process.<br>
 *   1 if the flip request has be initiated.
 *
 *  \see #BMP_hasFlipRequestPending()
 *  \see #BMP_waitWhileFlipRequestPending()
 */
u16 BMP_flip(u16 async);

/**
 *  \brief
 *      Clear bitmap buffer.
 */
void BMP_clear();

/**
 *  \brief
 *      Get write pointer for specified pixel.
 *
 *  \param x
 *      X pixel coordinate.
 *  \param y
 *      Y pixel coordinate.
 *
 * As coordinates are expressed for 4bpp pixel BMP_getWritePointer(0,0)
 * and BMP_getWritePointer(1,0) actually returns the same address.
 */
u8*  BMP_getWritePointer(u16 x, u16 y);
/**
 *  \brief
 *      Get read pointer for specified pixel.
 *
 *  \param x
 *      X pixel coordinate.
 *  \param y
 *      Y pixel coordinate.
 *
 * As coordinates are expressed for 4bpp pixel BMP_getReadPointer(0,0)
 * and BMP_getReadPointer(1,0) actually returns the same address.
 */
u8*  BMP_getReadPointer(u16 x, u16 y);


/**
 *  \brief
 *      Return true (!= 0) if a flip request is pending.
 */
u16  BMP_hasFlipRequestPending();
/**
 *  \brief
 *      Wait until no more flip request is pending.
 */
void BMP_waitWhileFlipRequestPending();
/**
 *  \brief
 *      Return true (!= 0) if a flip operation is in progress.
 */
u16  BMP_hasFlipInProgess();
/**
 *  \brief
 *      Wait until the asynchronous flip operation is completed.<br>
 *      Blit operation is the bitmap buffer transfer to VRAM.<br>
 */
void BMP_waitFlipComplete();

/**
 *  \brief
 *      Draw text in bitmap mode.
 *
 *  \param str
 *      String to display.
 *  \param x
 *      X coordinate (in tile).
 *  \param y
 *      y coordinate (in tile).
 */
void BMP_drawText(const char *str, u16 x, u16 y);
/**
 *  \brief
 *      Clear text in bitmap mode.
 *
 *  \param x
 *      X coordinate (in tile).
 *  \param y
 *      y coordinate (in tile).
 *  \param w
 *      Number of characters to clear.
 */
void BMP_clearText(u16 x, u16 y, u16 w);
/**
 *  \brief
 *      Clear a line of text in bitmap mode.
 *
 *  \param y
 *      y coordinate (in tile).
 */
void BMP_clearTextLine(u16 y);

/**
 *  \brief
 *      Show the frame rate in bitmap mode.
 *
 *  \param float_display
 *      If this value is true (!= 0) the frame rate is displayed as float (else it's integer).
 */
void BMP_showFPS(u16 float_display);


/**
 *  \brief
 *      Get pixel at specified position.
 *
 *  \param x
 *      X coordinate.
 *  \param y
 *      Y coordinate.
 *
 * Pixels are rendereded with X doubled resolution (byte operation) for performance reason.<br>
 * So BMP_getPixel(0,0) will actually returns same value as BMP_getPixel(1,0).
 *
 *  \see #BMP_GETPIXEL (faster but not as safe)
 */
u8   BMP_getPixel(u16 x, u16 y);
/**
 *  \brief
 *      Set pixel at specified position.
 *
 *  \param x
 *      X pixel coordinate.
 *  \param y
 *      Y pixel coordinate.
 *  \param col
 *      pixel color.
 *
 * Pixels are rendereded with X doubled resolution (byte operation) for performance reason.<br>
 * So BMP_setPixel(0,0,..) will actually set same pixel as BMP_setPixel(1,0,..).
 *
 *  \see #BMP_SETPIXEL (faster but not as safe)
 */
void BMP_setPixel(u16 x, u16 y, u8 col);
/**
 *  \brief
 *      Set pixels.
 *
 *  \param crd
 *      Coordinates buffer.
 *  \param col
 *      pixels color.
 *  \param num
 *      number of pixel to draw (lenght of coordinates buffer).
 *
 * Pixels are rendereded with X doubled resolution (byte operation) for performance reason.<br>
 * So BMP_setPixel(0,0,..) will actually set same pixel as BMP_setPixel(1,0,..).
 */
void BMP_setPixels_V2D(const Vect2D_u16 *crd, u8 col, u16 num);
/**
 *  \brief
 *      Set pixels.
 *
 *  \param pixels
 *      Pixels buffer.
 *  \param num
 *      number of pixel to draw (lenght of pixels buffer).
 *
 * Pixels are rendereded with X doubled resolution (byte operation) for performance reason.<br>
 * So BMP_setPixel(0,0,..) will actually set same pixel as BMP_setPixel(1,0,..).
 */
void BMP_setPixels(const Pixel *pixels, u16 num);

/**
 *  \brief
 *      Clip the specified line so it fit in bitmap screen (0, 0, BMP_WIDTH, BMP_HEIGHT)
 *
 *  \param l
 *      Line to clip.
 *  \return
 *      false (0) is the line is entirely outside bitmap screen.<br>
 *      true if at least one pixel is on screen.
 */
u8   BMP_clipLine(Line *l);
/**
 *  \brief
 *      Draw a line.
 *
 *  \param l
 *      Line to draw.
 *
 * Lines are rendereded with X doubled resolution (byte operation) for performance reason.<br>
 * So BMP_drawLine(0,0,0,159) will actually draw same line as BMP_drawLine(1,0,1,159).
 */
void BMP_drawLine(Line *l);
/**
 *  \brief
 *      Determine if the specified polygon is culled.<br>
 *      The polygon points should be defined in clockwise order.<br>
 *      The method returns 1 if the polygon is back faced and should be eliminated for 3D rendering.
 *
 *  \param pts
 *      Polygon points buffer.
 *  \param num
 *      number of point (lenght of points buffer).
 *  \return 1 if polygon is culled (should not be draw) and 0 otherwise.<br>
 */
u16 BMP_isPolygonCulled(const Vect2D_s16 *pts, u16 num);
/**
 *  \brief
 *      Draw a polygon.<br>
 *      The polygon points should be defined in clockwise order.<br>
 *      Use the BMP_isPolygonCulled(..) method to test if polygon should be draw or not.
 *
 *  \param pts
 *      Polygon points buffer.
 *  \param num
 *      number of point (lenght of points buffer).
 *  \param col
 *      fill color.
 *  \return 0 if polygon was not drawn (outside screen or whatever).
 */
u16 BMP_drawPolygon(const Vect2D_s16 *pts, u16 num, u8 col);

/**
 *  \brief
 *      Draw the specified 4BPP bitmap data.
 *
 *  \param data
 *      4BPP image data buffer.
 *  \param x
 *      X coordinate (should be an even value for byte alignment).
 *  \param y
 *      y coordinate.
 *  \param w
 *      width (should be an even value for byte alignment).
 *  \param h
 *      height.
 *  \param pitch
 *      bitmap pitch (number of bytes per bitmap scanline).
 *
 * X coordinate as Width are aligned to even value for performance reason.<br>
 * So BMP_drawBitmapData(data,0,0,w,h,w) will produce same result as BMP_drawBitmapData(data,1,0,w,h,w)
 */
void BMP_drawBitmapData(const u8 *data, u16 x, u16 y, u16 w, u16 h, u32 pitch);
/**
 *  \brief
 *      Draw a Genesis Bitmap.<br>
 *
 *      A Genesis bitmap is automatically created from .bmp or .png file via the rescomp tool.<br>
 *      The resulting file contains bitmap size info and 16 colors palette.<br>
 *
 *  \param bitmap
 *      Genesis Bitmap.<br>
 *      The Bitmap is unpacked "on the fly" if needed (require some memory).
 *  \param x
 *      X coordinate (should be an even value).
 *  \param y
 *      y coordinate.
 *  \param loadpal
 *      Load the bitmap palette information when non zero.
 *  \return
 *      FALSE if there is not enough memory to unpack the specified Bitmap (only if compression was enabled).
 *
 * X coordinate is aligned to even value for performance reason.<br>
 * So BMP_drawBitmap(bitmap,0,0,TRUE) will produce same result as BMP_drawBitmap(bitmap,1,0,TRUE)
 */
u16 BMP_drawBitmap(const Bitmap *bitmap, u16 x, u16 y, u16 loadpal);
/**
 *  \brief
 *      Load and draw a Genesis Bitmap with specified dimension.<br>
 *
 *      A Genesis bitmap is automatically created from .bmp or .png file via the rescomp tool.<br>
 *      The resulting file contains bitmap size info and 16 colors palette.<br>
 *      This method will scale and draw the bitmap with the specified dimension.
 *
 *  \param bitmap
 *      Genesis bitmap.<br>
 *      The Bitmap is unpacked "on the fly" if needed (require some memory).
 *  \param x
 *      X coordinate (should be an even value).
 *  \param y
 *      y coordinate.
 *  \param w
 *      final width.
 *  \param h
 *      final height.
 *  \param loadpal
 *      Load the bitmap palette information when non zero.
 *  \return
 *      FALSE if there is not enough memory to unpack the specified Bitmap (only if compression was enabled).
 *
 * X coordinate as width are aligned to even values for performance reason.<br>
 * So BMP_drawBitmapScaled(bitmap,0,0,w,h,pal) will produce same result as BMP_drawBitmapScaled(bitmap,1,0,w,h,pal)
 */
u16 BMP_drawBitmapScaled(const Bitmap *bitmap, u16 x, u16 y, u16 w, u16 h, u16 loadpal);

/**
 *  \deprecated
 *      Use BMP_drawBitmapData(..) instead.
 */
void BMP_loadBitmapData(const u8 *data, u16 x, u16 y, u16 w, u16 h, u32 pitch);
/**
 *  \deprecated
 *      Use BMP_drawBitmap(..) instead.
 */
void BMP_loadBitmap(const Bitmap *bitmap, u16 x, u16 y, u16 loadpal);
/**
 *  \deprecated
 *      Use BMP_drawBitmapEx(..) instead.
 */
void BMP_loadAndScaleBitmap(const Bitmap *bitmap, u16 x, u16 y, u16 w, u16 h, u16 loadpal);

/**
 *  \deprecated
 *      Uses bitmap->palette instead.
 */
void BMP_getBitmapPalette(const Bitmap *bitmap, u16 *pal);

/**
 *  \brief
 *      Scale the specified source bitmap image to specified dimension.<br>
 *
 *      Take a source bitmap with its specified dimension and scale it in the<br>
 *      destination buffer with specified dimension.<br>
 *
 *  \param src_buf
 *      source bitmap buffer.
 *  \param src_wb
 *      source width in byte.
 *  \param src_h
 *      source height.
 *  \param src_pitch
 *      source pitch (number of bytes per scanline).
 *  \param dst_buf
 *      destination bitmap buffer.
 *  \param dst_wb
 *      destination width in byte.
 *  \param dst_h
 *      destination height.
 *  \param dst_pitch
 *      destination pitch (number of bytes per scanline).
 */
void BMP_scale(const u8 *src_buf, u16 src_wb, u16 src_h, u16 src_pitch, u8 *dst_buf, u16 dst_wb, u16 dst_h, u16 dst_pitch);


#endif // _BMP_H_

