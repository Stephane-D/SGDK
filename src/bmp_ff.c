#include "config.h"
#include "types.h"

#include "bmp_cmn.h"
#include "bmp_intr.h"
#include "bmp_ff.h"

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


static u16 bmp_tilemap_0[BMP_CELLWIDTH * BMP_CELLHEIGHT];
static u16 bmp_tilemap_1[BMP_CELLWIDTH * BMP_CELLHEIGHT];

u16 *bmp_tilemap_read;
u16 *bmp_tilemap_write;

static u16 basetile_ind;


// forward
static u16 doBlitNorm_FF();
static u16 doBlitBlank_FF();
static u16 doBlitBlankExt_FF();
static void doBufferFlip_FF();

static void changeToUserTile(u32 offset);
static void drawLineFF(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col);


void BMP_FF_init(u16 flags)
{
    // common bmp init tasks
    _bmp_init();

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
}

void BMP_FF_reset()
{
    BMP_FF_init(bmp_flags);
}


void BMP_FF_setFlags(u16 value)
{
    if (bmp_flags == value) return;

    bmp_flags = value;

    // flag dependancies
    if (bmp_flags & BMP_ENABLE_EXTENDEDBLANK) bmp_flags |= BMP_ENABLE_BLITONBLANK;
    if (bmp_flags & BMP_ENABLE_BLITONBLANK) bmp_flags |= BMP_ENABLE_ASYNCFLIP;
    if (bmp_flags & BMP_ENABLE_ASYNCFLIP) bmp_flags |= BMP_ENABLE_WAITVSYNC;

    // clear pending task
    bmp_state &= ~(BMP_STAT_FLIPWAITING | BMP_STAT_BLITTING);

    // extended blank mode ?
    if HAS_FLAG(BMP_ENABLE_EXTENDEDBLANK)
    {
        // enable process on hint
        HBlankProcess |= PROCESS_BITMAP_TASK;
        VDP_setHInterrupt(1);
        // disable process on vhint
        VBlankProcess &= ~PROCESS_BITMAP_TASK;
        // define blit function
        doBlit = &doBlitBlankExt_FF;
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
        if HAS_FLAG(BMP_ENABLE_BLITONBLANK) doBlit = &doBlitBlank_FF;
        else doBlit = &doBlitNorm_FF;
    }
}


void BMP_FF_flip()
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
            doBufferFlip_FF();
            // request a flip (will be processed in blank period --> BMP_doBlankProcess)
            bmp_state |= BMP_STAT_FLIPWAITING;
        }
        else
        {
            VDP_waitVSync();
            // flip bitmap buffer
            doBufferFlip_FF();
            // blit buffer to VRAM and flip vdp display
            _bmp_doFlip();
        }
    }
    else
    {
         // flip bitmap buffer
         doBufferFlip_FF();
         // blit buffer to VRAM and flip vdp display
         _bmp_doFlip();
    }
}


static u16 doBlitNorm_FF()
{
    vu32 *plctrl;
    vu16 *pwdata;
    vu32 *pldata;
    u32 *src;
    u16 *srcmap;
    u32 addr_tile;
    u32 addr_tilemap;
    u16 i, j;

    VDP_setAutoInc(2);

    src = (u32 *) bmp_buffer_read;
    srcmap = bmp_tilemap_read;

    if READ_IS_FB0
    {
        addr_tile = BMP_FB0TILE;
        addr_tilemap = BMP_FB0TILEMAP_ADJ;
    }
    else
    {
        addr_tile = BMP_FB1TILE;
        addr_tilemap = BMP_FB1TILEMAP_ADJ;
    }

    // point to vdp port
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    i = BMP_CELLHEIGHT;
    while(i--)
    {
        // set destination address for tilemap
        *plctrl = GFX_WRITE_VRAM_ADDR(addr_tilemap);

        j = BMP_CELLWIDTH;
        while(j--)
        {
            // tile data
            const u16 tile = *srcmap;

            // write tile to VDP
            *pwdata = tile;
            addr_tilemap += 2;

            // are we using a user tile ?
            if (tile >= BMP_BASETILEINDEX)
            {
                // set destination address for tile
                *plctrl = GFX_WRITE_VRAM_ADDR(addr_tile);

                // send it to VRAM
                *pldata = src[(BMP_PITCH * 0) / 4];
                *pldata = src[(BMP_PITCH * 1) / 4];
                *pldata = src[(BMP_PITCH * 2) / 4];
                *pldata = src[(BMP_PITCH * 3) / 4];
                *pldata = src[(BMP_PITCH * 4) / 4];
                *pldata = src[(BMP_PITCH * 5) / 4];
                *pldata = src[(BMP_PITCH * 6) / 4];
                *pldata = src[(BMP_PITCH * 7) / 4];

                // set back destination address for tilemap
                *plctrl = GFX_WRITE_VRAM_ADDR(addr_tilemap);
            }

            addr_tile += 32;
            src++;
            srcmap++;
        }

        src += (7 * BMP_PITCH) / 4;
        addr_tilemap += (BMP_PLANWIDTH - BMP_CELLWIDTH) * 2;
    }

    return 1;
}

static u16 doBlitBlank_FF()
{
    vu32 *plctrl;
    vu16 *pwdata;
    vu32 *pldata;
    u32 *src;
    u16 *srcmap;
    u32 addr_tile;
    u32 addr_tilemap;
    u16 i, j;

    static u16 save_i, save_j;

    VDP_setAutoInc(2);

    src = (u32 *) bmp_buffer_read;
    srcmap = bmp_tilemap_read;

    if READ_IS_FB0
    {
        addr_tile = BMP_FB0TILE;
        addr_tilemap = BMP_FB0TILEMAP_ADJ;
    }
    else
    {
        addr_tile = BMP_FB1TILE;
        addr_tilemap = BMP_FB1TILEMAP_ADJ;
    }

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    // previous blit not completed ?
    if (bmp_state & BMP_STAT_BLITTING)
    {
        const u16 done_i = BMP_CELLHEIGHT - save_i;
        const u16 done_j = BMP_CELLWIDTH - save_j;
        const u16 off = (done_i * BMP_CELLWIDTH) + done_j;

        // adjust tile address
        addr_tile += off * 32;
        // adjust tilemap address
        addr_tilemap += ((done_i * BMP_PLANWIDTH) + done_j) * 2;
        // adjust src pointer
        src += (done_i * (BMP_YPIXPERTILE * (BMP_PITCH / 4))) + done_j;
        // adjust srcmap pointer
        srcmap += off;

        // restore j position
        j = save_j;

        // set destination address for tilemap
        *plctrl = GFX_WRITE_VRAM_ADDR(addr_tilemap);

        while(j--)
        {
            // tile data
            const u16 tile = *srcmap;

            // write tile to VDP
            *pwdata = tile;
            addr_tilemap += 2;

            // are we using a user tile ?
            if (tile >= BMP_BASETILEINDEX)
            {
                // set destination address for tile
                *plctrl = GFX_WRITE_VRAM_ADDR(addr_tile);

                // send it to VRAM
                *pldata = src[(BMP_PITCH * 0) / 4];
                *pldata = src[(BMP_PITCH * 1) / 4];
                *pldata = src[(BMP_PITCH * 2) / 4];
                *pldata = src[(BMP_PITCH * 3) / 4];
                *pldata = src[(BMP_PITCH * 4) / 4];
                *pldata = src[(BMP_PITCH * 5) / 4];
                *pldata = src[(BMP_PITCH * 6) / 4];
                *pldata = src[(BMP_PITCH * 7) / 4];

                // set back destination address for tilemap
                *plctrl = GFX_WRITE_VRAM_ADDR(addr_tilemap);
            }

            addr_tile += 32;
            src++;
            srcmap++;
        }

        src += (7 * (BMP_PITCH / 4));
        addr_tilemap += (BMP_PLANWIDTH - BMP_CELLWIDTH) * 2;

        // restore i position
        i = save_i - 1;
    }
    else
    {
        // start blit
        bmp_state |= BMP_STAT_BLITTING;
        i = BMP_CELLHEIGHT;
    }

    while(i--)
    {
        // set destination address for tilemap
        *plctrl = GFX_WRITE_VRAM_ADDR(addr_tilemap);

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

            // tile data
            const u16 tile = *srcmap;

            // write tile to VDP
            *pwdata = tile;
            addr_tilemap += 2;

            // are we using a user tile ?
            if (tile >= BMP_BASETILEINDEX)
            {
                // set destination address for tile
                *plctrl = GFX_WRITE_VRAM_ADDR(addr_tile);

                // send it to VRAM
                *pldata = src[(BMP_PITCH * 0) / 4];
                *pldata = src[(BMP_PITCH * 1) / 4];
                *pldata = src[(BMP_PITCH * 2) / 4];
                *pldata = src[(BMP_PITCH * 3) / 4];
                *pldata = src[(BMP_PITCH * 4) / 4];
                *pldata = src[(BMP_PITCH * 5) / 4];
                *pldata = src[(BMP_PITCH * 6) / 4];
                *pldata = src[(BMP_PITCH * 7) / 4];

                // set back destination address for tilemap
                *plctrl = GFX_WRITE_VRAM_ADDR(addr_tilemap);
            }

            addr_tile += 32;
            src++;
            srcmap++;
        }

        src += (7 * BMP_PITCH) / 4;
        addr_tilemap += (BMP_PLANWIDTH - BMP_CELLWIDTH) * 2;
    }

    // blit done
    bmp_state &= ~BMP_STAT_BLITTING;

    return 1;
}

static u16 doBlitBlankExt_FF()
{
    vu32 *plctrl;
    vu16 *pwdata;
    vu32 *pldata;
    vu16 *pwhvcnt;
    u32 *src;
    u16 *srcmap;
    u32 addr_tile;
    u32 addr_tilemap;
    u16 i, j;

    static u16 save_i, save_j;

    VDP_setAutoInc(2);

    src = (u32 *) bmp_buffer_read;
    srcmap = bmp_tilemap_read;
    if READ_IS_FB0
    {
        addr_tile = BMP_FB0TILE;
        addr_tilemap = BMP_FB0TILEMAP_ADJ;
    }
    else
    {
        addr_tile = BMP_FB1TILE;
        addr_tilemap = BMP_FB1TILEMAP_ADJ;
    }

    // point to vdp port
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    // previous blit not completed ?
    if (bmp_state & BMP_STAT_BLITTING)
    {
        const u16 done_i = BMP_CELLHEIGHT - save_i;
        const u16 done_j = BMP_CELLWIDTH - save_j;
        const u16 off = (done_i * BMP_CELLWIDTH) + done_j;

        // adjust tile address
        addr_tile += off * 32;
        // adjust tilemap address
        addr_tilemap += ((done_i * BMP_PLANWIDTH) + done_j) * 2;
        // adjust src pointer
        src += (done_i * (BMP_YPIXPERTILE * (BMP_PITCH / 4))) + done_j;
        // adjust srcmap pointer
        srcmap += off;

        // restore j position
        j = save_j;

        // set destination address for tilemap
        *plctrl = GFX_WRITE_VRAM_ADDR(addr_tilemap);

        while(j--)
        {
            // tile data
            const u16 tile = *srcmap;

            // write tile to VDP
            *pwdata = tile;
            addr_tilemap += 2;

            // are we using a user tile ?
            if (tile >= BMP_BASETILEINDEX)
            {
                // set destination address for tile
                *plctrl = GFX_WRITE_VRAM_ADDR(addr_tile);

                // send it to VRAM
                *pldata = src[(BMP_PITCH * 0) / 4];
                *pldata = src[(BMP_PITCH * 1) / 4];
                *pldata = src[(BMP_PITCH * 2) / 4];
                *pldata = src[(BMP_PITCH * 3) / 4];
                *pldata = src[(BMP_PITCH * 4) / 4];
                *pldata = src[(BMP_PITCH * 5) / 4];
                *pldata = src[(BMP_PITCH * 6) / 4];
                *pldata = src[(BMP_PITCH * 7) / 4];

                // set back destination address for tilemap
                *plctrl = GFX_WRITE_VRAM_ADDR(addr_tilemap);
            }

            addr_tile += 32;
            src++;
            srcmap++;
        }

        src += (7 * (BMP_PITCH / 4));
        addr_tilemap += (BMP_PLANWIDTH - BMP_CELLWIDTH) * 2;

        // restore i position
        i = save_i - 1;
    }
    else
    {
        // start blit
        bmp_state |= BMP_STAT_BLITTING;
        i = BMP_CELLHEIGHT;
    }

    // point to HV counter
    pwhvcnt = (u16 *) GFX_HVCOUNTER_PORT;

    while(i--)
    {
        // set destination address for tilemap
        *plctrl = GFX_WRITE_VRAM_ADDR(addr_tilemap);

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

            // tile data
            const u16 tile = *srcmap;

            // write tile to VDP
            *pwdata = tile;
            addr_tilemap += 2;

            // are we using a user tile ?
            if (tile >= BMP_BASETILEINDEX)
            {
                // set destination address for tile
                *plctrl = GFX_WRITE_VRAM_ADDR(addr_tile);

                // send it to VRAM
                *pldata = src[(BMP_PITCH * 0) / 4];
                *pldata = src[(BMP_PITCH * 1) / 4];
                *pldata = src[(BMP_PITCH * 2) / 4];
                *pldata = src[(BMP_PITCH * 3) / 4];
                *pldata = src[(BMP_PITCH * 4) / 4];
                *pldata = src[(BMP_PITCH * 5) / 4];
                *pldata = src[(BMP_PITCH * 6) / 4];
                *pldata = src[(BMP_PITCH * 7) / 4];

                // set back destination address for tilemap
                *plctrl = GFX_WRITE_VRAM_ADDR(addr_tilemap);
            }

            addr_tile += 32;
            src++;
            srcmap++;
        }

        src += (7 * BMP_PITCH) / 4;
        addr_tilemap += (BMP_PLANWIDTH - BMP_CELLWIDTH) * 2;
    }

    // blit done
    bmp_state &= ~BMP_STAT_BLITTING;

    return 1;
}

static void doBufferFlip_FF()
{
    if READ_IS_FB0
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


// graphic drawing functions
////////////////////////////

void BMP_FF_clear()
{
    fastmemset(bmp_tilemap_write, 0, sizeof(bmp_tilemap_0));
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
            drawLineFF(offset, dy, dx, step_y, step_x, l->col);
        else
            drawLineFF(offset, dx, dy, step_x, step_y, l->col);
    }
}

static void changeToUserTile(u32 offset)
{
    u16 tile_ind;
    u16 usr_tile;
    u16 *tile;

    tile_ind = (offset & (0x1F * (BMP_WIDTH * BMP_YPIXPERTILE))) >> (BMP_XPIXPERTILE_SFT + BMP_YPIXPERTILE_SFT);
    tile_ind |= (offset >> BMP_XPIXPERTILE_SFT) & BMP_CELLWIDTHMASK;
    tile = &bmp_tilemap_write[tile_ind];
    usr_tile = basetile_ind + tile_ind;

    // not an user tile ?
    if (*tile != usr_tile)
    {
        // set it as user tile
        *tile = usr_tile;

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

static void drawLineFF(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col)
{
    const u8 c = col;
    u8 *dst = bmp_buffer_write;
    u32 prev_off = -1;
    u32 off = offset;
    s16 delta = dx;
    s16 cnt = dx;

    while(cnt--)
    {
        // new tile ? change to user tile if needed
        if ((prev_off ^ off) & ~((BMP_YPIXPERTILEMASK * BMP_WIDTH) | BMP_XPIXPERTILEMASK))
        {
            u16 tile_ind;
            u16 usr_tile;
            u16 *tile;

            tile_ind = (off & (0x1F * (BMP_WIDTH * BMP_YPIXPERTILE))) >> (BMP_XPIXPERTILE_SFT + BMP_YPIXPERTILE_SFT);
            tile_ind |= (off >> BMP_XPIXPERTILE_SFT) & BMP_CELLWIDTHMASK;
            tile = &bmp_tilemap_write[tile_ind];
            usr_tile = basetile_ind + tile_ind;

            // not an user tile ?
            if (*tile != usr_tile)
            {
                // set it as user tile
                *tile = usr_tile;

                {
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
        }

        // save previous offset
        prev_off = off;

        // write pixel
        dst[off] = c;

        // adjust offset
        off += step_x;
        if ((delta -= dy) < 0)
        {
            off += step_y;
            delta += dx;
        }
    }
}



#endif // ENABLE_BMP
