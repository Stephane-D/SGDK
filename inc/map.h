/**
 *  \file map.h
 *  \brief MAP (large background map) management unit
 *  \author Stephane Dallongeville
 *  \date 11/2020
 *
 * This unit provides methods to manipulate large background MAP :<br>
 * - Create / load / release MAP object<br>
 * - MAP decoding functions<br>
 * - Large MAP scrolling engine
 */

#ifndef _MAP_H_
#define _MAP_H_


#include "vdp.h"
#include "vdp_tile.h"
#include "pal.h"


/**
 *  \brief
 *      MapDefinition structure which contains data for large level background.<br>
 *      It's optimized to encode large map using 128x128 block chunk (block chunk are organized in metatile).
 *
 *  \param w
 *      map width in block (128x128 pixels block).
 *  \param h
 *      map height in block (128x128 pixels block).
 *  \param numMetaTile
 *      number of MetaTile
 *  \param numBlock
 *      number of Block (128x128 pixels chunk)
 *  \param palette
 *      Palette data.
 *  \param tileset
 *      TileSet data structure (contains tiles definition for the image).
 *  \param metaTiles
 *      metatiles definition, each metatile is encoded as 2x2 tiles:<br>
 *      - b15: priority<br>
 *      - b14-b13: palette<br>
 *      - b12: vflip<br>
 *      - b11: hflip<br>
 *      - b10-b0: tile index (from tileset)
 *  \param blocks
 *      blocks definition, each block is encoded as 8x8 metatiles:<br>
 *      - b15: priority override<br>
 *      - b14-b13: free, can be used to encode collision info ?<br>
 *      - b12: combined vflip<br>
 *      - b11: combined hflip<br>
 *      - b10-b0: metatile index
 *  \param blockIndexes
 *      block index array (referencing blocks) for the w * h sized map
 *  \param blockRowOffsets
 *      block row offsets used internally for fast retrieval of block data (index = blockIndexes[blockRowOffsets[y] + x])
 */
typedef struct
{
    u16 w;
    u16 h;
    u16 numMetaTile;
    u16 numBlock;
    Palette *palette;
    TileSet *tileset;
    u16 *metaTiles;
    u16 *blocks;
    u16 *blockIndexes;
    u16 *blockRowOffsets;
} MapDefinition;


/**
 *  \brief
 *      Map structure containing information for background/plane update using MapDefinition
 *
 *  \param mapDefinition
 *      MapDefinition structure containing background/plane data.
 *  \param plane
 *      VDP plane where MAP is draw
 *  \param baseTile
 *      Base tile attributes used to provide base tile index offset and base palette index (see TILE_ATTR_FULL() macro)
 *  \param posX
 *      current view position X set using #MAP_scrollTo(..) method
 *  \param posY
 *      current view position Y set using #MAP_scrollTo(..) method
 *  \param planeWidthMask
 *      internal
 *  \param planeHeightMask
 *      internal
 *  \param lastXT
 *      internal
 *  \param lastYT
 *      internal
 */
typedef struct
{
    const MapDefinition *mapDefinition;
    VDPPlane plane;
    u16 baseTile;
    u32 posX;
    u32 posY;
    u16 planeWidthMask;
    u16 planeHeightMask;
    u16 lastXT;
    u16 lastYT;
    bool updateScroll;
} Map;


/**
 *  \brief
 *      Initialize Map structure and refresh whole wisible BG plane with map data at given position
 *
 *  \param mapDef
 *      MapDefinition structure containing background/plane data.
 *  \param plane
 *      Plane where we want to draw the Map.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *  \param basetile
 *      Used to provide base tile index and base palette index (see TILE_ATTR_FULL() macro)
 *  \param x
 *      default view position X
 *  \param y
 *      default view position Y
 *  \param map
 *      Map structure to initialize
 */
void MAP_init(const MapDefinition* mapDef, VDPPlane plane, u16 baseTile, u32 x, u32 y, Map *map);
/**
 *  \brief
 *      Scroll map to specified position
 *
 *  \param map
 *      Map structure containing map information.
 *  \param x
 *      view position X we want to scroll on
 *  \param y
 *      view position Y we want to scroll on
 */
void MAP_scrollTo(Map* map, u32 x, u32 y);

/**
 *  \brief
 *      Returns given metatile attribute
 *
 *  \param mapDef
 *      MapDefinition structure containing map information.
 *  \param x
 *      metatile X position (a metatile is basically a block of 2x2 tiles = 16x16 pixels)
 *  \param x
 *      metatile Y position (a metatile is basically a block of 2x2 tiles = 16x16 pixels)
 *
 *  \return
 *      metatile attribute:<br>
 *      - b15: priority override<br>
 *      - b14-b13: free, can be used to encode collision info ?<br>
 *      - b12: combined vflip<br>
 *      - b11: combined hflip<br>
 *      - b10-b0: metatile index<br>
 */
u16 MAP_getMetaTile(const MapDefinition* mapDef, u16 x, u16 y);
/**
 *  \brief
 *      Returns given tile attribute
 *
 *  \param mapDef
 *      MapDefinition structure containing map information.
 *  \param x
 *      tile X position
 *  \param x
 *      tile Y position
 *
 *  \return
 *      tile attribute:<br>
 *      - b15: priority<br>
 *      - b14-b13: palette<br>
 *      - b12: vflip<br>
 *      - b11: hflip<br>
 *      - b10-b0: tile index
 */
u16 MAP_getTile(const MapDefinition* mapDef, u16 x, u16 y);
/**
 *  \brief
 *      Returns metatiles attribute for the specified region
 *
 *  \param mapDef
 *      MapDefinition structure containing map information.
 *  \param basetile
 *      Base index and flag for tile attributes (see TILE_ATTR_FULL() macro).
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *  \param w
 *      Region Width (in tile).
 *  \param h
 *      Region Heigh (in tile).
 *  \param dest
 *      destination pointer receiving metatiles attribute data
 *
 *  \return
 *      metatiles attribute:<br>
 *      - b15: priority override<br>
 *      - b14-b13: free, can be used to encode collision info ?<br>
 *      - b12: combined vflip<br>
 *      - b11: combined hflip<br>
 *      - b10-b0: metatile index<br>
 */
void MAP_getMetaTilemapRect(const MapDefinition* mapDef, u16 x, u16 y, u16 w, u16 h, u16* dest);
/**
 *  \brief
 *      Returns tiles attribute data for the specified region
 *
 *  \param mapDef
 *      MapDefinition structure containing map information.
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *  \param w
 *      Region Width (in tile).
 *  \param h
 *      Region Heigh (in tile).
 *  \param column
 *      if set to TRUE then tilemap data is stored by column order [Y,X] instead of row order [X,Y].
 *  \param basetile
 *      Base index and flag for tile attributes (see TILE_ATTR_FULL() macro) to use to fill tiles attribute data.
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
 */
void MAP_getTilemapRect(const MapDefinition* mapDef, u16 x, u16 y, u16 w, u16 h, u16 baseTile, bool column, u16* dest);


#endif // _MAP_H_
