#include "config.h"
#include "types.h"

#include "bmp_cmn.h"
#include "bmp_intr.h"
#include "bmp.h"

#include "base.h"
#include "tools.h"
#include "maths3D.h"

#include "vdp.h"
#include "vdp_tile.h"
#include "vdp_pal.h"
#include "vdp_bg.h"

#include "tab_vram.h"

#ifdef ENABLE_BMP


// we don't want to share them
extern u32 VBlankProcess;
extern u32 HBlankProcess;


// used for polygone drawing
static s16 LeftPoly[BMP_HEIGHT * 2];
static s16 RightPoly[BMP_HEIGHT * 2];
static s16 minY;
static s16 maxY;


// forward
static void initTilemap(u16 num);
static u16 doBlitNorm();
static u16 doBlitBlank();
static u16 doBlitBlankExt();

static void calculatePolyEdge(const Vect2D_s16 *pt1, const Vect2D_s16 *pt2, u8 clockwise);
static void drawLine(u32 offset, s16 dx, s16 dy, u32 step_x, u32 step_y, u8 col);


void BMP_init(u16 flags)
{
    // common bmp init tasks
    _bmp_init();

    // do some init process
    BMP_setFlags(0);

    // prepare tilemap
    initTilemap(0);
    initTilemap(1);

    // first init, clear and flip
    BMP_clear();
    BMP_flip();
    // second init, clear and flip for correct init (double buffer)
    BMP_clear();
    BMP_flip();

    BMP_setFlags(flags);
}

void BMP_end()
{
    // end some stuff
    BMP_setFlags(0);
}

void BMP_reset()
{
    BMP_init(bmp_flags);
}


void BMP_setFlags(u16 value)
{
    if (bmp_flags == value) return;

    // common setFlags tasks
    _bmp_setFlags(value);

    // extended blank mode ?
    if HAS_FLAG(BMP_ENABLE_EXTENDEDBLANK)
    {
        // enable process on hint
        HBlankProcess |= PROCESS_BITMAP_TASK;
        VDP_setHInterrupt(1);
        // disable process on vhint
        VBlankProcess &= ~PROCESS_BITMAP_TASK;
        // define blit function
        doBlit = &doBlitBlankExt;
    }
    else
    {
        // disable process on hint
        HBlankProcess &= ~PROCESS_BITMAP_TASK;
        VDP_setHInterrupt(0);
        // enable process on vint if needed
        if HAS_FLAG(BMP_ENABLE_ASYNCFLIP) VBlankProcess |= PROCESS_BITMAP_TASK;
        else VBlankProcess &= ~PROCESS_BITMAP_TASK;
        // define blit function
        if HAS_FLAG(BMP_ENABLE_BLITONBLANK) doBlit = &doBlitBlank;
        else doBlit = &doBlitNorm;
    }
}

void BMP_enableWaitVSync()
{
    if (!(bmp_flags & BMP_ENABLE_WAITVSYNC))
        BMP_setFlags(bmp_flags | BMP_ENABLE_WAITVSYNC);
}

void BMP_disableWaitVSync()
{
    if (bmp_flags & BMP_ENABLE_WAITVSYNC)
        BMP_setFlags(bmp_flags & ~BMP_ENABLE_WAITVSYNC);
}

void BMP_enableASyncFlip()
{
    if (!(bmp_flags & BMP_ENABLE_ASYNCFLIP))
        BMP_setFlags(bmp_flags | BMP_ENABLE_ASYNCFLIP);
}

void BMP_disableASyncFlip()
{
    if (bmp_flags & BMP_ENABLE_ASYNCFLIP)
        BMP_setFlags(bmp_flags & ~BMP_ENABLE_ASYNCFLIP);
}

void BMP_enableFPSDisplay()
{
    if (!(bmp_flags & BMP_ENABLE_FPSDISPLAY))
        BMP_setFlags(bmp_flags | BMP_ENABLE_FPSDISPLAY);
}

void BMP_disableFPSDisplay()
{
    if (bmp_flags & BMP_ENABLE_FPSDISPLAY)
        BMP_setFlags(bmp_flags & ~BMP_ENABLE_FPSDISPLAY);
}

void BMP_enableBlitOnBlank()
{
    if (!(bmp_flags & BMP_ENABLE_BLITONBLANK))
        BMP_setFlags(bmp_flags | BMP_ENABLE_BLITONBLANK);
}

void BMP_disableBlitOnBlank()
{
    if (bmp_flags & BMP_ENABLE_BLITONBLANK)
        BMP_setFlags(bmp_flags & ~BMP_ENABLE_BLITONBLANK);
}

void BMP_enableExtendedBlank()
{
    if (!(bmp_flags & BMP_ENABLE_EXTENDEDBLANK))
        BMP_setFlags(bmp_flags | BMP_ENABLE_EXTENDEDBLANK);
}

void BMP_disableExtendedBlank()
{
    if (bmp_flags & BMP_ENABLE_EXTENDEDBLANK)
        BMP_setFlags(bmp_flags & ~BMP_ENABLE_EXTENDEDBLANK);
}

void BMP_flip()
{
    // wait for vsync ?
    if HAS_FLAG(BMP_ENABLE_WAITVSYNC)
    {
        // async flip ?
        if HAS_FLAG(BMP_ENABLE_ASYNCFLIP)
        {
            // wait for previous async flip to complete
            BMP_waitAsyncFlipComplete();
            // flip bitmap buffer
            BMP_internalBufferFlip();
            // request a flip (will be processed in blank period --> BMP_doBlankProcess)
            bmp_state |= BMP_STAT_FLIPWAITING;
        }
        else
        {
            VDP_waitVSync();
            // flip bitmap buffer
            BMP_internalBufferFlip();
            // blit buffer to VRAM and flip vdp display
            _bmp_doFlip();
        }
    }
    else
    {
         // flip bitmap buffer
         BMP_internalBufferFlip();
         // blit buffer to VRAM and flip vdp display
         _bmp_doFlip();
    }
}

void BMP_internalBufferFlip()
{
    if READ_IS_FB0
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


static void initTilemap(u16 num)
{
    vu32 *plctrl;
    vu16 *pwdata;
    u16 tile_ind;
    u32 addr_tilemap;
    u16 i, j;

    VDP_setAutoInc(2);

    if (num == 0)
    {
        addr_tilemap = BMP_FB0TILEMAP_ADJ;
        tile_ind = BMP_FB0TILEINDEX;
    }
    else
    {
        addr_tilemap = BMP_FB1TILEMAP_ADJ;
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
        j = (BMP_CELLWIDTH >> 3);
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

static u16 doBlitNorm()
{
    vu32 *plctrl;
    vu32 *pldata;
    u32 *src;
    u16 i, j;

    VDP_setAutoInc(2);

    src = (u32 *) bmp_buffer_read;

    // point to vdp port
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    // set destination address for tile
    if READ_IS_FB0
        *plctrl = GFX_WRITE_VRAM_ADDR(BMP_FB0TILE);
    else
        *plctrl = GFX_WRITE_VRAM_ADDR(BMP_FB1TILE);

    i = BMP_CELLHEIGHT;
    while(i--)
    {
        j = BMP_CELLWIDTH;
        while(j--)
        {
            // send it to VRAM
            *pldata = src[(BMP_PITCH * 0) / 4];
            *pldata = src[(BMP_PITCH * 1) / 4];
            *pldata = src[(BMP_PITCH * 2) / 4];
            *pldata = src[(BMP_PITCH * 3) / 4];
            *pldata = src[(BMP_PITCH * 4) / 4];
            *pldata = src[(BMP_PITCH * 5) / 4];
            *pldata = src[(BMP_PITCH * 6) / 4];
            *pldata = src[(BMP_PITCH * 7) / 4];

            src++;
        }

        src += (7 * BMP_PITCH) / 4;
    }

    return 1;
}

static u16 doBlitBlank()
{
    vu32 *plctrl;
    vu32 *pldata;
    u32 *src;
    u32 addr_tile;
    u16 i, j;

    static u16 save_i, save_j;

    VDP_setAutoInc(2);

    src = (u32 *) bmp_buffer_read;

    if READ_IS_FB0
        addr_tile = BMP_FB0TILE;
    else
        addr_tile = BMP_FB1TILE;

    // point to vdp port
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    // previous blit not completed ?
    if (bmp_state & BMP_STAT_BLITTING)
    {
        const u16 done_i = BMP_CELLHEIGHT - save_i;
        const u16 done_j = BMP_CELLWIDTH - save_j;

        // adjust tile address
        addr_tile += ((done_i * BMP_CELLWIDTH) + done_j) * 32;
        // adjust src pointer
        src += (done_i * (BMP_YPIXPERTILE * (BMP_PITCH / 4))) + done_j;

        // set destination address for tile
        *plctrl = GFX_WRITE_VRAM_ADDR(addr_tile);

        // restore j position
        j = save_j;

        while(j--)
        {
            // send it to VRAM
            *pldata = src[(BMP_PITCH * 0) / 4];
            *pldata = src[(BMP_PITCH * 1) / 4];
            *pldata = src[(BMP_PITCH * 2) / 4];
            *pldata = src[(BMP_PITCH * 3) / 4];
            *pldata = src[(BMP_PITCH * 4) / 4];
            *pldata = src[(BMP_PITCH * 5) / 4];
            *pldata = src[(BMP_PITCH * 6) / 4];
            *pldata = src[(BMP_PITCH * 7) / 4];

            addr_tile += 32;
            src++;
        }

        src += (7 * (BMP_PITCH / 4));

        // restore i position
        i = save_i - 1;
    }
    else
    {
        // start blit
        bmp_state |= BMP_STAT_BLITTING;
        i = BMP_CELLHEIGHT;

        // set destination address for tile
        *plctrl = GFX_WRITE_VRAM_ADDR(addr_tile);
    }

    while(i--)
    {
        j = BMP_CELLWIDTH;
        while(j--)
        {
            // blank period finished ?
            if (!GET_VDPSTATUS(VDP_VBLANK_FLAG))
            {
                // save current position and exit
                save_i = i + 1;
                save_j = j + 1;
                return 0;
            }

            // send it to VRAM
            *pldata = src[(BMP_PITCH * 0) / 4];
            *pldata = src[(BMP_PITCH * 1) / 4];
            *pldata = src[(BMP_PITCH * 2) / 4];
            *pldata = src[(BMP_PITCH * 3) / 4];
            *pldata = src[(BMP_PITCH * 4) / 4];
            *pldata = src[(BMP_PITCH * 5) / 4];
            *pldata = src[(BMP_PITCH * 6) / 4];
            *pldata = src[(BMP_PITCH * 7) / 4];

            src++;
        }

        src += (7 * BMP_PITCH) / 4;
    }

    // blit done
    bmp_state &= ~BMP_STAT_BLITTING;

    return 1;
}

static u16 doBlitBlankExt()
{
    vu32 *plctrl;
    vu32 *pldata;
    vu16 *pwhvcnt;
    u32 *src;
    u32 addr_tile;
    u16 i, j;

    static u16 save_i, save_j;

    VDP_setAutoInc(2);

    src = (u32 *) bmp_buffer_read;

    if READ_IS_FB0
        addr_tile = BMP_FB0TILE;
    else
        addr_tile = BMP_FB1TILE;

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    // previous blit not completed ?
    if (bmp_state & BMP_STAT_BLITTING)
    {
        const u16 done_i = BMP_CELLHEIGHT - save_i;
        const u16 done_j = BMP_CELLWIDTH - save_j;

        // adjust tile address
        addr_tile += ((done_i * BMP_CELLWIDTH) + done_j) * 32;
        // adjust src pointer
        src += (done_i * (BMP_YPIXPERTILE * (BMP_PITCH / 4))) + done_j;

        // set destination address for tile
        *plctrl = GFX_WRITE_VRAM_ADDR(addr_tile);

        // restore j position
        j = save_j;

        while(j--)
        {
            // send it to VRAM
            *pldata = src[(BMP_PITCH * 0) / 4];
            *pldata = src[(BMP_PITCH * 1) / 4];
            *pldata = src[(BMP_PITCH * 2) / 4];
            *pldata = src[(BMP_PITCH * 3) / 4];
            *pldata = src[(BMP_PITCH * 4) / 4];
            *pldata = src[(BMP_PITCH * 5) / 4];
            *pldata = src[(BMP_PITCH * 6) / 4];
            *pldata = src[(BMP_PITCH * 7) / 4];

            src++;
        }

        src += (7 * (BMP_PITCH / 4));

        // restore i position
        i = save_i - 1;
    }
    else
    {
        // start blit
        bmp_state |= BMP_STAT_BLITTING;
        i = BMP_CELLHEIGHT;

        // set destination address for tile
        *plctrl = GFX_WRITE_VRAM_ADDR(addr_tile);
    }

    // point to HV counter
    pwhvcnt = (u16 *) GFX_HVCOUNTER_PORT;

    while(i--)
    {
        j = BMP_CELLWIDTH;
        while(j--)
        {
            // test if we are out of time
            const u16 vcnt = *pwhvcnt;

            // blank period almost finish ?
            if ((vcnt <= (32 << 8)) && (vcnt >= (30 << 8)))
            {
                // save current position and exit
                save_i = i + 1;
                save_j = j + 1;
                return 0;
            }

            // send it to VRAM
            *pldata = src[(BMP_PITCH * 0) / 4];
            *pldata = src[(BMP_PITCH * 1) / 4];
            *pldata = src[(BMP_PITCH * 2) / 4];
            *pldata = src[(BMP_PITCH * 3) / 4];
            *pldata = src[(BMP_PITCH * 4) / 4];
            *pldata = src[(BMP_PITCH * 5) / 4];
            *pldata = src[(BMP_PITCH * 6) / 4];
            *pldata = src[(BMP_PITCH * 7) / 4];

            src++;
        }

        src += (7 * BMP_PITCH) / 4;
    }

    // blit done
    bmp_state &= ~BMP_STAT_BLITTING;

    return 1;
}


// graphic drawing functions
////////////////////////////

void BMP_clear()
{
    fastmemset(bmp_buffer_write, 0, sizeof(bmp_buffer_0));
}


u8* BMP_getWritePointer(u32 x, u32 y)
{
    // return read address
    return &bmp_buffer_write[(y * BMP_PITCH) + x];
}

u8* BMP_getReadPointer(u32 x, u32 y)
{
    // return read address
    return &bmp_buffer_read[(y * BMP_PITCH) + x];
}


u8 BMP_getPixel(u32 x, u32 y)
{
    // pixel in screen ?
    if ((x < BMP_WIDTH) && (y < BMP_HEIGHT))
        // read pixel
        return bmp_buffer_write[(y * BMP_PITCH) + x];

    return 0;
}

void BMP_setPixel(u32 x, u32 y, u8 col)
{
    // pixel in screen ?
    if ((x < BMP_WIDTH) && (y < BMP_HEIGHT))
        // write pixel
        bmp_buffer_write[(y * BMP_PITCH) + x] = col;
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
            // write pixel
            bmp_buffer_write[(y * BMP_PITCH) + x] = c;

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
            // write pixel
            bmp_buffer_write[(y * BMP_PITCH) + x] = p->col;

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
        const s16 offset = x1 + (y1 * BMP_WIDTH);

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

void BMP_drawPolygone(const Vect2D_s16 *pts, u16 num, u8 col)
{
    // calculate polygone "direction"
    const u8 clockwise = ((pts[2].x - pts[0].x) * (pts[1].y - pts[0].y)) > ((pts[2].y - pts[0].y) * (pts[1].x - pts[0].x));
    // backface culling enabled and clockwised polygone ? --> exit
    if (HAS_FLAG(BMP_ENABLE_BFCULLING) && (clockwise)) return;

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
        u8 *dst;
        u16 cnt;

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

            dst = &buf[x1];
            cnt = (x2 - x1) + 1;

            // draw horizontal line
            while(cnt--) *dst++ = c;

//                    u8 *dst8;
//                    u16 *dst16;
//                    u16 cnt;
//                    u16 cnt2;
//                    u16 c16;
//
//                    dst8 = &buf[x1];
//                    cnt = (x2 - x1) + 1;
//
//                    if ((u32) (dst8) & 1)
//                    {
//                        *dst8++ = c;
//                        cnt--;
//                    }
//
//                    c16 = c + (c << 8);
//                    dst16 = (u16 *) dst8;
//                    cnt2 = cnt >> 1;
//
//                    while (cnt2--) *dst16++ = c16;
//
//                    if (cnt & 1)
//                    {
//                        dst8 = (u8 *) dst16;
//                        *dst8 = c;
//                    }
        }

        buf += BMP_PITCH;
    }
}


void BMP_loadBitmap(const u8 *data, u32 x, u32 y, u16 w, u16 h, u32 pitch)
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
        fastmemcpy(dst, src, adj_w);
        src += pitch;
        dst += BMP_WIDTH;
    }
}

void BMP_loadGenBmp16(const u16 *genbmp16, u32 x, u32 y, u16 numpal)
{
    u32 w, h;

    // get the image width
    w = genbmp16[0];
    // get the image height
    h = genbmp16[1];

    // load the palette
    if (numpal < 4) VDP_setPalette(numpal, &genbmp16[2]);

    BMP_loadBitmap((u8*) &genbmp16[18], x, y, w / 2, h, w / 2);
}

void BMP_loadAndScaleGenBmp16(const u16 *genbmp16, u32 x, u32 y, u16 w, u16 h, u16 numpal)
{
    u16 bmp_w, bmp_h;

    // get the image width / 2
    bmp_w = genbmp16[0] >> 1;
    // get the image height
    bmp_h = genbmp16[1];

    // load the palette
    if (numpal < 4) VDP_setPalette(numpal, &genbmp16[2]);

    BMP_scale((u8*) &genbmp16[18], bmp_w, bmp_h, bmp_w, BMP_getWritePointer(x, y), w, h, BMP_WIDTH);
}

void BMP_getGenBmp16Palette(const u16 *genbmp16, u16 *pal)
{
    u16 i;
    const u16 *src;
    u16 *dst;

    src = &genbmp16[2];
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


static void calculatePolyEdge(const Vect2D_s16 *pt1, const Vect2D_s16 *pt2, u8 clockwise)
{
    s16 dx, dy;
    s16 x1 = pt1->x;
    s16 y1 = pt1->y;
    s16 x2 = pt2->x;
    s16 y2 = pt2->y;

    // calcul deltas
    dx = x2 - x1;
    dy = y2 - y1;

    // nothing to do
    if (dy == 0) return;

    // clip on y window
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

    // outside screen ? --> exit
    if (((x1 < 0) && (x2 < 0)) ||
        ((x1 >= BMP_WIDTH) && (x2 >= BMP_WIDTH)) ||
        ((y1 < 0) && (y2 < 0)) ||
        ((y1 >= BMP_HEIGHT) && (y2 >= BMP_HEIGHT))) return;

    s16 *src;
    fix16 step;
    s16 len;
    fix16 x;

    if ((y2 > y1) ^ (clockwise))
    {
        // left side
        len = y2 - y1;
        step = fix16Div(intToFix16(x2 - x1), intToFix16(len));
        if (y1 < minY) minY = y1;
        if (y2 > maxY) maxY = y2;
        src = &LeftPoly[y1];
        x = intToFix16(x1);
    }
    else
    {
        // right side
        len = y1 - y2;
        step = fix16Div(intToFix16(x1 - x2), intToFix16(len));
        if (y2 < minY) minY = y2;
        if (y1 > maxY) maxY = y1;
        src = &RightPoly[y2];
        x = intToFix16(x2);
    }

    while(len--)
    {
        *src++ = fix16ToInt(x);
        x += step;
    }
}

static void drawLine(u32 offset, s16 dx, s16 dy, u32 step_x, u32 step_y, u8 col)
{
    const u8 c = col;
    u8 *dst = &bmp_buffer_write[offset];
    s16 delta = dx;
    s16 cnt = dx;

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


#endif // ENABLE_BMP
