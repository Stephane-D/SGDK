#include "config.h"
#include "types.h"

#include "vdp.h"
#include "vdp_tile.h"

#include "tools.h"
#include "vdp_pal.h"
#include "vdp_dma.h"

#include "font.h"
#include "tab_cnv.h"
#include "memory.h"


u16 VDP_loadTileSet(const TileSet *tileset, u16 index, u8 use_dma)
{
    const u16 comp = tileset->compression;

    // compressed tileset ?
    if (comp != COMPRESSION_NONE)
    {
        // RLE compression ?
        if (comp == COMPRESSION_RLE)
        {
            // direct unpack tiles in vram
            rle4b_unpackVRam((u8*) tileset->tiles, index * 32);
        }
        else
        {
            // unpack first
            TileSet *t = unpackTileSet(tileset, NULL);

            if (t == NULL) return FALSE;

            // tiles
            VDP_loadTileData(t->tiles, index, t->numTile, use_dma);
            MEM_free(t);
        }
    }
    else
        // tiles
        VDP_loadTileData(tileset->tiles, index, tileset->numTile, use_dma);

    return TRUE;
}

void VDP_loadFontData(const u32 *font, u16 length, u8 use_dma)
{
    VDP_loadTileData(font, TILE_FONTINDEX, length, use_dma);
}

u16 VDP_loadFont(const TileSet *font, u8 use_dma)
{
    return VDP_loadTileSet(font, TILE_FONTINDEX, use_dma);
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


void VDP_setTileMap(u16 plan, u16 tile, u16 ind)
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

void VDP_setTileMapXY(u16 plan, u16 tile, u16 x, u16 y)
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
    VDP_fillTileMap(plan, tile, ind, num);
}

void VDP_fillTileMap(u16 plan, u16 tile, u16 ind, u16 num)
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
    VDP_clearTileMap(plan, ind, num, use_dma);
}

void VDP_clearTileMap(u16 plan, u16 ind, u16 num, u8 use_dma)
{
    if (use_dma)
    {
        // wait for previous DMA completion
        VDP_waitDMACompletion();
        // then do DMA
        VDP_doVRamDMAFill(plan + (ind * 2), num * 2, 0);
    }
    else VDP_fillTileMap(plan, 0, ind, num);
}

void VDP_clearTileMapRect(u16 plan, u16 x, u16 y, u16 w, u16 h)
{
    VDP_fillTileMapRect(plan, 0, x, y, w, h);
}

void VDP_fillTileMapRectIncByIndex(u16 plan, u16 basetile, u16 ind, u16 num)
{
    VDP_fillTileMapInc(plan, basetile, ind, num);
}

void VDP_fillTileMapInc(u16 plan, u16 basetile, u16 ind, u16 num)
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
    VDP_setTileMapData(plan, data, ind, num, use_dma);
}

void VDP_setTileMapData(u16 plan, const u16 *data, u16 ind, u16 num, u8 use_dma)
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

void VDP_setTileMapDataRect(u16 plan, const u16 *data, u16 x, u16 y, u16 w, u16 h)
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
    VDP_setTileMapDataEx(plan, data, baseflags | baseindex, ind, num);
}

void VDP_setTileMapDataEx(u16 plan, const u16 *data, u16 basetile, u16 ind, u16 num)
{
    vu32 *plctrl;
    vu16 *pwdata;
    vu32 *pldata;
    const u16 *src;
    const u32 *src32;
    u32 addr;
    u16 baseindex;
    u16 baseflags;
    u32 bi32;
    u32 bf32;
    u16 i;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    addr = plan + (ind * 2);

    *plctrl = GFX_WRITE_VRAM_ADDR(addr);

    src32 = (u32*) data;
    baseindex = basetile & TILE_INDEX_MASK;
    baseflags = basetile & TILE_ATTR_MASK;
    bi32 = (baseindex << 16) | baseindex;
    bf32 = (baseflags << 16) | baseflags;

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
    VDP_setTileMapDataRectEx(plan, data, baseflags | baseindex, x, y, w, h, w);
}

void VDP_setTileMapDataRectEx(u16 plan, const u16 *data, u16 basetile, u16 x, u16 y, u16 w, u16 h, u16 wm)
{
    vu32 *plctrl;
    vu16 *pwdata;
    const u16 *src;
    u32 addr;
    u32 planwidth;
    u16 baseindex;
    u16 baseflags;
    u16 i, j;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    planwidth = VDP_getPlanWidth();
    baseindex = basetile & TILE_INDEX_MASK;
    baseflags = basetile & TILE_ATTR_MASK;
    addr = plan + (2 * (x + (planwidth * y)));
    src = data;

    i = h;
    while (i--)
    {
        *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        j = w;
        while (j--) *pwdata = baseflags | (*src++ + baseindex);

        src += wm - w;
        addr += planwidth * 2;
    }
}


u16 VDP_setMap(u16 plan, const Map *map, u16 basetile, u16 x, u16 y)
{
    return VDP_setMapEx(plan, map, basetile, x, y, 0, 0, map->w, map->h);
}

u16 VDP_setMapEx(u16 plan, const Map *map, u16 basetile, u16 x, u16 y, u16 xm, u16 ym, u16 wm, u16 hm)
{
    const u16 comp = map->compression;
    const u16 offset = (ym * map->w) + xm;

    // compressed map ?
    if (comp != COMPRESSION_NONE)
    {
        // unpack first
        Map *m = unpackMap(map, NULL);

        if (m == NULL) return FALSE;

        // tilemap
        VDP_setTileMapDataRectEx(plan, m->tilemap + offset, basetile, x, y, wm, hm, m->w);
        MEM_free(m);
    }
    else
        // tilemap
        VDP_setTileMapDataRectEx(plan, map->tilemap + offset, basetile, x, y, wm, hm, map->w);

    return TRUE;
}
