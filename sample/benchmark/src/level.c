#include <genesis.h>

#include "inc/level.h"

#include "res/gfx.h"


// used for alternate map update mode
u16* tilemapBuf;
u16 bufOffset;

// maps (BGA and BGB) position (tile) for alternate method
s16 mapMetaTilePosX[2];
s16 mapMetaTilePosY[2];
// maps (BGA and BGB)
Map *bgb;
Map *bga;

// BG start tile index
u16 bgBaseTileIndex[2];


u16 LEVEL_init(u16 vramIndex)
{
    u16 ind;

    // 42 * 32 = complete tilemap update; * 2 as we have 2 full plans to update potentially
    tilemapBuf = MEM_alloc(42 * 32 * 2 * 2);
    // initialize variables
    bufOffset = 0;

    // BGB/BGA tile position (force refresh)
    mapMetaTilePosX[0] = -42;
    mapMetaTilePosY[0] = 0;
    mapMetaTilePosX[1] = -42;
    mapMetaTilePosY[1] = 0;

    // load background tilesets in VRAM
    ind = vramIndex;
    bgBaseTileIndex[0] = ind;
    VDP_loadTileSet(&bga_tileset, ind, DMA);
    ind += bga_tileset.numTile;
    bgBaseTileIndex[1] = ind;
    VDP_loadTileSet(&bgb_tileset, ind, DMA);
    ind += bgb_tileset.numTile;

    // init backgrounds
    bga = MAP_create(&bga_map, BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, bgBaseTileIndex[0]));
    bgb = MAP_create(&bgb_map, BG_B, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, bgBaseTileIndex[1]));

    // return end VRAM index
    return ind;
}

void LEVEL_end()
{
    MEM_free(tilemapBuf);
    MAP_release(bga);
    MAP_release(bgb);
}


// this is just to show how use the MAP_getTilemapRect(..) method
// if we weed to actually access tilemap data and do manual tilemap update to VDP
void LEVEL_updateMapAlternate(VDPPlane plane, Map* map, s16 xmt, s16 ymt)
{
    // BGA = 0; BGB = 1
    s16 cxmt = mapMetaTilePosX[plane];
    s16 cymt = mapMetaTilePosY[plane];
    s16 deltaX = xmt - cxmt;
    s16 deltaY = ymt - cymt;

    // no update --> exit
    if ((deltaX == 0) && (deltaY == 0)) return;

    // clip to 21 metatiles column max (full screen update)
    if (deltaX > 21)
    {
        cxmt += deltaX - 21;
        deltaX = 21;
        deltaY = 0;
    }
    // clip to 21 metatiles column max (full screen update)
    else if (deltaX < -21)
    {
        cxmt += deltaX + 21;
        deltaX = -21;
        deltaY = 0;
    }
    // clip to 16 metatiles row max (full screen update)
    else if (deltaY > 16)
    {
        cymt += deltaY - 16;
        deltaY = 16;
        deltaX = 0;
    }
    // clip to 16 metatiles row max (full screen update)
    else if (deltaY < -16)
    {
        cymt += deltaY + 16;
        deltaY = -16;
        deltaX = 0;
    }

    if (deltaX > 0)
    {
        // update on right
        cxmt += 21;

        // need to update map column on right
        while(deltaX--)
        {
            MAP_getTilemapRect(map, cxmt, ymt, 1, 16, TRUE, tilemapBuf + bufOffset);
            VDP_setTileMapDataColumnFast(plane, tilemapBuf + bufOffset, (cxmt * 2) + 0, ymt * 2, 16 * 2, DMA_QUEUE);
            // next column
            bufOffset += 16 * 2;
            VDP_setTileMapDataColumnFast(plane, tilemapBuf + bufOffset, (cxmt * 2) + 1, ymt * 2, 16 * 2, DMA_QUEUE);
            // next column
            bufOffset += 16 * 2;
            cxmt++;
        }
    }
    else
    {
        // need to update map column on left
        while(deltaX++)
        {
            cxmt--;
            MAP_getTilemapRect(map, cxmt, ymt, 1, 16, TRUE, tilemapBuf + bufOffset);
            VDP_setTileMapDataColumnFast(plane, tilemapBuf + bufOffset, (cxmt * 2) + 0, ymt * 2, 16 * 2, DMA_QUEUE);
            // next column
            bufOffset += 16 * 2;
            VDP_setTileMapDataColumnFast(plane, tilemapBuf + bufOffset, (cxmt * 2) + 1, ymt * 2, 16 * 2, DMA_QUEUE);
            // next column
            bufOffset += 16 * 2;
        }
    }

    if (deltaY > 0)
    {
        // update at bottom
        cymt += 16;

        // need to update map row on bottom
        while(deltaY--)
        {
            MAP_getTilemapRect(map, xmt, cymt, 21, 1, FALSE, tilemapBuf + bufOffset);
            VDP_setTileMapDataRow(plane, tilemapBuf + bufOffset, (cymt * 2) + 0, (xmt * 2), 21 * 2, DMA_QUEUE);
            // next row
            bufOffset += 21 * 2;
            VDP_setTileMapDataRow(plane, tilemapBuf + bufOffset, (cymt * 2) + 1, (xmt * 2), 21 * 2, DMA_QUEUE);
            // next row
            bufOffset += 21 * 2;
            cymt++;
        }
    }
    else
    {
        // need to update map row on top
        while(deltaY++)
        {
            cymt--;
            MAP_getTilemapRect(map, xmt, cymt, 21, 1, FALSE, tilemapBuf + bufOffset);
            VDP_setTileMapDataRow(plane, tilemapBuf + bufOffset, (cymt * 2) + 0, (xmt * 2), 21 * 2, DMA_QUEUE);
            // next row
            bufOffset += 21 * 2;
            VDP_setTileMapDataRow(plane, tilemapBuf + bufOffset, (cymt * 2) + 1, (xmt * 2), 21 * 2, DMA_QUEUE);
            // next row
            bufOffset += 21 * 2;
        }
    }

    mapMetaTilePosX[plane] = xmt;
    mapMetaTilePosY[plane] = ymt;
}


void LEVEL_onVBlank(void)
{
    // reset tilemap buffer position after update
    bufOffset = 0;
}

