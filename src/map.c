#include "config.h"
#include "types.h"

#include "map.h"

#include "sys.h"
#include "mapper.h"
#include "vdp_tile.h"
#include "tools.h"


//#define MAP_DEBUG
//#define MAP_PROFIL


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


static s16 scrollX[2];
static s16 scrollY[2];
static bool updateScroll[2];


Map* MAP_create(const MapDefinition* mapDef, VDPPlane plane, u16 baseTile)
{
    Map* result = allocateMap(mapDef);

    if (result == NULL) return result;

    result->w = mapDef->w;
    result->h = mapDef->h;

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
    if (comp != COMPRESSION_NONE) unpack(comp, (u8*) FAR_SAFE(mapDef->blockIndexes, mapDef->w * mapDef->hp * ((mapDef->numBlock > 256)?2:1)), (u8*) result->blockIndexes);
    // init FAR pointer
    else result->blockIndexes = FAR_SAFE(mapDef->blockIndexes, mapDef->w * mapDef->hp * ((mapDef->numBlock > 256)?2:1));

    // init FAR pointer
    result->blockRowOffsets = FAR_SAFE(mapDef->blockRowOffsets, mapDef->h * 2);

    // init base parameters
    result->plane = plane;
    // keep only base index and base palette
    result->baseTile = baseTile & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK | TILE_ATTR_PRIORITY_MASK);
    // mark for init
    result->planeWidthMask = 0;
    result->planeHeightMask = 0;

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

    return result;
}

void MAP_scrollTo(Map* map, u32 x, u32 y)
{
    // first scroll ?
    if (map->planeWidthMask == 0)
    {
        // init plane dimension
        map->planeWidthMask = (planeWidth >> 1) - 1;
        map->planeHeightMask = (planeHeight >> 1) - 1;

        // force full map update using row updates
        map->posX = x;
        map->posY = y - 256;
        map->lastXT = map->posX >> 4;
        map->lastYT = map->posY >> 4;
    }
    // nothing to do..
    else if ((x == map->posX) && (y == map->posY)) return;

    // update map
    updateMap(map, x >> 4, y >> 4);

    // store position
    map->posX = x;
    map->posY = y;

    // store info for scrolling
    scrollX[map->plane] = -x;
    scrollY[map->plane] = y;
    updateScroll[map->plane] = TRUE;
    // add task for vblank process
    VBlankProcess |= PROCESS_MAP_TASK;
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

    // clip to 21 metatiles column max (full screen update)
    if (deltaX > 21)
    {
        cxt += deltaX - 21;
        deltaX = 21;
        // as we have a full screen update, we don't need row update then
        deltaY = 0;
    }
    // clip to 21 metatiles column max (full screen update)
    else if (deltaX < -21)
    {
        cxt += deltaX + 21;
        deltaX = -21;
        // as we have a full screen update, we don't need row update then
        deltaY = 0;
    }
    // clip to 16 metatiles row max (full screen update)
    else if (deltaY > 16)
    {
        cyt += deltaY - 16;
        deltaY = 16;
        // as we have a full screen update, we don't need column update then
        deltaX = 0;
    }
    // clip to 16 metatiles row max (full screen update)
    else if (deltaY > 16)
    {
        cyt += deltaY - 16;
        deltaY = 16;
        // as we have a full screen update, we don't need column update then
        deltaX = 0;
    }

    if (deltaX > 0)
    {
        // update on right
        cxt += 21;

        // need to update map column on right
        while(deltaX--)
        {
            setMapColumn(map, cxt & map->planeWidthMask, cxt, yt);
            cxt++;
        }
    }
    else
    {
        // need to update map column on left
        while(deltaX++)
        {
            cxt--;
            setMapColumn(map, cxt & map->planeWidthMask, cxt, yt);
        }
    }

    if (deltaY > 0)
    {
        // update on bottom
        cyt += 16;

        // need to update map row on bottom
        while(deltaY--)
        {
            setMapRow(map, cyt & map->planeHeightMask, xt, cyt);
            cyt++;
        }
    }
    else
    {
        // need to update map row on top
        while(deltaY++)
        {
            cyt--;
            setMapRow(map, cyt & map->planeHeightMask, xt, cyt);
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

    const u16 ph = planeHeight;

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
    const u16 pw2 = planeWidth * 2;

    // queue DMA (first column)
    DMA_queueDmaFast(DMA_VRAM, buf, vramAddr + 0, ph, pw2);
    // queue DMA (second column)
    DMA_queueDmaFast(DMA_VRAM, buf + ph, vramAddr + 2, ph, pw2);

#ifdef MAP_PROFIL
    u16 end = GET_VCOUNTER;
    KLog_S1("setMapColumn - DMA queue operations duration=", end - start);
#endif

    // 16 metatile = 32 tiles = 256 pixels (full screen height + 16 pixels)
    const u16 h = 16;
    // clip Y against plane size
    const u16 yAdj = y & map->planeHeightMask;
    // get plane height
    const u16 ph2 = map->planeHeightMask + 1;

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

    const u16 pw = planeWidth;

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
    const u16 vramAddr = ((map->plane == BG_A)?VDP_BG_A:VDP_BG_B) + (row << (planeWidthSft + 2));

    // queue DMA (first column)
    DMA_queueDmaFast(DMA_VRAM, buf, vramAddr + (pw * 0), pw, 2);
    // queue DMA (second column)
    DMA_queueDmaFast(DMA_VRAM, buf + pw, vramAddr + (pw * 2), pw, 2);

#ifdef MAP_PROFIL
    u16 end = GET_VCOUNTER;
    KLog_S1("setMapRow - DMA queue operations duration=", end - start);
#endif

    // 21 metatile = 42 tiles = 336 pixels (full screen width + 16 pixels)
    u16 w = 21;
    // clip X against plane size
    const u16 xAdj = x & map->planeWidthMask;
    // get plane width
    const u16 pw2 = map->planeWidthMask + 1;

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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next row)
            blockGridIndex += map->w;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next row)
            blockGridIndex += map->w;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next row)
            blockGridIndex += map->w;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next row)
            blockGridIndex += map->w;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next row)
            blockGridIndex += map->w;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next row)
            blockGridIndex += map->w;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next row)
            blockGridIndex += map->w;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next row)
            blockGridIndex += map->w;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next column)
            blockGridIndex++;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next column)
            blockGridIndex++;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next column)
            blockGridIndex++;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next column)
            blockGridIndex++;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next column)
            blockGridIndex++;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next column)
            blockGridIndex++;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next column)
            blockGridIndex++;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[ym / 8] + (xm / 8);
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
            // increment block grid index (next column)
            blockGridIndex++;
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

    u16 xb = x / 8;
    u16 yb = y / 8;
    u8 blockInd = blockIndexes[map->blockRowOffsets[yb] + xb];
    u8* block = &blocks[8 * 8 * blockInd];
    u16 xi = x & 7;
    u16 yi = y & 7;

    return block[(yi * 8) + xi];
}

static u16 getMetaTile_MTI8_BI16(Map* map, u16 x, u16 y)
{
    u16* blockIndexes = map->blockIndexes;
    u8* blocks = map->blocks;

    u16 xb = x / 8;
    u16 yb = y / 8;
    u16 blockInd = blockIndexes[map->blockRowOffsets[yb] + xb];
    u8* block = &blocks[8 * 8 * blockInd];
    u16 xi = x & 7;
    u16 yi = y & 7;

    return block[(yi * 8) + xi];
}

static u16 getMetaTile_MTI16_BI8(Map* map, u16 x, u16 y)
{
    u8* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    u16 xb = x / 8;
    u16 yb = y / 8;
    u8 blockInd = blockIndexes[map->blockRowOffsets[yb] + xb];
    u16* block = &blocks[8 * 8 * blockInd];
    u16 xi = x & 7;
    u16 yi = y & 7;

    return block[(yi * 8) + xi];
}

static u16 getMetaTile_MTI16_BI16(Map* map, u16 x, u16 y)
{
    u16* blockIndexes = map->blockIndexes;
    u16* blocks = map->blocks;

    u16 xb = x / 8;
    u16 yb = y / 8;
    u16 blockInd = blockIndexes[map->blockRowOffsets[yb] + xb];
    u16* block = &blocks[8 * 8 * blockInd];
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[y / 8] + (x / 8);
    // get first block data pointer
    u8* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // block Y offset
    u16 blockYOffset = yi * 8;

    // remain metatile ?
    while(hi--)
    {
        // start x position inside block
        u16 xi = x & 7;
        u16 wi = w;

        // block start offset
        block += blockYOffset + xi;

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
                // increment block grid index (next column)
                blockGridIndex++;
                // get block data pointer
                block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
                // add Y offset
                block += blockYOffset;
            }
        }

        // next metatile Y
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // increment block grid index (next row)
            blockGridIndex += map->w;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // block Y offset
            blockYOffset = yi * 8;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[y / 8] + (x / 8);
    // get first block data pointer
    u8* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // block Y offset
    u16 blockYOffset = yi * 8;

    // remain metatile ?
    while(hi--)
    {
        // start x position inside block
        u16 xi = x & 7;
        u16 wi = w;

        // block start offset
        block += blockYOffset + xi;

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
                // increment block grid index (next column)
                blockGridIndex++;
                // get block data pointer
                block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
                // add Y offset
                block += blockYOffset;
            }
        }

        // next metatile Y
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // increment block grid index (next row)
            blockGridIndex += map->w;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // block Y offset
            blockYOffset = yi * 8;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[y / 8] + (x / 8);
    // get first block data pointer
    u16* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // block Y offset
    u16 blockYOffset = yi * 8;

    // remain metatile ?
    while(hi--)
    {
        // start x position inside block
        u16 xi = x & 7;
        u16 wi = w;

        // block start offset
        block += blockYOffset + xi;

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
                // increment block grid index (next column)
                blockGridIndex++;
                // get block data pointer
                block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
                // add Y offset
                block += blockYOffset;
            }
        }

        // next metatile Y
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // increment block grid index (next row)
            blockGridIndex += map->w;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // block Y offset
            blockYOffset = yi * 8;
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

    // block grid index
    u16 blockGridIndex = map->blockRowOffsets[y / 8] + (x / 8);
    // get first block data pointer
    u16* block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
    // block Y offset
    u16 blockYOffset = yi * 8;

    // remain metatile ?
    while(hi--)
    {
        // start x position inside block
        u16 xi = x & 7;
        u16 wi = w;

        // block start offset
        block += blockYOffset + xi;

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
                // increment block grid index (next column)
                blockGridIndex++;
                // get block data pointer
                block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
                // add Y offset
                block += blockYOffset;
            }
        }

        // next metatile Y
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // increment block grid index (next row)
            blockGridIndex += map->w;
            // get block data pointer
            block = &blocks[8 * 8 * blockIndexes[blockGridIndex]];
            // block Y offset
            blockYOffset = yi * 8;
        }
    }
}


bool MAP_doVBlankProcess()
{
    if (updateScroll[BG_A])
    {
        VDP_setHorizontalScroll(BG_A, scrollX[BG_A]);
        VDP_setVerticalScroll(BG_A, scrollY[BG_A]);
        updateScroll[BG_A] = FALSE;
    }
    if (updateScroll[BG_B])
    {
        VDP_setHorizontalScroll(BG_B, scrollX[BG_B]);
        VDP_setVerticalScroll(BG_B, scrollY[BG_B]);
        updateScroll[BG_B] = FALSE;
    }

    // no more task
    return FALSE;
}