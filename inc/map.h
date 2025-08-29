/**
 *  \file map.h
 *  \brief MAP (large background map) management unit
 *  \author Stephane Dallongeville
 *  \date 11/2020
 *
 * This unit provides methods to manipulate / scroll large background MAP:<br>
 * - Create / release MAP object<br>
 * - MAP decoding functions<br>
 * - Large MAP scrolling engine<br>
 * <br>
 * Using MAP resource you can encode large image as a #MapDefinition structure which will be
 * used to handle large background scrolling. The #MapDefinition structure is optimized to encode
 * large map efficiently (space wise), here's the encoding format:<br>
 * - background map is encoded as a grid of 128x128 pixels blocks<br>
 * - duplicated 128x128 blocks are optimized (keep only 1 reference)<br>
 * - each 128x128 blocks is encoded internally as a 8x8 grid of metatile<br>
 * - a metatile is a 16x16 pixels block (2x2 tiles block)<br>
 * - each duplicated / flipped metatile are optimized.<br>
 * <br>
 * Knowing that you can draw your level background to optimize its space usage by trying to optimize<br>
 * the number of unique 128x128 pixels block.
 *
 */

#ifndef _MAP_H_
#define _MAP_H_


#include "vdp.h"
#include "vdp_tile.h"
#include "pal.h"

// forward
typedef struct Map Map;

/**
 *  \brief
 *      Map data update type
 */
typedef enum
{
    ROW_UPDATE,        /** tilemap row update **/
    COLUMN_UPDATE      /** tilemap column update **/
} MapUpdateType;

/**
 *  \brief
 *      Map data patch callback.<br>
 *      It's used to modify/patch map data (for destructible blocks for instance) before sending it to VRAM.
 *
 *  \param map
 *      source Map structure containing map information.
 *  \param buf
 *      buffer containing the tilemap data to patch
 *  \param x
 *      tile X start update position
 *  \param y
 *      tile Y start update position
 *  \param updateType
 *      map data update type:<br>
 *      - ROW_UPDATE (tilemap row update)<br>
 *      - COLUMN_UPDATE (tilemap column update)<br>
 *  \param size
 *      size of the buffer (tilemap width or height depending we are on a row or column update type)
 */
typedef void MapDataPatchCallback(Map *map, u16 *buf, u16 x, u16 y, MapUpdateType updateType, u16 size);


/**
 *  \brief
 *      MapDefinition structure which contains data for large level background.<br>
 *      It's optimized to encode large map using 128x128 block chunk (block chunk are organized in metatile).
 *
 *  \param w
 *      map width in block (128x128 pixels block).
 *  \param h
 *      map height in block (128x128 pixels block).
 *  \param hp
 *      map height in block (128x128 pixels block) removing duplicated rows
 *  \param compression
 *      b0-b3=compression type for metaTiles<br>
 *      b4-b7=compression for blocks data<br>
 *      b8-b11=compression for blockIndexes data<br>
 *      Accepted values:<br>
 *        <b>COMPRESSION_NONE</b><br>
 *        <b>COMPRESSION_APLIB</b><br>
 *        <b>COMPRESSION_LZ4W</b><br>
 *  \param numMetaTile
 *      number of MetaTile
 *  \param numBlock
 *      number of Block (128x128 pixels chunk)
 *  \param metaTiles
 *      metatiles definition, each metatile is encoded as 2x2 tiles block:<br>
 *      - b15: priority<br>
 *      - b14-b13: palette<br>
 *      - b12: vflip<br>
 *      - b11: hflip<br>
 *      - b10-b0: tile index (from tileset)
 *  \param blocks
 *      blocks definition, each block is encoded as 8x8 metatiles:<br>
 *      if numMetaTile <= 256 --> 8 bit index for metaTile<br>
 *      else --> 16 bit index for metaTile
 *  \param blockIndexes
 *      block index array (referencing blocks) for the w * hp sized map<br>
 *      if numBlock <= 256 --> 8 bit index for block
 *      else --> 16 bit index for block
 *  \param blockRowOffsets
 *      block row offsets used internally for fast retrieval of block data (index = blockIndexes[blockRowOffsets[y] + x])
 */
typedef struct
{
    u16 w;
    u16 h;
    u16 hp;
    u16 compression;
    u16 numMetaTile;
    u16 numBlock;
    u16 *metaTiles;
    void* blocks;
    void* blockIndexes;
    u16* blockRowOffsets;
} MapDefinition;

/**
 *  \brief
 *      Map structure containing information for large background/plane update based on #MapDefinition
 *
 *  \param w
 *      map width in block (128x128 pixels block)
 *  \param h
 *      map height in block (128x128 pixels block)
 *  \param metaTiles
 *      internal - unpacked data of MapDefinition.metaTiles
 *  \param blocks
 *      internal - unpacked data of MapDefinition.blocks
 *  \param blockIndexes
 *      internal - unpacked data of MapDefinition.blockIndexes
 *  \param blockRowOffsets
 *      internal - direct access of MapDefinition.blockRowOffsets
 *  \param plane
 *      VDP plane where MAP is draw
 *  \param baseTile
 *      Base tile attributes used to provide base tile index offset and base palette index (see TILE_ATTR_FULL() macro)
 *  \param posX
 *      current view position X set using #MAP_scrollTo(..) method
 *  \param posY
 *      current view position Y set using #MAP_scrollTo(..) method
 *  \param wMask
 *      internal
 *  \param hMask
 *      internal
 *  \param planeWidthMask
 *      internal
 *  \param planeHeightMask
 *      internal
 *  \param lastXT
 *      internal
 *  \param lastYT
 *      internal
 *  \param hScrollTable
 *      internal
 *  \param vScrollTable
 *      internal
 *  \param prepareMapDataColumnCB
 *      internal
 *  \param prepareMapDataRowCB
 *      internal
 *  \param patchMapDataColumnCB
 *      internal
 *  \param patchMapDataRowCB
 *      internal
 *  \param getMetaTileCB
 *      internal
 *  \param getMetaTilemapRectCB
 *      internal
 */
typedef struct Map
{
    u16 w;
    u16 h;
    u16* metaTiles;
    void* blocks;
    void* blockIndexes;
    u16* blockRowOffsets;
    VDPPlane plane;
    u16 baseTile;
    u32 posX;
    u32 posY;
    u16 wMask;
    u16 hMask;
    u16 planeWidth;
    u16 planeHeight;
    u16 planeWidthMaskAdj;
    u16 planeHeightMaskAdj;
    u16 planeWidthSftAdj;
    u16 firstUpdate;
    u16 lastXT;
    u16 lastYT;
    u16 hScrollTable[240];
    u16 vScrollTable[20];
    void (*prepareMapDataColumnCB)(Map *map, u16 *bufCol1, u16 *bufCol2, u16 xm, u16 ym, u16 height);
    void (*prepareMapDataRowCB)(Map *map, u16 *bufRow1, u16 *bufRow2, u16 xm, u16 ym, u16 width);
    MapDataPatchCallback* mapDataPatchCB;
    u16  (*getMetaTileCB)(Map *map, u16 x, u16 y);
    void (*getMetaTilemapRectCB)(Map *map, u16 x, u16 y, u16 w, u16 h, u16* dest);
} Map;


/**
 *  \brief
 *      Create and return a Map structure required to use all MAP_xxx functions
 *      from a given MapDefinition.<br>
 *      When you're done with the map you shall use MAP_release(map) to release it.
 *
 *  \param mapDef
 *      MapDefinition structure containing background/plane data.
 *  \param plane
 *      Plane where we want to draw the Map (for #MAP_scrollTo(..) method).<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      If you want to use the map for collision or special behavior (using the MAP_getTile(..) method) then you can just let this parameter to 0.
 *  \param baseTile
 *      Used to provide base tile index and base palette index (see TILE_ATTR_FULL() macro).<br>
 *      Note that you can also use it to force HIGH priority but in that case your map should only contains LOW priority tiles
 *      otherwise the HIGH priority tiles will be set in LOW priority instead (mutually exclusive).
 *  \return initialized Map structure or <i>NULL</i> if there is not enough memory to allocate data for given MapDefinition.
 */
Map* MAP_create(const MapDefinition* mapDef, VDPPlane plane, u16 baseTile);

/**
 *  \brief
 *      Release the map and its resources (same as MEM_free(map))
 *
 *  \param map
 *      the Map structure to release
 */
void MAP_release(Map* map);

/**
 *  \brief
 *      Scroll map to specified position.<br>
 *      The fonction takes care of updating the VDP tilemap which will be transfered by DMA queue then
 *      VDP background scrolling is automatically set on VBlank (into the SYS_doVBlankProcess() tasks).<br>
 *      WARNING: first MAP_scrollTo(..) call will do a full plane update, for a 64x32 sized plane this represents 4KB of data.<br>
 *      That means you can't initialize 2 MAPs in the same frame (limited to 7.2 KB of data per frame) so take care of calling
 *      SYS_doVBlankProcess() in between.
 *
 *  \param map
 *      source Map structure containing map information.
 *  \param x
 *      view position X we want to scroll on
 *  \param y
 *      view position Y we want to scroll on
 *
 *  \see #MAP_create(..)
 */
void MAP_scrollTo(Map* map, u32 x, u32 y);
/**
 *  \brief
 *      Exactly as #MAP_scrollTo(..) except we can force complete map drawing
 *
 *  \param map
 *      source Map structure containing map information.
 *  \param x
 *      view position X we want to scroll on
 *  \param y
 *      view position Y we want to scroll on
 *  \param forceRedraw
 *      Set to <i>TRUE</i> to force a complete map redraw (take more time)
 *
 *  \see #MAP_scrollTo(..)
 */
void MAP_scrollToEx(Map* map, u32 x, u32 y, bool forceRedraw);

/**
 *  \brief
 *      Returns metatile index / number at given position (a metatile is a block of 2x2 tiles = 16x16 pixels)
 *
 *  \param map
 *      source Map structure containing map information.
 *  \param x
 *      metatile X position (16x16 pixels block)
 *  \param y
 *      metatile Y position (16x16 pixels block)
 *
 *  \return
 *      metatile index
 *
 *  \see #MAP_create(..)
 *  \see #MAP_getTile(..)
 *  \see #MAP_getMetaTilemapRect(..)
 */
u16 MAP_getMetaTile(Map* map, u16 x, u16 y);
/**
 *  \brief
 *      Returns given tile attribute (note than map->baseTile isn't added to the result)
 *
 *  \param map
 *      source Map structure containing map information.
 *  \param x
 *      tile X position
 *  \param y
 *      tile Y position
 *
 *  \return
 *      tile attribute:<br>
 *      - b15: priority<br>
 *      - b14-b13: palette<br>
 *      - b12: vflip<br>
 *      - b11: hflip<br>
 *      - b10-b0: tile index
 *
 *  \see #MAP_create(..)
 *  \see #MAP_getMetaTile(..)
 *  \see #MAP_getTilemapRect(..)
 */
u16 MAP_getTile(Map* map, u16 x, u16 y);
/**
 *  \brief
 *      Returns metatiles index for the specified region (a metatile is a block of 2x2 tiles = 16x16 pixels)
 *
 *  \param map
 *      source Map structure containing map information.
 *  \param x
 *      Region X start position (in metatile - 16x16 pixels block).
 *  \param y
 *      Region Y start position (in metatile - 16x16 pixels block).
 *  \param w
 *      Region Width (in metatile).
 *  \param h
 *      Region Heigh (in metatile).
 *  \param dest
 *      destination pointer receiving metatiles attribute data
 *
 *  \return
 *      metatiles index
 *
 *  \see #MAP_create(..)
 *  \see #MAP_getTilemapRect(..)
 */
void MAP_getMetaTilemapRect(Map* map, u16 x, u16 y, u16 w, u16 h, u16* dest);
/**
 *  \brief
 *      Returns tiles attribute data for the specified region (map->baseTile is used as base tiles attribute, see #MAP_create(..))
 *
 *  \param map
 *      source Map structure containing map information.
 *  \param x
 *      Region X start position <b>(in metatile)</b>
 *  \param y
 *      Region Y start position <b>(in metatile)</b>
 *  \param w
 *      Region Width <b>(in metatile)</b>
 *  \param h
 *      Region Heigh <b>(in metatile)</b>
 *  \param column
 *      if set to TRUE then tilemap data is stored by column order [Y,X] instead of row order [X,Y].
 *  \param dest
 *      destination pointer receiving tiles attribute data
 *
 *  \return
 *      tiles attribute:<br>
 *      - b15: priority<br>
 *      - b14-b13: palette<br>
 *      - b12: vflip<br>
 *      - b11: hflip<br>
 *      - b10-b0: tile index
 *
 *  \see #MAP_create(..)
 *  \see #MAP_getTile(..)
 *  \see #MAP_getMetaTilemapRect(..)
 */
void MAP_getTilemapRect(Map* map, u16 x, u16 y, u16 w, u16 h, bool column, u16* dest);

/**
 *  \brief
 *      Set the callback function to patch tilemap data.<br>
 *      Note that you need to set
 *<br>
 *      The method will be called when a new tilemap row / column is ready to be send to the VDP.<br>
 *      You can use this callback to modify the tilemap data before sending it to VRAM.<br>
 *      It can be useful, for instance, to implement destructibles blocks.
 *
 *  \param map
 *      source Map structure we want to set the patch data callback for.
 *  \param CB
 *      Callback to use to patch the new tilemap data (set to NULL by default = no callback).<br>
 *      See declaration of #MapDataPatchCallback to get information about the callback parameters.
 */
void MAP_setDataPatchCallback(Map* map, MapDataPatchCallback *CB);

/**
 *  \brief
 *      Override the system (VDP) plane size for this map (should be called after MAP_create(..))<br>
 *      Useful if you have VDP plane size set to 64x64 but you want to use 64x32 for a plane so you can use spare VRAM for something else.
 *
 *  \param map
 *      source Map structure we want to override VDP tilemap size for.
 *  \param w
 *      tilemap width (32, 64 or 128)
 *  \param h
 *      tilemap height (32, 64 or 128)
 */
void MAP_overridePlaneSize(Map* map, u16 w, u16 h);


#endif // _MAP_H_
