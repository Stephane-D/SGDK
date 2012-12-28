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

#include "memory.h"
#include "tools.h"
#include "string.h"
#include "tab_vram.h"


#define BMP_BASETILEINDEX       TILE_USERINDEX

#define BMP_FB0TILEINDEX        BMP_BASETILEINDEX
#define BMP_FB1TILEINDEX        (BMP_BASETILEINDEX + (BMP_CELLWIDTH * BMP_CELLHEIGHT))

#define BMP_SYSTEMTILE          (BMP_SYSTEMTILEINDEX * 32)
#define BMP_BASETILE            (BMP_BASETILEINDEX * 32)
#define BMP_FB0TILE             (BMP_FB0TILEINDEX * 32)
#define BMP_FB1TILE             (BMP_FB1TILEINDEX * 32)

#define BMP_FB0TILEMAP_BASE     BMP_PLAN
#define BMP_FB1TILEMAP_BASE     (BMP_PLAN + ((BMP_PLANWIDTH * (BMP_PLANHEIGHT / 2)) * 2))
#define BMP_FBTILEMAP_OFFSET    (((BMP_PLANWIDTH * BMP_CELLYOFFSET) + BMP_CELLXOFFSET) * 2)
//#define BMP_FB0TILEMAP_ADJ      (BMP_FB0TILEMAP + BMP_FBTILEMAPOFFSET)
//#define BMP_FB1TILEMAP_ADJ      (BMP_FB1TILEMAP + BMP_FBTILEMAPOFFSET)

#define BMP_STAT_FLIPPING       (1 << 0)
#define BMP_STAT_BLITTING       (1 << 1)
#define BMP_STAT_FLIPWAITING    (1 << 2)

#define HAS_FLAG(f)             (bmp_flags & (f))
#define HAS_FLAGS(f)            ((bmp_flags & (f)) == (f))

#define READ_IS_FB0             (bmp_buffer_read == bmp_buffer_0)
#define READ_IS_FB1             (bmp_buffer_read == bmp_buffer_1)
#define WRITE_IS_FB0            (bmp_buffer_write == bmp_buffer_0)
#define WRITE_IS_FB1            (bmp_buffer_write == bmp_buffer_1)


#define NTSC_TILES_BW       7
#define PAL_TILES_BW        10


// we don't want to share them
extern u32 VIntProcess;
extern u32 HIntProcess;
extern u16 textBasetile;


u8 *bmp_buffer_0 = NULL;
u8 *bmp_buffer_1 = NULL;

u8 *bmp_buffer_read;
u8 *bmp_buffer_write;

// used for polygon drawing
s16 *LeftPoly = NULL;
s16 *RightPoly = NULL;

s16 minY;
s16 maxY;

// internals
u16 bmp_state;
u16 phase;


// forward
extern void calculatePolyEdge(const Vect2D_s16 *pt1, const Vect2D_s16 *pt2, u8 clockwise);

static u16 getYOffset();
static void doFlip();
static void flipBuffer();
static void initTilemap(u16 num);
static u16 doBlit();
static void clearBuffer(u8 *bmp_buffer);
static void drawLine(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col);


void BMP_init()
{
    // release buffers if needed
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

    // prepare tilemap
    initTilemap(0);
    initTilemap(1);

    // first init, clear and flip
    BMP_clear();
    BMP_flip();
    // second init, clear and flip for correct init (double buffer)
    BMP_clear();
    BMP_flip();

    const u16 vcnt = GET_VCOUNTER;
    const u16 scrh = VDP_getScreenHeight();

    // modify HIntCounter for extended blank blit
    VDP_setHIntCounter(scrh - (VDP_getHIntCounter() + vcnt + ((scrh - BMP_HEIGHT) >> 1) + 3));
    // enabled bitmap Int processing
    HIntProcess |= PROCESS_BITMAP_TASK;
    VIntProcess |= PROCESS_BITMAP_TASK;
    VDP_setHInterrupt(1);
}

void BMP_end()
{
    // re enabled VDP if it was disabled because of extended blank
    VDP_setEnable(1);
    // disabled bitmap Int processing
    VDP_setHInterrupt(0);
    HIntProcess &= ~PROCESS_BITMAP_TASK;
    VIntProcess &= ~PROCESS_BITMAP_TASK;

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

void BMP_reset()
{
    BMP_init();
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


u16 BMP_flip()
{
    // wait until pending flip is processed
    BMP_waitWhileFlipRequestPending();

    // currently flipping ?
    if (bmp_state & BMP_STAT_FLIPPING)
    {
        // set a pending flip
        bmp_state |= BMP_STAT_FLIPWAITING;
        return 1;
    }

    // flip bitmap buffer
    flipBuffer();
    // flip started (will be processed in blank period --> BMP_doBlankProcess)
    bmp_state |= BMP_STAT_FLIPPING;

    return 0;
}


void BMP_drawText(const char *str, u16 x, u16 y)
{
    const u16 adjy = y + getYOffset();

    VDP_drawTextBG(BMP_PLAN, str, textBasetile, x, adjy);
}

void BMP_clearText(u16 x, u16 y, u16 w)
{
    const u16 adjy = y + getYOffset();

    VDP_clearTextBG(BMP_PLAN, x, adjy, w);
}

void BMP_clearTextLine(u16 y)
{
    const u16 adjy = y + getYOffset();

    VDP_clearTextLineBG(BMP_PLAN, adjy);
}


void BMP_showFPS(u16 float_display)
{
    char str[16];
    const u16 y = getYOffset() + 1;

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


// graphic drawing functions
////////////////////////////

void BMP_clear()
{
    clearBuffer(bmp_buffer_write);
}


u8* BMP_getWritePointer(u16 x, u16 y)
{
    const u16 off = (y * BMP_PITCH) + x;

    // return write address
    return &bmp_buffer_write[off];
}

u8* BMP_getReadPointer(u16 x, u16 y)
{
    const u16 off = (y * BMP_PITCH) + x;

    // return read address
    return &bmp_buffer_read[off];
}


u8 BMP_getPixel(u16 x, u16 y)
{
    // pixel in screen ?
    if ((x < BMP_WIDTH) && (y < BMP_HEIGHT))
    {
        const u16 off = (y * BMP_PITCH) + x;

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
        const u16 off = (y * BMP_PITCH) + x;

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
            const u16 off = (y * BMP_PITCH) + x;

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
            const u16 off = (y * BMP_PITCH) + x;

            // write pixel
            bmp_buffer_write[off] = p->col;
        }

        // next pixel
        p++;
    }
}


void BMP_drawLine(Line *l)
{
    // process clipping (exit if outside screen)
    if (BMP_clipLine(l))
    {
        s16 dx, dy;
        s16 step_x;
        s16 step_y;

        const s16 x1 = l->pt1.x;
        const s16 y1 = l->pt1.y;
        // calcul new deltas
        dx = l->pt2.x - x1;
        dy = l->pt2.y - y1;
        // prepare offset
        const u16 offset = x1 + (y1 * BMP_WIDTH);

        if (dx < 0)
        {
            dx = -dx;
            step_x = -1;
        }
        else
            step_x = 1;

        if (dy < 0)
        {
            dy = -dy;
            step_y = -BMP_WIDTH;
        }
        else
            step_y = BMP_WIDTH;

        // reverse X and Y on deltas and steps
        if (dx < dy)
            drawLine(offset, dy, dx, step_y, step_x, l->col);
        else
            drawLine(offset, dx, dy, step_x, step_y, l->col);
    }
}

void BMP_drawPolygon(const Vect2D_s16 *pts, u16 num, u8 col, u8 culling)
{
    // calculate polygon "direction"
    const u8 clockwise = ((pts[2].x - pts[0].x) * (pts[1].y - pts[0].y)) > ((pts[2].y - pts[0].y) * (pts[1].x - pts[0].x));
    // backface culling enabled and clockwised polygon ? --> exit
    if ((culling) && (clockwise)) return;

    // prepare polygon edge calculation
    minY = BMP_HEIGHT;
    maxY = 0;

    const Vect2D_s16 *curpts;
    u16 i;

    curpts = pts;
    i = num - 1;
    while(i--)
    {
        calculatePolyEdge(curpts, curpts + 1, clockwise);
        curpts++;
    }

    // last line
    calculatePolyEdge(curpts, pts, clockwise);

    // number of scanline to draw
    s16 len = maxY - minY;
    // nothing to draw
    if (len <= 0) return;

    s16 *left;
    s16 *right;
    u8 *buf;
    u8 c;

    left = &LeftPoly[minY];
    right = &RightPoly[minY];
    buf = &bmp_buffer_write[minY * BMP_PITCH];
    c = col | (col << 4);

    while (len--)
    {
        s16 x1, x2;

        x1 = *left++;
        x2 = *right++;

        // backface ?
        if (x1 > x2) SWAP_s16(x1, x2)

        // something to draw ?
        if ((x1 < BMP_WIDTH) && (x2 >= 0))
        {
            // clip x
            if (x1 < 0) x1 = 0;
            if (x2 >= BMP_WIDTH) x2 = BMP_WIDTH - 1;

            // draw horizontal line
            memset(&buf[x1], c, x2 - x1);
        }

        buf += BMP_PITCH;
    }
}


void BMP_loadBitmap(const u8 *data, u16 x, u16 y, u16 w, u16 h, u32 pitch)
{
    // pixel out screen ?
    if ((x >= BMP_WIDTH) || (y >= BMP_HEIGHT))
        return;

    u16 adj_w, adj_h;
    const u8 *src;
    u8 *dst;

    // limit bitmap size if larger than bitmap screen
    if ((w + x) > BMP_WIDTH) adj_w = BMP_WIDTH - (w + x);
    else adj_w = w;
    if ((h + y) > BMP_HEIGHT) adj_h = BMP_HEIGHT - (h + y);
    else adj_h = h;

    // prepare source and destination
    src = data;
    dst = BMP_getWritePointer(x, y);

    while(adj_h--)
    {
        memcpy(dst, src, adj_w);
        src += pitch;
        dst += BMP_WIDTH;
    }
}

void BMP_loadGenBitmap(const Bitmap *bitmap, u16 x, u16 y, u16 numpal)
{
    u16 w, h;

    // get the image width (bitmap size / 2 as we double X resolution)
    w = bitmap->w >> 1;
    // get the image height
    h = bitmap->h;

    // load the palette
    if (numpal < 4) VDP_setPalette(numpal, bitmap->palette);

    BMP_loadBitmap((u8*) bitmap->image, x, y, w, h, w);
}

void BMP_loadAndScaleGenBitmap(const Bitmap *bitmap, u16 x, u16 y, u16 w, u16 h, u16 numpal)
{
    u16 bmp_w, bmp_h;

    // get the image width (bitmap size / 2 as we double X resolution)
    bmp_w = bitmap->w >> 1;
    // get the image height
    bmp_h = bitmap->h;

    // load the palette
    if (numpal < 4) VDP_setPalette(numpal, bitmap->palette);

    BMP_scale((u8*) bitmap->image, bmp_w, bmp_h, bmp_w, BMP_getWritePointer(x, y), w, h, BMP_WIDTH);
}

void BMP_getGenBitmapPalette(const Bitmap *bitmap, u16 *pal)
{
    u16 i;
    const u16 *src;
    u16 *dst;

    src = bitmap->palette;
    dst = pal;
    i = 16;
    while(i--) *dst++ = *src++;
}


// works only for 8 bits image (x doubled)
void BMP_scale(const u8 *src_buf, u16 src_w, u16 src_h, u16 src_pitch, u8 *dst_buf, u16 dst_w, u16 dst_h, u16 dst_pitch)
{
    const s32 yd = ((src_h / dst_h) * src_w) - src_w;
    const u16 yr = src_h % dst_h;
    const s32 xd = src_w / dst_w;
    const u16 xr = src_w % dst_w;

    const u32 adj_src = src_pitch - src_w;
    const u32 adj_dst = dst_pitch - dst_w;

    const u8 *src = src_buf;
    u8 *dst = dst_buf;

    u16 y = dst_h;
    s16 ye = 0;

    while(y--)
    {
        u16 x = dst_w;
        s16 xe = 0;

        while(x--)
        {
            // write pixel
            *dst++ = *src;

            // adjust offset
            src += xd;
            if ((xe += xr) >= (s16) dst_w)
            {
                xe -= dst_w;
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
            src += src_w;
        }
    }
}


// internals blank processes
////////////////////////////

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
        if (bmp_state & BMP_STAT_FLIPPING) doFlip();
    }

    return 1;
}


// internals helper methods
///////////////////////////

static u16 getYOffset()
{
    u16 res;

    res = 4;
    if (READ_IS_FB1) res += BMP_PLANHEIGHT / 2;

    return res;
}

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
        tile_ind = BMP_FB0TILEINDEX;
    }
    else
    {
        addr_tilemap = BMP_FB1TILEMAP_BASE + offset;
        tile_ind = BMP_FB1TILEINDEX;
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

static void clearBuffer(u8 *bmp_buffer)
{
    u32 *src;
    u16 i;

    // prevent compiler to use slow CLR instruction
    const u32 v = getZeroU32();

    src = (u32*) bmp_buffer;

    i = (BMP_WIDTH * BMP_HEIGHT) / (4 * 8);
    while(i--)
    {
        *src++ = v;
        *src++ = v;
        *src++ = v;
        *src++ = v;
        *src++ = v;
        *src++ = v;
        *src++ = v;
        *src++ = v;
    }
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
}

static void doFlip()
{
    // wait for DMA completion if used otherwise VDP writes can be corrupted
    VDP_waitDMACompletion();

    // copy tile buffer to VRAM
    if (doBlit())
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
            // flip buffers
            flipBuffer();
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

    if (READ_IS_FB0)
        addr_tile = BMP_FB0TILE;
    else
        addr_tile = BMP_FB1TILE;

    /* point to vdp ctrl port */
    plctrl = (u32 *) GFX_CTRL_PORT;

    // previous blit not completed ?
    if (bmp_state & BMP_STAT_BLITTING)
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
        bmp_state |= BMP_STAT_BLITTING;
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
    bmp_state &= ~BMP_STAT_BLITTING;

    return 1;
}

static void drawLine(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col)
{
    const u8 c = col;
    u8 *dst = &bmp_buffer_write[offset];
    s16 delta = dx >> 1;
    s16 cnt = dx + 1;

    while(cnt--)
    {
        // write pixel
        *dst = c;

        // adjust offset
        dst += step_x;
        if ((delta -= dy) < 0)
        {
            dst += step_y;
            delta += dx;
        }
    }
}
