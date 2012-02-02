/**
 * \file bmp_cmn.h
 * \brief Software bitmap engine (common methods)
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides common methods for bitmap engines.
 */

#ifndef _BMP_CMN_H_
#define _BMP_CMN_H_

#include "maths.h"


/**
 *      \def BMP_PLAN
 *          Plan used to draw bitmap.
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


/**
 *      \def BMP_ENABLE_WAITVSYNC
 *          wait VBlank before doing flip operation (see BMP_flip()).
 */
#define BMP_ENABLE_WAITVSYNC     (1 << 0)
/**
 *      \def BMP_ENABLE_ASYNCFLIP
 *          Asynch flip operation.<br>
 *          When this flag is enabled BMP_flip() will return immediatly even if flip wait for VBlank.<br>
 *          Note that this flag automatically enable BMP_ENABLE_WAITVSYNC.
 */
#define BMP_ENABLE_ASYNCFLIP     (1 << 1)
/**
 *      \def BMP_ENABLE_BLITONBLANK
 *          Process blit only during VDP blanking.<br>
 *          VRAM access is faster during blanking so this permit to optimize blit processing on best period<br>
 *          and keep the rest of time available for others processing.<br>
 *          By default on NTSC system the blanking period is very short so it takes approximately 15 frames<br>
 *          to blit the entire bitmap screen (blit is done in software).<br>
 *          Note that this flag automatically enable BMP_ENABLE_ASYNCFLIP.
 */
#define BMP_ENABLE_BLITONBLANK   (1 << 2)
/**
 *      \def BMP_ENABLE_EXTENDEDBLANK
 *          Extend blanking period to fit bitmap height resolution (160 pixels).<br>
 *          This permit to improve blit process time (reduce 15 frames to approximately 4 frames on NTSC system).<br>
 *          Note that this flag automatically enable BMP_ENABLE_BLITONBLANK.
 */
#define BMP_ENABLE_EXTENDEDBLANK (1 << 3)
/**
 *      \def BMP_ENABLE_FPSDISPLAY
 *          Display frame rate (number of bitmap flip per second).
 */
#define BMP_ENABLE_FPSDISPLAY    (1 << 4)
/**
 *      \def BMP_ENABLE_BFCULLING
 *          Enabled culling (used only for polygon drawing, see BMP_setFlags()).
 */
#define BMP_ENABLE_BFCULLING     (1 << 5)


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

extern u16 bmp_flags;
extern u16 bmp_state;


/**
 * \brief
 *      Get bitmap engine flags.
 */
u16  BMP_getFlags();

/**
 * \brief
 *      Return true (!= 0) if a blit operation is in progress.
 */
u16  BMP_hasBlitInProgress();
/**
 * \brief
 *      Wait for blit to complete.<br>
 *      Return immediately if no blit is currently in progress.
 */
void BMP_waitBlitComplete();
/**
 * \brief
 *      Return true (!= 0) if a flip operation is pending.
 */
u16  BMP_hasFlipWaiting();
/**
 * \brief
 *      Wait until the asynchronous flip operation is completed.
 */
void BMP_waitAsyncFlipComplete();

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


u8   BMP_clipLine_old(Line *l);
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


#endif // _BMP_CMN_H_
