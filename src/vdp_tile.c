#include "config.h"
#include "types.h"

#include "vdp.h"
#include "vdp_tile.h"

#include "memory.h"
#include "vdp_pal.h"
#include "vdp_dma.h"
#include "dma.h"
#include "tools.h"

#include "font.h"
#include "tab_cnv.h"


u16 VDP_loadTileSet(const TileSet *tileset, u16 index, TransferMethod tm)
{
    const u16 comp = tileset->compression;

    // compressed tileset ?
    if (comp != COMPRESSION_NONE)
    {
        // unpack first
        TileSet *t = unpackTileSet(tileset, NULL);

        if (t == NULL) return FALSE;

        // tiles
        VDP_loadTileData(t->tiles, index, t->numTile, tm);
        MEM_free(t);
    }
    else
        // tiles
        VDP_loadTileData(tileset->tiles, index, tileset->numTile, tm);

    return TRUE;
}

void VDP_loadFontData(const u32 *font, u16 length, TransferMethod tm)
{
    VDP_loadTileData(font, TILE_FONTINDEX, length, tm);
}

u16 VDP_loadFont(const TileSet *font, TransferMethod tm)
{
    return VDP_loadTileSet(font, TILE_FONTINDEX, tm);
}

void VDP_loadBMPTileDataEx(const u32 *data, u16 index, u16 x, u16 y, u16 w, u16 h, u16 bmp_w)
{
    VDP_loadBMPTileData(&data[x + (y * bmp_w)], index, w, h, bmp_w);
}


void VDP_fillTileData(u8 value, u16 index, u16 num, u16 wait)
{
    // do DMA fill
    DMA_doVRamFill(index * 32, num * 32, value, 1);
    // wait for DMA completion
    if (wait)
        VDP_waitDMACompletion();
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


static u16 getPlanAddress(VDPPlan plan, u16 x, u16 y)
{
    switch(plan.value)
    {
        default:
        case CONST_PLAN_A:
            return VDP_PLAN_A + (((x & (planWidth - 1)) + ((y & (planHeight - 1)) << planWidthSft)) * 2);

        case CONST_PLAN_B:
            return VDP_PLAN_B + (((x & (planWidth - 1)) + ((y & (planHeight - 1)) << planWidthSft)) * 2);

        case CONST_PLAN_WINDOW:
            return VDP_PLAN_WINDOW + (((x & (windowWidth - 1)) + ((y & (32 - 1)) << windowWidthSft)) * 2);
    }
}


void VDP_setTileMapXY(VDPPlan plan, u16 tile, u16 x, u16 y)
{
    vu32 *plctrl;
    vu16 *pwdata;
    u16 addr;

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    // get address
    addr = getPlanAddress(plan, x, y);

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

void VDP_fillTileMapRect(VDPPlan plan, u16 tile, u16 x, u16 y, u16 w, u16 h)
{
    vu32 *plctrl;
    vu16 *pwdata;
    u16 addr;
    u16 width;
    u16 i, j;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    addr = getPlanAddress(plan, x, y);
    if (plan.value == CONST_PLAN_WINDOW) width = windowWidth;
    else width = planWidth;

    i = h;
    while (i--)
    {
        *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        j = w;

        while (j--) *pwdata = tile;

        addr += width * 2;
    }
}

void VDP_clearTileMapRectByIndex(u16 plan, u16 ind, u16 num, u16 wait)
{
    VDP_clearTileMap(plan, ind, num, wait);
}

void VDP_clearTileMap(u16 plan, u16 ind, u16 num, u16 wait)
{
    // do DMA fill
    DMA_doVRamFill(plan + (ind * 2), num * 2, 0, 1);
    // wait for DMA completion
    if (wait)
        VDP_waitDMACompletion();
}

void VDP_clearTileMapRect(VDPPlan plan, u16 x, u16 y, u16 w, u16 h)
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

void VDP_fillTileMapRectInc(VDPPlan plan, u16 basetile, u16 x, u16 y, u16 w, u16 h)
{
    vu32 *plctrl;
    vu16 *pwdata;
    u16 addr;
    u16 width;
    u16 tile;
    u16 i, j;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    addr = getPlanAddress(plan, x, y);
    if (plan.value == CONST_PLAN_WINDOW) width = windowWidth;
    else width = planWidth;
    tile = basetile;

    i = h;
    while (i--)
    {
        *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        j = w;

        while (j--) *pwdata = tile++;

        addr += width * 2;
    }
}

void VDP_setTileMapRectByIndex(u16 plan, const u16 *data, u16 ind, u16 num, TransferMethod tm)
{
    VDP_setTileMapData(plan, data, ind, num, tm);
}

void VDP_setTileMapData(u16 plan, const u16 *data, u16 ind, u16 num, TransferMethod tm)
{
    u16 addr = plan + (ind * 2);

    if (tm == DMA_QUEUE) DMA_queueDma(DMA_VRAM, (u32) data, addr, num, 2);
    else if (tm == DMA) DMA_doDma(DMA_VRAM, (u32) data, addr, num, 2);
    else
    {
        vu32 *plctrl;
        vu16 *pwdata;
        vu32 *pldata;
        const u16 *src;
        const u32 *src32;
        u16 i;

        VDP_setAutoInc(2);

        /* point to vdp port */
        plctrl = (u32 *) GFX_CTRL_PORT;
        pldata = (u32 *) GFX_DATA_PORT;

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

void VDP_setTileMapDataRect(VDPPlan plan, const u16 *data, u16 x, u16 y, u16 w, u16 h)
{
    vu32 *plctrl;
    vu16 *pwdata;
    const u16 *src;
    u16 addr;
    u16 width;
    u16 i, j;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    addr = getPlanAddress(plan, x, y);
    if (plan.value == CONST_PLAN_WINDOW) width = windowWidth;
    else width = planWidth;
    src = data;

    i = h;
    while (i--)
    {
        *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        j = w;
        while (j--) *pwdata = *src++;

        addr += width * 2;
    }
}

void VDP_setTileMapRectExByIndex(u16 plan, const u16 *data, u16 baseindex, u16 baseflag, u16 ind, u16 num)
{
    VDP_setTileMapDataEx(plan, data, baseflag | baseindex, ind, num);
}

void VDP_setTileMapDataEx(u16 plan, const u16 *data, u16 basetile, u16 ind, u16 num)
{
    vu32 *plctrl;
    vu16 *pwdata;
    vu32 *pldata;
    const u16 *src;
    const u32 *src32;
    u32 addr;
    u16 baseinc;
    u16 baseor;
    u32 bi32;
    u32 bo32;
    u16 i;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    addr = plan + (ind * 2);

    *plctrl = GFX_WRITE_VRAM_ADDR(addr);

    src32 = (u32*) data;
    // we can increment both index and palette
    baseinc = basetile & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK);
    // we can only do logical OR on priority and HV flip
    baseor = basetile & (TILE_ATTR_PRIORITY_MASK | TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK);
    bi32 = (baseinc << 16) | baseinc;
    bo32 = (baseor << 16) | baseor;

    i = num >> 3;
    while (i--)
    {
        *pldata = bo32 | (*src32++ + bi32);
        *pldata = bo32 | (*src32++ + bi32);
        *pldata = bo32 | (*src32++ + bi32);
        *pldata = bo32 | (*src32++ + bi32);
    }

    pwdata = (u16 *) GFX_DATA_PORT;

    src = (u16*) src32;

    i = num & 7;
    while (i--) *pwdata = baseor | (*src++ + baseinc);
}

void VDP_setTileMapRectEx(VDPPlan plan, const u16 *data, u16 baseindex, u16 baseflag, u16 x, u16 y, u16 w, u16 h)
{
    VDP_setTileMapDataRectEx(plan, data, baseflag | baseindex, x, y, w, h, w);
}

void VDP_setTileMapDataRectEx(VDPPlan plan, const u16 *data, u16 basetile, u16 x, u16 y, u16 w, u16 h, u16 wm)
{
    vu32 *plctrl;
    vu16 *pwdata;
    const u16 *src;
    u16 addr;
    u16 width;
    u16 baseinc;
    u16 baseor;
    u16 i, j;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    addr = getPlanAddress(plan, x, y);
    if (plan.value == CONST_PLAN_WINDOW) width = windowWidth;
    else width = planWidth;

    // we can increment both index and palette
    baseinc = basetile & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK);
    // we can only do logical OR on priority and HV flip
    baseor = basetile & (TILE_ATTR_PRIORITY_MASK | TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK);
    src = data;

    i = h;
    while (i--)
    {
        *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        j = w;
        while (j--) *pwdata = baseor | (*src++ + baseinc);

        src += wm - w;
        addr += width * 2;
    }
}


u16 VDP_setMap(VDPPlan plan, const Map *map, u16 basetile, u16 x, u16 y)
{
    return VDP_setMapEx(plan, map, basetile, x, y, 0, 0, map->w, map->h);
}

u16 VDP_setMapEx(VDPPlan plan, const Map *map, u16 basetile, u16 x, u16 y, u16 xm, u16 ym, u16 wm, u16 hm)
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
