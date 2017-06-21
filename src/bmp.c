#include "config.h"
#include "types.h"

#include "bmp.h"

#include "sys.h"
#include "memory.h"
#include "maths3D.h"

#include "vdp.h"
#include "vdp_tile.h"
#include "vdp_pal.h"
#include "vdp_bg.h"

#include "dma.h"

#include "memory.h"
#include "tools.h"
#include "string.h"


#define BMP_FLAG_DOUBLEBUFFER   (1 << 0)
#define BMP_FLAG_COPYBUFFER     (1 << 1)

#define BMP_STAT_FLIPPING       (1 << 0)
#define BMP_STAT_BLITTING       (1 << 1)
#define BMP_STAT_FLIPWAITING    (1 << 2)

#define HAS_DOUBLEBUFFER        (flag & BMP_FLAG_DOUBLEBUFFER)
#define HAS_BUFFERCOPY          (flag & BMP_FLAG_COPYBUFFER)

#define READ_IS_FB0             (bmp_buffer_read == bmp_buffer_0)
#define READ_IS_FB1             (bmp_buffer_read == bmp_buffer_1)
#define WRITE_IS_FB0            (bmp_buffer_write == bmp_buffer_0)
#define WRITE_IS_FB1            (bmp_buffer_write == bmp_buffer_1)

#define GET_YOFFSET             ((HAS_DOUBLEBUFFER && READ_IS_FB1)?((BMP_PLANHEIGHT / 2) + 4):4)

#define BMP_PLAN_ADR            (*bmp_plan_adr)

#define BMP_FB0TILEMAP_BASE     BMP_PLAN_ADR
#define BMP_FB1TILEMAP_BASE     (BMP_PLAN_ADR + ((BMP_PLANWIDTH * (BMP_PLANHEIGHT / 2)) * 2))
#define BMP_FBTILEMAP_OFFSET    (((BMP_PLANWIDTH * BMP_CELLYOFFSET) + BMP_CELLXOFFSET) * 2)


#define NTSC_TILES_BW           7
#define PAL_TILES_BW            10


// we don't want to share them
extern vu32 VIntProcess;
extern vu32 HIntProcess;
extern u16 text_basetile;

u8 *bmp_buffer_read;
u8 *bmp_buffer_write;

static u8 *bmp_buffer_0;
static u8 *bmp_buffer_1;

VDPPlan bmp_plan;
u16 *bmp_plan_adr;

// internals
static u16 flag;
static u16 pal;
static u16 prio;
static u16 state;
static s16 phase;


// ASM methods
extern void clearBitmapBuffer(u8 *bmp_buffer);
extern void copyBitmapBuffer(u8 *src, u8 *dst);

static void doFlip();
static void flipBuffer();
static void initTilemap(u16 num);
static void clearVRAMBuffer(u16 num);
static u16 doBlit();
//static void drawLine(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col);


void BMP_init(u16 double_buffer, VDPPlan plan, u16 palette, u16 priority)
{
    flag = (double_buffer) ? BMP_FLAG_DOUBLEBUFFER : 0;
    bmp_plan = plan;
    pal = palette & 3;
    prio = priority & 1;

    switch(plan.value)
    {
        default:
        case CONST_PLAN_B:
            bmp_plan_adr = &bplan_addr;
            break;

        case CONST_PLAN_A:
            bmp_plan_adr = &aplan_addr;
            break;

        case CONST_PLAN_WINDOW:
            bmp_plan_adr = &window_addr;
            break;
    }

    bmp_buffer_0 = NULL;
    bmp_buffer_1 = NULL;

    BMP_reset();
}

void BMP_end()
{
    // cancel interrupt processing
    HIntProcess &= ~PROCESS_BITMAP_TASK;
    VIntProcess &= ~PROCESS_BITMAP_TASK;

    // disable H-Int
    VDP_setHInterrupt(0);
    // re enabled VDP if it was disabled because of extended blank
    VDP_setEnable(1);
    // reset hint counter
    VDP_setHIntCounter(255);

    // reset back vertical scroll to 0
    VDP_setVerticalScroll(bmp_plan, 0);

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
}

void BMP_reset()
{
    // cancel bitmap interrupt processing
    HIntProcess &= ~PROCESS_BITMAP_TASK;
    VIntProcess &= ~PROCESS_BITMAP_TASK;

    // disable H-Int
    VDP_setHInterrupt(0);
    // re enabled VDP if it was disabled because of extended blank
    VDP_setEnable(1);
    // reset hint counter
    VDP_setHIntCounter(255);

    // allocate bitmap buffer if needed
    if (!bmp_buffer_0)
        bmp_buffer_0 = MEM_alloc(BMP_PITCH * BMP_HEIGHT * sizeof(u8));
    if (!bmp_buffer_1)
        bmp_buffer_1 = MEM_alloc(BMP_PITCH * BMP_HEIGHT * sizeof(u8));

    // need 64x64 cells sized plan
    VDP_setPlanSize(64, 64);

    // clear plan (complete tilemap)
    VDP_clearPlan(bmp_plan, TRUE);
    VDP_waitDMACompletion();

    // reset state and phase
    state = 0;
    phase = -1;

    // default
    bmp_buffer_read = bmp_buffer_0;
    bmp_buffer_write = bmp_buffer_1;

    // prepare tilemap
    initTilemap(0);
    clearVRAMBuffer(0);
    if (HAS_DOUBLEBUFFER)
    {
        initTilemap(1);
        clearVRAMBuffer(1);
    }

    // clear both buffer in memory
    clearBitmapBuffer(bmp_buffer_0);
    clearBitmapBuffer(bmp_buffer_1);

    // set back vertical scroll to 0
    VDP_setVerticalScroll(bmp_plan, 0);

    // prepare hint for extended blank on next frame
    VDP_setHIntCounter(((screenHeight - BMP_HEIGHT) >> 1) - 1);
    // enabled bitmap interrupt processing
    HIntProcess |= PROCESS_BITMAP_TASK;
    VIntProcess |= PROCESS_BITMAP_TASK;
    VDP_setHInterrupt(1);

//    // first init, clear and flip
//    BMP_clear();
//    BMP_flip(FALSE);
//    // second init, clear and flip for correct init (double buffer)
//    BMP_clear();
//    BMP_flip(FALSE);
}

void BMP_setBufferCopy(u16 value)
{
    if (value) flag |= BMP_FLAG_COPYBUFFER;
    else flag &= ~BMP_FLAG_COPYBUFFER;
}


u16 BMP_hasFlipRequestPending()
{
    if (state & BMP_STAT_FLIPWAITING) return 1;

    return 0;
}

void BMP_waitWhileFlipRequestPending()
{
    vu16* pw = &state;

    while (*pw & BMP_STAT_FLIPWAITING);
}

u16 BMP_hasFlipInProgess()
{
    if (state & BMP_STAT_FLIPPING) return 1;

    return 0;
}

void BMP_waitFlipComplete()
{
    vu16* pw = &state;

    while (*pw & BMP_STAT_FLIPPING);
}


u16 BMP_flip(u16 async)
{
    // wait until pending flip is processed
    BMP_waitWhileFlipRequestPending();

    // currently flipping ?
    if (state & BMP_STAT_FLIPPING)
    {
        // set a pending flip
        state |= BMP_STAT_FLIPWAITING;

        // wait completion
        if (!async) BMP_waitFlipComplete();

        return 1;
    }

    // flip bitmap buffer
    flipBuffer();
    // flip started (will be processed in blank period --> BMP_doBlankProcess)
    state |= BMP_STAT_FLIPPING;

    // wait completion
    if (!async) BMP_waitFlipComplete();

    return 0;
}


void BMP_drawText(const char *str, u16 x, u16 y)
{
    VDP_drawTextBG(bmp_plan, str, x, y + GET_YOFFSET);
}

void BMP_clearText(u16 x, u16 y, u16 w)
{
    VDP_clearTextBG(bmp_plan, x, y + GET_YOFFSET, w);
}

void BMP_clearTextLine(u16 y)
{
    VDP_clearTextLineBG(bmp_plan, y + GET_YOFFSET);
}


void BMP_showFPS(u16 float_display)
{
    char str[16];
    const u16 y = GET_YOFFSET + 1;

    if (float_display)
    {
        fix32ToStr(getFPS_f(), str, 1);
        VDP_clearTextBG(bmp_plan, 2, y, 5);
    }
    else
    {
        uintToStr(getFPS(), str, 1);
        VDP_clearTextBG(bmp_plan, 2, y, 2);
    }

    // display FPS
    VDP_drawTextBG(bmp_plan, str, 1, y);
}


// graphic drawing functions
////////////////////////////

void BMP_clear()
{
    clearBitmapBuffer(bmp_buffer_write);
}


u8* BMP_getWritePointer(u16 x, u16 y)
{
    const u16 off = (y * BMP_PITCH) + (x >> 1);

    // return write address
    return &bmp_buffer_write[off];
}

u8* BMP_getReadPointer(u16 x, u16 y)
{
    const u16 off = (y * BMP_PITCH) + (x >> 1);

    // return read address
    return &bmp_buffer_read[off];
}


u8 BMP_getPixel(u16 x, u16 y)
{
    // pixel in screen ?
    if ((x < BMP_WIDTH) && (y < BMP_HEIGHT))
    {
        const u16 off = (y * BMP_PITCH) + (x >> 1);

        // read pixel
        return bmp_buffer_write[off];
    }

    return 0;
}

void BMP_setPixel(u16 x, u16 y, u8 col)
{
    // pixel in screen ?
    if ((x < BMP_WIDTH) && (y < BMP_HEIGHT))
    {
        const u16 off = (y * BMP_PITCH) + (x >> 1);

        // write pixel
        bmp_buffer_write[off] = col;
    }
}

void BMP_setPixels_V2D(const Vect2D_u16 *crd, u8 col, u16 num)
{
    const u8 c = col;
    const Vect2D_u16 *v;
    u16 i;

    v = crd;
    i = num;

    while (i--)
    {
        const u16 x = v->x;
        const u16 y = v->y;

        // pixel inside screen ?
        if ((x < BMP_WIDTH) && (y < BMP_HEIGHT))
        {
            const u16 off = (y * BMP_PITCH) + (x >> 1);

            // write pixel
            bmp_buffer_write[off] = c;
        }

        // next pixel
        v++;
    }
}

void BMP_setPixels(const Pixel *pixels, u16 num)
{
    const Pixel *p;
    u16 i;

    p = pixels;
    i = num;

    while (i--)
    {
        const u16 x = p->pt.x;
        const u16 y = p->pt.y;

        // pixel inside screen ?
        if ((x < BMP_WIDTH) && (y < BMP_HEIGHT))
        {
            const u16 off = (y * BMP_PITCH) + (x >> 1);

            // write pixel
            bmp_buffer_write[off] = p->col;
        }

        // next pixel
        p++;
    }
}


//void BMP_drawLineFast(Line *l)
//{
//    s16 x0, y0;
//    s16 x1, y1;
//    s16 dx, dy;
//    s16 stepx, stepy;
//    s16 length, extras;
//    s16 incr1, incr2
//    s16 i, c, d;
//
//    x0 = l->x0;
//    y0 = l->y0;
//    x1 = l->x1;
//    y1 = l->y1;
//    dy = y1 - y0;
//    dx = x1 - x0;
//
//    if (dy < 0)
//    {
//        dy = -dy;
//        stepy = -1;
//    }
//    else stepy = 1;
//    if (dx < 0)
//    {
//        dx = -dx;
//        stepx = -1;
//    }
//    else stepx = 1;
//
//    setpix(x0, y0);
//    setpix(x1, y1);
//
//    if (dx > dy)
//    {
//        length = (dx - 1) >> 2;
//        extras = (dx - 1) & 3;
//        incr2 = (dy << 2) - (dx << 1);
//
//        if (incr2 < 0)
//        {
//            c = dy << 1;
//            incr1 = c << 1;
//
//            d = incr1 - dx;
//
//            for (i = 0; i < length; i++)
//            {
//                x0 += stepx;
//                x1 -= stepx;
//
//                if (d < 0)                // Pattern:
//                {
//                    setpix(x0, y0);       //
//                    x0 += stepx;          //
//                    setpix(x0, y0);       //  x o o
//                    setpix(x1, y1);       //
//                    x1 -= stepx;
//                    setpix(x1, y1);
//                    d += incr1;
//                }
//                else
//                {
//                    if (d < c)              // Pattern:
//                    {
//                        setpix(x0, y0);     //
//                        x0 += stepx;        //
//                        y0 += stepy;        //      o
//                        setpix(x0, y0);     //  x o
//                        setpix(x1, y1);     //
//                        x1 -= stepx;
//                        y1 -= stepy;
//                        setpix(x1, y1);
//                    }
//                    else
//                    {
//                        y0 += stepy;
//                        setpix(x0, y0);     // Pattern:
//                        x0 += stepx;        //
//                        setpix(x0, y0);     //    o o
//                        y1 -= stepy;        //  x
//                        setpix(x1, y1);
//                        x1 -= stepx;
//                        setpix(x1, y1);
//                    }
//
//                    d += incr2;
//                }
//            }
//
//            if (extras > 0)
//            {
//                if (d < 0)
//                {
//                    x0 += stepx;
//                    setpix(x0, y0);
//
//                    if (extras > 1)
//                    {
//                        x0 += stepx;
//                        setpix(x0, y0);
//                    }
//
//                    if (extras > 2)
//                    {
//                        x1 -= stepx;
//                        setpix(x1, y1);
//                    }
//                }
//                else if (d < c)
//                {
//                    x0 += stepx;
//                    setpix(x0, y0);
//
//                    if (extras > 1)
//                    {
//                        x0 += stepx;
//                        y0 += stepy;
//                        setpix(x0, y0);
//                    }
//
//                    if (extras > 2)
//                    {
//                        x1 -= stepx;
//                        setpix(x1, y1);
//                    }
//                }
//                else
//                {
//                    x0 += stepx;
//                    y0 += stepy;
//                    setpix(x0, y0);
//
//                    if (extras > 1)
//                    {
//                        x0 += stepx;
//                        setpix(x0, y0);
//                    }
//
//                    if (extras > 2)
//                    {
//                        x1 -= stepx;
//                        y1 -= stepy;
//                        setpix(x1, y1);
//                    }
//                }
//            }
//        }
//        else
//        {
//            c = (dy - dx) << 1;
//            incr1 = c << 1;
//            d =  incr1 + dx;
//
//            for (i = 0; i < length; i++)
//            {
//                x0 += stepx;
//                x1 -= stepx;
//
//                if (d > 0)
//                {
//                    y0 += stepy;
//                    setpix(x0, y0);
//                    x0 += stepx;            // Pattern:
//                    y0 += stepy;            //
//                    setpix(x0, y0);         //      o
//                    y1 -= stepy;            //    o
//                    setpix(x1, y1);         //  x
//                    x1 -= stepx;
//                    y1 -= stepy;
//                    setpix(x1, y1);
//                    d += incr1;
//                }
//                else
//                {
//                    if (d < c)
//                    {
//                        setpix(x0, y0);
//                        x0 += stepx;        // Pattern:
//                        y0 += stepy;        //
//                        setpix(x0, y0);     //      o
//                        setpix(x1, y1);     //  x o
//                        x1 -= stepx;
//                        y1 -= stepy;
//                        setpix(x1, y1);
//                    }
//                    else
//                    {
//                        y0 += stepy;
//                        setpix(x0, y0);     // Pattern:
//                        x0 += stepx;
//                        setpix(x0, y0);     //    o o
//                        y1 -= stepy;        //  x
//                        setpix(x1, y1);
//                        x1 -= stepx;
//                        setpix(x1, y1);
//                    }
//
//                    d += incr2;
//                }
//            }
//
//            if (extras > 0)
//            {
//                if (d > 0)
//                {
//                    x0 += stepx;
//                    y0 += stepy;
//                    setpix(x0, y0);
//
//                    if (extras > 1)
//                    {
//                        x0 += stepx;
//                        y0 += stepy;
//                        setpix(x0, y0);
//                    }
//
//                    if (extras > 2)
//                    {
//                        x1 -= stepx;
//                        y1 -= stepy;
//                        setpix(x1, y1);
//                    }
//                }
//                else if (d < c)
//                {
//                    x0 += stepx;
//                    setpix(x0, y0);
//
//                    if (extras > 1)
//                    {
//                        x0 += stepx;
//                        y0 += stepy;
//                        setpix(x0, y0);
//                    }
//
//                    if (extras > 2)
//                    {
//                        x1 -= stepx;
//                        setpix(x1, y1);
//                    }
//                }
//                else
//                {
//                    x0 += stepx;
//                    y0 += stepy;
//                    setpix(x0, y0);
//
//                    if (extras > 1)
//                    {
//                        x0 += stepx;
//                        setpix(x0, y0);
//                    }
//
//                    if (extras > 2)
//                    {
//                        if (d > c)
//                        {
//                            x1 -= stepx;
//                            y1 -= stepy;
//                            setpix(x1, y1);
//                        }
//                        else
//                        {
//                            x1 -= stepx;
//                            setpix(x1, y1);
//                        }
//                    }
//                }
//            }
//        }
//    }
//    else
//    {
//        length = (dy - 1) >> 2;
//        extras = (dy - 1) & 3;
//        incr2 = (dx << 2) - (dy << 1);
//
//        if (incr2 < 0)
//        {
//            c = dx << 1;
//            incr1 = c << 1;
//            d =  incr1 - dy;
//
//            for (i = 0; i < length; i++)
//            {
//                y0 += stepy;
//                y1 -= stepy;
//
//                if (d < 0)
//                {
//                    setpix(x0, y0);
//                    y0 += stepy;
//                    setpix(x0, y0);
//                    setpix(x1, y1);
//                    y1 -= stepy;
//                    setpix(x1, y1);
//                    d += incr1;
//                }
//                else
//                {
//                    if (d < c)
//                    {
//                        setpix(x0, y0);
//                        x0 += stepx;
//                        y0 += stepy;
//                        setpix(x0, y0);
//                        setpix(x1, y1);
//                        x1 -= stepx;
//                        y1 -= stepy;
//                        setpix(x1, y1);
//                    }
//                    else
//                    {
//                        x0 += stepx;
//                        setpix(x0, y0);
//                        y0 += stepy;
//                        setpix(x0,y0);
//                        x1 -= stepx;
//                        setpix(x1,y1);
//                        y1 -= stepy;
//                        setpix(x1,y1);
//                    }
//
//                    d += incr2;
//                }
//            }
//
//            if (extras > 0)
//            {
//                if (d < 0)
//                {
//                    y0 += stepy;
//                    setpix(x0, y0);
//
//                    if (extras > 1)
//                    {
//                        y0 += stepy;
//                        setpix(x0,y0);
//                    }
//
//                    if (extras > 2)
//                    {
//                        y1 -= stepy;
//                        setpix(x1,y1);
//                    }
//                }
//                else
//                {
//                    if (d < c)
//                    {
//                        y0 += stepy;
//                        setpix(x0,y0);
//
//                        if (extras > 1)
//                        {
//                            x0 += stepx;
//                            y0 += stepy;
//                            setpix(x0,y0);
//                        }
//
//                        if (extras > 2)
//                        {
//                            y1 -= stepy;
//                            setpix(x1,y1);
//                        }
//                    }
//                    else
//                    {
//                        x0 += stepx;
//                        y0 += stepy;
//                        setpix(x0,y0);
//
//                        if (extras > 1)
//                        {
//                            y0 += stepy;
//                            setpix(x0,y0);
//                        }
//                        if (extras > 2)
//                        {
//                            x1 -= stepx;
//                            y1 -= stepy;
//                            setpix(x1,y1);
//                        }
//                    }
//                }
//            }
//        }
//        else
//        {
//            c = (dx - dy) << 1;
//            incr1 = c << 1;
//            d =  incr1 + dy;
//
//            for (i = 0; i < length; i++)
//            {
//                y0 += stepy;
//                y1 -= stepy;
//
//                if (d > 0)
//                {
//                    x0 += stepx;
//                    setpix(x0,y0);
//                    x0 += stepx;
//                    y0 += stepy;
//                    setpix(x0,y0);
//                    x1 -= stepy;
//                    setpix(x1,y1);
//                    x1 -= stepx;
//                    y1 -= stepy;
//                    setpix(x1,y1);
//                    d+=incr1;
//                }
//                else
//                {
//                    if (d < c)
//                    {
//                        setpix(x0,y0);
//                        x0 += stepx;
//                        y0 += stepy;
//                        setpix(x0,y0);
//                        setpix(x1,y1);
//                        x1 -= stepx;
//                        y1 -= stepy;
//                        setpix(x1,y1);
//                    }
//                    else
//                    {
//                        x0 += stepx;
//                        setpix(x0,y0);
//                        y0 += stepy;
//                        setpix(x0,y0);
//                        x1 -= stepy;
//                        setpix(x1,y1);
//                        y1 -= stepy;
//                        setpix(x1,y1);
//                    }
//
//                    d += incr2;
//                }
//            }
//
//            if (extras > 0)
//            {
//                if (d > 0)
//                {
//                    x0 += stepx;
//                    y0 += stepy;
//                    setpix(x0,y0);
//
//                    if (extras > 1)
//                    {
//                        x0 += stepx;
//                        y0 += stepy;
//                        setpix(x0,y0);
//                    }
//
//                    if (extras > 2)
//                    {
//                        x1 -= stepx;
//                        y1 -= stepy;
//                        setpix(x1,y1);
//                    }
//                }
//                else
//                {
//                    if (d < c)
//                    {
//                        y0 += stepy;
//                        setpix(x0,y0);
//
//                        if (extras > 1)
//                        {
//                            x0 += stepx;
//                            y0 += stepy;
//                            setpix(x0,y0);
//                        }
//
//                        if (extras > 2)
//                        {
//                            y1 -= stepy;
//                            setpix(x1,y1);
//                        }
//                    }
//                    else
//                    {
//                        x0 += stepx;
//                        y0 += stepy;
//                        setpix(x0,y0);
//
//                        if (extras > 1)
//                        {
//                            y0 += stepy;
//                            setpix(x0,y0);
//                        }
//
//                        if (extras > 2)
//                        {
//                            if (d > c)
//                            {
//                                x1 -= stepx;
//                                y1 -= stepy;
//                                setpix(x1,y1);
//                            }
//                            else
//                            {
//                                y1 -= stepy;
//                                setpix(x1,y1);
//                            }
//                        }
//                    }
//                }
//            }
//        }
//    }
//}

//void BMP_drawLine_old(Line *l)
//{
//    // process clipping (exit if outside screen)
//    if (BMP_clipLine(l))
//    {
//        s16 dx, dy;
//        s16 step_x;
//        s16 step_y;
//
//        const s16 x1 = l->pt1.x >> 1;
//        const s16 y1 = l->pt1.y;
//        // calcul new deltas
//        dx = (l->pt2.x >> 1) - x1;
//        dy = l->pt2.y - y1;
//        // prepare offset
//        const u16 offset = x1 + (y1 * BMP_PITCH);
//
//        if (dx < 0)
//        {
//            dx = -dx;
//            step_x = -1;
//        }
//        else
//            step_x = 1;
//
//        if (dy < 0)
//        {
//            dy = -dy;
//            step_y = -BMP_PITCH;
//        }
//        else
//            step_y = BMP_PITCH;
//
//        // reverse X and Y on deltas and steps
//        if (dx < dy)
//            drawLine(offset, dy, dx, step_y, step_x, l->col);
//        else
//            drawLine(offset, dx, dy, step_x, step_y, l->col);
//    }
//}



void BMP_drawBitmapData(const u8 *image, u16 x, u16 y, u16 w, u16 h, u32 pitch)
{
    // pixel out screen ?
    if ((x >= BMP_WIDTH) || (y >= BMP_HEIGHT))
        return;

    u16 adj_w, adj_h;
    const u8 *src;
    u8 *dst;

    // limit bitmap size if larger than bitmap screen
    if ((w + x) > BMP_WIDTH) adj_w = (BMP_WIDTH - (w + x)) >> 1;
    else adj_w = w >> 1;

    if ((h + y) > BMP_HEIGHT) adj_h = BMP_HEIGHT - (h + y);
    else adj_h = h;

    // prepare source and destination
    src = image;
    dst = BMP_getWritePointer(x, y);

    while(adj_h--)
    {
        memcpy(dst, src, adj_w);
        src += pitch;
        dst += BMP_PITCH;
    }
}

u16 BMP_drawBitmap(const Bitmap *bitmap, u16 x, u16 y, u16 loadpal)
{
    u16 w, h;

    // get the image width
    w = bitmap->w;
    // get the image height
    h = bitmap->h;

    // compressed bitmap ?
    if (bitmap->compression != COMPRESSION_NONE)
    {
        Bitmap *b = unpackBitmap(bitmap, NULL);

        if (b == NULL) return FALSE;

        BMP_drawBitmapData(b->image, x, y, w, h, w >> 1);
        MEM_free(b);
    }
    else
        BMP_drawBitmapData(bitmap->image, x, y, w, h, w >> 1);

    // load the palette
    if (loadpal)
    {
        const Palette *palette = bitmap->palette;
        VDP_setPaletteColors((pal << 4) + (palette->index & 0xF), palette->data, palette->length);
    }

    return TRUE;
}

u16 BMP_drawBitmapScaled(const Bitmap *bitmap, u16 x, u16 y, u16 w, u16 h, u16 loadpal)
{
    u16 bmp_wb, bmp_h;

    // get the image width in byte
    bmp_wb = bitmap->w >> 1;
    // get the image height
    bmp_h = bitmap->h;

    // compressed bitmap ?
    if (bitmap->compression != COMPRESSION_NONE)
    {
        Bitmap *b = unpackBitmap(bitmap, NULL);

        if (b == NULL) return FALSE;

        BMP_scale(b->image, bmp_wb, bmp_h, bmp_wb, BMP_getWritePointer(x, y), w >> 1, h, BMP_PITCH);
        MEM_free(b);
    }
    else
        BMP_scale(bitmap->image, bmp_wb, bmp_h, bmp_wb, BMP_getWritePointer(x, y), w >> 1, h, BMP_PITCH);

    // load the palette
    if (loadpal)
    {
        const Palette *palette = bitmap->palette;
        VDP_setPaletteColors((pal << 4) + (palette->index & 0xF), palette->data, palette->length);
    }

    return TRUE;
}

void BMP_loadBitmapData(const u8 *image, u16 x, u16 y, u16 w, u16 h, u32 pitch)
{
    BMP_drawBitmapData(image, x, y, w, h, pitch);
}

void BMP_loadBitmap(const Bitmap *bitmap, u16 x, u16 y, u16 loadpal)
{
    BMP_drawBitmap(bitmap, x, y, loadpal);
}

void BMP_loadAndScaleBitmap(const Bitmap *bitmap, u16 x, u16 y, u16 w, u16 h, u16 loadpal)
{
    BMP_drawBitmapScaled(bitmap, x, y, w, h, loadpal);
}

void BMP_getBitmapPalette(const Bitmap *bitmap, u16 *dest)
{
    memcpy(dest, bitmap->palette, 16 * 2);
}


// works only for 8 bits image (x doubled)
void BMP_scale(const u8 *src_buf, u16 src_wb, u16 src_h, u16 src_pitch, u8 *dst_buf, u16 dst_wb, u16 dst_h, u16 dst_pitch)
{
    const s32 yd = ((src_h / dst_h) * src_wb) - src_wb;
    const u16 yr = src_h % dst_h;
    const s32 xd = src_wb / dst_wb;
    const u16 xr = src_wb % dst_wb;

    const u32 adj_src = src_pitch - src_wb;
    const u32 adj_dst = dst_pitch - dst_wb;

    const u8 *src = src_buf;
    u8 *dst = dst_buf;

    u16 y = dst_h;
    s16 ye = 0;

    while(y--)
    {
        u16 x = dst_wb;
        s16 xe = 0;

        while(x--)
        {
            // write pixel
            *dst++ = *src;

            // adjust offset
            src += xd;

            if ((xe += xr) >= (s16) dst_wb)
            {
                xe -= dst_wb;
                src++;
            }
        }

        src += adj_src;
        dst += adj_dst;

        // adjust offset
        src += yd;

        if ((ye += yr) >= (s16) dst_h)
        {
            ye -= dst_h;
            src += src_wb;
        }
    }
}


// internals blank processes
////////////////////////////

void BMP_doVBlankProcess()
{
    // reset phase
    phase = 0;
}

u16 BMP_doHBlankProcess()
{
    // vborder low
    if (phase == 0)
    {
        const u16 vcnt = GET_VCOUNTER;
        const u16 scrh = screenHeight;
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
        VDP_setHIntCounter(((screenHeight - BMP_HEIGHT) >> 1) - 1);
        // update phase
        phase = 3;

        // flip requested or not complete ? --> start / continu flip
        if (state & BMP_STAT_FLIPPING) doFlip();
    }

    return 1;
}


// internals helper methods
///////////////////////////

static void initTilemap(u16 num)
{
    vu32 *plctrl;
    vu16 *pwdata;
    u16 tile_ind;
    u32 addr_tilemap;
    u16 i, j;

    VDP_setAutoInc(2);

    // calculated
    const u32 offset = BMP_FBTILEMAP_OFFSET;

    if (num == 0)
    {
        addr_tilemap = BMP_FB0TILEMAP_BASE + offset;
        tile_ind = TILE_ATTR_FULL(pal, prio, 0, 0, BMP_FB0TILEINDEX);
    }
    else
    {
        addr_tilemap = BMP_FB1TILEMAP_BASE + offset;
        tile_ind = TILE_ATTR_FULL(pal, prio, 0, 0, BMP_FB1TILEINDEX);
    }

    // point to vdp port
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    i = BMP_CELLHEIGHT;

    while(i--)
    {
        // set destination address for tilemap
        *plctrl = GFX_WRITE_VRAM_ADDR(addr_tilemap);

        // write tilemap line to VDP
        j = BMP_CELLWIDTH >> 3;

        while(j--)
        {
            *pwdata = tile_ind++;
            *pwdata = tile_ind++;
            *pwdata = tile_ind++;
            *pwdata = tile_ind++;
            *pwdata = tile_ind++;
            *pwdata = tile_ind++;
            *pwdata = tile_ind++;
            *pwdata = tile_ind++;
        }

        addr_tilemap += BMP_PLANWIDTH * 2;
    }
}

static void clearVRAMBuffer(u16 num)
{
    if (num) DMA_doVRamFill(BMP_FB1TILE, BMP_PITCH * BMP_HEIGHT, 0, 1);
    else DMA_doVRamFill(BMP_FB0TILE, BMP_PITCH * BMP_HEIGHT, 0, 1);
    DMA_waitCompletion();
}

static void flipBuffer()
{
    if (READ_IS_FB0)
    {
        bmp_buffer_read = bmp_buffer_1;
        bmp_buffer_write = bmp_buffer_0;
    }
    else
    {
        bmp_buffer_read = bmp_buffer_0;
        bmp_buffer_write = bmp_buffer_1;
    }

    // we want buffer preservation ?
    if (HAS_BUFFERCOPY)
        copyBitmapBuffer(bmp_buffer_read, bmp_buffer_write);
}

static void doFlip()
{
    // wait for DMA completion if used otherwise VDP writes can be corrupted
    VDP_waitDMACompletion();

    // copy tile buffer to VRAM
    if (doBlit())
    {
        // at this point copy to VRAM is done !

        // flip displayed buffer
        if (HAS_DOUBLEBUFFER)
        {
            u16 vscr;

            // switch displayed buffer
            if (READ_IS_FB0) vscr = 0;
            else vscr = (BMP_PLANHEIGHT * 8) / 2;

            VDP_setVerticalScroll(bmp_plan, vscr);
        }

        // get bitmap state
        u16 s = state;

        // we had a pending flip request ?
        if (s & BMP_STAT_FLIPWAITING)
        {
            // flip buffers
            flipBuffer();
            // clear only pending flag and process new flip
            s &= ~BMP_STAT_FLIPWAITING;
        }
        else
            // flip done
            s &= ~BMP_STAT_FLIPPING;

        // save back bitmap state
        state = s;
    }
}

#define TRANSFER(x)                                     \
    *pldata = src[((BMP_PITCH * 0) / 4) + (x)]; \
    *pldata = src[((BMP_PITCH * 1) / 4) + (x)]; \
    *pldata = src[((BMP_PITCH * 2) / 4) + (x)]; \
    *pldata = src[((BMP_PITCH * 3) / 4) + (x)]; \
    *pldata = src[((BMP_PITCH * 4) / 4) + (x)]; \
    *pldata = src[((BMP_PITCH * 5) / 4) + (x)]; \
    *pldata = src[((BMP_PITCH * 6) / 4) + (x)]; \
    *pldata = src[((BMP_PITCH * 7) / 4) + (x)];

#define TRANSFER8(x)                \
    TRANSFER((8 * x) + 0)   \
    TRANSFER((8 * x) + 1)   \
    TRANSFER((8 * x) + 2)   \
    TRANSFER((8 * x) + 3)   \
    TRANSFER((8 * x) + 4)   \
    TRANSFER((8 * x) + 5)   \
    TRANSFER((8 * x) + 6)   \
    TRANSFER((8 * x) + 7)

static u16 doBlit()
{
    static u16 pos_i;
    vu32 *plctrl;
    vu32 *pldata;
    u32 *src;
    u32 addr_tile;
    u16 i;

    VDP_setAutoInc(2);

    src = (u32 *) bmp_buffer_read;

    if (HAS_DOUBLEBUFFER && READ_IS_FB1)
        addr_tile = BMP_FB1TILE;
    else
        addr_tile = BMP_FB0TILE;

    /* point to vdp ctrl port */
    plctrl = (u32 *) GFX_CTRL_PORT;

    // previous blit not completed ?
    if (state & BMP_STAT_BLITTING)
    {
        // adjust tile address
        addr_tile += pos_i * BMP_CELLWIDTH * 32;
        // adjust src pointer
        src += pos_i * (BMP_YPIXPERTILE * (BMP_PITCH / 4));

        // set destination address for tile
        *plctrl = GFX_WRITE_VRAM_ADDR(addr_tile);
    }
    else
    {
        // start blit
        state |= BMP_STAT_BLITTING;
        pos_i = 0;

        // set destination address for tile
        *plctrl = GFX_WRITE_VRAM_ADDR(addr_tile);
    }

    const u16 remain = BMP_CELLHEIGHT - pos_i;

    if (IS_PALSYSTEM)
    {
        if (remain < PAL_TILES_BW) i = remain;
        else i = PAL_TILES_BW;
    }
    else
    {
        if (remain < NTSC_TILES_BW) i = remain;
        else i = NTSC_TILES_BW;
    }

    // save position
    pos_i += i;

    /* point to vdp data port */
    pldata = (u32 *) GFX_DATA_PORT;

    while(i--)
    {
        // send it to VRAM
        TRANSFER8(0)
        TRANSFER8(1)
        TRANSFER8(2)
        TRANSFER8(3)

        src += (8 * BMP_PITCH) / 4;
    }

    // blit not yet done
    if (pos_i < BMP_CELLHEIGHT) return 0;

    // blit done
    state &= ~BMP_STAT_BLITTING;

    return 1;
}

//static void drawLine_old(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col)
//{
//    const u8 c = col;
//    u8 *dst = &bmp_buffer_write[offset];
//    s16 delta = dx >> 1;
//    s16 cnt = dx + 1;
//
//    while(cnt--)
//    {
//        // write pixel
//        *dst = c;
//        // adjust offset
//        dst += step_x;
//
//        if ((delta -= dy) < 0)
//        {
//            dst += step_y;
//            delta += dx;
//        }
//    }
//}
