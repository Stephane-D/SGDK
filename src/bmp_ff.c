#include "config.h"
#include "types.h"

#include "bmp_cmn.h"
#include "bmp_intr.h"
#include "bmp_ff.h"

#include "sys.h"
#include "memory.h"
#include "tools.h"
#include "maths3D.h"

#include "vdp.h"
#include "vdp_tile.h"
#include "vdp_pal.h"
#include "vdp_bg.h"

#include "tab_vram.h"
#include "tab_bmp.h"

#define DEFAULT_TILE_DATA       TILE_ATTR_FULL(0, 0, 0, 0, TILE_SYSTEMINDEX)


#if ((BMP_TABLES != 0) && (VRAM_TABLE != 0))

// we don't want to share them
extern u32 VIntProcess;
extern u32 HIntProcess;

extern s16 *LeftPoly;
extern s16 *RightPoly;
extern s16 minY;
extern s16 maxY;


static u16 *bmp_tilemap_0 = NULL;
static u16 *bmp_tilemap_1 = NULL;

u16 *bmp_tilemap_read;
u16 *bmp_tilemap_write;

static u16 basetile_ind;


// ASM procedures (defined in bmp_ff_a.s)
extern void blitTileMap();
extern u16 getTile(u16 offset);
extern u8 isUserTile(u16 offset);
extern void setUserTile(u16 offset);
extern void drawLine(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col);

// forward
extern void calculatePolyEdge(const Vect2D_s16 *pt1, const Vect2D_s16 *pt2, u8 clockwise);
extern void calculatePolyEdge_old(const Vect2D_s16 *pt1, const Vect2D_s16 *pt2, u8 clockwise);

static u16 doBlitNorm();
static u16 doBlitBlank();
static u16 doBlitBlankExt();
static void internalBufferFlip();
// replaced by ASM version (see bmp_ff_a.s file)
static void blitTileMap_old();
static u16 getTile_old(u16 offset);
static u8 isUserTile_old(u16 offset);
static void setUserTile_old(u16 offset);
static void drawLine_old(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col);


void BMP_FF_init(u16 flags)
{
    // common bmp init tasks and memory allocation
    _bmp_init();

    // release first if needed
    if (bmp_tilemap_0) MEM_free(bmp_tilemap_0);
    if (bmp_tilemap_1) MEM_free(bmp_tilemap_1);

    // tile map allocation
    bmp_tilemap_0 = MEM_alloc(BMP_CELLWIDTH * BMP_CELLHEIGHT * sizeof(u16));
    bmp_tilemap_1 = MEM_alloc(BMP_CELLWIDTH * BMP_CELLHEIGHT * sizeof(u16));

    // default
    bmp_tilemap_read = bmp_tilemap_0;
    bmp_tilemap_write = bmp_tilemap_1;
    basetile_ind = BMP_FB1TILEINDEX;

    // do some init process
    BMP_FF_setFlags(0);

    // first init, clear and flip
    BMP_FF_clear();
    BMP_FF_flip();
    // second init, clear and flip for correct init (double buffer)
    BMP_FF_clear();
    BMP_FF_flip();

    BMP_FF_setFlags(flags);
}

void BMP_FF_end()
{
    // end some stuff
    BMP_FF_setFlags(0);

    // release tile map
    if (bmp_tilemap_0)
    {
        MEM_free(bmp_tilemap_0);
        bmp_tilemap_0 = NULL;
    }

    if (bmp_tilemap_1)
    {
        MEM_free(bmp_tilemap_1);
        bmp_tilemap_1 = NULL;
    }

    // release others stuff
    _bmp_end();
}

void BMP_FF_reset()
{
    BMP_FF_init(bmp_flags);
}


void BMP_FF_setFlags(u16 value)
{
    const u16 oldf = bmp_flags;

    // common setFlags tasks (also handle flags constraints)
    _bmp_setFlags(value);

    // if internals flags hasn't changed --> exit
    if (bmp_flags == oldf) return;

    // re enabled VDP if it was disabled because of extended blank
    if (oldf & BMP_ENABLE_EXTENDEDBLANK)
        VDP_setEnable(1);

    // async blit (H Int processing)
    if (HAS_FLAG(BMP_ENABLE_ASYNCFLIP))
    {
        const u16 scrh = VDP_getScreenHeight();

        if (HAS_FLAG(BMP_ENABLE_EXTENDEDBLANK))
        {
            VDP_setHIntCounter(scrh - (((scrh - BMP_HEIGHT) >> 1) + 1));
            // blit method
            doBlit = &doBlitBlankExt;
        }
        else
        {
            VDP_setHIntCounter(scrh - 1);

            // blit method
            if (HAS_FLAG(BMP_ENABLE_BLITONBLANK)) doBlit = &doBlitBlank;
            else doBlit = &doBlitNorm;
        }

        // enabled bitmap H Int processing
        HIntProcess |= PROCESS_BITMAP_TASK;
        VDP_setHInterrupt(1);
    }
    else
    {
        // normal blit
        doBlit = &doBlitNorm;
        // disabled bitmap H Int processing
        VDP_setHInterrupt(0);
        HIntProcess &= ~PROCESS_BITMAP_TASK;
    }
}

void BMP_FF_enableWaitVSync()
{
    if (!HAS_FLAG(BMP_ENABLE_WAITVSYNC))
        BMP_FF_setFlags(bmp_flags | BMP_ENABLE_WAITVSYNC);
}

void BMP_FF_disableWaitVSync()
{
    if (HAS_FLAG(BMP_ENABLE_WAITVSYNC))
        BMP_FF_setFlags(bmp_flags & ~BMP_ENABLE_WAITVSYNC);
}

void BMP_FF_enableASyncFlip()
{
    if (!HAS_FLAG(BMP_ENABLE_ASYNCFLIP))
        BMP_FF_setFlags(bmp_flags | BMP_ENABLE_ASYNCFLIP);
}

void BMP_FF_disableASyncFlip()
{
    if (HAS_FLAG(BMP_ENABLE_ASYNCFLIP))
        BMP_FF_setFlags(bmp_flags & ~BMP_ENABLE_ASYNCFLIP);
}

void BMP_FF_enableFPSDisplay()
{
    if (!HAS_FLAG(BMP_ENABLE_FPSDISPLAY))
        BMP_FF_setFlags(bmp_flags | BMP_ENABLE_FPSDISPLAY);
}

void BMP_FF_disableFPSDisplay()
{
    if (HAS_FLAG(BMP_ENABLE_FPSDISPLAY))
        BMP_FF_setFlags(bmp_flags & ~BMP_ENABLE_FPSDISPLAY);
}

void BMP_FF_enableBlitOnBlank()
{
    if (!HAS_FLAG(BMP_ENABLE_BLITONBLANK))
        BMP_FF_setFlags(bmp_flags | BMP_ENABLE_BLITONBLANK);
}

void BMP_FF_disableBlitOnBlank()
{
    if (HAS_FLAG(BMP_ENABLE_BLITONBLANK))
        BMP_FF_setFlags(bmp_flags & ~BMP_ENABLE_BLITONBLANK);
}

void BMP_FF_enableExtendedBlank()
{
    if (!HAS_FLAG(BMP_ENABLE_EXTENDEDBLANK))
        BMP_FF_setFlags(bmp_flags | BMP_ENABLE_EXTENDEDBLANK);
}

void BMP_FF_disableExtendedBlank()
{
    if (bmp_flags & BMP_ENABLE_EXTENDEDBLANK)
        BMP_FF_setFlags(bmp_flags & ~BMP_ENABLE_EXTENDEDBLANK);
}


void BMP_FF_flip()
{
    // wait for vsync ?
    if (HAS_FLAG(BMP_ENABLE_WAITVSYNC))
    {
        // async flip ?
        if (HAS_FLAG(BMP_ENABLE_ASYNCFLIP))
        {
            // wait for previous async flip to complete
            BMP_waitAsyncFlipComplete();
            // flip bitmap buffer
            internalBufferFlip();
            // request a flip (will be processed in blank period --> BMP_doBlankProcess)
            bmp_state |= BMP_STAT_FLIPWAITING;
        }
        else
        {
            VDP_waitVSync();
            // flip bitmap buffer
            internalBufferFlip();
            // blit buffer to VRAM and flip vdp display
            _bmp_doFlip();
        }
    }
    else
    {
        // flip bitmap buffer
        internalBufferFlip();
        // blit buffer to VRAM and flip vdp display
        _bmp_doFlip();
    }
}

static void internalBufferFlip()
{
    if (READ_IS_FB0)
    {
        bmp_tilemap_read = bmp_tilemap_1;
        bmp_buffer_read = bmp_buffer_1;
        bmp_tilemap_write = bmp_tilemap_0;
        bmp_buffer_write = bmp_buffer_0;
        basetile_ind = BMP_FB0TILEINDEX;
    }
    else
    {
        bmp_tilemap_read = bmp_tilemap_0;
        bmp_buffer_read = bmp_buffer_0;
        bmp_tilemap_write = bmp_tilemap_1;
        bmp_buffer_write = bmp_buffer_1;
        basetile_ind = BMP_FB1TILEINDEX;
    }
}

// replaced by ASM version (see bmp_ff_a.s file)
static void blitTileMap_old()
{
    vu32 *plctrl;
    vu32 *pldata;
    u32 *src;
    const u32 *vramwrite_addr;
    u16 i, j;

    // calculated
    const u32 offset = BMP_FBTILEMAP_OFFSET;

    if (READ_IS_FB0)
        vramwrite_addr = &vramwrite_tab[BMP_FB0TILEMAP_BASE + offset];
    else
        vramwrite_addr = &vramwrite_tab[BMP_FB1TILEMAP_BASE + offset];

    src = (u32*) bmp_tilemap_read;

    // point to vdp port
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    i = BMP_CELLHEIGHT;

    while(i--)
    {
        // set destination address for tilemap
        *plctrl = *vramwrite_addr;

        j = BMP_CELLWIDTH >> 4;

        while(j--)
        {
            // write tilemap to VDP
            // warning : GCC can't optimize correctly this part of code :(
            *pldata = *src++;
            *pldata = *src++;
            *pldata = *src++;
            *pldata = *src++;
            *pldata = *src++;
            *pldata = *src++;
            *pldata = *src++;
            *pldata = *src++;
        }

        vramwrite_addr += BMP_PLANWIDTH * 2;
    }
}


static u16 doBlitNorm()
{
    vu32 *plctrl;
    vu32 *pldata;
    u32 *src;
    u16 *srcmap;
    const u32 *vramwrite_addr;
    u16 i, j;

    VDP_setAutoInc(2);

    // copy tilemap
    blitTileMap();

    if (READ_IS_FB0)
        vramwrite_addr = &vramwrite_tab[BMP_FB0TILE];
    else
        vramwrite_addr = &vramwrite_tab[BMP_FB1TILE];

    // point to vdp port
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    src = (u32 *) bmp_buffer_read;
    srcmap = bmp_tilemap_read;

    i = BMP_CELLHEIGHT;

    while(i--)
    {
        j = BMP_CELLWIDTH;

        while(j--)
        {
            // are we using a user tile ?
            if (*srcmap++ >= BMP_BASETILEINDEX)
            {
                // force set destination address for tile
                *plctrl = *vramwrite_addr;

                // send it to VRAM
                *pldata = src[(BMP_PITCH * 0) / 4];
                *pldata = src[(BMP_PITCH * 1) / 4];
                *pldata = src[(BMP_PITCH * 2) / 4];
                *pldata = src[(BMP_PITCH * 3) / 4];
                *pldata = src[(BMP_PITCH * 4) / 4];
                *pldata = src[(BMP_PITCH * 5) / 4];
                *pldata = src[(BMP_PITCH * 6) / 4];
                *pldata = src[(BMP_PITCH * 7) / 4];
            }

            vramwrite_addr += 32;
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
    u16 *srcmap;
    const u32 *vramwrite_addr;
    u16 i, j;

    static u16 save_i, save_j;

    VDP_setAutoInc(2);

    if (READ_IS_FB0)
        vramwrite_addr = &vramwrite_tab[BMP_FB0TILE];
    else
        vramwrite_addr = &vramwrite_tab[BMP_FB1TILE];

    // point to vdp port
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    src = (u32 *) bmp_buffer_read;
    srcmap = bmp_tilemap_read;

    // previous blit not completed ?
    if (bmp_state & BMP_STAT_BLITTING)
    {
        const u16 done_i = BMP_CELLHEIGHT - save_i;
        const u16 done_j = BMP_CELLWIDTH - save_j;

        // adjust src pointer
        src += (done_i * (BMP_YPIXPERTILE * (BMP_PITCH / 4))) + done_j;

        const u16 off = (done_i * BMP_CELLWIDTH) + done_j;

        // adjust tile address
        vramwrite_addr += off * 32;
        // adjust srcmap pointer
        srcmap += off;

        // restore j position
        j = save_j;

        while(j--)
        {
            // are we using a user tile ?
            if (*srcmap++ >= BMP_BASETILEINDEX)
            {
                // force set destination address for tile
                *plctrl = *vramwrite_addr;

                // send it to VRAM
                *pldata = src[(BMP_PITCH * 0) / 4];
                *pldata = src[(BMP_PITCH * 1) / 4];
                *pldata = src[(BMP_PITCH * 2) / 4];
                *pldata = src[(BMP_PITCH * 3) / 4];
                *pldata = src[(BMP_PITCH * 4) / 4];
                *pldata = src[(BMP_PITCH * 5) / 4];
                *pldata = src[(BMP_PITCH * 6) / 4];
                *pldata = src[(BMP_PITCH * 7) / 4];
            }

            vramwrite_addr += 32;
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

        // copy tilemap
        blitTileMap();

        i = BMP_CELLHEIGHT;
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

            // are we using a user tile ?
            if (*srcmap++ >= BMP_BASETILEINDEX)
            {
                // force set destination address for tile
                *plctrl = *vramwrite_addr;

                // send it to VRAM
                *pldata = src[(BMP_PITCH * 0) / 4];
                *pldata = src[(BMP_PITCH * 1) / 4];
                *pldata = src[(BMP_PITCH * 2) / 4];
                *pldata = src[(BMP_PITCH * 3) / 4];
                *pldata = src[(BMP_PITCH * 4) / 4];
                *pldata = src[(BMP_PITCH * 5) / 4];
                *pldata = src[(BMP_PITCH * 6) / 4];
                *pldata = src[(BMP_PITCH * 7) / 4];
            }

            vramwrite_addr += 32;
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
    u32 *src;
    u16 *srcmap;
    const u32 *vramwrite_addr;
    u16 i, j;

    static u16 save_i, save_j;

    VDP_setAutoInc(2);

    if (READ_IS_FB0)
        vramwrite_addr = &vramwrite_tab[BMP_FB0TILE];
    else
        vramwrite_addr = &vramwrite_tab[BMP_FB1TILE];

    // point to vdp port
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    src = (u32 *) bmp_buffer_read;
    srcmap = bmp_tilemap_read;

    // previous blit not completed ?
    if (bmp_state & BMP_STAT_BLITTING)
    {
        const u16 done_i = BMP_CELLHEIGHT - save_i;
        const u16 done_j = BMP_CELLWIDTH - save_j;

        // adjust src pointer
        src += (done_i * (BMP_YPIXPERTILE * (BMP_PITCH / 4))) + done_j;

        const u16 off = (done_i * BMP_CELLWIDTH) + done_j;

        // adjust tile address
        vramwrite_addr += off * 32;
        // adjust srcmap pointer
        srcmap += off;

        // restore j position
        j = save_j;

        while(j--)
        {
            // are we using a user tile ?
            if (*srcmap++ >= BMP_BASETILEINDEX)
            {
                // set destination address for tile
                *plctrl = *vramwrite_addr;

                // send it to VRAM
                *pldata = src[(BMP_PITCH * 0) / 4];
                *pldata = src[(BMP_PITCH * 1) / 4];
                *pldata = src[(BMP_PITCH * 2) / 4];
                *pldata = src[(BMP_PITCH * 3) / 4];
                *pldata = src[(BMP_PITCH * 4) / 4];
                *pldata = src[(BMP_PITCH * 5) / 4];
                *pldata = src[(BMP_PITCH * 6) / 4];
                *pldata = src[(BMP_PITCH * 7) / 4];
            }

            vramwrite_addr += 32;
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

        // copy tilemap
        blitTileMap();

        i = BMP_CELLHEIGHT;
    }

    // point to V counter
    const vu8 *pbvcnt = (u8 *) GFX_HVCOUNTER_PORT;
    // scanline where we have to stop blit (end of H Blank)
    const u8 startLine = ((VDP_getScreenHeight() - BMP_HEIGHT) >> 1) - 2;

    while(i--)
    {
        j = BMP_CELLWIDTH;

        while(j--)
        {
            // (! this loop have to take less than 488 cycles else we can miss the check !)

            // blank period is finish ?
            if (*pbvcnt == startLine)
            {
                // save current position and exit
                save_i = i + 1;
                save_j = j + 1;
                return 0;
            }

            // are we using a user tile ?
            if (*srcmap++ >= BMP_BASETILEINDEX)
            {
                // set destination address for tile
                *plctrl = *vramwrite_addr;

                // send it to VRAM
                *pldata = src[(BMP_PITCH * 0) / 4];
                *pldata = src[(BMP_PITCH * 1) / 4];
                *pldata = src[(BMP_PITCH * 2) / 4];
                *pldata = src[(BMP_PITCH * 3) / 4];
                *pldata = src[(BMP_PITCH * 4) / 4];
                *pldata = src[(BMP_PITCH * 5) / 4];
                *pldata = src[(BMP_PITCH * 6) / 4];
                *pldata = src[(BMP_PITCH * 7) / 4];
            }

            vramwrite_addr += 32;
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


void BMP_FF_clear()
{
    u32 *src;
    u16 i;
//    const u32 v = (DEFAULT_TILE_DATA << 0) | (DEFAULT_TILE_DATA << 16);
    // prevent compiler to use the slow CLR instruction
    const u32 v = getZeroU32();

    src = (u32*) bmp_tilemap_write;

    i = (BMP_CELLWIDTH * BMP_CELLHEIGHT) / (2 * 8);

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


u8 BMP_FF_getPixel(u16 x, u16 y)
{
    // pixel in screen ?
    if ((x < BMP_WIDTH) && (y < BMP_HEIGHT))
    {
        const u16 offset = (y * BMP_PITCH) + x;
        const u16 tile = getTile(offset);

        if (tile >= TILE_USERINDEX)
            return bmp_buffer_write[offset];
        else
            // tile index = palette color entry
            return tile;
    }

    return 0;
}

void BMP_FF_setPixel(u16 x, u16 y, u8 col)
{
    // pixel in screen ?
    if ((x < BMP_WIDTH) && (y < BMP_HEIGHT))
    {
        const u16 off = (y * BMP_PITCH) + x;

        // set user tile
        setUserTile(off);
        // write pixel
        bmp_buffer_write[off] = col;
    }
}

void BMP_FF_setPixels_V2D(const Vect2D_u16 *crd, u8 col, u16 num)
{
    const u8 c = col;
    const Vect2D_u16 *v;
    u8* dst = bmp_buffer_write;
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

            // set user tile
            setUserTile(off);
            // write pixel
            dst[off] = c;
        }

        // next pixel
        v++;
    }
}

void BMP_FF_setPixels(const Pixel *pixels, u16 num)
{
    const Pixel *p;
    u8* dst = bmp_buffer_write;
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

            // set user tile
            setUserTile(off);
            // write pixel
            dst[off] = p->col;
        }

        // next pixel
        p++;
    }
}


void BMP_FF_drawLine(Line *l)
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

        // draw line
        if (dx < dy)
            drawLine(offset, dy, dx, step_y, step_x, l->col);
        else
            drawLine(offset, dx, dy, step_x, step_y, l->col);
    }
}


// Replaced by ASM version (see bmp_ff_a.s file)
static u16 getTile_old(u16 offset)
{
    // user tile ?
    return bmp_tilemap_write[offset2tile[offset]];
}

// Replaced by ASM version (see bmp_ff_a.s file)
static u8 isUserTile_old(u16 offset)
{
    // user tile ?
    return (bmp_tilemap_write[offset2tile[offset]] >= TILE_USERINDEX);
}

// Replaced by ASM version (see bmp_ff_a.s file)
static void setUserTile_old(u16 offset)
{
    const u16 tile_ind = offset2tile[offset];
    u16* tile = &bmp_tilemap_write[tile_ind];

    // not an user tile ?
    if (*tile < TILE_USERINDEX)
    {
        // set it as user tile
        *tile = basetile_ind + tile_ind;

        // clear the user tile
        u32 *pix = (u32*) &bmp_buffer_write[offset & ~((BMP_YPIXPERTILEMASK * BMP_WIDTH) | BMP_XPIXPERTILEMASK)];

        pix[(BMP_PITCH * 0) / 4] = 0;
        pix[(BMP_PITCH * 1) / 4] = 0;
        pix[(BMP_PITCH * 2) / 4] = 0;
        pix[(BMP_PITCH * 3) / 4] = 0;
        pix[(BMP_PITCH * 4) / 4] = 0;
        pix[(BMP_PITCH * 5) / 4] = 0;
        pix[(BMP_PITCH * 6) / 4] = 0;
        pix[(BMP_PITCH * 7) / 4] = 0;
    }
}


// Replaced by ASM version (see bmp_ff_a.s file)
static void drawLine_old(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col)
{
    const u8 c = col;
    u8 *dst = bmp_buffer_write;
    u16 prev_off = -1;
    u16 off = offset;
    s16 delta = dx;
    s16 cnt = dx;

    while(cnt--)
    {
        // new tile ? change to user tile if needed
        if (((prev_off ^ off) & ~((BMP_YPIXPERTILEMASK * BMP_WIDTH) | BMP_XPIXPERTILEMASK)) != 0)
        {
            const u16 tile_ind = offset2tile[off];
            const u16 usr_tile = basetile_ind + tile_ind;
            u16 *tile = &bmp_tilemap_write[tile_ind];

            // not an user tile ?
            if (*tile != usr_tile)
            {
                // set it as user tile
                *tile = usr_tile;

                // clear the user tile
                u32 *pix = (u32*) &bmp_buffer_write[off & ~((BMP_YPIXPERTILEMASK * BMP_WIDTH) | BMP_XPIXPERTILEMASK)];

                pix[(BMP_PITCH * 0) / 4] = 0;
                pix[(BMP_PITCH * 1) / 4] = 0;
                pix[(BMP_PITCH * 2) / 4] = 0;
                pix[(BMP_PITCH * 3) / 4] = 0;
                pix[(BMP_PITCH * 4) / 4] = 0;
                pix[(BMP_PITCH * 5) / 4] = 0;
                pix[(BMP_PITCH * 6) / 4] = 0;
                pix[(BMP_PITCH * 7) / 4] = 0;
            }

        }

        // write pixel
        dst[off] = c;

        // save previous offset
        prev_off = off;
        // adjust offset
        off += step_x;

        if ((delta -= dy) < 0)
        {
            off += step_y;
            delta += dx;
        }
    }
}

#endif
