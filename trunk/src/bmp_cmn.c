#include "config.h"
#include "types.h"

#include "bmp_cmn.h"
#include "bmp_intr.h"

#include "vdp.h"
#include "vdp_tile.h"
#include "vdp_pal.h"
#include "vdp_bg.h"

#include "memory.h"
#include "tools.h"
#include "string.h"
#include "tab_vram.h"


// don't want to share it
extern u16 textBasetile;

extern s16 *LeftPoly;
extern s16 *RightPoly;


u8 *bmp_buffer_0 = NULL;
u8 *bmp_buffer_1 = NULL;

u8 *bmp_buffer_read;
u8 *bmp_buffer_write;

// used for polygone drawing
s16 *LeftPoly = NULL;
s16 *RightPoly = NULL;

s16 minY;
s16 maxY;

// internals
u16 bmp_state;
u16 phase;

void (*doFlip)();
u16 (*doBlit)();


// forward
static u16 _bmp_getYOffset();


void _bmp_init()
{
    // release first if needed
    if (bmp_buffer_0) MEM_free(bmp_buffer_0);
    if (bmp_buffer_1) MEM_free(bmp_buffer_1);
    if (LeftPoly) MEM_free(LeftPoly);
    if (RightPoly) MEM_free(RightPoly);

    // tile map allocation
    bmp_buffer_0 = MEM_alloc(BMP_WIDTH * BMP_HEIGHT * sizeof(u8));
    bmp_buffer_1 = MEM_alloc(BMP_WIDTH * BMP_HEIGHT * sizeof(u8));
    // polygon edge buffer allocation
    LeftPoly = MEM_alloc(BMP_HEIGHT * sizeof(s16));
    RightPoly = MEM_alloc(BMP_HEIGHT * sizeof(s16));

    // need 64x64 cells sized plan
    VDP_setPlanSize(BMP_PLANWIDTH, BMP_PLANHEIGHT);

    // clear plan (complete tilemap)
    VDP_clearPlan(BMP_PLAN, 1);
    VDP_waitDMACompletion();

    bmp_state = 0;

    // default
    bmp_buffer_read = bmp_buffer_0;
    bmp_buffer_write = bmp_buffer_1;
}

void _bmp_end()
{
    // release memory
    if (bmp_buffer_0)
    {
        MEM_free(bmp_buffer_0);
        bmp_buffer_0 = NULL;
    }
    if (bmp_buffer_1)
    {
        MEM_free(bmp_buffer_1);
        bmp_buffer_1 = NULL;
    }
    if (LeftPoly)
    {
        MEM_free(LeftPoly);
        LeftPoly = NULL;
    }
    if (RightPoly)
    {
        MEM_free(RightPoly);
        RightPoly = NULL;
    }
}


u16 BMP_hasFlipRequestPending()
{
    if (bmp_state & BMP_STAT_FLIPWAITING) return 1;

    return 0;
}

void BMP_waitWhileFlipRequestPending()
{
    vu16* pw = &bmp_state;

    while (*pw & BMP_STAT_FLIPWAITING);
}

u16 BMP_hasFlipInProgess()
{
    if (bmp_state & BMP_STAT_FLIPPING) return 1;

    return 0;
}

void BMP_waitFlipComplete()
{
    vu16* pw = &bmp_state;

    while (*pw & BMP_STAT_FLIPPING);
}


void BMP_drawText(const char *str, u16 x, u16 y)
{
    const u16 adjy = y + _bmp_getYOffset();

    VDP_drawTextBG(BMP_PLAN, str, textBasetile, x, adjy);
}

void BMP_clearText(u16 x, u16 y, u16 w)
{
    const u16 adjy = y + _bmp_getYOffset();

    VDP_clearTextBG(BMP_PLAN, x, adjy, w);
}

void BMP_clearTextLine(u16 y)
{
    const u16 adjy = y + _bmp_getYOffset();

    VDP_clearTextLineBG(BMP_PLAN, adjy);
}


void BMP_showFPS(u16 float_display)
{
    char str[16];
    const u16 y = _bmp_getYOffset() + 1;

    if (float_display)
    {
        fix32ToStr(getFPS_f(), str, 1);
        VDP_clearTextBG(BMP_PLAN, 2, y, 5);
    }
    else
    {
        uintToStr(getFPS(), str, 1);
        VDP_clearTextBG(BMP_PLAN, 2, y, 2);
    }

    // display FPS
    VDP_drawTextBG(BMP_PLAN, str, 0x8000, 1, y);
}


u16 BMP_doVBlankProcess()
{
    // reset phase
    phase = 0;

    return 1;
}

u16 BMP_doHBlankProcess()
{
    // vborder low
    if (phase == 0)
    {
        const u16 vcnt = GET_VCOUNTER;
        const u16 scrh = VDP_getScreenHeight();
        const u16 vborder = (scrh - BMP_HEIGHT) >> 1;

        // enable VDP
        VDP_setEnable(1);
        // prepare hint to disable VDP and doing blit process
        VDP_setHIntCounter((scrh - vborder) - (VDP_getHIntCounter() + vcnt + 3));
        // update phase
        phase = 1;
    }
    // in active screen
    else if (phase == 1)
    {
        phase = 2;
    }
    // vborder high
    else if (phase == 2)
    {
        // disable VDP
        VDP_setEnable(0);
        // prepare hint to re enable VDP
        VDP_setHIntCounter(((VDP_getScreenHeight() - BMP_HEIGHT) >> 1) - 1);
        // update phase
        phase = 3;
        // flip requested or not complete ? --> start / continu flip
        if (bmp_state & BMP_STAT_FLIPPING) _bmp_doFlip();
    }

    return 1;
}


void _bmp_doFlip()
{
    // wait for DMA completion if used otherwise VDP writes can be corrupted
    VDP_waitDMACompletion();

    // copy tile buffer to VRAM
    if ((*doBlit)())
    {
        u16 vscr;

        // switch displayed buffer
        if (READ_IS_FB0) vscr = ((BMP_PLANHEIGHT * BMP_YPIXPERTILE) * 0) / 2;
        else vscr = ((BMP_PLANHEIGHT * BMP_YPIXPERTILE) * 1) / 2;

        VDP_setVerticalScroll(BMP_PLAN, vscr);

        // get bitmap state
        u16 state = bmp_state;

        // flip pending ?
        if (state & BMP_STAT_FLIPWAITING)
        {
            // process flip
            (*doFlip)();
            // clear pending flag
            state &= ~BMP_STAT_FLIPWAITING;
        }
        else
            // flip done
            state &= ~BMP_STAT_FLIPPING;

        // save back bitmap state
        bmp_state = state;
    }
}


static u16 _bmp_getYOffset()
{
    u16 res;

    res = 4;
    if (READ_IS_FB1) res += BMP_PLANHEIGHT / 2;

    return res;
}


// graphic drawing/helping functions
////////////////////////////

// replaced by ASM function (see bmp_cmn_a.s file)
u8 BMP_clipLine_old(Line *l)
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
            y1 -= (x1 * dy) / dx;
            x1 = 0;
        }
        else if (x1 >= BMP_WIDTH)
        {
            y1 += (((BMP_WIDTH - 1) - x1) * dy) / dx;
            x1 = BMP_WIDTH - 1;
        }

        if (x2 < 0)
        {
            y2 -= (x2 * dy) / dx;
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
            x1 -= (y1 * dx) / dy;
            y1 = 0;
        }
        else if (y1 >= BMP_HEIGHT)
        {
            x1 += (((BMP_HEIGHT - 1) - y1) * dx) / dy;
            y1 = BMP_HEIGHT - 1;
        }

        if (y2 < 0)
        {
            x2 -= (y2 * dx) / dy;
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

// replaced by ASM function (see bmp_cmn_a.s file)
void calculatePolyEdge_old(const Vect2D_s16 *pt1, const Vect2D_s16 *pt2, u8 clockwise)
{
    s16 dx, dy;
    s16 x1 = pt1->x;
    s16 y1 = pt1->y;
    s16 x2 = pt2->x;
    s16 y2 = pt2->y;

    // outside screen ? --> exit
    if (((x1 < 0) && (x2 < 0)) ||
        ((x1 >= BMP_WIDTH) && (x2 >= BMP_WIDTH)) ||
        ((y1 < 0) && (y2 < 0)) ||
        ((y1 >= BMP_HEIGHT) && (y2 >= BMP_HEIGHT))) return;

    dy = y2 - y1;
    // nothing to do
    if (dy == 0) return;

    dx = x2 - x1;

    // clip on y window
    if (y1 < 0)
    {
        x1 -= (y1 * dx) / dy;
        y1 = 0;
    }
    else if (y1 >= BMP_HEIGHT)
    {
        x1 += (((BMP_HEIGHT - 1) - y1) * dx) / dy;
        y1 = BMP_HEIGHT - 1;
    }
    if (y2 < 0)
    {
        x2 -= (y2 * dx) / dy;
        y2 = 0;
    }
    else if (y2 >= BMP_HEIGHT)
    {
        x2 += (((BMP_HEIGHT - 1) - y2) * dx) / dy;
        y2 = BMP_HEIGHT - 1;
    }

    // outside screen ? --> exit
    if (((x1 < 0) && (x2 < 0)) ||
        ((x1 >= BMP_WIDTH) && (x2 >= BMP_WIDTH)) ||
        ((y1 < 0) && (y2 < 0)) ||
        ((y1 >= BMP_HEIGHT) && (y2 >= BMP_HEIGHT))) return;

    // nothing to do
    if (y2 == y1) return;

    s16 *src;
    fix16 step;
    fix16 x;
    s16 len8;
    s16 len;

    if ((y2 > y1) ^ (clockwise))
    {
        // left side
        dx = x2 - x1;
        len = y2 - y1;
        if (y1 < minY) minY = y1;
        if (y2 > maxY) maxY = y2;
        src = &LeftPoly[y1];
        x = intToFix16(x1);
    }
    else
    {
        // right side
        dx = x1 - x2;
        len = y1 - y2;
        if (y2 < minY) minY = y2;
        if (y1 > maxY) maxY = y1;
        src = &RightPoly[y2];
        x = intToFix16(x2);
    }

    step = fix16Div(intToFix16(dx), intToFix16(len));
    x += step >> 1;
    len8 = len >> 3;

    while(len8--)
    {
        *src++ = fix16ToInt(x);
        x += step;
        *src++ = fix16ToInt(x);
        x += step;
        *src++ = fix16ToInt(x);
        x += step;
        *src++ = fix16ToInt(x);
        x += step;
        *src++ = fix16ToInt(x);
        x += step;
        *src++ = fix16ToInt(x);
        x += step;
        *src++ = fix16ToInt(x);
        x += step;
        *src++ = fix16ToInt(x);
        x += step;
    }

    len &= 7;
    while(len--)
    {
        *src++ = fix16ToInt(x);
        x += step;
    }
}
