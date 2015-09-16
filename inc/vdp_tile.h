/**
 *  \file vdp_tile.h
 *  \brief VDP General Tile / Tilemap operations
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides methods to manipulate VDP tiles and tilemap :<br/>
 * - upload tiles to VDP memory<br/>
 * - upload tiles to VDP memory from bitmap data<br/>
 * - clear / fill / set tile map data<br/>
 */

#ifndef _VDP_TILE_H_
#define _VDP_TILE_H_

/**
 *  \brief
 *      Mask for the tile priority attribute in tilemap data.
 */
#define TILE_ATTR_PRIORITY_MASK     0x8000

/**
 *  \brief
 *      Mask for the tile palette attribute in tilemap data.
 */
#define TILE_ATTR_PALETTE_MASK      0x6000

/**
 *  \brief
 *      Mask for the tile V flip attribute in tilemap data.
 */
#define TILE_ATTR_VFLIP_MASK        0x1000

/**
 *  \brief
 *      Mask for the tile H flip attribute in tilemap data.
 */
#define TILE_ATTR_HFLIP_MASK        0x0800

/**
 *  \brief
 *      Mask for the tile attributes (priority, palette and flip) in tilemap data.
 */
#define TILE_ATTR_MASK              (TILE_ATTR_PRIORITY_MASK | TILE_ATTR_PALETTE_MASK | TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK)

/**
 *  \brief
 *      Encode tile attributes for tilemap data.
 *
 *  \param pal
 *      Palette index
 *  \param prio
 *      Tile priority
 *  \param flipV
 *      Vertical flip
 *  \param flipH
 *      Horizontal flip
 */
#define TILE_ATTR(pal, prio, flipV, flipH)               (((flipH) << 11) + ((flipV) << 12) + ((pal) << 13) + ((prio) << 15))
/**
 *  \brief
 *      Encode tile attributes for tilemap data.
 *
 *  \param pal
 *      Palette index
 *  \param prio
 *      Tile priority
 *  \param flipV
 *      Vertical flip
 *  \param flipH
 *      Horizontal flip
 *  \param index
 *      Tile index
 */
#define TILE_ATTR_FULL(pal, prio, flipV, flipH, index)   (((flipH) << 11) + ((flipV) << 12) + ((pal) << 13) + ((prio) << 15) + (index))


/**
 *  \brief
 *      Tile set structure which contains tiles definition.<br/>
 *      Use the unpackTileSet() method to unpack if compression is enabled.
 *
 *  \param compression
 *      compression type, accepted values:<br/>
 *      <b>COMPRESSION_NONE</b><br/>
 *      <b>COMPRESSION_APLIB</b><br/>
 *      <b>COMPRESSION_RLE</b><br/>
 *  \param numTile
 *      number of tile in the <i>tiles</i> buffer.
 *  \param tiles
 *      Tiles data.
 */
typedef struct
{
    u16 compression;
    u16 numTile;
    u32 *tiles;
} TileSet;

/**
 *  \brief
 *      Reference a sub set of an existing tileSet.<br/>
 *      Use the getTileSet() method to extract the sub tileset from the tileset.
 *
 *  \param tileset
 *      reference tileset
 *  \param index
 *      the index of the first tile relative to the reference TileSet
 *  \param numTile
 *      number of tile in this sub set of tiles.
 *  \param offset
 *      offset from reference TileSet (this parameter is used for compressed TileSet).
 *  \param size
 *      number of element (this parameter is used for compressed TileSet).
 */
typedef struct
{
    TileSet *tileSet;
    u16 index;
    u16 numTile;
    u32 offset;
    u16 size;
} SubTileSet;

/**
 *  \brief
 *      Map structure which contains tilemap background definition.<br/>
 *      Use the unpackMap() method to unpack if compression is enabled.
 *  \param compression
 *      compression type, accepted values:<br/>
 *      <b>COMPRESSION_NONE</b><br/>
 *      <b>COMPRESSION_APLIB</b><br/>
 *      <b>COMPRESSION_RLE</b><br/>
 *      <b>COMPRESSION_MAP_RLE</b><br/>
 *  \param w
 *      tilemap width in tile.
 *  \param h
 *      tilemap height in tile.
 *  \param tilemap
 *      Tilemap data.
 */
typedef struct
{
    u16 compression;
    u16 w;
    u16 h;
    u16 *tilemap;
} Map;


/**
 *  \brief
 *      Load tile data (pattern) in VRAM.
 *
 *  \param data
 *      Pointer to tile data.
 *  \param index
 *      Tile index where start tile data load (use TILE_USERINDEX as base user index).
 *  \param num
 *      Number of tile to load.
 *  \param use_dma
 *      Use DMA transfert (faster but can lock Z80 execution).
 *
 *  Transfert rate:<br/>
 *  ~90 bytes per scanline in software (during blanking)<br/>
 *  ~190 bytes per scanline in hardware (during blanking)
 */
void VDP_loadTileData(const u32 *data, u16 index, u16 num, u8 use_dma);
/**
 *  \brief
 *      Load tile data (pattern) in VRAM.
 *
 *  \param tileset
 *      Pointer to TileSet structure.<br/>
 *      The TileSet is unpacked "on-the-fly" if needed (require some memory)
 *  \param index
 *      Tile index where start tile data load (use TILE_USERINDEX as base user index).
 *  \param use_dma
 *      Use DMA transfert (faster but can lock Z80 execution).
 *  \return
 *      FALSE if there is not enough memory to unpack the specified TileSet (only if compression was enabled).
 *
 *  Transfert rate:<br/>
 *  ~90 bytes per scanline in software (during blanking)<br/>
 *  ~190 bytes per scanline in hardware (during blanking)
 */
u16 VDP_loadTileSet(const TileSet *tileset, u16 index, u8 use_dma);
/**
 *  \brief
 *      Load font tile data in VRAM.<br/>
 *      Note that you should prefer the VDP_loadBMPFont(..) method to this one (easier to use).
 *
 *  \param font
 *      Pointer to font tile data.
 *  \param length
 *      Number of characters of the font (max = FONT_LEN).
 *  \param use_dma
 *      Use DMA transfert (faster but can lock Z80 execution).
 *
 *  This fonction permits to replace system font by user font.<br/>
 *  The font tile data are loaded to TILE_FONTINDEX and can contains FONT_LEN characters at max.<br/>
 *  Each character should fit in one tile (8x8 pixels bloc).<br/>
 *  See also VDP_loadFont(..) and VDP_loadTileData(..)
 */
void VDP_loadFontData(const u32 *font, u16 length, u8 use_dma);
/**
 *  \brief
 *      Load font from the specified TileSet structure.
 *
 *  \param font
 *      TileSet containing the font.<br/>
 *      The TileSet is unpacked "on-the-fly" if needed (require some memory)
 *  \param use_dma
 *      Use DMA transfert (faster but can lock Z80 execution).
 *  \return
 *      FALSE if there is not enough memory to unpack the specified font (only if compression was enabled).
 *
 *  This fonction permits to replace system font by user font.<br/>
 *  The font tile data are loaded to TILE_FONTINDEX and can contains FONT_LEN characters at max.<br/>
 *  Each character should fit in one tile (8x8 pixels bloc).<br/>
 *  See also VDP_loadFontData(..)
 */
u16 VDP_loadFont(const TileSet *font, u8 use_dma);

/**
 *  \brief
 *      Load 4bpp bitmap tile data in VRAM.
 *
 *  \param data
 *      Pointer to 4bpp bitmap tile data.
 *  \param index
 *      Tile index where start tile data load (use TILE_USERINDEX as base user index).
 *  \param w
 *      Width of bitmap region to load (in tile).
 *  \param h
 *      Heigh of bitmap region to load (in tile).
 *  \param bmp_w
 *      Width of bitmap (in tile), it can differ from 'w' parameter.
 *
 *  This function does "on the fly" 4bpp bitmap conversion to tile data and transfert them to VRAM.<br/>
 *  It's very helpful when you use bitmap images but the conversion eats sometime so you should use it only for static screen only.<br/>
 *  For "in-game" condition you should use VDP_loadTileData() method with converted tile data.<br/>
 *  See also VDP_loadBMPTileDataEx().
 */
void VDP_loadBMPTileData(const u32 *data, u16 index, u16 w, u16 h, u16 bmp_w);
/**
 *  \brief
 *      Load 4bpp bitmap tile data in VRAM.
 *
 *  \param data
 *      Pointer to 4bpp bitmap tile data.
 *  \param index
 *      Tile index where start tile data load (use TILE_USERINDEX as base user index).
 *  \param x
 *      X start position of bitmap region to load (in tile).
 *  \param y
 *      Y start position of bitmap region to load (in tile).
 *  \param w
 *      Width of bitmap region to load (in tile).
 *  \param h
 *      Heigh of bitmap region to load (in tile).
 *  \param bmp_w
 *      Width of bitmap (in tile), it can differ from 'w' parameter.
 *
 *  This function does "on the fly" 4bpp bitmap conversion to tile data and transfert them to VRAM.<br/>
 *  It's very helpful when you use bitmap images but the conversion eats sometime so you should use it only for static screen only.<br/>
 *  For "in-game" condition you should use VDP_loadTileData() method with converted tile data.
 *  See also VDP_loadBMPTileData()
 */
void VDP_loadBMPTileDataEx(const u32 *data, u16 index, u16 x, u16 y, u16 w, u16 h, u16 bmp_w);

/**
 *  \brief
 *      Fill tile data in VRAM.
 *
 *  \param value
 *      Value (byte) used to fill VRAM tile data.
 *  \param index
 *      Tile index where start tile data fill (use TILE_USERINDEX as base user index).
 *  \param num
 *      Number of tile to fill.
 *  \param use_dma
 *      Use DMA transfert (faster but can lock Z80 execution).
 *
 *  This function is generally used to clear tile data in VRAM.
 */
void VDP_fillTileData(u8 value, u16 index, u16 num, u8 use_dma);

/**
 *  \brief
 *      Set tilemap data at single position.
 *
 *  \param plan
 *      Plan where we want to set tilemap data.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param tile
 *      tile attributes data (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param ind
 *      position in tilemap.
 */
void VDP_setTileMap(u16 plan, u16 tile, u16 ind);
/**
 *  \brief
 *      Set tilemap data at single position.
 *
 *  \param plan
 *      Plan where we want to set tilemap data.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param tile
 *      tile attributes data (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param x
 *      X position (in tile).
 *  \param y
 *      y position (in tile).
 */
void VDP_setTileMapXY(u16 plan, u16 tile, u16 x, u16 y);
/**
 *  \deprecated
 *      Use VDP_fillTileMap() instead
 */
void VDP_fillTileMapRectByIndex(u16 plan, u16 tile, u16 ind, u16 num);
/**
 *  \brief
 *      Fill tilemap data.
 *
 *  \param plan
 *      Plan where we want to fill tilemap data.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param tile
 *      tile attributes data (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param ind
 *      tile index where to start fill.
 *  \param num
 *      Number of tile to fill.
 *
 *  \see VDP_fillTileMapRect()
 *  \see VDP_fillTileMapRectInc()
 */
void VDP_fillTileMap(u16 plan, u16 tile, u16 ind, u16 num);
/**
 *  \brief
 *      Fill tilemap data at specified region.
 *
 *  \param plan
 *      Plan where we want to fill tilemap region.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param tile
 *      tile attributes data (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *  \param w
 *      Region Width (in tile).
 *  \param h
 *      Region Heigh (in tile).
 *
 *  Fill the specified tilemap region with specified tile attributes values.<br/>
 *
 *  \see VDP_fillTileMap() (faster method)
 *  \see VDP_fillTileMapRectInc()
 */
void VDP_fillTileMapRect(u16 plan, u16 tile, u16 x, u16 y, u16 w, u16 h);
/**
 *  \deprecated
 *      Use VDP_clearTileMap() instead
 */
void VDP_clearTileMapRectByIndex(u16 plan, u16 ind, u16 num, u8 use_dma);
/**
 *  \brief
 *      Clear tilemap data.
 *
 *  \param plan
 *      Plan where we want to clear tilemap region.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param ind
 *      Tile index where to start fill.
 *  \param num
 *      Number of tile to fill.
 *  \param use_dma
 *      Use DMA transfert (faster but can lock Z80 execution).
 *
 *  \see VDP_fillTileMap() (faster method)
 *  \see VDP_fillTileMapRectInc()
 *
 *  \see VDP_clearTileMapRect()
 */
void VDP_clearTileMap(u16 plan, u16 ind, u16 num, u8 use_dma);
/**
 *  \brief
 *      Clear tilemap data at specified region.
 *
 *  \param plan
 *      Plan where we want to clear tilemap region.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *  \param w
 *      Region Width (in tile).
 *  \param h
 *      Region Heigh (in tile).
 *
 *  \see VDP_clearTileMap() (faster method)
 */
void VDP_clearTileMapRect(u16 plan, u16 x, u16 y, u16 w, u16 h);
/**
 *  \deprecated
 *      Use VDP_fillTileMapInc() instead.
 */
void VDP_fillTileMapRectIncByIndex(u16 plan, u16 basetile, u16 ind, u16 num);
/**
 *  \brief
 *      Fill tilemap data with index auto increment.
 *
 *  \param plan
 *      Plan where we want to fill tilemap region.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param basetile
 *      Base tile attributes data (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param ind
 *      tile index where to start fill.
 *  \param num
 *      Number of tile to fill.
 *
 *  Set the specified tilemap with specified tile attributes values.<br/>
 *  The function auto increments tile index in tile attribute :<br/>
 *  tilemap at index : basetile, basetile+1, basetile+2, basetile+3, ...<br/>
 *  ...<br/>
 *  So this function is pratical to display image.<br/>
 *
 *  \see also VDP_fillTileMap()
 *  \see also VDP_fillTileMapRectInc()
 */
void VDP_fillTileMapInc(u16 plan, u16 basetile, u16 ind, u16 num);
/**
 *  \brief
 *      Fill tilemap data with index auto increment at specified region.
 *
 *  \param plan
 *      Plan where we want to fill tilemap region.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param basetile
 *      Base tile attributes data (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *  \param w
 *      Region Width (in tile).
 *  \param h
 *      Region Heigh (in tile).
 *
 *  Set the specified tilemap region with specified tile attributes values.<br/>
 *  The function auto increments tile index in tile attribute :<br/>
 *  tilemap line 0 : basetile, basetile+1, basetile+2, basetile+3, ...<br/>
 *  tilemap line 1 : basetile+w, basetile+w+1, basetile+w+2, ...<br/>
 *  ...<br/>
 *  So this function is pratical to display image.<br/>
 *
 *  \see also VDP_fillTileMapInc() (faster method)
 *  \see also VDP_fillTileMapRect()
 */
void VDP_fillTileMapRectInc(u16 plan, u16 basetile, u16 x, u16 y, u16 w, u16 h);
/**
 *  \deprecated
 *      Use VDP_setTileMapData() instead.
 */
void VDP_setTileMapRectByIndex(u16 plan, const u16 *data, u16 ind, u16 num, u8 use_dma);
/**
 *  \brief
 *      Load tilemap data at specified index.
 *
 *  \param plan
 *      Plan where we want to load tilemap data.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param data
 *      Tile attributes data pointer (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param ind
 *      Tile index where to start to set tilemap.
 *  \param num
 *      Number of tile to set.
 *  \param use_dma
 *      Use DMA transfert (faster but can lock Z80 execution).
 *
 *  Set the specified tilemap with specified tile attributes values.<br/>
 *  Transfert rate:<br/>
 *  ~90 bytes per scanline in software (during blanking)<br/>
 *  ~190 bytes per scanline in hardware (during blanking)
 *
 *  \see VDP_setTileMapDataEx().
 *  \see VDP_setTileMapDataRect().
 */
void VDP_setTileMapData(u16 plan, const u16 *data, u16 ind, u16 num, u8 use_dma);
/**
 *  \brief
 *      Load tilemap data at specified region.
 *
 *  \param plan
 *      Plan where we want to load tilemap data.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param data
 *      tile attributes data pointer (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *  \param w
 *      Region Width (in tile).
 *  \param h
 *      Region Heigh (in tile).
 *
 *  Set the specified tilemap region with specified tile attributes values.
 *
 *  \see VDP_setTileMapDataRectEx().
 *  \see VDP_setTileMapData().
 */
void VDP_setTileMapDataRect(u16 plan, const u16 *data, u16 x, u16 y, u16 w, u16 h);
/**
 *  \brief
 *      Use VDP_setTileMapDataEx() instead.
 */
void VDP_setTileMapRectExByIndex(u16 plan, const u16 *data, u16 baseindex, u16 baseflags, u16 ind, u16 num);
/**
 *  \brief
 *      Load tilemap data at specified index (extended version).
 *
 *  \param plan
 *      Plan where we want to load tilemap data.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param data
 *      tile attributes data pointer (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param basetile
 *      Base tile index and flags for tile attributes (see TILE_ATTR_FULL() macro).
 *  \param ind
 *      Tile index where to start to set tilemap.
 *  \param num
 *      Number of tile to set.
 *
 *  Set the specified tilemap with specified tile attributes values.
 *
 *  \see VDP_setTileMapData()
 *  \see VDP_setTileMapDataRectEx()
 */
void VDP_setTileMapDataEx(u16 plan, const u16 *data, u16 basetile, u16 ind, u16 num);
/**
 *  \deprecated
 *      Use VDP_setTileMapDataRectEx() instead.
 */
void VDP_setTileMapRectEx(u16 plan, const u16 *data, u16 baseindex, u16 baseflags, u16 x, u16 y, u16 w, u16 h);
/**
 *  \brief
 *      Load tilemap data at specified region (extended version).
 *
 *  \param plan
 *      Plan where we want to load tilemap data.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param data
 *      tile attributes data pointer (see TILE_ATTR_FULL() macro).
 *  \param basetile
 *      Base index and flags for tile attributes (see TILE_ATTR_FULL() macro).
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *  \param w
 *      Region Width (in tile).
 *  \param h
 *      Region Heigh (in tile).
 *  \param wm
 *      Source tilemap data width (in tile).
 *
 *  Set the specified tilemap region with specified tile attributes values.
 *
 *  \see VDP_setTileMapDataRect()
 *  \see VDP_setTileMapDataEx()
 */
void VDP_setTileMapDataRectEx(u16 plan, const u16 *data, u16 basetile, u16 x, u16 y, u16 w, u16 h, u16 wm);

/**
 *  \brief
 *      Load Map at specified position.
 *
 *  \param plan
 *      Plan where we want to load Map.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param map
 *      Map to load.
 *  \param basetile
 *      Base index and flags for tile attributes (see TILE_ATTR_FULL() macro).
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *
 *  Load the specified Map at specified plan position.
 *
 *  \see VDP_setTileMapData()
 *  \see VDP_setTileMapDataEx()
 */
u16 VDP_setMap(u16 plan, const Map *map, u16 basetile, u16 x, u16 y);
/**
 *  \brief
 *      Load Map region at specified position.
 *
 *  \param plan
 *      Plan where we want to load Map.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 *  \param map
 *      Map to load.
 *  \param basetile
 *      Base index and flags for tile attributes (see TILE_ATTR_FULL() macro).
 *  \param x
 *      Plan X destination position (in tile).
 *  \param y
 *      Plan Y destination position (in tile).
 *  \param xm
 *      Map region X start position (in tile).
 *  \param ym
 *      Map region Y start position (in tile).
 *  \param wm
 *      Map region Width (in tile).
 *  \param hm
 *      Map region Heigh (in tile).
 *
 *  Load the specified Map region at specified plan position.
 *
 *  \see VDP_setTileMapDataRect()
 *  \see VDP_setTileMapDataRectEx()
 */
u16 VDP_setMapEx(u16 plan, const Map *map, u16 basetile, u16 x, u16 y, u16 xm, u16 ym, u16 wm, u16 hm);


#endif // _VDP_TILE_H_
