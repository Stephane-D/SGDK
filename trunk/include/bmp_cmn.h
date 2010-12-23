#ifndef _BMP_CMN_H_
#define _BMP_CMN_H_

#include "maths.h"

#ifdef ENABLE_BMP


#define BMP_PLAN                    APLAN

#define BMP_PLANWIDTH_SFT           6
#define BMP_PLANHEIGHT_SFT          6
#define BMP_PLANWIDTH               (1 << BMP_PLANWIDTH_SFT)
#define BMP_PLANHEIGHT              (1 << BMP_PLANHEIGHT_SFT)

#define BMP_CELLWIDTH_SFT           5
#define BMP_CELLWIDTH               (1 << BMP_CELLWIDTH_SFT)
#define BMP_CELLHEIGHT              20
#define BMP_CELLWIDTHMASK           (BMP_CELLWIDTH - 1)

#define BMP_CELLXOFFSET             (((VDP_getScreenWidth() >> 3) - BMP_CELLWIDTH) / 2)
#define BMP_CELLYOFFSET             (((VDP_getScreenHeight() >> 3) - BMP_CELLHEIGHT) / 2)

#define BMP_XPIXPERTILE_SFT         2
#define BMP_YPIXPERTILE_SFT         3
#define BMP_XPIXPERTILE             (1 << BMP_XPIXPERTILE_SFT)
#define BMP_YPIXPERTILE             (1 << BMP_YPIXPERTILE_SFT)
#define BMP_XPIXPERTILEMASK         (BMP_XPIXPERTILE - 1)
#define BMP_YPIXPERTILEMASK         (BMP_YPIXPERTILE - 1)

#define BMP_WIDTH_SFT               (BMP_CELLWIDTH_SFT + BMP_XPIXPERTILE_SFT)
#define BMP_WIDTH                   (1 << BMP_WIDTH_SFT)
#define BMP_HEIGHT                  (BMP_CELLHEIGHT * BMP_YPIXPERTILE)
#define BMP_WIDTH_MASK              (BMP_WIDTH - 1)

#define BMP_PITCH_SFT               (BMP_CELLWIDTH_SFT + 2)
#define BMP_PITCH                   (1 << BMP_PITCH_SFT)
#define BMP_PITCH_MASK              (BMP_PITCH - 1)

#define BMP_GETTILE(x, y)           ((BMP_CELLWIDTH * (y)) + (x))
#define BMP_GETTILEFROMPIX(x, y)    BMP_GETTILE((x) >> BMP_XPIXPERTILE_SFT, (y) >> BMP_YPIXPERTILE_SFT)
#define BMP_GETPIXINTILE(x, y)      ((((y) & BMP_YPIXPERTILEMASK) << BMP_XPIXPERTILE_SFT) + ((x) & BMP_XPIXPERTILEMASK))


// BMP_ENABLE_ASYNCFLIP automatically enable the BMP_ENABLE_WAITVSYNC flag
// BMP_ENABLE_BLITONBLANK automatically enable the BMP_ENABLE_ASYNCFLIP flag
// BMP_ENABLE_EXTENDEDBLANK automatically enable the BMP_ENABLE_BLITONBLANK flag
#define BMP_ENABLE_WAITVSYNC     (1 << 0)
#define BMP_ENABLE_ASYNCFLIP     (1 << 1)
#define BMP_ENABLE_BLITONBLANK   (1 << 2)
#define BMP_ENABLE_EXTENDEDBLANK (1 << 3)
#define BMP_ENABLE_FPSDISPLAY    (1 << 4)
#define BMP_ENABLE_BFCULLING     (1 << 5)


typedef struct
{
    Vect2D_s16 pt;
    u8 col;
} Pixel;

typedef struct
{
    Vect2D_s16 pt1;
    Vect2D_s16 pt2;
    u8 col;
} Line;

typedef struct
{
    Vect2D_s16 pt1;
    Vect2D_s16 pt2;
    Vect2D_s16 pt3;
    u8 col;
} Triangle;



extern u8 *bmp_buffer_read;
extern u8 *bmp_buffer_write;

extern u16 bmp_flags;
extern u16 bmp_state;


u16  BMP_getFlags();

u16  BMP_hasBlitInProgress();
void BMP_waitBlitComplete();
u16  BMP_hasFlipWaiting();
void BMP_waitAsyncFlipComplete();

void BMP_showFPS();

u8   BMP_clipLine(Line *l);


#endif // ENABLE_BMP

#endif // _BMP_CMN_H_
