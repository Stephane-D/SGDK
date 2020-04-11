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
#include "tools.h"


// forward
static void setTileMapDataRowPart(VDPPlane plane, const u16 *data, u16 row, u16 x, u16 w, TransferMethod tm);
static void setTileMapDataRowPartEx(VDPPlane plane, const u16 *data, u16 basetile, u16 row, u16 x, u16 w, TransferMethod tm);
static void setTileMapDataColumnPart(VDPPlane plane, const u16 *data, u16 column, u16 y, u16 h, u16 wm, TransferMethod tm);
static void setTileMapDataColumnPartEx(VDPPlane plane, const u16 *data, u16 basetile, u16 column, u16 y, u16 h, u16 wm, TransferMethod tm);

static void prepareTileMapDataColumn(u16* dest, u16 height, const u16 *data, u16 wm);
static void prepareTileMapDataRowEx(u16* dest, u16 width, const u16* data, u16 basetile);
static void prepareTileMapDataColumnEx(u16* dest, u16 height, const u16 *mapData, u16 basetile, u16 wm);


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
    switch(plane)
    {
        default:
        case BG_A:
            return VDP_BG_A + (((x & (planeWidth - 1)) + ((y & (planeHeight - 1)) << planeWidthSft)) * 2);

        case BG_B:
            return VDP_BG_B + (((x & (planeWidth - 1)) + ((y & (planeHeight - 1)) << planeWidthSft)) * 2);

        case WINDOW:
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

void VDP_clearTileMap(u16 planeAddr, u16 ind, u16 num, bool wait)
{
    // do DMA fill
    DMA_doVRamFill(planeAddr + (ind * 2), num * 2, 0, 1);
    // wait for DMA completion
    if (wait)
        VDP_waitDMACompletion();
}

void VDP_fillTileMap(u16 planeAddr, u16 tile, u16 ind, u16 num)
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

    addr = planeAddr + (ind * 2);

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

void VDP_setTileMapData(u16 planeAddr, const u16 *data, u16 ind, u16 num, TransferMethod tm)
{
    DMA_transfer(tm, DMA_VRAM, (void*) data,  planeAddr + (ind * 2), num, 2);
}

void VDP_setTileMapDataEx(u16 planeAddr, const u16 *data, u16 basetile, u16 ind, u16 num)
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

    addr = planeAddr + (ind * 2);

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
    width = (plane == WINDOW)?windowWidth:planeWidth;

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
    width = (plane == WINDOW)?windowWidth:planeWidth;
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

void VDP_setTileMapDataRect(VDPPlane plane, const u16 *data, u16 x, u16 y, u16 w, u16 h, u16 wm, TransferMethod tm)
{
    const u16* src = data;

    // if half less number of column than number of row then we use column transfer
    if (w < (h / 2))
    {
        u16 ph = (plane == WINDOW)?32:planeHeight;
        u16 h1, h2;
        u32 off;

        // larger than plane height ? --> need to split
        if ((y + h) > ph)
        {
            // first part
            h1 = ph - y;
            // second part
            h2 = h - h1;
            // offset
            off = wm * h1;
        }
        else
        {
            // no split
            h1 = h;
            h2 = 0;
            off = 0;
        }

        u16 col = x;
        u16 i = w;

        // set region by column
        while (i--)
        {
            // first part
            setTileMapDataColumnPart(plane, src, col, y, h1, wm, tm);
            // second part
            if (h2 != 0) setTileMapDataColumnPart(plane, src + off, col, 0, h2, wm, tm);
            col++;
            src++;
        }
    }
    else
    {
        u16 pw = (plane == WINDOW)?windowWidth:planeWidth;
        u16 w1, w2;

        // larger than plane width ? --> need to split
        if ((x + w) > pw)
        {
            // first part
            w1 = pw - x;
            // second part
            w2 = w - w2;
        }
        else
        {
            w1 = w;
            w2 = 0;
        }

        u16 row = y;
        u16 i = h;

        // otherwise we set region by row (faster)
        while (i--)
        {
            // first part
            setTileMapDataRowPart(plane, src, row, x, w1, tm);
            // second part
            if (w2 != 0) setTileMapDataRowPart(plane, src + w1, row, 0, w2, tm);
            row++;
            src += wm;
        }
    }
}

void VDP_setTileMapDataRectEx(VDPPlane plane, const u16 *data, u16 basetile, u16 x, u16 y, u16 w, u16 h, u16 wm, TransferMethod tm)
{
    const u16* src = data;

    // if half less number of column than number of row then we use column transfer
    if (w < (h / 2))
    {
        const u16 ph = (plane == WINDOW)?32:planeHeight;
        u16 h1, h2;
        u32 off;

        // larger than plane height ? --> need to split
        if ((y + h) > ph)
        {
            // first part
            h1 = ph - y;
            // second part
            h2 = h - h1;
            // offset
            off = wm * h1;
        }
        else
        {
            // no split
            h1 = h;
            h2 = 0;
            off = 0;
        }

        u16 col = x;
        u16 i = w;

        // set region by column
        while (i--)
        {
            // first part
            setTileMapDataColumnPartEx(plane, src, basetile, col, y, h1, wm, tm);
            // second part
            if (h2 != 0) setTileMapDataColumnPartEx(plane, src + off, basetile, col, 0, h2, wm, tm);
            col++;
            src++;
        }
    }
    else
    {
        const u16 pw = (plane == WINDOW)?windowWidth:planeWidth;
        u16 w1, w2;

        // larger than plane width ? --> need to split
        if ((x + w) > pw)
        {
            // first part
            w1 = pw - x;
            // second part
            w2 = w - w1;
        }
        else
        {
            w1 = w;
            w2 = 0;
        }

        u16 row = y;
        u16 i = h;

        // otherwise we set region by row (faster)
        while (i--)
        {
            // first part
            setTileMapDataRowPartEx(plane, src, basetile, row, x, w1, tm);
            // second part
            if (w2 != 0) setTileMapDataRowPartEx(plane, src + w1, basetile, row, 0, w2, tm);
            row++;
            src += wm;
        }
    }
}


void VDP_setTileMapDataRow(VDPPlane plane, const u16 *data, u16 row, TransferMethod tm)
{
    setTileMapDataRowPart(plane, data, row, 0, (plane == WINDOW)?windowWidth:planeWidth, tm);
}

void VDP_setTileMapDataRowEx(VDPPlane plane, const u16 *data, u16 basetile, u16 row, TransferMethod tm)
{
    setTileMapDataRowPartEx(plane, data, basetile, row, 0, (plane == WINDOW)?windowWidth:planeWidth, tm);
}

static void setTileMapDataRowPart(VDPPlane plane, const u16 *data, u16 row, u16 x, u16 w, TransferMethod tm)
{
    DMA_transfer(tm, DMA_VRAM, (void*) data, getPlanAddress(plane, x, row), w, 2);
}

static void setTileMapDataRowPartEx(VDPPlane plane, const u16 *data, u16 basetile, u16 row, u16 x, u16 w, TransferMethod tm)
{
    u16 addr = getPlanAddress(plane, x, row);

    if (tm >= DMA_QUEUE)
    {
        // get temp buffer and schedule DMA
        u16* buf = DMA_allocateAndQueueDma(DMA_VRAM, addr, w, 2);

#if (LIB_DEBUG != 0)
        if (!buf)
        {
            KLog("VDP_setTileMapDataRowEx failed: DMA temporary buffer is full");
            return;
        }
#endif
        // then prepare data in buffer that will be transfered by DMA
        prepareTileMapDataRowEx(buf, w, data, basetile);
    }
    else
    {
        // allocate on DMA buffer
        u16* buf = DMA_allocateTemp(w);

        // prepare tilemap data into temp buffer
        prepareTileMapDataRowEx(buf, w, data, basetile);

        // transfer the buffer data to VRAM
        if (tm == DMA) DMA_doDma(DMA_VRAM, buf, addr, w, 2);
        else DMA_doCPUCopy(DMA_VRAM, buf, addr, w, 2);

        // release allocated buffer
        DMA_releaseTemp(w);
    }
}

void VDP_setTileMapDataRowPart(VDPPlane plane, const u16 *data, u16 row, u16 x, u16 w, TransferMethod tm)
{
    const u16 pw = (plane == WINDOW)?windowWidth:planeWidth;

    // larger than plane width ? --> need to split
    if ((x + w) > pw)
    {
        u16 w1 = pw - x;

        // first part
        setTileMapDataRowPart(plane, data, row, x, w1, tm);
        // second part
        setTileMapDataRowPart(plane, data + w1, row, 0, w - w1, tm);
    }
    // no split needed
    else setTileMapDataRowPart(plane, data, row, x, w, tm);
}

void VDP_setTileMapDataRowPartEx(VDPPlane plane, const u16 *data, u16 basetile, u16 row, u16 x, u16 w, TransferMethod tm)
{
    const u16 pw = (plane == WINDOW)?windowWidth:planeWidth;

    // larger than plane width ? --> need to split
    if ((x + w) > pw)
    {
        u16 w1 = pw - x;

        // first part
        setTileMapDataRowPartEx(plane, data, basetile, row, x, w1, tm);
        // second part
        setTileMapDataRowPartEx(plane, data + w1, basetile, row, 0, w - w1, tm);
    }
    // no split needed
    else setTileMapDataRowPartEx(plane, data, basetile, row, x, w, tm);
}

void VDP_setTileMapDataColumnFast(VDPPlane plane, u16* data, u16 column, TransferMethod tm)
{
    u16 addr;
    u16 width;
    u16 height;

    addr = getPlanAddress(plane, column, 0);
    if (plane == WINDOW)
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

void VDP_setTileMapDataColumn(VDPPlane plane, const u16 *data, u16 column, u16 wm, TransferMethod tm)
{
    setTileMapDataColumnPart(plane, data, column, 0, (plane == WINDOW)?32:planeHeight, wm, tm);
}

void VDP_setTileMapDataColumnEx(VDPPlane plane, const u16 *data, u16 basetile, u16 column, u16 wm, TransferMethod tm)
{
    setTileMapDataColumnPartEx(plane, data, basetile, column, 0, (plane == WINDOW)?32:planeHeight, wm, tm);
}

static void setTileMapDataColumnPart(VDPPlane plane, const u16 *data, u16 column, u16 y, u16 h, u16 wm, TransferMethod tm)
{
    u16 addr = getPlanAddress(plane, column, y);
    u16 width = (plane == WINDOW)?windowWidth:planeWidth;

    if (tm >= DMA_QUEUE)
    {
        // get temp buffer and schedule DMA
        u16* buf = DMA_allocateAndQueueDma(DMA_VRAM, addr, h, width * 2);

#if (LIB_DEBUG != 0)
        if (!buf)
        {
            KLog("VDP_setTileMapDataColumn failed: DMA temporary buffer is full");
            return;
        }
#endif
        // then prepare data in buffer that will be transfered by DMA
        prepareTileMapDataColumn(buf, h, data, wm);
    }
    else
    {
        // allocate on DMA buffer
        u16* buf = DMA_allocateTemp(h);

        // prepare tilemap data into temp buffer
        prepareTileMapDataColumn(buf, h, data, wm);

        // transfer the temp data to VRAM
        if (tm == DMA) DMA_doDma(DMA_VRAM, buf, addr, h, width * 2);
        else DMA_doCPUCopy(DMA_VRAM, buf, addr, h, width * 2);

        // release allocated buffer
        DMA_releaseTemp(h);
    }
}

static void setTileMapDataColumnPartEx(VDPPlane plane, const u16 *data, u16 basetile, u16 column, u16 y, u16 h, u16 wm, TransferMethod tm)
{
    u16 addr = getPlanAddress(plane, column, y);
    u16 width = (plane == WINDOW)?windowWidth:planeWidth;

    if (tm >= DMA_QUEUE)
    {
        // get temp buffer and schedule DMA
        u16* buf = DMA_allocateAndQueueDma(DMA_VRAM, addr, h, width * 2);

#if (LIB_DEBUG != 0)
        if (!buf)
        {
            KLog("VDP_setTileMapDataColumnEx failed: DMA temporary buffer is full");
            return;
        }
#endif
        // then prepare data in buffer that will be transfered by DMA
        prepareTileMapDataColumnEx(buf, h, data, basetile, wm);
    }
    else
    {
        // allocate on DMA buffer
        u16* buf = DMA_allocateTemp(h);

        // prepare tilemap data into temp buffer
        prepareTileMapDataColumnEx(buf, h, data, basetile, wm);

        // transfer the buffer data to VRAM
        if (tm == DMA) DMA_doDma(DMA_VRAM, buf, addr, h, width * 2);
        else DMA_doCPUCopy(DMA_VRAM, buf, addr, h, width * 2);

        // release allocated buffer
        DMA_releaseTemp(h);
    }
}

void VDP_setTileMapDataColumnPart(VDPPlane plane, const u16 *data, u16 column, u16 y, u16 h, u16 wm, TransferMethod tm)
{
    const u16 ph = (plane == WINDOW)?32:planeHeight;

    // larger than plane height ? --> need to split
    if ((y + h) > ph)
    {
        u16 h1 = ph - y;

        // first part
        setTileMapDataColumnPart(plane, data, column, y, h1, wm, tm);
        // second part
        setTileMapDataColumnPart(plane, data + (wm * h1), column, 0, h - h1, wm, tm);
    }
    // no split needed
    else setTileMapDataColumnPart(plane, data, column, y, h, wm, tm);
}

void VDP_setTileMapDataColumnPartEx(VDPPlane plane, const u16 *data, u16 basetile, u16 column, u16 y, u16 h, u16 wm, TransferMethod tm)
{
    const u16 ph = (plane == WINDOW)?32:planeHeight;

    // larger than plane height ? --> need to split
    if ((y + h) > ph)
    {
        u16 h1 = ph - y;

        // first part
        setTileMapDataColumnPartEx(plane, data, basetile, column, y, h1, wm, tm);
        // second part
        setTileMapDataColumnPartEx(plane, data + (wm * h1), basetile, column, 0, h - h1, wm, tm);
    }
    // no split needed
    else setTileMapDataColumnPartEx(plane, data, basetile, column, y, h, wm, tm);
}


bool VDP_setTileMap(VDPPlane plane, const TileMap *tilemap, u16 x, u16 y, u16 w, u16 h, TransferMethod tm)
{
    const u32 offset = (y * tilemap->w) + x;

    // compressed tilemap ?
    if (tilemap->compression != COMPRESSION_NONE)
    {
        // unpack first
        TileMap *m = unpackTileMap(tilemap, NULL);

        if (m == NULL) return FALSE;

        // tilemap
        VDP_setTileMapDataRect(plane, m->tilemap + offset, x, y, w, h, m->w, tm);
        MEM_free(m);
    }
    else
        // tilemap
        VDP_setTileMapDataRect(plane, (u16*) FAR(tilemap->tilemap + offset), x, y, w, h, tilemap->w, tm);

    return TRUE;
}

bool VDP_setTileMapEx(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 xp, u16 yp, u16 x, u16 y, u16 w, u16 h, TransferMethod tm)
{
    const u32 offset = (y * tilemap->w) + x;

    // compressed tilemap ?
    if (tilemap->compression != COMPRESSION_NONE)
    {
        // unpack first
        TileMap *m = unpackTileMap(tilemap, NULL);

        if (m == NULL) return FALSE;

        // tilemap
        VDP_setTileMapDataRectEx(plane, m->tilemap + offset, basetile, xp, yp, w, h, m->w, tm);
        MEM_free(m);
    }
    else
        // tilemap
        VDP_setTileMapDataRectEx(plane, (u16*) FAR(tilemap->tilemap + offset), basetile, xp, yp, w, h, tilemap->w, tm);

    return TRUE;
}

bool VDP_setTileMapRow(VDPPlane plane, const TileMap *tilemap, u16 row, u16 x, TransferMethod tm)
{
    const u32 offset = (row * tilemap->w) + x;

    // compressed tilemap ?
    if (tilemap->compression != COMPRESSION_NONE)
    {
        // unpack first
        TileMap *m = unpackTileMap(tilemap, NULL);

        if (m == NULL) return FALSE;

        // tilemap
        VDP_setTileMapDataRow(plane, m->tilemap + offset, row,  tm);
        MEM_free(m);
    }
    else
        // tilemap
        VDP_setTileMapDataRow(plane, (u16*) FAR(tilemap->tilemap + offset), row, tm);

    return TRUE;
}

bool VDP_setTileMapColumn(VDPPlane plane, const TileMap *tilemap, u16 column, u16 y, TransferMethod tm)
{
    const u32 offset = (y * tilemap->w) + column;

    // compressed tilemap ?
    if (tilemap->compression != COMPRESSION_NONE)
    {
        // unpack first
        TileMap *m = unpackTileMap(tilemap, NULL);

        if (m == NULL) return FALSE;

        // tilemap
        VDP_setTileMapDataColumn(plane, m->tilemap + offset, column, m->w, tm);
        MEM_free(m);
    }
    else
        // tilemap
        VDP_setTileMapDataColumn(plane, (u16*) FAR(tilemap->tilemap + offset), column, tilemap->w, tm);

    return TRUE;
}

bool VDP_setTileMapRowEx(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 row, u16 x, u16 y, TransferMethod tm)
{
    const u32 offset = (y * tilemap->w) + x;

    // compressed tilemap ?
    if (tilemap->compression != COMPRESSION_NONE)
    {
        // unpack first
        TileMap *m = unpackTileMap(tilemap, NULL);

        if (m == NULL) return FALSE;

        // tilemap
        VDP_setTileMapDataRowEx(plane, m->tilemap + offset, basetile, row, tm);
        MEM_free(m);
    }
    else
        // tilemap
        VDP_setTileMapDataRowEx(plane, (u16*) FAR(tilemap->tilemap + offset), basetile, row, tm);

    return TRUE;
}

bool VDP_setTileMapColumnEx(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 column, u16 x, u16 y, TransferMethod tm)
{
    const u32 offset = (y * tilemap->w) + x;

    // compressed tilemap ?
    if (tilemap->compression != COMPRESSION_NONE)
    {
        // unpack first
        TileMap *m = unpackTileMap(tilemap, NULL);

        if (m == NULL) return FALSE;

        // tilemap
        VDP_setTileMapDataColumnEx(plane, m->tilemap + offset, basetile, column, m->w, tm);
        MEM_free(m);
    }
    else
        // tilemap
        VDP_setTileMapDataColumnEx(plane, (u16*) FAR(tilemap->tilemap + offset), basetile, column, tilemap->w, tm);

    return TRUE;
}


bool VDP_setMap(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 x, u16 y)
{
    return VDP_setTileMapEx(plane, tilemap, basetile, x, y, 0, 0, tilemap->w, tilemap->h, CPU);
}

bool VDP_setMapEx(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 x, u16 y, u16 xm, u16 ym, u16 wm, u16 hm)
{
    return VDP_setTileMapEx(plane, tilemap, basetile, x, y, xm, ym, wm, hm, CPU);
}


static void prepareTileMapDataColumn(u16* dest, u16 height, const u16 *data, u16 wm)
{
    u16* d = dest;
    const u16* s = data;
    u16 i;

    i = height >> 2;
    while (i--)
    {
        *d++ = *s;
        s += wm;
        *d++ = *s;
        s += wm;
        *d++ = *s;
        s += wm;
        *d++ = *s;
        s += wm;
    }

    i = height & 3;
    while (i--)
    {
        *d++ = *s;
        s += wm;
    }
}

static void prepareTileMapDataRowEx(u16* dest, u16 width, const u16* data, u16 basetile)
{
    // we can increment both index and palette
    const u16 baseinc = basetile & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK);
    // we can only do logical OR on priority and HV flip
    const u16 baseor = basetile & (TILE_ATTR_PRIORITY_MASK | TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK);

    const u16* s = data;
    u16* d = dest;
    u16 i;

    i = width >> 2;
    // prepare map data for row update
    while (i--)
    {
        *d++ = baseor | (*s++ + baseinc);
        *d++ = baseor | (*s++ + baseinc);
        *d++ = baseor | (*s++ + baseinc);
        *d++ = baseor | (*s++ + baseinc);
    }

    i = width & 3;
    // prepare map data for row update
    while (i--) *d++ = baseor | (*s++ + baseinc);
}

static void prepareTileMapDataColumnEx(u16* dest, u16 height, const u16 *data, u16 basetile, u16 wm)
{
    // we can increment both index and palette
    const u16 baseinc = basetile & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK);
    // we can only do logical OR on priority and HV flip
    const u16 baseor = basetile & (TILE_ATTR_PRIORITY_MASK | TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK);

    // prepare map data for column update
    u16* d = dest;
    const u16* s = data;
    u16 i;

    i = height >> 2;
    while (i--)
    {
        *d++ = baseor | (*s + baseinc);
        s += wm;
        *d++ = baseor | (*s + baseinc);
        s += wm;
        *d++ = baseor | (*s + baseinc);
        s += wm;
        *d++ = baseor | (*s + baseinc);
        s += wm;
    }

    i = height & 3;
    while (i--)
    {
        *d++ = baseor | (*s + baseinc);
        s += wm;
    }
}
