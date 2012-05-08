#include "config.h"
#include "types.h"

#include "font.h"
#include "vdp.h"
#include "vdp_tile.h"

#include "tools.h"
#include "vdp_dma.h"

#include "tab_vram.h"


void VDP_loadTileData_old(const u32 *data, u16 index, u16 num, u8 use_dma)
{
    const u16 addr = index * 32;

    if (use_dma)
    {
        // wait for previous DMA completion
        VDP_waitDMACompletion();
        // then do DMA
        VDP_doVRamDMA((u32) data, addr, num * 32);
    }
    else
    {
        vu32 *plctrl;
        vu32 *pldata;
        const u32 *src;
        u16 i;

        VDP_setAutoInc(2);

        /* point to vdp port */
        plctrl = (u32 *) GFX_CTRL_PORT;
        pldata = (u32 *) GFX_DATA_PORT;

        *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        src = data;

        i = num;
        while(i--)
        {
            *pldata = *src++;
            *pldata = *src++;
            *pldata = *src++;
            *pldata = *src++;
            *pldata = *src++;
            *pldata = *src++;
            *pldata = *src++;
            *pldata = *src++;
        }
    }
}

void VDP_loadFont(const u32 *font, u8 use_dma)
{
    VDP_loadTileData(font, TILE_FONTINDEX, FONT_LEN, use_dma);
}

void VDP_loadBMPTileData_old(const u32 *data, u16 index, u16 w, u16 h, u16 bmp_w)
{
    vu32 *plctrl;
    vu32 *pldata;
    const u32 *src;
    u16 i, j;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    const u16 addr = index * 32;
    *plctrl = GFX_WRITE_VRAM_ADDR(addr);

    src = data;

    i = h;
    while(i--)
    {
        j = w;
        while(j--)
        {
            // send it to VRAM
            *pldata = *src;
            src += bmp_w;
            *pldata = *src;
            src += bmp_w;
            *pldata = *src;
            src += bmp_w;
            *pldata = *src;
            src += bmp_w;
            *pldata = *src;
            src += bmp_w;
            *pldata = *src;
            src += bmp_w;
            *pldata = *src;
            src += bmp_w;
            *pldata = *src;

            src -= (bmp_w * 7) - 1;
        }

        src += (bmp_w * 7) + (bmp_w - w);
    }
}

void VDP_loadBMPTileDataEx(const u32 *data, u16 index, u16 x, u16 y, u16 w, u16 h, u16 bmp_w)
{
    VDP_loadBMPTileData(&data[x + (y * bmp_w)], index, w, h, bmp_w);
}

void VDP_fillTileData(u8 value, u16 index, u16 num, u8 use_dma)
{
    u16 addr;

    addr = index * 32;
    if (use_dma)
    {
        // wait for previous DMA completion
        VDP_waitDMACompletion();
        // then do DMA
        VDP_doVRamDMAFill(addr, num * 32, value);
    }
    else
    {
        vu32 *plctrl;
        vu32 *pldata;
        u32 data32;
        u16 i;

        VDP_setAutoInc(2);

        /* point to vdp port */
        plctrl = (u32 *) GFX_CTRL_PORT;
        pldata = (u32 *) GFX_DATA_PORT;

        *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        data32 = value;
        data32 |= data32 << 8;
        data32 |= data32 << 16;

        i = num;
        while(i--)
        {
            *pldata = data32;
            *pldata = data32;
            *pldata = data32;
            *pldata = data32;
            *pldata = data32;
            *pldata = data32;
            *pldata = data32;
            *pldata = data32;
        }
    }
}


void VDP_setTileMap(u16 plan, u16 tile, u16 x, u16 y)
{
    vu32 *plctrl;
    vu16 *pwdata;

    const u32 addr = plan + (2 * (x + (VDP_getPlanWidth() * y)));

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    *plctrl = GFX_WRITE_VRAM_ADDR(addr);
    *pwdata = tile;
}

void VDP_fillTileMapRect(u16 plan, u16 tile, u16 x, u16 y, u16 w, u16 h)
{
    vu32 *plctrl;
    vu16 *pwdata;
    u32 addr;
    u32 planwidth;
    u16 i, j;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    planwidth = VDP_getPlanWidth();
    addr = plan + (2 * (x + (planwidth * y)));

    i = h;
    while (i--)
    {
	    *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        j = w;
        while (j--) *pwdata = tile;

		addr += planwidth * 2;
	}
}

void VDP_clearTileMapRect(u16 plan, u16 x, u16 y, u16 w, u16 h)
{
    VDP_fillTileMapRect(plan, 0, x, y, w, h);
}

void VDP_fillTileMapRectInc(u16 plan, u16 basetile, u16 x, u16 y, u16 w, u16 h)
{
    vu32 *plctrl;
    vu16 *pwdata;
    u32 addr;
    u32 planwidth;
    u16 tile;
    u16 i, j;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    planwidth = VDP_getPlanWidth();
    addr = plan + (2 * (x + (planwidth * y)));
    tile = basetile;

    i = h;
    while (i--)
    {
	    *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        j = w;
        while (j--) *pwdata = tile++;

		addr += planwidth * 2;
	}
}

void VDP_setTileMapRect(u16 plan, const u16 *data, u16 x, u16 y, u16 w, u16 h)
{
    vu32 *plctrl;
    vu16 *pwdata;
    const u16 *src;
    u32 addr;
    u32 planwidth;
    u16 i, j;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    planwidth = VDP_getPlanWidth();
    addr = plan + (2 * (x + (planwidth * y)));
    src = data;

    i = h;
    while (i--)
    {
	    *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        j = w;
        while (j--) *pwdata = *src++;

		addr += planwidth * 2;
	}
}

void VDP_setTileMapRectEx(u16 plan, const u16 *data, u16 baseindex, u16 baseflags, u16 x, u16 y, u16 w, u16 h)
{
    vu32 *plctrl;
    vu16 *pwdata;
    const u16 *src;
    u32 addr;
    u32 planwidth;
    u16 i, j;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    planwidth = VDP_getPlanWidth();
    addr = plan + (2 * (x + (planwidth * y)));
    src = data;

    i = h;
    while (i--)
    {
	    *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        j = w;
        while (j--) *pwdata = baseflags | (*src++ + baseindex);

		addr += planwidth * 2;
	}
}
