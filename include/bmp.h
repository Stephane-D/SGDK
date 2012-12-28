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

#include "maths.h"


/**
 *      \def BMP_PLAN
 *          Plan used to draw bitmap (plan A).
 */
#define BMP_PLAN                    APLAN

#define BMP_PLANWIDTH_SFT           6
#define BMP_PLANHEIGHT_SFT          6
/**
 *      \def BMP_PLANWIDTH
 *          Bitmap plan width (in tile) : 64
 */
#define BMP_PLANWIDTH               (1 << BMP_PLANWIDTH_SFT)
/**
 *      \def BMP_PLANHEIGHT
 *          Bitmap plan height (in tile) : 64
 */
#define BMP_PLANHEIGHT              (1 << BMP_PLANHEIGHT_SFT)

#define BMP_CELLWIDTH_SFT           5
/**
 *      \def BMP_CELLWIDTH
 *          Bitmap width (in tile) : 32
 */
#define BMP_CELLWIDTH               (1 << BMP_CELLWIDTH_SFT)
/**
 *      \def BMP_CELLHEIGHT
 *          Bitmap height (in tile) : 20
 */
#define BMP_CELLHEIGHT              20
#define BMP_CELLWIDTHMASK           (BMP_CELLWIDTH - 1)

#define BMP_CELLXOFFSET             (((VDP_getScreenWidth() >> 3) - BMP_CELLWIDTH) / 2)
#define BMP_CELLYOFFSET             (((VDP_getScreenHeight() >> 3) - BMP_CELLHEIGHT) / 2)

#define BMP_XPIXPERTILE_SFT         2
#define BMP_YPIXPERTILE_SFT         3
/**
 *      \def BMP_XPIXPERTILE
 *          Number of X pixel per tile : 4 pixels per tile (1 soft pixel = 2 hard pixels).
 */
#define BMP_XPIXPERTILE             (1 << BMP_XPIXPERTILE_SFT)
/**
 *      \def BMP_YPIXPERTILE
 *          Number of y pixel per tile : 8 pixels per tile.
 */
#define BMP_YPIXPERTILE             (1 << BMP_YPIXPERTILE_SFT)
#define BMP_XPIXPERTILEMASK         (BMP_XPIXPERTILE - 1)
#define BMP_YPIXPERTILEMASK         (BMP_YPIXPERTILE - 1)

#define BMP_WIDTH_SFT               (BMP_CELLWIDTH_SFT + BMP_XPIXPERTILE_SFT)
/**
 *      \def BMP_WIDTH
 *          Bitmap width (in pixel) : 128
 */
#define BMP_WIDTH                   (1 << BMP_WIDTH_SFT)
/**
 *      \def BMP_HEIGHT
 *          Bitmap height (in pixel) : 160
 */
#define BMP_HEIGHT                  (BMP_CELLHEIGHT * BMP_YPIXPERTILE)
#define BMP_WIDTH_MASK              (BMP_WIDTH - 1)

#define BMP_PITCH_SFT               (BMP_CELLWIDTH_SFT + 2)
/**
 *      \def BMP_PITCH
 *          Bitmap scanline pitch (number of bytes per scanline) : 128
 */
#define BMP_PITCH                   (1 << BMP_PITCH_SFT)
#define BMP_PITCH_MASK              (BMP_PITCH - 1)

/**
 *      \def BMP_GET_TILE
 *          Get bitmap tile index from specified tile coordinates.
 */
#define BMP_GET_TILE(x, y)           ((BMP_CELLWIDTH * (y)) + (x))
/**
 *      \def BMP_GET_TILEFROMPIX
 *          Get bitmap tile index from specified pixel coordinates.
 */
#define BMP_GET_TILEFROMPIX(x, y)    BMP_GETTILE((x) >> BMP_XPIXPERTILE_SFT, (y) >> BMP_YPIXPERTILE_SFT)
/**
 *      \def BMP_GET_PIXINTILE
 *          Get pixel offset in tile from specified pixel coordinates (0 <= x <=3 and 0 <= y <= 7).
 */
#define BMP_GET_PIXINTILE(x, y)      ((((y) & BMP_YPIXPERTILEMASK) << BMP_XPIXPERTILE_SFT) + ((x) & BMP_XPIXPERTILEMASK))

/**
 *      \def BMP_GENBMP16_WIDTH
 *          Get width of genesis bitmap 16 object.
 */
#define BMP_GENBMP16_WIDTH(genbmp16)    ((genbmp16)[0])
/**
 *      \def BMP_GENBMP16_HEIGHT
 *          Get height of genesis bitmap 16 object.
 */
#define BMP_GENBMP16_HEIGHT(genbmp16)   ((genbmp16)[1])
/**
 *      \def BMP_GENBMP16_PALETTE
 *          Return pointer to palette of genesis bitmap 16 object.
 */
#define BMP_GENBMP16_PALETTE(genbmp16)  (&((genbmp16)[2]))
/**
 *      \def BMP_GENBMP16_IMAGE
 *          Return pointer to image data of genesis bitmap 16 object.
 */
#define BMP_GENBMP16_IMAGE(genbmp16)    (&((genbmp16)[18]))


#define BMP_ENABLE_FPSDISPLAY    (1 << 0)


/**
 *      \struct Bitmap
 *          Genesis 4bpp Bitmap structure definition.<br>
 *      \param w
 *          Width in pixel.
 *      \param h
 *          Height in pixel.
 *      \param palette
 *          Palette.
 *      \param image
 *          Image data, array size = (w * h / 2).
 */
typedef struct
{
    u16 w;
    u16 h;
    u16 palette[16];
    u8 image[0];
} Bitmap;

/**
 *      \struct Pixel
 *          Pixel definition.<br>
 *      \param pt
 *          Coordinates.
 *      \param col
 *          Color.
 */
typedef struct
{
    Vect2D_s16 pt;
    u8 col;
} Pixel;

/**
 *      \struct Line
 *          Line definition.<br>
 *      \param pt1
 *          Start point.
 *      \param pt2
 *          End point.
 *      \param col
 *          Color.
 */
typedef struct
{
    Vect2D_s16 pt1;
    Vect2D_s16 pt2;
    u8 col;
} Line;

/**
 *      \struct Triangle
 *          Triangle definition.<br>
 *      \param pt1
 *          Start point.
 *      \param pt2
 *          Second point.
 *      \param pt3
 *          End point.
 *      \param col
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
 *      Return true (!= 0) if a flip request is pending.
 */
u16  BMP_hasFlipRequestPending();
/**
 * \brief
 *      Wait until no more flip request is pending.
 */
void BMP_waitWhileFlipRequestPending();
/**
 * \brief
 *      Return true (!= 0) if a flip operation is in progress.
 */
u16  BMP_hasFlipInProgess();
/**
 * \brief
 *      Wait until the asynchronous flip operation is completed.<br>
 *      Blit operation is the bitmap buffer transfer to VRAM.<br>
 */
void BMP_waitFlipComplete();

/**
 * \brief
 *      Draw text in bitmap mode.
 *
 * \param str
 *      String to display.
 * \param x
 *      X coordinate (in tile).
 * \param y
 *      y coordinate (in tile).
 */
void BMP_drawText(const char *str, u16 x, u16 y);
/**
 * \brief
 *      Clear text in bitmap mode.
 *
 * \param x
 *      X coordinate (in tile).
 * \param y
 *      y coordinate (in tile).
 * \param w
 *      Number of characters to clear.
 */
void BMP_clearText(u16 x, u16 y, u16 w);
/**
 * \brief
 *      Clear a line of text in bitmap mode.
 *
 * \param y
 *      y coordinate (in tile).
 */
void BMP_clearTextLine(u16 y);

/**
 * \brief
 *      Show the frame rate in bitmap mode.
 *
 * \param float_display
 *      If this value is true (!= 0) the frame rate is displayed as float (else it's integer).
 */
void BMP_showFPS(u16 float_display);


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
 *      Clip the specified line so it fit in bitmap screen (0, 0, BMP_WIDTH, BMP_HEIGHT)
 *
 * \param l
 *      Line to clip.
 * \return
 *      false (0) is the line is entirely outside bitmap screen.<br>
 *      true if at least one pixel is on screen.
 */
u8   BMP_clipLine(Line *l);
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

