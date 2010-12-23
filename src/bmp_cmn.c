#include "config.h"
#include "types.h"

#include "bmp_cmn.h"
#include "bmp_intr.h"

#include "vdp.h"
#include "vdp_tile.h"
#include "vdp_bg.h"

#include "tools.h"

#ifdef ENABLE_BMP



u8 *bmp_buffer_read;
u8 *bmp_buffer_write;

u16 bmp_flags;
u16 bmp_state;


u16 BMP_getFlags()
{
    return bmp_flags;
}


u16 BMP_hasBlitInProgress()
{
    if (bmp_state & BMP_STAT_BLITTING) return 1;
    else return 0;
}

void BMP_waitBlitComplete()
{
    vu16 *pw;

    // prevent the compiler to cache it
    pw = &bmp_state;

    while (*pw & BMP_STAT_BLITTING);
}

u16 BMP_hasFlipWaiting()
{
    if (bmp_state & BMP_STAT_FLIPWAITING) return 1;
    else return 0;
}

void BMP_waitAsyncFlipComplete()
{
    vu16 *pw;

    // prevent the compiler to cache it
    pw = &bmp_state;

    while (*pw & BMP_STAT_FLIPWAITING);
}


void BMP_showFPS()
{
    char str[16];
    u16 y;

    if HAS_FLAG(BMP_ENABLE_EXTENDEDBLANK) y = 4;
    else y = 1;
    if READ_IS_FB1 y += BMP_PLANHEIGHT / 2;

    // display FPS
    intToStr(getFPS(), str, 1);
    VDP_clearTextBG(BMP_PLAN, 1, y, 3);
    VDP_drawTextBG(BMP_PLAN, str, 0x8000, 1, y);
}


u16 BMP_doBlankProcess()
{
    // use extended blank ?
    if HAS_FLAG(BMP_ENABLE_EXTENDEDBLANK)
    {
        const u16 vcnt = GET_VCOUNTER;

        // first part of screen
        if (vcnt < 160)
        {
            // enable VDP
            VDP_setEnable(1);
            // prepare hint to disable VDP
            {
                const u16 scrh = VDP_getScreenHeight();
                VDP_setHIntCounter(scrh - (VDP_getHIntCounter() + vcnt + ((scrh - BMP_HEIGHT) >> 1) + 3));
            }

            // nothing more to do here
            return 1;
        }
        else
        {
            // disable VDP
            VDP_setEnable(0);
            // prepare hint to re-enable VDP
            VDP_setHIntCounter(((VDP_getScreenHeight() - BMP_HEIGHT) >> 1) - 1);
        }
    }

    // flip requested or not complete ? --> start / continu flip
    if (bmp_state & BMP_STAT_FLIPWAITING) _bmp_doFlip();

    return 1;
}


// graphic drawing/helping functions
////////////////////////////

u8 BMP_clipLine(Line *l)
{
    s16 x1, y1, x2, y2;

    // load crd
    x1 = l->pt1.x;
    y1 = l->pt1.y;
    x2 = l->pt2.x;
    y2 = l->pt2.y;

    // trivial accept ?
    if (((u16) x1 < BMP_WIDTH) && ((u16) x2 < BMP_WIDTH) &&
        ((u16) y1 < BMP_HEIGHT) && ((u16) y2 < BMP_HEIGHT)) return 1;

    // trivial reject ?
    if (((x1 < 0) && (x2 < 0)) ||
        ((x1 >= BMP_WIDTH) && (x2 >= BMP_WIDTH)) ||
        ((y1 < 0) && (y2 < 0)) ||
        ((y1 >= BMP_HEIGHT) && (y2 >= BMP_HEIGHT))) return 0;

    // iterate until trivial accept or reject
    while(1)
    {
        // calcul deltas
        const s16 dx = x2 - x1;
        const s16 dy = y2 - y1;

        // limit x
        if (x1 < 0)
        {
            y1 += ((0 - x1) * dy) / dx;
            x1 = 0;
        }
        else if (x1 >= BMP_WIDTH)
        {
            y1 += (((BMP_WIDTH - 1) - x1) * dy) / dx;
            x1 = BMP_WIDTH - 1;
        }
        if (x2 < 0)
        {
            y2 += ((0 - x2) * dy) / dx;
            x2 = 0;
        }
        else if (x2 >= BMP_WIDTH)
        {
            y2 += (((BMP_WIDTH - 1) - x2) * dy) / dx;
            x2 = BMP_WIDTH - 1;
        }
        // limit y
        if (y1 < 0)
        {
            x1 += ((0 - y1) * dx) / dy;
            y1 = 0;
        }
        else if (y1 >= BMP_HEIGHT)
        {
            x1 += (((BMP_HEIGHT - 1) - y1) * dx) / dy;
            y1 = BMP_HEIGHT - 1;
        }
        if (y2 < 0)
        {
            x2 += ((0 - y2) * dx) / dy;
            y2 = 0;
        }
        else if (y2 >= BMP_HEIGHT)
        {
            x2 += (((BMP_HEIGHT - 1) - y2) * dx) / dy;
            y2 = BMP_HEIGHT - 1;
        }

        // trivial accept ?
        if (((u16) x1 < BMP_WIDTH) && ((u16) x2 < BMP_WIDTH) &&
            ((u16) y1 < BMP_HEIGHT) && ((u16) y2 < BMP_HEIGHT))
        {
            // save back new crd
            l->pt1.x = x1;
            l->pt1.y = y1;
            l->pt2.x = x2;
            l->pt2.y = y2;

            return 1;
        }

        // trivial reject ?
        if (((x1 < 0) && (x2 < 0)) ||
            ((x1 >= BMP_WIDTH) && (x2 >= BMP_WIDTH)) ||
            ((y1 < 0) && (y2 < 0)) ||
            ((y1 >= BMP_HEIGHT) && (y2 >= BMP_HEIGHT)))
        {
            // rejected, no need to save back calculated coordinates
            return 0;
        }
    }
}


#else // ENABLE_BMP

u16 BMP_doBlankProcess()
{
    return 0;
}

#endif // ENABLE_BMP
