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

u16 basetile_ind;


// ASM procedures (defined in bmp_ff_a.s)
extern void blitTileMap();
extern u16 getTile(u16 offset);
extern u8 isUserTile(u16 offset);
extern void setUserTile(u16 offset);
extern void drawLine(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col);

// forward
extern void calculatePolyEdge(const Vect2D_s16 *pt1, const Vect2D_s16 *pt2, u8 clockwise);
extern void calculatePolyEdge_old(const Vect2D_s16 *pt1, const Vect2D_s16 *pt2, u8 clockwise);

static u16 doBlitInternal();
static void doFlipInternal();

// replaced by ASM version (see bmp_ff_a.s file)
static void blitTileMap_old();
static u16 getTile_old(u16 offset);
static u8 isUserTile_old(u16 offset);
static void setUserTile_old(u16 offset);
static void drawLine_old(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col);


void BMP_FF_init()
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

    // first init, clear and flip
    BMP_FF_clear();
    BMP_FF_flip();
    // second init, clear and flip for correct init (double buffer)
    BMP_FF_clear();
    BMP_FF_flip();

    // flip method
    doFlip = &doFlipInternal;
    // blit method
    doBlit = &doBlitInternal;

    const u16 vcnt = GET_VCOUNTER;
    const u16 scrh = VDP_getScreenHeight();

    // modify HIntCounter for extended blank blit
    VDP_setHIntCounter(scrh - (VDP_getHIntCounter() + vcnt + ((scrh - BMP_HEIGHT) >> 1) + 3));
    // enabled bitmap Int processing
    HIntProcess |= PROCESS_BITMAP_TASK;
    VIntProcess |= PROCESS_BITMAP_TASK;
    VDP_setHInterrupt(1);
}

void BMP_FF_end()
{
    // re enabled VDP if it was disabled because of extended blank
    VDP_setEnable(1);
    // disabled bitmap Int processing
    VDP_setHInterrupt(0);
    HIntProcess &= ~PROCESS_BITMAP_TASK;
    VIntProcess &= ~PROCESS_BITMAP_TASK;

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
    BMP_FF_init();
}


u16 BMP_FF_flip()
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
    doFlipInternal();
    // flip started (will be processed in blank period --> BMP_doBlankProcess)
    bmp_state |= BMP_STAT_FLIPPING;

    return 0;
}

static void doFlipInternal()
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


static u16 doBlitInternal()
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
