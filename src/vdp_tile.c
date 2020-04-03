#include "config.h"
#include "types.h"

#include "vdp.h"
#include "vdp_tile.h"

#include "memory.h"
#include "vdp_pal.h"
#include "vdp_dma.h"
#include "dma.h"
#include "tools.h"
#include "mapper.h"

#include "font.h"
#include "tab_cnv.h"


// forward
static void prepareTileMapDataRow(u16* dest, u16 width, const u16* mapData, u16 xm, u16 wm);
static void prepareTileMapDataColumn(u16* dest, u16 height, const u16 *mapData, u16 ym, u16 wm, u16 hm);
static void prepareTileMapDataRowEx(u16* dest, u16 width, const u16* mapData, u16 basetile, u16 xm, u16 wm);
static void prepareTileMapDataColumnEx(u16* dest, u16 height, const u16 *mapData, u16 basetile, u16 ym, u16 wm, u16 hm);


void VDP_loadTileData(const u32 *data, u16 index, u16 num, TransferMethod tm)
{
    DMA_transfer(tm, DMA_VRAM, (void*) data, index * 32, num * 16, 2);
}

void VDP_loadFontData(const u32 *font, u16 length, TransferMethod tm)
{
    VDP_loadTileData(font, TILE_FONTINDEX, length, tm);
}

u16 VDP_loadTileSet(const TileSet *tileset, u16 index, TransferMethod tm)
{
    // compressed tileset ?
    if (tileset->compression != COMPRESSION_NONE)
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
        VDP_loadTileData(FAR(tileset->tiles), index, tileset->numTile, tm);

    return TRUE;
}

u16 VDP_loadFont(const TileSet *font, TransferMethod tm)
{
    return VDP_loadTileSet(font, TILE_FONTINDEX, tm);
}

void VDP_loadBMPTileDataEx(const u32 *data, u16 index, u16 x, u16 y, u16 w, u16 h, u16 bmp_w)
{
    VDP_loadBMPTileData(&data[x + (y * bmp_w)], index, w, h, bmp_w);
}

void VDP_fillTileData(u8 value, u16 index, u16 num, bool wait)
{
    // do DMA fill
    DMA_doVRamFill(index * 32, num * 32, value, 1);
    // wait for DMA completion
    if (wait)
        VDP_waitDMACompletion();
}


static u16 getPlanAddress(VDPPlane plane, u16 x, u16 y)
{
    switch(plane.value)
    {
        default:
        case CONST_BG_A:
            return VDP_BG_A + (((x & (planeWidth - 1)) + ((y & (planeHeight - 1)) << planeWidthSft)) * 2);

        case CONST_BG_B:
            return VDP_BG_B + (((x & (planeWidth - 1)) + ((y & (planeHeight - 1)) << planeWidthSft)) * 2);

        case CONST_WINDOW:
            return VDP_WINDOW + (((x & (windowWidth - 1)) + ((y & (32 - 1)) << windowWidthSft)) * 2);
    }
}


//void VDP_setTileMap(u16 plane, u16 tile, u16 ind)
//{
//    vu32 *plctrl;
//    vu16 *pwdata;
//
//    const u32 addr = plane + (ind * 2);
//
//    /* point to vdp port */
//    plctrl = (u32 *) GFX_CTRL_PORT;
//    pwdata = (u16 *) GFX_DATA_PORT;
//
//    *plctrl = GFX_WRITE_VRAM_ADDR(addr);
//    *pwdata = tile;
//}

void VDP_clearTileMap(u16 plane, u16 ind, u16 num, bool wait)
{
    // do DMA fill
    DMA_doVRamFill(plane + (ind * 2), num * 2, 0, 1);
    // wait for DMA completion
    if (wait)
        VDP_waitDMACompletion();
}

void VDP_fillTileMap(u16 plane, u16 tile, u16 ind, u16 num)
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

    addr = plane + (ind * 2);

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

void VDP_setTileMapData(u16 plane, const u16 *data, u16 ind, u16 num, TransferMethod tm)
{
    DMA_transfer(tm, DMA_VRAM, (void*) data,  plane + (ind * 2), num, 2);
}

void VDP_setTileMapDataEx(u16 plane, const u16 *data, u16 basetile, u16 ind, u16 num)
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

    addr = plane + (ind * 2);

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


void VDP_setTileMapXY(VDPPlane plane, u16 tile, u16 x, u16 y)
{
    vu32 *plctrl;
    vu16 *pwdata;
    u16 addr;

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    // get address
    addr = getPlanAddress(plane, x, y);

    *plctrl = GFX_WRITE_VRAM_ADDR(addr);
    *pwdata = tile;
}

void VDP_clearTileMapRect(VDPPlane plane, u16 x, u16 y, u16 w, u16 h)
{
    VDP_fillTileMapRect(plane, 0, x, y, w, h);
}

void VDP_fillTileMapRect(VDPPlane plane, u16 tile, u16 x, u16 y, u16 w, u16 h)
{
    vu32 *plctrl;
    vu32 *pldata;
    vu16 *pwdata;
    u16 addr;
    u16 width;
    u16 i, j;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    addr = getPlanAddress(plane, x, y);
    if (plane.value == CONST_WINDOW) width = windowWidth;
    else width = planeWidth;

    const u32 tile32 = (tile << 16) | tile;

    i = h;
    while (i--)
    {
        *plctrl = GFX_WRITE_VRAM_ADDR(addr);

        j = w >> 3;
        while (j--)
        {
            *pldata = tile32;
            *pldata = tile32;
            *pldata = tile32;
            *pldata = tile32;
        }

        j = w & 7;
        while (j--) *pwdata = tile;

        addr += width * 2;
    }
}

void VDP_fillTileMapRectInc(VDPPlane plane, u16 basetile, u16 x, u16 y, u16 w, u16 h)
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

    addr = getPlanAddress(plane, x, y);
    if (plane.value == CONST_WINDOW) width = windowWidth;
    else width = planeWidth;
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

void VDP_setTileMapDataRect(VDPPlane plane, const u16 *data, u16 x, u16 y, u16 w, u16 h)
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

    addr = getPlanAddress(plane, x, y);
    if (plane.value == CONST_WINDOW) width = windowWidth;
    else width = planeWidth;
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

void VDP_setTileMapDataRectEx(VDPPlane plane, const u16 *data, u16 basetile, u16 x, u16 y, u16 w, u16 h, u16 wm)
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

    addr = getPlanAddress(plane, x, y);
    if (plane.value == CONST_WINDOW) width = windowWidth;
    else width = planeWidth;

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

void VDP_setTileMapDataRowFast(VDPPlane plane, u16* data, u16 row, TransferMethod tm)
{
    u16 addr;
    u16 width;

    addr = getPlanAddress(plane, 0, row);
    if (plane.value == CONST_WINDOW) width = windowWidth;
    else width = planeWidth;

    DMA_transfer(tm, DMA_VRAM, data, addr, width, 2);
}

void VDP_setTileMapDataColumnFast(VDPPlane plane, u16* data, u16 column, TransferMethod tm)
{
    u16 addr;
    u16 width;
    u16 height;

    addr = getPlanAddress(plane, column, 0);
    if (plane.value == CONST_WINDOW)
    {
        width = windowWidth;
        height = 32;
    }
    else
    {
        width = planeWidth;
        height = planeHeight;
    }

    DMA_transfer(tm, DMA_VRAM, data, addr, height, width * 2);
}

void VDP_setTileMapDataRow(VDPPlane plane, const u16 *mapData, u16 row, u16 xm, u16 ym, u16 wm, TransferMethod tm)
{
    const u16* src = mapData + (ym * wm);
    u16 addr = getPlanAddress(plane, 0, row);
    u16 width = (plane.value == CONST_WINDOW)?windowWidth:planeWidth;

    if (tm >= DMA_QUEUE)
    {
        // get temp buffer and schedule DMA
        u16* buf = DMA_allocateAndQueueDma(DMA_VRAM, addr, width, 2);

#if (LIB_DEBUG != 0)
        if (!buf) KDebug_Alert("VDP_setTileMapDataRows failed: DMA temporary buffer is full");
        else
#endif
        // then prepare data in buffer that will be transfered by DMA
        prepareTileMapDataRow(buf, width, src, xm, wm);
    }
    else
    {
        // maximum plane width or height
        u16 buf[1024];

        // prepare tilemap data and copy it into temp buffer
        prepareTileMapDataRow(buf, width, src, xm, wm);
        // transfer the buffer data to VRAM
        if (tm == DMA) DMA_doDma(DMA_VRAM, buf, addr, width, 2);
        else DMA_doCPUCopy(DMA_VRAM, buf, addr, width, 2);
    }
}

void VDP_setTileMapDataColumn(VDPPlane plane, const u16 *mapData, u16 column, u16 xm, u16 ym, u16 wm, u16 hm, TransferMethod tm)
{
    const u16* src = mapData + xm;
    u16 addr = getPlanAddress(plane, column, 0);
    u16 width;
    u16 height;

    if (plane.value == CONST_WINDOW)
    {
        width = windowWidth;
        height = 32;
    }
    else
    {
        width = planeWidth;
        height = planeHeight;
    }

    if (tm >= DMA_QUEUE)
    {
        // get temp buffer and schedule DMA
        u16* buf = DMA_allocateAndQueueDma(DMA_VRAM, addr, height, width * 2);

#if (LIB_DEBUG != 0)
        if (!buf) KDebug_Alert("VDP_setTileMapDataRows failed: DMA temporary buffer is full");
        else
#endif
        // then prepare data in buffer that will be transfered by DMA
        prepareTileMapDataColumn(buf, height, src, ym, wm, hm);
    }
    else
    {
        // maximum plane width or height
        u16 buf[1024];

        // prepare tilemap data and copy it into temp buffer
        prepareTileMapDataColumn(buf, height, src, ym, wm, hm);
        // transfer the buffer data to VRAM
        if (tm == DMA) DMA_doDma(DMA_VRAM, buf, addr, height, width * 2);
        else DMA_doCPUCopy(DMA_VRAM, buf, addr, height, width * 2);
    }
}

void VDP_setTileMapDataRowEx(VDPPlane plane, const u16 *mapData, u16 basetile, u16 row, u16 xm, u16 ym, u16 wm, TransferMethod tm)
{
    const u16* src = mapData + (ym * wm);
    u16 addr = getPlanAddress(plane, 0, row);
    u16 width = (plane.value == CONST_WINDOW)?windowWidth:planeWidth;

    if (tm >= DMA_QUEUE)
    {
        // get temp buffer and schedule DMA
        u16* buf = DMA_allocateAndQueueDma(DMA_VRAM, addr, width, 2);

#if (LIB_DEBUG != 0)
        if (!buf) KDebug_Alert("VDP_setTileMapDataRows failed: DMA temporary buffer is full");
        else
#endif
        // then prepare data in buffer that will be transfered by DMA
        prepareTileMapDataRowEx(buf, width, src, basetile, xm, wm);
    }
    else
    {
        // maximum plane width or height
        u16 buf[1024];

        // prepare tilemap data and copy it into temp buffer
        prepareTileMapDataRowEx(buf, width, src, basetile, xm, wm);
        // transfer the buffer data to VRAM
        if (tm == DMA) DMA_doDma(DMA_VRAM, buf, addr, width, 2);
        else DMA_doCPUCopy(DMA_VRAM, buf, addr, width, 2);
    }
}

void VDP_setTileMapDataColumnEx(VDPPlane plane, const u16 *mapData, u16 basetile, u16 column, u16 xm, u16 ym, u16 wm, u16 hm, TransferMethod tm)
{
    const u16* src = mapData + xm;
    u16 addr = getPlanAddress(plane, column, 0);
    u16 width;
    u16 height;

    if (plane.value == CONST_WINDOW)
    {
        width = windowWidth;
        height = 32;
    }
    else
    {
        width = planeWidth;
        height = planeHeight;
    }

    if (tm >= DMA_QUEUE)
    {
        // get temp buffer and schedule DMA
        u16* buf = DMA_allocateAndQueueDma(DMA_VRAM, addr, height, width * 2);

#if (LIB_DEBUG != 0)
        if (!buf) KDebug_Alert("VDP_setTileMapDataRows failed: DMA temporary buffer is full");
        else
#endif
        // then prepare data in buffer that will be transfered by DMA
        prepareTileMapDataColumnEx(buf, height, src, basetile, ym, wm, hm);
    }
    else
    {
        // maximum plane width or height
        u16 buf[1024];

        // prepare tilemap data and copy it into temp buffer
        prepareTileMapDataColumnEx(buf, height, src, basetile, ym, wm, hm);
        // transfer the buffer data to VRAM
        if (tm == DMA) DMA_doDma(DMA_VRAM, buf, addr, height, width * 2);
        else DMA_doCPUCopy(DMA_VRAM, buf, addr, height, width * 2);
    }
}


bool VDP_setTileMap(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 x, u16 y)
{
    return VDP_setTileMapEx(plane, tilemap, basetile, x, y, 0, 0, tilemap->w, tilemap->h);
}

bool VDP_setTileMapEx(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 x, u16 y, u16 xm, u16 ym, u16 wm, u16 hm)
{
    const u16 offset = (ym * tilemap->w) + xm;

    // compressed tilemap ?
    if (tilemap->compression != COMPRESSION_NONE)
    {
        // unpack first
        TileMap *m = unpackTileMap(tilemap, NULL);

        if (m == NULL) return FALSE;

        // tilemap
        VDP_setTileMapDataRectEx(plane, m->tilemap + offset, basetile, x, y, wm, hm, m->w);
        MEM_free(m);
    }
    else
        // tilemap
        VDP_setTileMapDataRectEx(plane, (u16*) FAR(tilemap->tilemap + offset), basetile, x, y, wm, hm, tilemap->w);

    return TRUE;
}

bool VDP_setTileMapRow(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 row, u16 xm, u16 ym, TransferMethod tm)
{
    // compressed tilemap ?
    if (tilemap->compression != COMPRESSION_NONE)
    {
        // unpack first
        TileMap *m = unpackTileMap(tilemap, NULL);

        if (m == NULL) return FALSE;

        // tilemap
        VDP_setTileMapDataRowEx(plane, m->tilemap, basetile, row, xm, ym, m->w, tm);
        MEM_free(m);
    }
    else
        // tilemap
        VDP_setTileMapDataRowEx(plane, (u16*) FAR(tilemap->tilemap), basetile, row, xm, ym, tilemap->w, tm);

    return TRUE;
}

bool VDP_setTileMapColumn(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 column, u16 xm, u16 ym, TransferMethod tm)
{
    // compressed tilemap ?
    if (tilemap->compression != COMPRESSION_NONE)
    {
        // unpack first
        TileMap *m = unpackTileMap(tilemap, NULL);

        if (m == NULL) return FALSE;

        // tilemap
        VDP_setTileMapDataColumnEx(plane, m->tilemap, basetile, column, xm, ym, m->w, m->h, tm);
        MEM_free(m);
    }
    else
        // tilemap
        VDP_setTileMapDataColumnEx(plane, (u16*) FAR(tilemap->tilemap), basetile, column, xm, ym, tilemap->w, tilemap->h, tm);

    return TRUE;
}


bool VDP_setMap(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 x, u16 y)
{
    return VDP_setTileMapEx(plane, tilemap, basetile, x, y, 0, 0, tilemap->w, tilemap->h);
}

bool VDP_setMapEx(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 x, u16 y, u16 xm, u16 ym, u16 wm, u16 hm)
{
    return VDP_setTileMapEx(plane, tilemap, basetile, x, y, xm, ym, wm, hm);
}


static void copyColumnW(u16* dest, const u16* src, u16 step, u16 size)
{
    u16* d = dest;
    const u16* s = src;
    u16 i;

    i = size >> 2;
    while (i--)
    {
        *d++ = *s;
        s += step;
        *d++ = *s;
        s += step;
        *d++ = *s;
        s += step;
        *d++ = *s;
        s += step;
    }

    i = size & 3;
    while (i--)
    {
        *d++ = *s;
        s += step;
    }
}

static void copyColumnWEx(u16* dest, const u16* src, u16 baseinc, u16 baseor, u16 step, u16 size)
{
    u16* d = dest;
    const u16* s = src;
    u16 i;

    i = size >> 2;
    while (i--)
    {
        *d++ = baseor | (*s + baseinc);
        s += step;
        *d++ = baseor | (*s + baseinc);
        s += step;
        *d++ = baseor | (*s + baseinc);
        s += step;
        *d++ = baseor | (*s + baseinc);
        s += step;
    }

    i = size & 3;
    while (i--)
    {
        *d++ = baseor | (*s + baseinc);
        s += step;
    }
}

// mapData should point on current row of tilemap data we want to prepare. i.e: mapData + (ym * wm)
static void prepareTileMapDataRow(u16* dest, u16 width, const u16* mapData, u16 xm, u16 wm)
{
    if ((xm + width) > wm)
    {
        const u16 len = wm - xm;
        memcpyU16(dest, mapData + xm, len);
        memcpyU16(dest + len, mapData, width - len);
    }
    else memcpyU16(dest, mapData + xm, width);
}

// mapData should point on current column of tilemap data we want to prepare. i.e: mapData + xm
static void prepareTileMapDataColumn(u16* dest, u16 height, const u16 *mapData, u16 ym, u16 wm, u16 hm)
{
    if ((ym + height) > hm)
    {
        const u16 len = hm - ym;
        copyColumnW(dest, mapData + (ym * wm), wm, len);
        copyColumnW(dest + len, mapData, wm, height - len);
    }
    else copyColumnW(dest, mapData + (ym * wm), wm, height);
}

// mapData should point on current row of tilemap data we want to prepare. i.e: mapData + (ym * wm)
static void prepareTileMapDataRowEx(u16* dest, u16 width, const u16* mapData, u16 basetile, u16 xm, u16 wm)
{
    const u16* s;
    u16* d = dest;
    // we can increment both index and palette
    const u16 baseinc = basetile & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK);
    // we can only do logical OR on priority and HV flip
    const u16 baseor = basetile & (TILE_ATTR_PRIORITY_MASK | TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK);
    u16 i;

    if ((xm + width) > wm)
    {
        const u16 len = wm - xm;

        s = mapData + xm;
        i = len;
        while (i--) *d++ = baseor | (*s++ + baseinc);

        s = mapData;
        i = width - len;
        while (i--) *d++ = baseor | (*s++ + baseinc);
    }
    else
    {
        s = mapData + xm;
        i = width;
        while (i--) *d++ = baseor | (*s++ + baseinc);
    }
}

// mapData should point on current column of tilemap data we want to prepare. i.e: mapData + xm
static void prepareTileMapDataColumnEx(u16* dest, u16 height, const u16 *mapData, u16 basetile, u16 ym, u16 wm, u16 hm)
{
    // we can increment both index and palette
    const u16 baseinc = basetile & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK);
    // we can only do logical OR on priority and HV flip
    const u16 baseor = basetile & (TILE_ATTR_PRIORITY_MASK | TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK);

    if ((ym + height) > hm)
    {
        const u16 len = hm - ym;
        copyColumnWEx(dest, mapData + (ym * wm), baseinc, baseor, wm, len);
        copyColumnWEx(dest + len, mapData, baseinc, baseor, wm, height - len);
    }
    else copyColumnWEx(dest, mapData + (ym * wm), baseinc, baseor, wm, height);
}
