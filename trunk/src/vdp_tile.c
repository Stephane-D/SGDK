#include "config.h"
#include "types.h"

#include "font.h"
#include "vdp.h"
#include "vdp_tile.h"

#include "tools.h"
#include "vdp_dma.h"

#include "tab_cnv.h"


void VDP_loadFont(const u32 *font, u8 use_dma)
{
    VDP_loadTileData(font, TILE_FONTINDEX, FONT_LEN, use_dma);
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
        u16 i;

        VDP_setAutoInc(2);

        /* point to vdp port */
        plctrl = (u32 *) GFX_CTRL_PORT;
        pldata = (u32 *) GFX_DATA_PORT;

        *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        const u32 data32 = cnv_8to32_tab[value];

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


void VDP_setTileMapByIndex(u16 plan, u16 tile, u16 ind)
{
    vu32 *plctrl;
    vu16 *pwdata;

    const u32 addr = plan + (ind * 2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    *plctrl = GFX_WRITE_VRAM_ADDR(addr);
    *pwdata = tile;
}

void VDP_setTileMap(u16 plan, u16 tile, u16 x, u16 y)
{
    vu32 *plctrl;
    vu16 *pwdata;

    const u32 addr = plan + ((x + (VDP_getPlanWidth() * y)) * 2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    *plctrl = GFX_WRITE_VRAM_ADDR(addr);
    *pwdata = tile;
}

void VDP_fillTileMapRectByIndex(u16 plan, u16 tile, u16 ind, u16 num)
{
    vu32 *plctrl;
    vu16 *pwdata;
    vu32 *pldata;
    u32 addr;
    u16 i;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    addr = plan + (ind * 2);

    *plctrl = GFX_WRITE_VRAM_ADDR(addr);

    const u32 tile32 = (tile << 16) | tile;

    i = num >> 3;
    while (i--)
    {
        *pldata = tile32;
        *pldata = tile32;
        *pldata = tile32;
        *pldata = tile32;
    }

    pwdata = (u16 *) GFX_DATA_PORT;

    i = num & 7;
    while (i--) *pwdata = tile;
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

void VDP_clearTileMapRectByIndex(u16 plan, u16 ind, u16 num, u8 use_dma)
{
    if (use_dma)
    {
        // wait for previous DMA completion
        VDP_waitDMACompletion();
        // then do DMA
        VDP_doVRamDMAFill(plan + (ind * 2), num * 2, 0);
    }
    else VDP_fillTileMapRectByIndex(plan, 0, ind, num);
}

void VDP_clearTileMapRect(u16 plan, u16 x, u16 y, u16 w, u16 h)
{
    VDP_fillTileMapRect(plan, 0, x, y, w, h);
}

void VDP_fillTileMapRectIncByIndex(u16 plan, u16 basetile, u16 ind, u16 num)
{
    vu32 *plctrl;
    vu16 *pwdata;
    vu32 *pldata;
    u32 addr;
    u16 tile;
    u32 tile32;
    u32 step;
    u16 i;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    addr = plan + (ind * 2);

    *plctrl = GFX_WRITE_VRAM_ADDR(addr);

    tile = basetile;
    tile32 = (tile << 16) | tile;
    step = 0x10001;

    i = num >> 3;
    while (i--)
    {
        *pldata = tile32;
        tile32 += step;
        *pldata = tile32;
        tile32 += step;
        *pldata = tile32;
        tile32 += step;
        *pldata = tile32;
        tile32 += step;
    }

    pwdata = (vu16 *) GFX_DATA_PORT;

    i = num & 7;
    while (i--) *pwdata = tile++;
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

void VDP_setTileMapRectByIndex(u16 plan, const u16 *data, u16 ind, u16 num, u8 use_dma)
{
    if (use_dma)
    {
        // wait for previous DMA completion
        VDP_waitDMACompletion();
        // then do DMA
        VDP_doVRamDMA((u32) data, plan + (ind * 2), num);
    }
    else
    {
        vu32 *plctrl;
        vu16 *pwdata;
        vu32 *pldata;
        const u16 *src;
        const u32 *src32;
        u32 addr;
        u16 i;

        VDP_setAutoInc(2);

        /* point to vdp port */
        plctrl = (u32 *) GFX_CTRL_PORT;
        pldata = (u32 *) GFX_DATA_PORT;

        addr = plan + (ind * 2);

        *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        src32 = (u32*) data;

        i = num >> 3;
        while (i--)
        {
            *pldata = *src32++;
            *pldata = *src32++;
            *pldata = *src32++;
            *pldata = *src32++;
        }

        pwdata = (u16 *) GFX_DATA_PORT;

        src = (u16*) src32;

        i = num & 7;
        while (i--) *pwdata = *src++;
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

void VDP_setTileMapRectExByIndex(u16 plan, const u16 *data, u16 baseindex, u16 baseflags, u16 ind, u16 num)
{
    vu32 *plctrl;
    vu16 *pwdata;
    vu32 *pldata;
    const u16 *src;
    const u32 *src32;
    u32 addr;
    u32 bf32;
    u32 bi32;
    u16 i;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    addr = plan + (ind * 2);

    *plctrl = GFX_WRITE_VRAM_ADDR(addr);

    src32 = (u32*) data;
    bf32 = (baseflags << 16) | baseflags;
    bi32 = (baseindex << 16) | baseindex;

    i = num >> 3;
    while (i--)
    {
        *pldata = bf32 | (*src32++ + bi32);
        *pldata = bf32 | (*src32++ + bi32);
        *pldata = bf32 | (*src32++ + bi32);
        *pldata = bf32 | (*src32++ + bi32);
    }

    pwdata = (u16 *) GFX_DATA_PORT;

    src = (u16*) src32;

    i = num & 7;
    while (i--) *pwdata = baseflags | (*src++ + baseindex);
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
