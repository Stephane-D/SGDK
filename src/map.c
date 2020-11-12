#include "config.h"
#include "types.h"

#include "map.h"

#include "sys.h"
#include "vdp_tile.h"
#include "tools.h"


// we don't want to share them
extern vu16 VBlankProcess;


// forward
static void updateMap(Map* map, s16 xt, s16 yt);

static void setMapColumn(Map *map, u16 column, u16 x, u16 y);
static void setMapColumnEx(Map *map, u16 column, u16 y, u16 h, u16 xm, u16 ym);
static void setMapRow(Map *map, u16 row, u16 x, u16 y);
static void setMapRowEx(Map *map, u16 row, u16 x, u16 w, u16 xm, u16 ym);

static void prepareMapDataColumn(const MapDefinition *mapDef, u16 baseTile, u16* bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height);
static void prepareMapDataRow(const MapDefinition *mapDef, u16 baseTile, u16* bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width);


static s16 scrollX[2];
static s16 scrollY[2];
static bool updateScroll[2];


void MAP_init(const MapDefinition* mapDef, VDPPlane plane, u16 baseTile, u32 x, u32 y, Map *map)
{
    map->mapDefinition = mapDef;
    map->plane = plane;
    // keep only base index and base palette
    map->baseTile = baseTile & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK);
    // we want plane size mask in metatile
    map->planeWidthMask = (planeWidth >> 1) - 1;
    map->planeHeightMask = (planeHeight >> 1) - 1;

    // force full map update using row updates
    map->posX = x;
    map->posY = y - 240;
    map->lastXT = map->posX >> 4;
    map->lastYT = map->posY >> 4;

    // update map
    updateMap(map, x >> 4, y >> 4);

    // store position
    map->posX = x;
    map->posY = y;

    // store info for scrolling
    scrollX[plane] = -x;
    scrollY[plane] = y;
    updateScroll[plane] = TRUE;
    // add task for vblank process
    VBlankProcess |= PROCESS_MAP_TASK;
}

void MAP_scrollTo(Map* map, u32 x, u32 y)
{
    // nothing to do..
    if ((x == map->posX) && (y == map->posY)) return;

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

    if (deltaX > 0)
    {
        // clip to 21 metatiles max (full screen update)
        if (deltaX > 21)
        {
            cxt += deltaX - 21;
            deltaX = 21;
        }

        // need to update map column on right
        while(deltaX--)
        {
            setMapColumn(map, (cxt + 21) & map->planeWidthMask, cxt + 21, yt);
            cxt++;
        }
    }
    else if (deltaX < 0)
    {
        // clip to 21 metatiles max (full screen update)
        if (deltaX < -21)
        {
            cxt += deltaX + 21;
            deltaX = -21;
        }

        // need to update map column on left
        while(deltaX++)
        {
            cxt--;
            setMapColumn(map, cxt & map->planeWidthMask, cxt, yt);
        }
    }

    if (deltaY > 0)
    {
        // clip to 16 metatiles max (full screen update)
        if (deltaY > 16)
        {
            cyt += deltaY - 16;
            deltaY = 16;
        }

        // need to update map row on bottom
        while(deltaY--)
        {
            setMapRow(map, (cyt + 16) & map->planeHeightMask, xt, cyt + 16);
            cyt++;
        }
    }
    else if (deltaY < 0)
    {
        // clip to 21 metatiles max (full screen update)
        if (deltaY < -16)
        {
            cyt += deltaY + 16;
            deltaY = -16;
        }

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
    // 16 metatile = 32 tiles = 256 pixels (full screen height + 16 pixels)
    u16 h = 16;

//    KLog_U3("setMapColumn column=", column, " x=", x, " y=", y);

    // clip Y against plane size
    const u16 yAdj = y & map->planeHeightMask;
    // get plane height
    const u16 ph = map->planeHeightMask + 1;

    // larger than plane height ? --> need to split
    if ((yAdj + h) > ph)
    {
        u16 h1 = ph - yAdj;

        // first part
        setMapColumnEx(map, column, yAdj, h1, x, y);
        // second part
        setMapColumnEx(map, column, 0, h - h1, x, y + h1);
    }
    // no split needed
    setMapColumnEx(map, column, yAdj, h, x, y);
}

static void setMapColumnEx(Map *map, u16 column, u16 y, u16 h, u16 xm, u16 ym)
{
    const u16 addr = VDP_getPlaneAddress(map->plane, column * 2, y * 2);
    const u16 pw = planeWidth;

    // get temp buffer for first tile column and schedule DMA
    u16* bufCol1 = DMA_allocateAndQueueDma(DMA_VRAM, addr + 0, h * 2, pw * 2);
    // get temp buffer for second tile column and schedule DMA
    u16* bufCol2 = DMA_allocateAndQueueDma(DMA_VRAM, addr + 2, h * 2, pw * 2);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    if (!bufCol1 || !bufCol2)
    {
        KLog("MAP - setMapColumnEx(..) failed: DMA temporary buffer is full");
        return;
    }
#endif

    // then prepare data in buffer that will be transferred by DMA
    prepareMapDataColumn(map->mapDefinition, map->baseTile, bufCol1, bufCol2, xm, ym, h);
}

static void setMapRow(Map *map, u16 row, u16 x, u16 y)
{
    // 21 metatile = 42 tiles = 336 pixels (full screen width + 16 pixels)
    u16 w = 21;

//    KLog_U3("setMapRow row=", row, " x=", x, " y=", y);

    // clip X against plane size
    const u16 xAdj = x & map->planeWidthMask;
    // get plane width
    const u16 pw = map->planeWidthMask + 1;

    // larger than plane width ? --> need to split
    if ((xAdj + w) > pw)
    {
        u16 w1 = pw - xAdj;

        // first part
        setMapRowEx(map, row, xAdj, w1, x, y);
        // second part
        setMapRowEx(map, row, 0, w - w1, x + w1, y);
    }
    // no split needed
    else setMapRowEx(map, row, xAdj, w, x, y);
}

static void setMapRowEx(Map *map, u16 row, u16 x, u16 w, u16 xm, u16 ym)
{
    const u16 addr = VDP_getPlaneAddress(map->plane, x * 2, row * 2);

    // get temp buffer for first tile row and schedule DMA
    u16* bufRow1 = DMA_allocateAndQueueDma(DMA_VRAM, addr + 0, w * 2, 2);
    // get temp buffer for second tile row and schedule DMA
    u16* bufRow2 = DMA_allocateAndQueueDma(DMA_VRAM, addr + (planeWidth * 2), w * 2, 2);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    if (!bufRow1 || !bufRow2)
    {
        KLog("MAP - setMapRowEx(..) failed: DMA temporary buffer is full");
        return;
    }
#endif

    // then prepare data in buffer that will be transferred by DMA
    prepareMapDataRow(map->mapDefinition, map->baseTile, bufRow1, bufRow2, xm, ym, w);
}


static void prepareMapDataColumn(const MapDefinition *mapDef, u16 baseTile, u16 *bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height)
{
    // we can add both base index and base palette
    const u16 baseAttr = baseTile & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK);

    u16 *d1 = bufCol1;
    u16 *d2 = bufCol2;
    // number of metatile to decode
    u16 h = height;

    // block grid index
    u16 blockGridIndex = mapDef->blockRowOffsets[ym / 8] + (xm / 8);
    // get first block data pointer
    u16* block = &mapDef->blocks[8 * 8 * mapDef->blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = xm & 7;
    // start y position inside block
    u16 yi = ym & 7;

    // block start offset
    block += blockFixedOffset + (yi * 8);

    // remain metatile ?
    while(h--)
    {
        // metatile attribute
        u16 metaTileAttr = *block;
        // next row
        block += 8;

        // add priority bit to base attribut
        u16 ba = baseAttr + (metaTileAttr & TILE_ATTR_PRIORITY_MASK);
        u16 flip = metaTileAttr & (TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK);

        // get metatile pointeur
        u16* metaTile = &mapDef->metaTiles[2 * 2 * (metaTileAttr & TILE_INDEX_MASK)];

        switch(flip)
        {
            // no flip
            case 0:
                // 0,0
                *d1++ = *metaTile++ + ba;
                // 1,0
                *d2++ = *metaTile++ + ba;
                // 0,1
                *d1++ = *metaTile++ + ba;
                // 1,1
                *d2++ = *metaTile + ba;
                break;

            // V flip
            case TILE_ATTR_VFLIP_MASK:
                // 0,1
                *d1++ = (metaTile[2] ^ flip) + ba;
                // 1,1
                *d2++ = (metaTile[3] ^ flip) + ba;
                // 0,0
                *d1++ = (metaTile[0] ^ flip) + ba;
                // 1,0
                *d2++ = (metaTile[1] ^ flip) + ba;
                break;

            // H flip
            case TILE_ATTR_HFLIP_MASK:
                // 0,0
                *d2++ = (*metaTile++ ^ flip) + ba;
                // 1,0
                *d1++ = (*metaTile++ ^ flip) + ba;
                // 0,1
                *d2++ = (*metaTile++ ^ flip) + ba;
                // 1,1
                *d1++ = (*metaTile ^ flip) + ba;
                break;

            // HV flip
            case TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK:
                // 1,1
                *d1++ = (metaTile[3] ^ flip) + ba;
                // 0,1
                *d2++ = (metaTile[2] ^ flip) + ba;
                // 1,0
                *d1++ = (metaTile[1] ^ flip) + ba;
                // 0,0
                *d2++ = (metaTile[0] ^ flip) + ba;
                break;
        }

        // next metatile
        yi++;
        // new block ?
        if (yi == 8)
        {
            yi = 0;
            // increment block grid index (next row)
            blockGridIndex += mapDef->w;
            // get block data pointer
            block = &mapDef->blocks[8 * 8 * mapDef->blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}

static void prepareMapDataRow(const MapDefinition *mapDef, u16 baseTile, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width)
{
    // we can add both base index and base palette
    const u16 baseAttr = baseTile & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK);

    u16 *d1 = bufRow1;
    u16 *d2 = bufRow2;
    // number of metatile to decode
    u16 w = width;

    // block grid index
    u16 blockGridIndex = mapDef->blockRowOffsets[ym / 8] + (xm / 8);
    // get first block data pointer
    u16* block = &mapDef->blocks[8 * 8 * mapDef->blockIndexes[blockGridIndex]];
    // internal block fixed offset (will never change)
    u16 blockFixedOffset = (ym & 7) * 8;
    // start x position inside block
    u16 xi = xm & 7;

    // block start offset
    block += blockFixedOffset + xi;

    // remain metatile ?
    while(w--)
    {
        // metatile attribute; next col
        u16 metaTileAttr = *block++;

        // add priority bit to base attribut
        u16 ba = baseAttr + (metaTileAttr & TILE_ATTR_PRIORITY_MASK);
        u16 flip = metaTileAttr & (TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK);

        // get metatile pointeur
        u16* metaTile = &mapDef->metaTiles[2 * 2 * (metaTileAttr & TILE_INDEX_MASK)];

        switch(flip)
        {
            // no flip
            case 0:
                // 0,0
                *d1++ = *metaTile++ + ba;
                // 1,0
                *d1++ = *metaTile++ + ba;
                // 0,1
                *d2++ = *metaTile++ + ba;
                // 1,1
                *d2++ = *metaTile + ba;
                break;

            // V flip
            case TILE_ATTR_VFLIP_MASK:
                // 0,0
                *d2++ = (*metaTile++ ^ flip) + ba;
                // 1,0
                *d2++ = (*metaTile++ ^ flip) + ba;
                // 0,1
                *d1++ = (*metaTile++ ^ flip) + ba;
                // 1,1
                *d1++ = (*metaTile ^ flip) + ba;
                break;

            // H flip
            case TILE_ATTR_HFLIP_MASK:
                // 1,0
                *d1++ = (metaTile[1] ^ flip) + ba;
                // 0,0
                *d1++ = (metaTile[0] ^ flip) + ba;
                // 1,1
                *d2++ = (metaTile[3] ^ flip) + ba;
                // 0,1
                *d2++ = (metaTile[2] ^ flip) + ba;
                break;

            // HV flip
            case TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK:
                // 1,1
                *d1++ = (metaTile[3] ^ flip) + ba;
                // 0,1
                *d1++ = (metaTile[2] ^ flip) + ba;
                // 1,0
                *d2++ = (metaTile[1] ^ flip) + ba;
                // 0,0
                *d2++ = (metaTile[0] ^ flip) + ba;
                break;
        }

        // next metatile
        xi++;
        // new block ?
        if (xi == 8)
        {
            xi = 0;
            // increment block grid index (next column)
            blockGridIndex++;
            // get block data pointer
            block = &mapDef->blocks[8 * 8 * mapDef->blockIndexes[blockGridIndex]];
            // add base offset
            block += blockFixedOffset;
        }
    }
}


u16 MAP_getMetaTile(const MapDefinition* mapDef, u16 x, u16 y)
{
    u16 xb = x / 8;
    u16 yb = y / 8;
    u16 blockInd = mapDef->blockIndexes[mapDef->blockRowOffsets[yb] + xb];
    u16* block = &mapDef->blocks[8 * 8 * blockInd];
    u16 xi = x & 7;
    u16 yi = y & 7;

    return block[(yi * 8) + xi];
}

u16 MAP_getTile(const MapDefinition* mapDef, u16 x, u16 y)
{
    u16 metaTileAttr = MAP_getMetaTile(mapDef, x / 2, y / 2);
    u16 metaPrio = metaTileAttr & TILE_ATTR_PRIORITY_MASK;
    u16 metaFlip = metaTileAttr & (TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK);
    u16 metaInd = metaTileAttr & TILE_INDEX_MASK;
    u16* metaTile = &mapDef->metaTiles[2 * 2 * metaInd];

    u16 xi = x & 1;
    u16 yi = y & 1;

    if (metaFlip & TILE_ATTR_HFLIP_MASK) xi ^= 1;
    if (metaFlip & TILE_ATTR_VFLIP_MASK) yi ^= 1;

    return (metaTile[(yi * 2) + xi] ^ metaFlip) | metaPrio;
}


void MAP_getMetaTilemapRect(const MapDefinition* mapDef, u16 x, u16 y, u16 w, u16 h, u16* dest)
{
    // start y position inside block
    u16 yi = y & 7;
    u16 hi = h;

    // block grid index
    u16 blockGridIndex = mapDef->blockRowOffsets[y / 8] + (x / 8);
    // get first block data pointer
    u16* block = &mapDef->blocks[8 * 8 * mapDef->blockIndexes[blockGridIndex]];
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
                block = &mapDef->blocks[8 * 8 * mapDef->blockIndexes[blockGridIndex]];
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
            blockGridIndex += mapDef->w;
            // get block data pointer
            block = &mapDef->blocks[8 * 8 * mapDef->blockIndexes[blockGridIndex]];
            // block Y offset
            blockYOffset = yi * 8;
        }
    }
}

void MAP_getTilemapRect(const MapDefinition* mapDef, u16 x, u16 y, u16 w, u16 h, u16 baseTile, bool column, u16* dest)
{
    // destination
    u16 *d1 = dest;

    // column arrangment ?
    if (column)
    {
        u16 xi = x;
        u16 wi = w;

        // secondary destination
        u16 *d2 = dest + h;

        while(wi--)
        {
            prepareMapDataColumn(mapDef, baseTile, d1, d2, xi, y, h);
            // next metatile X
            xi++;
            // next metatile column
            d1 += h * 2;
            d2 += h * 2;
        }
    }
    // classic row arrangment
    else
    {
        u16 yi = y;
        u16 hi = h;

        // secondary destination
        u16 *d2 = dest + w;

        while(hi--)
        {
            prepareMapDataRow(mapDef, baseTile, d1, d2, x, yi, w);
            // next metatile Y
            yi++;
            // next metatile row
            d1 += w * 2;
            d2 += w * 2;
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