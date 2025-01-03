#include "config.h"
#include "types.h"

#include "map.h"

#include "sys.h"
#include "mapper.h"
#include "vdp_tile.h"
#include "memory.h"
#include "tools.h"


//#define MAP_DEBUG
//#define MAP_PROFIL

// max screen width = 320   - (320 / 16) = 20
#define COLUMN_AHEAD    (20 + 1)
// max screen heigth = 240  - (240 / 16) = 15
#define ROW_AHEAD       (15 + 1)


// we don't want to share it
extern vu16 VBlankProcess;


// forward
static void updateMap(Map *map, s16 xt, s16 yt);
static void setMapColumn(Map *map, u16 column, u16 x, u16 y);
static void setMapRow(Map *map, u16 row, u16 x, u16 y);

static void prepareMapDataColumn(Map* map, u16* bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height);
static void prepareMapDataColumn_MTI8_BI8(Map* map, u16* bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height);
static void prepareMapDataColumn_MTI8_BI16(Map* map, u16* bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height);
static void prepareMapDataColumn_MTI16_BI8(Map* map, u16* bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height);
static void prepareMapDataColumn_MTI16_BI16(Map* map, u16* bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height);
static void prepareMapDataColumnEx_MTI8_BI8(Map* map, u16* bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height);
static void prepareMapDataColumnEx_MTI8_BI16(Map* map, u16* bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height);
static void prepareMapDataColumnEx_MTI16_BI8(Map* map, u16* bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height);
static void prepareMapDataColumnEx_MTI16_BI16(Map* map, u16* bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height);

static void prepareMapDataRow(Map* map, u16* bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width);
static void prepareMapDataRow_MTI8_BI8(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width);
static void prepareMapDataRow_MTI8_BI16(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width);
static void prepareMapDataRow_MTI16_BI8(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width);
static void prepareMapDataRow_MTI16_BI16(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width);
static void prepareMapDataRowEx_MTI8_BI8(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width);
static void prepareMapDataRowEx_MTI8_BI16(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width);
static void prepareMapDataRowEx_MTI16_BI8(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width);
static void prepareMapDataRowEx_MTI16_BI16(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width);

static u16 getMetaTile_MTI8_BI8(Map* map, u16 x, u16 y);
static u16 getMetaTile_MTI8_BI16(Map* map, u16 x, u16 y);
static u16 getMetaTile_MTI16_BI8(Map* map, u16 x, u16 y);
static u16 getMetaTile_MTI16_BI16(Map* map, u16 x, u16 y);

static void getMetaTilemapRect_MTI8_BI8(Map* map, u16 x, u16 y, u16 w, u16 h, u16* dest);
static void getMetaTilemapRect_MTI8_BI16(Map* map, u16 x, u16 y, u16 w, u16 h, u16* dest);
static void getMetaTilemapRect_MTI16_BI8(Map* map, u16 x, u16 y, u16 w, u16 h, u16* dest);
static void getMetaTilemapRect_MTI16_BI16(Map* map, u16 x, u16 y, u16 w, u16 h, u16* dest);


Map* NO_INLINE MAP_create(const MapDefinition* mapDef, VDPPlane plane, u16 baseTile)
{
    Map* result = allocateMap(mapDef);

    if (result == NULL) return result;

    result->w = mapDef->w;
    result->h = mapDef->h;

    // wrapping: note that it works only if w and h are power of 2
    result->wMask = getNextPow2(mapDef->w) - 1;
    result->hMask = getNextPow2(mapDef->h) - 1;

    u16 compression = mapDef->compression;
    u16 comp;

    // get metaTiles compression
    comp = compression & 0xF;
    // metaTiles are compressed ?
    if (comp != COMPRESSION_NONE) unpack(comp, (u8*) FAR_SAFE(mapDef->metaTiles, mapDef->numMetaTile * 2 * 4), (u8*) result->metaTiles);
    // init FAR pointer
    else result->metaTiles = FAR_SAFE(mapDef->metaTiles, mapDef->numMetaTile * 2 * 4);

    // get blocks data compression
    comp = (compression >> 4) & 0xF;
    // blocks data are compressed ?
    if (comp != COMPRESSION_NONE) unpack(comp, (u8*) FAR_SAFE(mapDef->blocks, mapDef->numBlock * 64 * ((mapDef->numMetaTile > 256)?2:1)), (u8*) result->blocks);
    // init FAR pointer
    else result->blocks = FAR_SAFE(mapDef->blocks, mapDef->numBlock * 64 * ((mapDef->numMetaTile > 256)?2:1));

    // get blocks indexes data compression
    comp = (compression >> 8) & 0xF;
    // blocks indexes data are compressed ?
    if (comp != COMPRESSION_NONE) unpack(comp, (u8*) FAR_SAFE(mapDef->blockIndexes, mulu(mapDef->w, mapDef->hp) * ((mapDef->numBlock > 256)?2:1)), (u8*) result->blockIndexes);
    // init FAR pointer
    else result->blockIndexes = FAR_SAFE(mapDef->blockIndexes, mapDef->w * mapDef->hp * ((mapDef->numBlock > 256)?2:1));

    // init FAR pointer
    result->blockRowOffsets = FAR_SAFE(mapDef->blockRowOffsets, mapDef->h * 2);

    // init base parameters
    result->plane = plane;
    // keep only base index and base palette
    result->baseTile = baseTile & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK | TILE_ATTR_PRIORITY_MASK);
    // init plane dimension
    result->planeWidth = planeWidth;
    result->planeHeight = planeHeight;
    result->planeWidthMaskAdj = (planeWidth >> 1) - 1;
    result->planeHeightMaskAdj = (planeHeight >> 1) - 1;
    result->planeWidthSftAdj = planeWidthSft + 2;
    // need full update on start
    result->firstUpdate = TRUE;

    // prepare function pointers
    if (mapDef->numMetaTile > 256)
    {
        // 16 bit for metatile index
        if (mapDef->numBlock > 256)
        {
            // 16 bit for block index
            if (result->baseTile == 0)
            {
                // no base attribute
                result->prepareMapDataColumnCB = &prepareMapDataColumn_MTI16_BI16;
                result->prepareMapDataRowCB = &prepareMapDataRow_MTI16_BI16;
            }
            else
            {
                // with base attribute
                result->prepareMapDataColumnCB = &prepareMapDataColumnEx_MTI16_BI16;
                result->prepareMapDataRowCB = &prepareMapDataRowEx_MTI16_BI16;
            }

            result->getMetaTileCB = &getMetaTile_MTI16_BI16;
            result->getMetaTilemapRectCB = &getMetaTilemapRect_MTI16_BI16;
        }
        else
        {
            // 8 bit for block index
            if (result->baseTile == 0)
            {
                // no base attribute
                result->prepareMapDataColumnCB = &prepareMapDataColumn_MTI16_BI8;
                result->prepareMapDataRowCB = &prepareMapDataRow_MTI16_BI8;
            }
            else
            {
                // with base attribute
                result->prepareMapDataColumnCB = &prepareMapDataColumnEx_MTI16_BI8;
                result->prepareMapDataRowCB = &prepareMapDataRowEx_MTI16_BI8;
            }

            result->getMetaTileCB = &getMetaTile_MTI16_BI8;
            result->getMetaTilemapRectCB = &getMetaTilemapRect_MTI16_BI8;
        }
    }
    else
    {
        // 8 bit for metatile index
        if (mapDef->numBlock > 256)
        {
            // 16 bit for block index
            if (result->baseTile == 0)
            {
                // no base attribute
                result->prepareMapDataColumnCB = &prepareMapDataColumn_MTI8_BI16;
                result->prepareMapDataRowCB = &prepareMapDataRow_MTI8_BI16;
            }
            else
            {
                // with base attribute
                result->prepareMapDataColumnCB = &prepareMapDataColumnEx_MTI8_BI16;
                result->prepareMapDataRowCB = &prepareMapDataRowEx_MTI8_BI16;
            }

            result->getMetaTileCB = &getMetaTile_MTI8_BI16;
            result->getMetaTilemapRectCB = &getMetaTilemapRect_MTI8_BI16;
        }
        else
        {
            // 8 bit for block index
            if (result->baseTile == 0)
            {
                // no base attribute
                result->prepareMapDataColumnCB = &prepareMapDataColumn_MTI8_BI8;
                result->prepareMapDataRowCB = &prepareMapDataRow_MTI8_BI8;
            }
            else
            {
                // with base attribute
                result->prepareMapDataColumnCB = &prepareMapDataColumnEx_MTI8_BI8;
                result->prepareMapDataRowCB = &prepareMapDataRowEx_MTI8_BI8;
            }

            result->getMetaTileCB = &getMetaTile_MTI8_BI8;
            result->getMetaTilemapRectCB = &getMetaTilemapRect_MTI8_BI8;
        }
    }

    // patch callback
    result->mapDataPatchCB = NULL;

    return result;
}

void MAP_release(Map* map)
{
    MEM_free(map);
}


void NO_INLINE MAP_scrollToEx(Map* map, u32 x, u32 y, bool forceRedraw)
{
    bool redraw = forceRedraw || map->firstUpdate;

    map->firstUpdate = FALSE;

    if (redraw)
    {
        // force full map update using row updates
        map->posX = x;
        map->posY = y - 256;
        map->lastXT = map->posX >> 4;
        map->lastYT = map->posY >> 4;
    }

    // update map
    updateMap(map, x >> 4, y >> 4);

    // X scrolling changed ?
    if (redraw || (map->posX != x))
    {
        u16 len;

        switch(VDP_getHorizontalScrollingMode())
        {
            case HSCROLL_PLANE:
                VDP_setHorizontalScrollVSync(map->plane, -x);
                break;

            case HSCROLL_TILE:
                // important to set scroll for all tile
                len = screenHeight / 8;
                memsetU16(map->hScrollTable, -x, len);
                VDP_setHorizontalScrollTile(map->plane, 0, (s16*) map->hScrollTable, len, DMA_QUEUE);
                break;

            case HSCROLL_LINE:
                // important to set scroll for all line
                len = screenHeight;
                memsetU16(map->hScrollTable, -x, len);
                VDP_setHorizontalScrollLine(map->plane, 0, (s16*) map->hScrollTable, len, DMA_QUEUE);
                break;
        }

        // store X position
        map->posX = x;
    }
    // Y scrolling changed ?
    if (redraw || (map->posY != y))
    {
        switch(VDP_getVerticalScrollingMode())
        {
            case VSCROLL_PLANE:
                VDP_setVerticalScrollVSync(map->plane, y);
                break;

            case VSCROLL_COLUMN:
                // important to set scroll for all column
                memsetU16(map->vScrollTable, y, 20);
                VDP_setVerticalScrollTile(map->plane, 0, (s16*) map->vScrollTable, 20, DMA_QUEUE);
                break;
        }

        // store Y position
        map->posY = y;
    }
}

void MAP_scrollTo(Map* map, u32 x, u32 y)
{
    MAP_scrollToEx(map, x, y, FALSE);
}


// xt, yt are in *meta* tile position
static void updateMap(Map* map, s16 xt, s16 yt)
{
    s16 cxt = map->lastXT;
    s16 cyt = map->lastYT;
    s16 deltaX = xt - cxt;
    s16 deltaY = yt - cyt;

    // no update --> exit
    if ((deltaX == 0) && (deltaY == 0)) return;

#ifdef MAP_DEBUG
    KLog_S4("updateMap xt=", xt, " yt=", yt, " deltaX=", deltaX, " deltaY=", deltaY);
#endif

    // clip to 16 metatiles row max (full screen update)
    if (deltaY > ROW_AHEAD)
    {
        cyt += deltaY - ROW_AHEAD;
        deltaY = ROW_AHEAD;
        // as we have a full screen update, we don't need column update then
        deltaX = 0;
    }
    // clip to 16 metatiles row max (full screen update)
    else if (deltaY < -ROW_AHEAD)
    {
        cyt += deltaY + ROW_AHEAD;
        deltaY = -ROW_AHEAD;
        // as we have a full screen update, we don't need column update then
        deltaX = 0;
    }
    // clip to 21 metatiles column max (full screen update)
    else if (deltaX > COLUMN_AHEAD)
    {
        cxt += deltaX - COLUMN_AHEAD;
        deltaX = COLUMN_AHEAD;
        // as we have a full screen update, we don't need row update then
        deltaY = 0;
    }
    // clip to 21 metatiles column max (full screen update)
    else if (deltaX < -COLUMN_AHEAD)
    {
        cxt += deltaX + COLUMN_AHEAD;
        deltaX = -COLUMN_AHEAD;
        // as we have a full screen update, we don't need row update then
        deltaY = 0;
    }

    if (deltaX > 0)
    {
        // update on right
        cxt += COLUMN_AHEAD;

        // need to update map column on right
        while(deltaX--)
        {
            setMapColumn(map, cxt & map->planeWidthMaskAdj, cxt, yt);
            cxt++;
        }
    }
    else
    {
        // need to update map column on left
        while(deltaX++)
        {
            cxt--;
            setMapColumn(map, cxt & map->planeWidthMaskAdj, cxt, yt);
        }
    }

    if (deltaY > 0)
    {
        // update on bottom
        cyt += ROW_AHEAD;

        // need to update map row on bottom
        while(deltaY--)
        {
            setMapRow(map, cyt & map->planeHeightMaskAdj, xt, cyt);
            cyt++;
        }
    }
    else
    {
        // need to update map row on top
        while(deltaY++)
        {
            cyt--;
            setMapRow(map, cyt & map->planeHeightMaskAdj, xt, cyt);
        }
    }

    map->lastXT = xt;
    map->lastYT = yt;
}

static void setMapColumn(Map *map, u16 column, u16 x, u16 y)
{
#ifdef MAP_DEBUG
    KLog_U3("setMapColumn column=", column, " x=", x, " y=", y);
#endif

#ifdef MAP_PROFIL
    u16 start = GET_VCOUNTER;
#endif

    const u16 ph = map->planeHeight;

    // allocate temp buffer for tilemap
    u16* buf = DMA_allocateTemp(ph * 2);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    if (!buf)
    {
        KLog("MAP - setMapColumn(..) failed: DMA temporary buffer is full");
        return;
    }
#endif

    // VRAM destination address
    const u16 vramAddr = ((map->plane == BG_A)?VDP_BG_A:VDP_BG_B) + (column * 4);
    // get plane width * 2
    const u16 pw2 = map->planeWidth * 2;

    // queue DMA (first column)
    DMA_queueDmaFast(DMA_VRAM, buf, vramAddr + 0, ph, pw2);
    // queue DMA (second column)
    DMA_queueDmaFast(DMA_VRAM, buf + ph, vramAddr + 2, ph, pw2);

#ifdef MAP_PROFIL
    u16 end = GET_VCOUNTER;
    KLog_S1("setMapColumn - DMA queue operations duration=", end - start);
#endif

    // 16 metatile = 32 tiles = 256 pixels (full screen height + 16 pixels)
    const u16 h = ROW_AHEAD;
    // clip Y against plane size
    const u16 yAdj = y & map->planeHeightMaskAdj;
    // get plane height
    const u16 ph2 = map->planeHeightMaskAdj + 1;

    // larger than plane height ? --> need to split
    if ((yAdj + h) > ph2)
    {
        const u16 h1 = ph2 - yAdj;

        // prepare first part of column data
        prepareMapDataColumn(map, buf + (yAdj * 2), buf + (yAdj * 2) + ph, x, y, h1);
        // prepare second part of column data
        prepareMapDataColumn(map, buf, buf + ph, x, y + h1, h - h1);
    }
    // no split needed
    else prepareMapDataColumn(map, buf + (yAdj * 2), buf + (yAdj * 2) + ph, x, y, h);
}

static void setMapRow(Map *map, u16 row, u16 x, u16 y)
{
#ifdef MAP_DEBUG
    KLog_U3("setMapRow row=", row, " x=", x, " y=", y);
#endif

#ifdef MAP_PROFIL
    u16 start = GET_VCOUNTER;
#endif

    const u16 pw = map->planeWidth;

    // allocate temp buffer for tilemap
    u16* buf = DMA_allocateTemp(pw * 2);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    if (!buf)
    {
        KLog("MAP - setMapRow(..) failed: DMA temporary buffer is full");
        return;
    }
#endif

    // VRAM destination address
    const u16 vramAddr = ((map->plane == BG_A)?VDP_BG_A:VDP_BG_B) + (row << map->planeWidthSftAdj);

    // queue DMA (first row)
    DMA_queueDmaFast(DMA_VRAM, buf, vramAddr + (pw * 0), pw, 2);
    // queue DMA (second row)
    DMA_queueDmaFast(DMA_VRAM, buf + pw, vramAddr + (pw * 2), pw, 2);

#ifdef MAP_PROFIL
    u16 end = GET_VCOUNTER;
    KLog_S1("setMapRow - DMA queue operations duration=", end - start);
#endif

    // 21 metatile = 42 tiles = 336 pixels (full screen width + 16 pixels)
    u16 w = COLUMN_AHEAD;
    // clip X against plane size
    const u16 xAdj = x & map->planeWidthMaskAdj;
    // get plane width (metatile)
    const u16 pw2 = map->planeWidthMaskAdj + 1;

    // larger than plane width ? --> need to split
    if ((xAdj + w) > pw2)
    {
        const u16 w1 = pw2 - xAdj;

        // prepare first part of row data
        prepareMapDataRow(map, buf + (xAdj * 2), buf + (xAdj * 2) + pw, x, y, w1);
        // prepare second part of row data
        prepareMapDataRow(map, buf, buf + pw, x + w1, y, w - w1);
    }
    // no split needed
    else prepareMapDataRow(map, buf + (xAdj * 2), buf + (xAdj * 2) + pw, x, y, w);
}

static void prepareMapDataColumn(Map *map, u16 *bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height)
{
#ifdef MAP_PROFIL
    u16 start = GET_VCOUNTER;
#endif

    map->prepareMapDataColumnCB(map, bufCol1, bufCol2, xm, ym, height);

    // patch data callback set ?
    if (map->mapDataPatchCB != NULL)
    {
        map->mapDataPatchCB(map, bufCol1, (xm * 2) + 0, ym * 2, COLUMN_UPDATE, height * 2);
        map->mapDataPatchCB(map, bufCol2, (xm * 2) + 1, ym * 2, COLUMN_UPDATE, height * 2);
    }

#ifdef MAP_PROFIL
    u16 end = GET_VCOUNTER;
    KLog_S2("prepareMapDataColumn - duration=", end - start, " h=", height);
#endif
}

static void prepareMapDataRow(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width)
{
#ifdef MAP_PROFIL
    u16 start = GET_VCOUNTER;
#endif

    map->prepareMapDataRowCB(map, bufRow1, bufRow2, xm, ym, width);

    // patch data callback set ?
    if (map->mapDataPatchCB != NULL)
    {
        map->mapDataPatchCB(map, bufRow1, xm * 2, (ym * 2) + 0, ROW_UPDATE, width * 2);
        map->mapDataPatchCB(map, bufRow2, xm * 2, (ym * 2) + 1, ROW_UPDATE, width * 2);
    }

#ifdef MAP_PROFIL
    u16 end = GET_VCOUNTER;
    KLog_S2("prepareMapDataRow - duration=", end - start, " w=", width);
#endif
}


static void prepareMapDataColumn_MTI8_BI8(Map *map, u16 *bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height)
{
    u16 *d1 = bufCol1;
    u16 *d2 = bufCol2;
    // number of metatile to decode
    u16 h = height;

    u8* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    // block X position
    const u16 xb = (xm / 8) & map->wMask;
    // block Y position
    u16 yb = (ym / 8) & map->hMask;
    // get block grid index
    u16 blockGridIndex = map->blockRowOffsets[yb] + xb;
    // get first block data pointer
    u8* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = xm & 7;
    // start y position inside block
    u16 yi = ym & 7;

    // block start offset
    block += blockFixedOffset + (yi * 8);

    u16* metaTiles = map->metaTiles;

    // remain metatile ?
    while(h--)
    {
        // get metatile index
        u8 metaTileInd = *block;
        // next row
        block += 8;

        // get metatile pointeur
        u16* metaTile = &metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++;
        *d2++ = *metaTile++;
        *d1++ = *metaTile++;
        *d2++ = *metaTile;

        // next metatile
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // next row
            yb = (yb + 1) & map->hMask;
            // get new block grid index
            blockGridIndex = map->blockRowOffsets[yb] + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataColumn_MTI8_BI16(Map *map, u16 *bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height)
{
    u16 *d1 = bufCol1;
    u16 *d2 = bufCol2;
    // number of metatile to decode
    u16 h = height;

    u16* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    // block X position
    const u16 xb = (xm / 8) & map->wMask;
    // block Y position
    u16 yb = (ym / 8) & map->hMask;
    // get block grid index
    u16 blockGridIndex = map->blockRowOffsets[yb] + xb;
    // get first block data pointer
    u8* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = xm & 7;
    // start y position inside block
    u16 yi = ym & 7;

    // block start offset
    block += blockFixedOffset + (yi * 8);

    u16* metaTiles = map->metaTiles;

    // remain metatile ?
    while(h--)
    {
        // get metatile index
        u8 metaTileInd = *block;
        // next row
        block += 8;

        // get metatile pointeur
        u16* metaTile = &metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++;
        *d2++ = *metaTile++;
        *d1++ = *metaTile++;
        *d2++ = *metaTile;

        // next metatile
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // next row
            yb = (yb + 1) & map->hMask;
            // get new block grid index
            blockGridIndex = map->blockRowOffsets[yb] + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataColumn_MTI16_BI8(Map *map, u16 *bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height)
{
    u16 *d1 = bufCol1;
    u16 *d2 = bufCol2;
    // number of metatile to decode
    u16 h = height;

    u8* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    // block X position
    const u16 xb = (xm / 8) & map->wMask;
    // block Y position
    u16 yb = (ym / 8) & map->hMask;
    // get block grid index
    u16 blockGridIndex = map->blockRowOffsets[yb] + xb;
    // get first block data pointer
    u16* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = xm & 7;
    // start y position inside block
    u16 yi = ym & 7;

    // block start offset
    block += blockFixedOffset + (yi * 8);

    u16* metaTiles = map->metaTiles;

    // remain metatile ?
    while(h--)
    {
        // get metatile index
        u16 metaTileInd = *block & TILE_INDEX_MASK;
        // next row
        block += 8;

        // get metatile pointeur
        u16* metaTile = &metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++;
        *d2++ = *metaTile++;
        *d1++ = *metaTile++;
        *d2++ = *metaTile;

        // next metatile
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // next row
            yb = (yb + 1) & map->hMask;
            // get new block grid index
            blockGridIndex = map->blockRowOffsets[yb] + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataColumn_MTI16_BI16(Map *map, u16 *bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height)
{
    u16 *d1 = bufCol1;
    u16 *d2 = bufCol2;
    // number of metatile to decode
    u16 h = height;

    u16* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    // block X position
    const u16 xb = (xm / 8) & map->wMask;
    // block Y position
    u16 yb = (ym / 8) & map->hMask;
    // get block grid index
    u16 blockGridIndex = map->blockRowOffsets[yb] + xb;
    // get first block data pointer
    u16* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = xm & 7;
    // start y position inside block
    u16 yi = ym & 7;

    // block start offset
    block += blockFixedOffset + (yi * 8);

    u16* metaTiles = map->metaTiles;

    // remain metatile ?
    while(h--)
    {
        // get metatile index
        u16 metaTileInd = *block & TILE_INDEX_MASK;
        // next row
        block += 8;

        // get metatile pointeur
        u16* metaTile = &metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++;
        *d2++ = *metaTile++;
        *d1++ = *metaTile++;
        *d2++ = *metaTile;

        // next metatile
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // next row
            yb = (yb + 1) & map->hMask;
            // get new block grid index
            blockGridIndex = map->blockRowOffsets[yb] + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataColumnEx_MTI8_BI8(Map *map, u16 *bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height)
{
    // we can add both base index and base palette
    const u16 baseAttr = map->baseTile;

    u16 *d1 = bufCol1;
    u16 *d2 = bufCol2;
    // number of metatile to decode
    u16 h = height;

    u8* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    // block X position
    const u16 xb = (xm / 8) & map->wMask;
    // block Y position
    u16 yb = (ym / 8) & map->hMask;
    // get block grid index
    u16 blockGridIndex = map->blockRowOffsets[yb] + xb;
    // get first block data pointer
    u8* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = xm & 7;
    // start y position inside block
    u16 yi = ym & 7;

    // block start offset
    block += blockFixedOffset + (yi * 8);

    u16* metaTiles = map->metaTiles;

    // remain metatile ?
    while(h--)
    {
        // get metatile index
        u8 metaTileInd = *block;
        // next row
        block += 8;

        // get metatile pointeur
        u16* metaTile = &metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile++ + baseAttr;
        *d1++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile + baseAttr;

        // next metatile
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // next row
            yb = (yb + 1) & map->hMask;
            // get new block grid index
            blockGridIndex = map->blockRowOffsets[yb] + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataColumnEx_MTI8_BI16(Map *map, u16 *bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height)
{
    // we can add both base index and base palette
    const u16 baseAttr = map->baseTile;

    u16 *d1 = bufCol1;
    u16 *d2 = bufCol2;
    // number of metatile to decode
    u16 h = height;

    u16* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    // block X position
    const u16 xb = (xm / 8) & map->wMask;
    // block Y position
    u16 yb = (ym / 8) & map->hMask;
    // get block grid index
    u16 blockGridIndex = map->blockRowOffsets[yb] + xb;
    // get first block data pointer
    u8* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = xm & 7;
    // start y position inside block
    u16 yi = ym & 7;

    // block start offset
    block += blockFixedOffset + (yi * 8);

    u16* metaTiles = map->metaTiles;

    // remain metatile ?
    while(h--)
    {
        // get metatile index
        u8 metaTileInd = *block;
        // next row
        block += 8;

        // get metatile pointeur
        u16* metaTile = &metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile++ + baseAttr;
        *d1++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile + baseAttr;

        // next metatile
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // next row
            yb = (yb + 1) & map->hMask;
            // get new block grid index
            blockGridIndex = map->blockRowOffsets[yb] + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataColumnEx_MTI16_BI8(Map *map, u16 *bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height)
{
    // we can add both base index and base palette
    const u16 baseAttr = map->baseTile;

    u16 *d1 = bufCol1;
    u16 *d2 = bufCol2;
    // number of metatile to decode
    u16 h = height;

    u8* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    // block X position
    const u16 xb = (xm / 8) & map->wMask;
    // block Y position
    u16 yb = (ym / 8) & map->hMask;
    // get block grid index
    u16 blockGridIndex = map->blockRowOffsets[yb] + xb;
    // get first block data pointer
    u16* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = xm & 7;
    // start y position inside block
    u16 yi = ym & 7;

    // block start offset
    block += blockFixedOffset + (yi * 8);

    u16* metaTiles = map->metaTiles;

    // remain metatile ?
    while(h--)
    {
        // get metatile index
        u16 metaTileInd = *block & TILE_INDEX_MASK;
        // next row
        block += 8;

        // get metatile pointeur
        u16* metaTile = &metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile++ + baseAttr;
        *d1++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile + baseAttr;

        // next metatile
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // next row
            yb = (yb + 1) & map->hMask;
            // get new block grid index
            blockGridIndex = map->blockRowOffsets[yb] + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataColumnEx_MTI16_BI16(Map *map, u16 *bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height)
{
    // we can add both base index and base palette
    const u16 baseAttr = map->baseTile;

    u16 *d1 = bufCol1;
    u16 *d2 = bufCol2;
    // number of metatile to decode
    u16 h = height;

    u16* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    // block X position
    const u16 xb = (xm / 8) & map->wMask;
    // block Y position
    u16 yb = (ym / 8) & map->hMask;
    // get block grid index
    u16 blockGridIndex = map->blockRowOffsets[yb] + xb;
    // get first block data pointer
    u16* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = xm & 7;
    // start y position inside block
    u16 yi = ym & 7;

    // block start offset
    block += blockFixedOffset + (yi * 8);

    u16* metaTiles = map->metaTiles;

    // remain metatile ?
    while(h--)
    {
        // get metatile index
        u16 metaTileInd = *block & TILE_INDEX_MASK;
        // next row
        block += 8;

        // get metatile pointeur
        u16* metaTile = &metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile++ + baseAttr;
        *d1++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile + baseAttr;

        // next metatile
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // next row
            yb = (yb + 1) & map->hMask;
            // get new block grid index
            blockGridIndex = map->blockRowOffsets[yb] + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataRow_MTI8_BI8(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width)
{
    u16 *d1 = bufRow1;
    u16 *d2 = bufRow2;
    // number of metatile to decode
    u16 w = width;

    u8* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    // block X position
    u16 xb = (xm / 8) & map->wMask;
    // base block grid index
    const u16 baseBlockGridIndex = map->blockRowOffsets[(ym / 8) & map->hMask];
    // block grid index
    u16 blockGridIndex = baseBlockGridIndex + xb;
    // get first block data pointer
    u8* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = (ym & 7) * 8;
    // start x position inside block
    u16 xi = xm & 7;

    // block start offset
    block += blockFixedOffset + xi;

    // remain metatile ?
    while(w--)
    {
        // metatile index; next col
        u8 metaTileInd = *block++;
        // get metatile pointeur
        u16* metaTile = &map->metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++;
        *d1++ = *metaTile++;
        *d2++ = *metaTile++;
        *d2++ = *metaTile;

        // next metatile
        xi++;
        // new block ?
        if (xi == 8)
        {
            xi = 0;
            // next column
            xb = (xb + 1) & map->wMask;
            // get new block grid index
            blockGridIndex = baseBlockGridIndex + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataRow_MTI8_BI16(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width)
{
    u16 *d1 = bufRow1;
    u16 *d2 = bufRow2;
    // number of metatile to decode
    u16 w = width;

    u16* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    // block X position
    u16 xb = (xm / 8) & map->wMask;
    // base block grid index
    const u16 baseBlockGridIndex = map->blockRowOffsets[(ym / 8) & map->hMask];
    // block grid index
    u16 blockGridIndex = baseBlockGridIndex + xb;
    // get first block data pointer
    u8* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = (ym & 7) * 8;
    // start x position inside block
    u16 xi = xm & 7;

    // block start offset
    block += blockFixedOffset + xi;

    // remain metatile ?
    while(w--)
    {
        // metatile index; next col
        u8 metaTileInd = *block++;
        // get metatile pointeur
        u16* metaTile = &map->metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++;
        *d1++ = *metaTile++;
        *d2++ = *metaTile++;
        *d2++ = *metaTile;

        // next metatile
        xi++;
        // new block ?
        if (xi == 8)
        {
            xi = 0;
            // next column
            xb = (xb + 1) & map->wMask;
            // get new block grid index
            blockGridIndex = baseBlockGridIndex + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataRow_MTI16_BI8(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width)
{
    u16 *d1 = bufRow1;
    u16 *d2 = bufRow2;
    // number of metatile to decode
    u16 w = width;

    u8* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    // block X position
    u16 xb = (xm / 8) & map->wMask;
    // base block grid index
    const u16 baseBlockGridIndex = map->blockRowOffsets[(ym / 8) & map->hMask];
    // block grid index
    u16 blockGridIndex = baseBlockGridIndex + xb;
    // get first block data pointer
    u16* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = (ym & 7) * 8;
    // start x position inside block
    u16 xi = xm & 7;

    // block start offset
    block += blockFixedOffset + xi;

    // remain metatile ?
    while(w--)
    {
        // metatile index; next col
        u16 metaTileInd = *block++ & TILE_INDEX_MASK;
        // get metatile pointeur
        u16* metaTile = &map->metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++;
        *d1++ = *metaTile++;
        *d2++ = *metaTile++;
        *d2++ = *metaTile;

        // next metatile
        xi++;
        // new block ?
        if (xi == 8)
        {
            xi = 0;
            // next column
            xb = (xb + 1) & map->wMask;
            // get new block grid index
            blockGridIndex = baseBlockGridIndex + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataRow_MTI16_BI16(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width)
{
    u16 *d1 = bufRow1;
    u16 *d2 = bufRow2;
    // number of metatile to decode
    u16 w = width;

    u16* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    // block X position
    u16 xb = (xm / 8) & map->wMask;
    // base block grid index
    const u16 baseBlockGridIndex = map->blockRowOffsets[(ym / 8) & map->hMask];
    // block grid index
    u16 blockGridIndex = baseBlockGridIndex + xb;
    // get first block data pointer
    u16* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = (ym & 7) * 8;
    // start x position inside block
    u16 xi = xm & 7;

    // block start offset
    block += blockFixedOffset + xi;

    // remain metatile ?
    while(w--)
    {
        // metatile index; next col
        u16 metaTileInd = *block++ & TILE_INDEX_MASK;
        // get metatile pointeur
        u16* metaTile = &map->metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++;
        *d1++ = *metaTile++;
        *d2++ = *metaTile++;
        *d2++ = *metaTile;

        // next metatile
        xi++;
        // new block ?
        if (xi == 8)
        {
            xi = 0;
            // next column
            xb = (xb + 1) & map->wMask;
            // get new block grid index
            blockGridIndex = baseBlockGridIndex + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataRowEx_MTI8_BI8(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width)
{
    // we can add both base index and base palette
    const u16 baseAttr = map->baseTile;

    u16 *d1 = bufRow1;
    u16 *d2 = bufRow2;
    // number of metatile to decode
    u16 w = width;

    u8* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    // block X position
    u16 xb = (xm / 8) & map->wMask;
    // base block grid index
    const u16 baseBlockGridIndex = map->blockRowOffsets[(ym / 8) & map->hMask];
    // block grid index
    u16 blockGridIndex = baseBlockGridIndex + xb;
    // get first block data pointer
    u8* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = (ym & 7) * 8;
    // start x position inside block
    u16 xi = xm & 7;

    // block start offset
    block += blockFixedOffset + xi;

    // remain metatile ?
    while(w--)
    {
        // metatile index; next col
        u8 metaTileInd = *block++;
        // get metatile pointeur
        u16* metaTile = &map->metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++ + baseAttr;
        *d1++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile + baseAttr;

        // next metatile
        xi++;
        // new block ?
        if (xi == 8)
        {
            xi = 0;
            // next column
            xb = (xb + 1) & map->wMask;
            // get new block grid index
            blockGridIndex = baseBlockGridIndex + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataRowEx_MTI8_BI16(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width)
{
    // we can add both base index and base palette
    const u16 baseAttr = map->baseTile;

    u16 *d1 = bufRow1;
    u16 *d2 = bufRow2;
    // number of metatile to decode
    u16 w = width;

    u16* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    // block X position
    u16 xb = (xm / 8) & map->wMask;
    // base block grid index
    const u16 baseBlockGridIndex = map->blockRowOffsets[(ym / 8) & map->hMask];
    // block grid index
    u16 blockGridIndex = baseBlockGridIndex + xb;
    // get first block data pointer
    u8* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = (ym & 7) * 8;
    // start x position inside block
    u16 xi = xm & 7;

    // block start offset
    block += blockFixedOffset + xi;

    // remain metatile ?
    while(w--)
    {
        // metatile index; next col
        u8 metaTileInd = *block++;
        // get metatile pointeur
        u16* metaTile = &map->metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++ + baseAttr;
        *d1++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile + baseAttr;

        // next metatile
        xi++;
        // new block ?
        if (xi == 8)
        {
            xi = 0;
            // next column
            xb = (xb + 1) & map->wMask;
            // get new block grid index
            blockGridIndex = baseBlockGridIndex + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataRowEx_MTI16_BI8(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width)
{
    // we can add both base index and base palette
    const u16 baseAttr = map->baseTile;

    u16 *d1 = bufRow1;
    u16 *d2 = bufRow2;
    // number of metatile to decode
    u16 w = width;

    u8* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    // block X position
    u16 xb = (xm / 8) & map->wMask;
    // base block grid index
    const u16 baseBlockGridIndex = map->blockRowOffsets[(ym / 8) & map->hMask];
    // block grid index
    u16 blockGridIndex = baseBlockGridIndex + xb;
    // get first block data pointer
    u16* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = (ym & 7) * 8;
    // start x position inside block
    u16 xi = xm & 7;

    // block start offset
    block += blockFixedOffset + xi;

    // remain metatile ?
    while(w--)
    {
        // metatile index; next col
        u16 metaTileInd = *block++ & TILE_INDEX_MASK;
        // get metatile pointeur
        u16* metaTile = &map->metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++ + baseAttr;
        *d1++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile + baseAttr;

        // next metatile
        xi++;
        // new block ?
        if (xi == 8)
        {
            xi = 0;
            // next column
            xb = (xb + 1) & map->wMask;
            // get new block grid index
            blockGridIndex = baseBlockGridIndex + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataRowEx_MTI16_BI16(Map* map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width)
{
    // we can add both base index and base palette
    const u16 baseAttr = map->baseTile;

    u16 *d1 = bufRow1;
    u16 *d2 = bufRow2;
    // number of metatile to decode
    u16 w = width;

    u16* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    // block X position
    u16 xb = (xm / 8) & map->wMask;
    // base block grid index
    const u16 baseBlockGridIndex = map->blockRowOffsets[(ym / 8) & map->hMask];
    // block grid index
    u16 blockGridIndex = baseBlockGridIndex + xb;
    // get first block data pointer
    u16* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = (ym & 7) * 8;
    // start x position inside block
    u16 xi = xm & 7;

    // block start offset
    block += blockFixedOffset + xi;

    // remain metatile ?
    while(w--)
    {
        // metatile index; next col
        u16 metaTileInd = *block++ & TILE_INDEX_MASK;
        // get metatile pointeur
        u16* metaTile = &map->metaTiles[2 * 2 * metaTileInd];

        // copy tiles attribute
        *d1++ = *metaTile++ + baseAttr;
        *d1++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile++ + baseAttr;
        *d2++ = *metaTile + baseAttr;

        // next metatile
        xi++;
        // new block ?
        if (xi == 8)
        {
            xi = 0;
            // next column
            xb = (xb + 1) & map->wMask;
            // get new block grid index
            blockGridIndex = baseBlockGridIndex + xb;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}


u16 MAP_getMetaTile(Map* map, u16 x, u16 y)
{
    return map->getMetaTileCB(map, x, y);
}

u16 MAP_getTile(Map* map, u16 x, u16 y)
{
    u16 metaTileInd = map->getMetaTileCB(map, x / 2, y / 2) & TILE_INDEX_MASK;
    u16* metaTile = &map->metaTiles[2 * 2 * metaTileInd];
    return metaTile[((y & 1) * 2) + (x & 1)];
}

void MAP_getMetaTilemapRect(Map* map, u16 x, u16 y, u16 w, u16 h, u16* dest)
{
    return map->getMetaTilemapRectCB(map, x, y, w, h, dest);
}

void MAP_getTilemapRect(Map* map, u16 x, u16 y, u16 w, u16 h, bool column, u16* dest)
{
    // destination
    u16 *d1 = dest;

    // column arrangment ?
    if (column)
    {
        u16 xi = x;
        u16 wi = w;

        // secondary destination
        u16 *d2 = dest + (h * 2);
        // get map col update function pointer
        void (*updateCol)(Map *map, u16 *d1, u16 *d2, u16 xm, u16 ym, u16 h) = map->prepareMapDataColumnCB;

        while(wi--)
        {
            updateCol(map, d1, d2, xi, y, h);
            // next metatile X
            xi++;
            // next metatile column
            d1 += h * 4;
            d2 += h * 4;
        }
    }
    // classic row arrangment
    else
    {
        u16 yi = y;
        u16 hi = h;

        // secondary destination
        u16 *d2 = dest + (w * 2);
        // get map row update function pointer
        void (*updateRow)(Map *map, u16 *d1, u16 *d2, u16 xm, u16 ym, u16 h) = map->prepareMapDataRowCB;

        while(hi--)
        {
            updateRow(map, d1, d2, x, yi, w);
            // next metatile Y
            yi++;
            // next metatile row
            d1 += w * 4;
            d2 += w * 4;
        }
    }
}


static u16 getMetaTile_MTI8_BI8(Map* map, u16 x, u16 y)
{
    u8* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    u16 xb = (x / 8) & map->wMask;
    u16 yb = (y / 8) & map->hMask;
    u8* block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
    u16 xi = x & 7;
    u16 yi = y & 7;

    return block[(yi * 8) + xi];
}

static u16 getMetaTile_MTI8_BI16(Map* map, u16 x, u16 y)
{
    u16* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    u16 xb = (x / 8) & map->wMask;
    u16 yb = (y / 8) & map->hMask;
    u8* block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
    u16 xi = x & 7;
    u16 yi = y & 7;

    return block[(yi * 8) + xi];
}

static u16 getMetaTile_MTI16_BI8(Map* map, u16 x, u16 y)
{
    u8* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    u16 xb = (x / 8) & map->wMask;
    u16 yb = (y / 8) & map->hMask;
    u16* block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
    u16 xi = x & 7;
    u16 yi = y & 7;

    return block[(yi * 8) + xi];
}

static u16 getMetaTile_MTI16_BI16(Map* map, u16 x, u16 y)
{
    u16* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    u16 xb = (x / 8) & map->wMask;
    u16 yb = (y / 8) & map->hMask;
    u16* block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
    u16 xi = x & 7;
    u16 yi = y & 7;

    return block[(yi * 8) + xi];
}

static void getMetaTilemapRect_MTI8_BI8(Map* map, u16 x, u16 y, u16 w, u16 h, u16* dest)
{
    u8* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    // start y position inside block
    u16 yi = y & 7;
    u16 hi = h;

    // block X position
    u16 xb = (x / 8) & map->wMask;
    // block Y position
    u16 yb = (y / 8) & map->hMask;
    // get first block data pointer
    u8* block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
    // add Y offset
    block += yi * 8;

    // remain metatile ?
    while(hi--)
    {
        // start x position inside block
        u16 xi = x & 7;
        u16 wi = w;

        // block start X offset
        block += xi;

        // remain metatile ?
        while(wi--)
        {
            // store metatile attribute; next col
            *dest++ = *block++;

            // next metatile X
            xi++;
            // new block ?
            if (xi == 8)
            {
                xi = 0;
                // next column
                xb = (xb + 1) & map->wMask;
                // get block data pointer
                block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
                // add Y offset
                block += yi * 8;
            }
        }

        // next metatile Y
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // next row
            yb = (yb + 1) & map->hMask;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
        }
    }
}

static void getMetaTilemapRect_MTI8_BI16(Map* map, u16 x, u16 y, u16 w, u16 h, u16* dest)
{
    u16* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    // start y position inside block
    u16 yi = y & 7;
    u16 hi = h;

    // block X position
    u16 xb = (x / 8) & map->wMask;
    // block Y position
    u16 yb = (y / 8) & map->hMask;
    // get first block data pointer
    u8* block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
    // add Y offset
    block += yi * 8;

    // remain metatile ?
    while(hi--)
    {
        // start x position inside block
        u16 xi = x & 7;
        u16 wi = w;

        // block start X offset
        block += xi;

        // remain metatile ?
        while(wi--)
        {
            // store metatile attribute; next col
            *dest++ = *block++;

            // next metatile X
            xi++;
            // new block ?
            if (xi == 8)
            {
                xi = 0;
                // next column
                xb = (xb + 1) & map->wMask;
                // get block data pointer
                block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
                // add Y offset
                block += yi * 8;
            }
        }

        // next metatile Y
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // next row
            yb = (yb + 1) & map->hMask;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
        }
    }
}

static void getMetaTilemapRect_MTI16_BI8(Map* map, u16 x, u16 y, u16 w, u16 h, u16* dest)
{
    u8* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    // start y position inside block
    u16 yi = y & 7;
    u16 hi = h;

    // block X position
    u16 xb = (x / 8) & map->wMask;
    // block Y position
    u16 yb = (y / 8) & map->hMask;
    // get first block data pointer
    u16* block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
    // add Y offset
    block += yi * 8;

    // remain metatile ?
    while(hi--)
    {
        // start x position inside block
        u16 xi = x & 7;
        u16 wi = w;

        // block start X offset
        block += xi;

        // remain metatile ?
        while(wi--)
        {
            // store metatile attribute; next col
            *dest++ = *block++;

            // next metatile X
            xi++;
            // new block ?
            if (xi == 8)
            {
                xi = 0;
                // next column
                xb = (xb + 1) & map->wMask;
                // get block data pointer
                block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
                // add Y offset
                block += yi * 8;
            }
        }

        // next metatile Y
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // next row
            yb = (yb + 1) & map->hMask;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
        }
    }
}

static void getMetaTilemapRect_MTI16_BI16(Map* map, u16 x, u16 y, u16 w, u16 h, u16* dest)
{
    u16* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    // start y position inside block
    u16 yi = y & 7;
    u16 hi = h;

    // block X position
    u16 xb = (x / 8) & map->wMask;
    // block Y position
    u16 yb = (y / 8) & map->hMask;
    // get first block data pointer
    u16* block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
    // add Y offset
    block += yi * 8;

    // remain metatile ?
    while(hi--)
    {
        // start x position inside block
        u16 xi = x & 7;
        u16 wi = w;

        // block start X offset
        block += xi;

        // remain metatile ?
        while(wi--)
        {
            // store metatile attribute; next col
            *dest++ = *block++;

            // next metatile X
            xi++;
            // new block ?
            if (xi == 8)
            {
                xi = 0;
                // next column
                xb = (xb + 1) & map->wMask;
                // get block data pointer
                block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
                // add Y offset
                block += yi * 8;
            }
        }

        // next metatile Y
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // next row
            yb = (yb + 1) & map->hMask;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[map->blockRowOffsets[yb] + xb]];
        }
    }
}


void MAP_setDataPatchCallback(Map* map, MapDataPatchCallback *CB)
{
    map->mapDataPatchCB = CB;
}


void MAP_overridePlaneSize(Map* map, u16 w, u16 h)
{
    // only 32, 64 or 128 accepted here
    if (w & 0x80)
    {
        map->planeWidth = 128;
        map->planeWidthMaskAdj = (128 >> 1) - 1;
        map->planeWidthSftAdj = 7 + 2;

        // plane height fixed to 32
        map->planeHeight = 32;
        map->planeHeightMaskAdj = (32 >> 1) - 1;
    }
    else if (w & 0x40)
    {
        map->planeWidth = 64;
        map->planeWidthMaskAdj = (64 >> 1) - 1;
        map->planeWidthSftAdj = 6 + 2;

        // only 64 or 32 accepted for plane height
        if (h & 0x40)
        {
            map->planeHeight = 64;
            map->planeHeightMaskAdj = (64 >> 1) - 1;
        }
        else
        {
            map->planeHeight = 32;
            map->planeHeightMaskAdj = (32 >> 1) - 1;
        }
    }
    else
    {
        map->planeWidth = 32;
        map->planeWidthMaskAdj = (32 >> 1) - 1;
        map->planeWidthSftAdj = 5 + 2;

        // plane height can be 128, 64 or 32
        if (h & 0x80)
        {
            map->planeHeight = 128;
            map->planeHeightMaskAdj = (128 >> 1) - 1;
        }
        else if (h & 0x40)
        {
            map->planeHeight = 64;
            map->planeHeightMaskAdj = (64 >> 1) - 1;
        }
        else
        {
            map->planeHeight = 32;
            map->planeHeightMaskAdj = (32 >> 1) - 1;
        }
    }
}
