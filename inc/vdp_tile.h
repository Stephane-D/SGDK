/**
 *  \file vdp_tile.h
 *  \brief VDP General Tile / Tilemap operations
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides methods to manipulate VDP tiles and tilemap :<br>
 * - upload tiles to VDP memory<br>
 * - upload tiles to VDP memory from bitmap data<br>
 * - clear / fill / set tile map data<br>
 */

#ifndef _VDP_TILE_H_
#define _VDP_TILE_H_


#include "vdp.h"
#include "dma.h"


/**
 *  \brief
 *      Bit shift for the tile priority attribute in tilemap data.
 */
#define TILE_ATTR_PRIORITY_SFT      15
/**
 *  \brief
 *      Bit shift for the tile palette attribute in tilemap data.
 */
#define TILE_ATTR_PALETTE_SFT       13
/**
 *  \brief
 *      Bit shift for the tile V flip attribute in tilemap data.
 */
#define TILE_ATTR_VFLIP_SFT         12
/**
 *  \brief
 *      Bit shift for the tile H flip attribute in tilemap data.
 */
#define TILE_ATTR_HFLIP_SFT         11
/**
 *  \brief
 *      Mask for the tile priority attribute in tilemap data.
 */
#define TILE_ATTR_PRIORITY_MASK     (1 << TILE_ATTR_PRIORITY_SFT)
/**
 *  \brief
 *      Mask for the tile palette attribute in tilemap data.
 */
#define TILE_ATTR_PALETTE_MASK      (3 << TILE_ATTR_PALETTE_SFT)
/**
 *  \brief
 *      Mask for the tile V flip attribute in tilemap data.
 */
#define TILE_ATTR_VFLIP_MASK        (1 << TILE_ATTR_VFLIP_SFT)
/**
 *  \brief
 *      Mask for the tile H flip attribute in tilemap data.
 */
#define TILE_ATTR_HFLIP_MASK        (1 << TILE_ATTR_HFLIP_SFT)
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
#define TILE_ATTR(pal, prio, flipV, flipH)               (((flipH) << TILE_ATTR_HFLIP_SFT) + ((flipV) << TILE_ATTR_VFLIP_SFT) + ((pal) << TILE_ATTR_PALETTE_SFT) + ((prio) << TILE_ATTR_PRIORITY_SFT))
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
#define TILE_ATTR_FULL(pal, prio, flipV, flipH, index)   (((flipH) << TILE_ATTR_HFLIP_SFT) + ((flipV) << TILE_ATTR_VFLIP_SFT) + ((pal) << TILE_ATTR_PALETTE_SFT) + ((prio) << TILE_ATTR_PRIORITY_SFT) + (index))

/**
 *  \brief
 *      Tile set structure which contains tiles definition.<br>
 *      Use the unpackTileSet() method to unpack if compression is enabled.
 *
 *  \param compression
 *      compression type, accepted values:<br>
 *      <b>COMPRESSION_NONE</b><br>
 *      <b>COMPRESSION_APLIB</b><br>
 *      <b>COMPRESSION_LZ4W</b><br>
 *  \param numTile
 *      number of tile in the <i>tiles</i> buffer.
 *  \param tiles
 *      Tiles data (packed or not depending compression field).
 */
typedef struct
{
    u16 compression;
    u16 numTile;
    u32 *tiles;
} TileSet;

/**
 *  \brief
 *      TileMap structure which contains tilemap background definition.<br>
 *      Use the unpackTileMap() method to unpack if compression is enabled.
 *  \param compression
 *      compression type, accepted values:<br>
 *      <b>COMPRESSION_NONE</b><br>
 *      <b>COMPRESSION_APLIB</b><br>
 *      <b>COMPRESSION_LZ4W</b><br>
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
} TileMap;

/**
 *  \brief
 *      Return the VRAM tilemap address for the specified plane position
 *
 *  \param plane
 *      Plane we want to get the VRAM tilemap address for a given position.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param x
 *      X position (in tile).
 *  \param y
 *      Y position (in tile).
 */
u16 VDP_getPlaneAddress(VDPPlane plane, u16 x, u16 y);

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
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *
 *  Transfert rate:<br>
 *  ~90 bytes per scanline in software (during blanking)<br>
 *  ~190 bytes per scanline in hardware (during blanking)
 */
void VDP_loadTileData(const u32 *data, u16 index, u16 num, TransferMethod tm);
/**
 *  \brief
 *      Load tile data (pattern) in VRAM.
 *
 *  \param tileset
 *      Pointer to TileSet structure.<br>
 *      The TileSet is unpacked "on-the-fly" if needed (require some memory).<br>
 *      Using DMA_QUEUE for packed resource is unsafe as the resource will be released and eventually
 *      can be overwritten before DMA operation so use DMA_QUEUE_COPY in that case or unpack the resource first.
 *  \param index
 *      Tile index where start tile data load (use TILE_USERINDEX as base user index).
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *  \return
 *      FALSE if there is not enough memory to unpack the specified TileSet (only if compression was enabled).
 *
 *  Transfert rate:<br>
 *  ~90 bytes per scanline in software (during blanking)<br>
 *  ~190 bytes per scanline in hardware (during blanking)
 */
u16 VDP_loadTileSet(const TileSet *tileset, u16 index, TransferMethod tm);
/**
 *  \brief
 *      Load font tile data in VRAM.<br>
 *      Note that you should prefer the VDP_loadBMPFont(..) method to this one (easier to use).
 *
 *  \param font
 *      Pointer to font tile data.
 *  \param length
 *      Number of characters of the font (max = FONT_LEN).
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *
 *  This fonction permits to replace system font by user font.<br>
 *  The font tile data are loaded to TILE_FONTINDEX and can contains FONT_LEN characters at max.<br>
 *  Each character should fit in one tile (8x8 pixels bloc).<br>
 *  See also VDP_loadFont(..) and VDP_loadTileData(..)
 */
void VDP_loadFontData(const u32 *font, u16 length, TransferMethod tm);
/**
 *  \brief
 *      Load font from the specified TileSet structure.
 *
 *  \param font
 *      TileSet containing the font.<br>
 *      The TileSet is unpacked "on-the-fly" if needed (require some memory).<br>
 *      Using DMA_QUEUE for packed resource is unsafe as the resource will be released and eventually
 *      can be overwritten before DMA operation so use DMA_QUEUE_COPY in that case or unpack the resource first.
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *  \return
 *      FALSE if there is not enough memory to unpack the specified font (only if compression was enabled).
 *
 *  This fonction permits to replace system font by user font.<br>
 *  The font tile data are loaded to TILE_FONTINDEX and can contains FONT_LEN characters at max.<br>
 *  Each character should fit in one tile (8x8 pixels bloc).<br>
 *  See also VDP_loadFontData(..)
 */
u16 VDP_loadFont(const TileSet *font, TransferMethod tm);

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
 *  This function does "on the fly" 4bpp bitmap conversion to tile data and transfert them to VRAM.<br>
 *  It's very helpful when you use bitmap images but the conversion eats sometime so you should use it only for static screen only.<br>
 *  For "in-game" condition you should use VDP_loadTileData() method with converted tile data.<br>
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
 *  This function does "on the fly" 4bpp bitmap conversion to tile data and transfert them to VRAM.<br>
 *  It's very helpful when you use bitmap images but the conversion eats sometime so you should use it only for static screen only.<br>
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
 *  \param wait
 *      Wait the operation to complete when set to TRUE otherwise it returns immediately
 *      but then you will require to wait for DMA completion (#DMA_waitCompletion()) before accessing the VDP.
 *
 *  This function is generally used to clear tile data in VRAM.
 */
void VDP_fillTileData(u8 value, u16 index, u16 num, bool wait);

/**
 *  \brief
 *      Clear tilemap.
 *
 *  \param planeAddr
 *      Plane address where we want to clear tilemap.<br>
 *      Accepted values are:<br>
 *      - VDP_BG_A<br>
 *      - VDP_BG_B<br>
 *      - VDP_WINDOW<br>
 *  \param ind
 *      Tile index where to start clear.
 *  \param num
 *      Number of tile to clear.
 *  \param wait
 *      Wait the operation to complete when set to TRUE otherwise it returns immediately
 *      but then you will require to wait for DMA completion (#DMA_waitCompletion()) before accessing the VDP.
 *
 *  \see VDP_clearTileMapRect()
 *  \see VDP_fillTileMap()
 */
void VDP_clearTileMap(u16 planeAddr, u16 ind, u16 num, bool wait);
/**
 *  \brief
 *      Fill tilemap.
 *
 *  \param planeAddr
 *      Plane address where we want to fill tilemap.<br>
 *      Accepted values are:<br>
 *      - VDP_BG_A<br>
 *      - VDP_BG_B<br>
 *      - VDP_WINDOW<br>
 *  \param tile
 *      Tile attributes (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param ind
 *      tile index where to start fill.
 *  \param num
 *      Number of tile to fill.
 *
 *  \see VDP_fillTileMapRect()
 */
void VDP_fillTileMap(u16 planeAddr, u16 tile, u16 ind, u16 num);
/**
 *  \brief
 *      Set tilemap data at specified index.
 *
 *  \param planeAddr
 *      Plane address where we want to set tilemap data.<br>
 *      Accepted values are:<br>
 *      - VDP_BG_A<br>
 *      - VDP_BG_B<br>
 *      - VDP_WINDOW<br>
 *  \param data
 *      Tile attributes data (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param ind
 *      Tile index where to start to set tilemap data.
 *  \param num
 *      Number of tile to set.
 *  \param vramStep
 *      VRAM address increment after each write (default value = 2)
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *
 *  Set the specified tilemap with specified tile attributes values.<br>
 *  You can use this method when you are using the 'mapbase' parameter on your resource definition to set the base attributes<br>
 *  (palette, priority and base tile index) so you don't need to provide them here.<br>
 *  This method is faster than using #VDP_setTileMapDataEx(..) which allow to override base tile attributes though the 'basetile' parameter.
 *
 *  \see VDP_setTileMapDataEx().
 *  \see VDP_setTileMapDataRect().
 */
void VDP_setTileMapData(u16 planeAddr, const u16 *data, u16 ind, u16 num, u16 vramStep, TransferMethod tm);
/**
 *  \brief
 *      Set tilemap data at specified index (extended version).
 *
 *  \param planeAddr
 *      Plane where we want to set tilemap data.<br>
 *      Accepted values are:<br>
 *      - VDP_BG_A<br>
 *      - VDP_BG_B<br>
 *      - VDP_WINDOW<br>
 *  \param data
 *      Tile attributes data (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param basetile
 *      Base tile index and flag for tile attributes (see TILE_ATTR_FULL() macro).
 *  \param ind
 *      Tile index where to start to set tilemap data.
 *  \param num
 *      Number of tile to set.
 *  \param vramStep
 *      VRAM address increment after each write (default value = 2).
 *
 *  Set the specified tilemap with specified tile attributes values.<br>
 *  Unlike #VDP_setTileMapData(..) this method let you to override the base tile attributes (priority, palette and base index)<br>
 *  at the expense of more computation time. If you want faster tilemap processing (using #VDP_setTileMapData(..)), you can use<br>
 *  the 'mapbase' parameter when declaring your IMAGE resource to set base tile attributes but then you have fixed/static tile allocation.
 *
 *  \see VDP_setTileMapData()
 *  \see VDP_setTileMapDataRectEx()
 */
void VDP_setTileMapDataEx(u16 planeAddr, const u16 *data, u16 basetile, u16 ind, u16 num, u16 vramStep);

/**
 *  \brief
 *      Set tilemap data (single position).
 *
 *  \param plane
 *      Plane where we want to set tilemap data.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param tile
 *      tile attributes (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param x
 *      X position (in tile).
 *  \param y
 *      y position (in tile).
 *
 *  Set the specified tilemap position (tilemap wrapping supported) with given tile attributes.
 */
void VDP_setTileMapXY(VDPPlane plane, u16 tile, u16 x, u16 y);
/**
 *  \brief
 *      Clear specified region of tilemap.
 *
 *  \param plane
 *      Plane where we want to clear tilemap region.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
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
void VDP_clearTileMapRect(VDPPlane plane, u16 x, u16 y, u16 w, u16 h);
/**
 *  \brief
 *      Fill speficied region of tilemap.
 *
 *  \param plane
 *      Plane where we want to fill tilemap region.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param tile
 *      tile attributes (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *  \param w
 *      Region Width (in tile).
 *  \param h
 *      Region Heigh (in tile).
 *
 *  Fill the specified tilemap region with specified tile attributes value.
 *
 *  \see VDP_fillTileMap() (faster method)
 *  \see VDP_fillTileMapRectInc()
 */
void VDP_fillTileMapRect(VDPPlane plane, u16 tile, u16 x, u16 y, u16 w, u16 h);
/**
 *  \brief
 *      Fill tilemap with index auto increment at specified region.
 *
 *  \param plane
 *      Plane where we want to fill tilemap region.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param basetile
 *      Base tile attributes (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *  \param w
 *      Region Width (in tile).
 *  \param h
 *      Region Heigh (in tile).
 *
 *  Set the specified tilemap region with specified tile attributes value.<br>
 *  The function auto increments tile index in tile attributes like this:<br>
 *  tilemap line 0 : basetile, basetile+1, basetile+2, basetile+3, ...<br>
 *  tilemap line 1 : basetile+w, basetile+w+1, basetile+w+2, ...<br>
 *  ...<br>
 *  So this function is convenient to display generated image or simulate a frame buffer.<br>
 *
 *  \see also VDP_fillTileMapRect()
 */
void VDP_fillTileMapRectInc(VDPPlane plane, u16 basetile, u16 x, u16 y, u16 w, u16 h);

/**
 *  \brief
 *      Set tilemap data for specified region.
 *
 *  \param plane
 *      Plane where we want to set tilemap data.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param data
 *      Source tilemap data containing tile attributes (see TILE_ATTR_FULL() macro).
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *  \param w
 *      Region Width (in tile).
 *  \param h
 *      Region Heigh (in tile).
 *  \param wm
 *      Source tilemap width (in tile).
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *
 *  Set the specified tilemap region (tilemap wrapping supported) with specified tile attributes values.<br>
 *  You can use this method when you are using the 'mapbase' parameter on your resource definition to set the base attributes<br>
 *  (palette, priority and base tile index) so you don't need to provide them here.<br>
 *  This method is faster than using #VDP_setTileMapDataRectEx(..) which allow to override base tile attributes though the 'basetile' parameter.
 *
 *  \see VDP_setTileMapDataRectEx().
 *  \see VDP_setTileMapData().
 */
void VDP_setTileMapDataRect(VDPPlane plane, const u16 *data, u16 x, u16 y, u16 w, u16 h, u16 wm, TransferMethod tm);
/**
 *  \brief
 *      Set tilemap data for specified region (extended version).
 *
 *  \param plane
 *      Plane where we want to set tilemap data.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param data
 *      Source tilemap data containing tile attributes (see TILE_ATTR_FULL() macro).
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
 *  \param wm
 *      Source tilemap width (in tile).
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY<br>
 *      But i highly discourage of using DMA here as VDP_setTileMapDataRectEx(..) requires to prepare data in a temporary buffer first<br>
 *      to use DMA, resulting in a slower process than using CPU. However DMA_QUEUE is very useful as it wil prepare the data<br>
 *      and transfer the data as fast as possible during VBlank.
 *
 *  Set the specified tilemap region (tilemap wrapping supported) with specified tile attributes values.<br>
 *  Unlike #VDP_setTileMapDataRect(..) this method let you to override the base tile attributes (priority, palette and base index)<br>
 *  at the expense of more computation time. If you want faster tilemap processing (using #VDP_setTileMapDataRect(..)), you can use<br>
 *  the 'mapbase' parameter when declaring your IMAGE resource to set base tile attributes but then you have fixed/static tile allocation.
 *
 *  \see VDP_setTileMapDataRect()
 *  \see VDP_setTileMapDataEx()
 */
void VDP_setTileMapDataRectEx(VDPPlane plane, const u16 *data, u16 basetile, u16 x, u16 y, u16 w, u16 h, u16 wm, TransferMethod tm);

/**
 *  \brief
 *      Set a row of tilemap data.
 *
 *  \param plane
 *      Plane where we want to set tilemap data.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param data
 *      Source tilemap data containing tile attributes (see TILE_ATTR_FULL() macro).
 *  \param row
 *      Plane row we want to set data
 *  \param x
 *      Row X start position (in tile)
 *  \param w
 *      Row width to update (in tile)
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *
 *  Set a row of tilemap data (tilemap wrapping supported) with given tile attributes values.
 *  You can use this method when you are using the 'mapbase' parameter on your resource definition to set the base attributes<br>
 *  (palette, priority and base tile index) so you don't need to provide them here.<br>
 *  This method is faster than using #VDP_setTileMapDataRowEx(..) which allow to override base tile attributes though the 'basetile' parameter.
 *
 *  \see VDP_setTileMapDataRowPartEx()
 *  \see VDP_setTileMapDataRow()
 */
void VDP_setTileMapDataRow(VDPPlane plane, const u16 *data, u16 row, u16 x, u16 w, TransferMethod tm);
/**
 *  \brief
 *      Set a row of tilemap data - extended version.
 *
 *  \param plane
 *      Plane where we want to set tilemap data.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param data
 *      Source tilemap data containing tile attributes (see TILE_ATTR_FULL() macro).
 *  \param basetile
 *      Base index and flag for tile attributes (see TILE_ATTR_FULL() macro).
 *  \param row
 *      Plane row we want to set data
 *  \param x
 *      Row X start position (in tile)
 *  \param w
 *      Row width to update (in tile)
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY<br>
 *      But i highly discourage of using DMA here as VDP_setTileMapDataRowEx(..) requires to prepare data in a temporary buffer first<br>
 *      to use DMA, resulting in a slower process than using CPU. However DMA_QUEUE is very useful as it wil prepare the data<br>
 *      and transfer the data as fast as possible during VBlank.
 *
 *  Set a row of tilemap data (tilemap wrapping supported) with given tile attributes values.<br>
 *  Unlike #VDP_setTileMapDataRow(..) this method let you to override the base tile attributes (priority, palette and base index)<br>
 *  at the expense of more computation time. If you want faster tilemap processing (using #VDP_setTileMapDataRow(..)), you can use<br>
 *  the 'mapbase' parameter when declaring your IMAGE resource to set base tile attributes but then you have fixed/static tile allocation.
 *
 *  \see VDP_setTileMapDataRowPart()
 *  \see VDP_setTileMapDataRow()
 */
void VDP_setTileMapDataRowEx(VDPPlane plane, const u16 *data, u16 basetile, u16 row, u16 x, u16 w, TransferMethod tm);
/**
 *  \brief
 *      Set a complete column of pre-arranged tilemap data (not supported when plane width is set to 128).
 *
 *  \param plane
 *      Plane where we want to set tilemap data.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param data
 *      Prepared tile attributes data (see TILE_ATTR_FULL() macro).<br>
 *      Column data are already arranged to be transferred as a single contiguous data block.
 *  \param column
 *      Plane column we want to set data
 *  \param y
 *      Column Y start position (in tile)
 *  \param h
 *      Column height to update (in tile)
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *
 *  Set a complete column of tilemap data with given tile attributes values.<br>
 *  This method is faster than #VDP_setTileMapDataColumn(..) or #VDP_setTileMapDataColumnEx(..) as it assumes<br>
 *  that data buffer is properly prepared and arranged to be directly copied as it.<br>
 *  <b>WARNING:</b> this function doesn't work when plane width is set to 128 (see #VDP_setPlaneSize(..) method).
 *
 *  \see VDP_setTileMapDataRowFast()
 *  \see VDP_setTileMapDataColumn()
 *  \see VDP_setTileMapData()
 */
void VDP_setTileMapDataColumnFast(VDPPlane plane, u16* data, u16 column, u16 y, u16 h, TransferMethod tm);
/**
 *  \brief
 *      Set a column of tilemap data (not supported when plane width is set to 128).
 *
 *  \param plane
 *      Plane where we want to set tilemap data.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param data
 *      Source tilemap data containing tile attributes (see TILE_ATTR_FULL() macro).
 *  \param column
 *      Plane column we want to set data
 *  \param y
 *      Column Y start position (in tile)
 *  \param h
 *      Column height to update (in tile)
 *  \param wm
 *      Source tilemap width (in tile).
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *
 *  Set a column of tilemap data (tilemap wrapping supported) with given tile attributes values.<br>
 *  You can use this method when you are using the 'mapbase' parameter on your resource definition to set the base tile attributes<br>
 *  (palette, priority and base tile index) so you don't need to provide them here.<br>
 *  This method is faster than using #VDP_setTileMapDataColumnEx(..) which allow to override base tile attributes though the 'basetile' parameter.<br>
 *  <b>WARNING:</b> this function doesn't work when plane width is set to 128 (see #VDP_setPlaneSize(..) method)
 *
 *  \see VDP_setTileMapDataColumnPartEx()
 *  \see VDP_setTileMapDataColumn()
 */
void VDP_setTileMapDataColumn(VDPPlane plane, const u16 *data, u16 column, u16 y, u16 h, u16 wm, TransferMethod tm);
/**
 *  \brief
 *      Set a column of tilemap data - extended version (not supported when plane width is set to 128).
 *
 *  \param plane
 *      Plane where we want to set tilemap data.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param data
 *      Source tilemap data containing tile attributes (see TILE_ATTR_FULL() macro).
 *  \param basetile
 *      Base index and flag for tile attributes (see TILE_ATTR_FULL() macro).
 *  \param column
 *      Plane column we want to set data
 *  \param y
 *      Column Y start position (in tile)
 *  \param h
 *      Column height to update (in tile)
 *  \param wm
 *      Source tilemap width (in tile).
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY<br>
 *      But i highly discourage of using DMA here as VDP_setTileMapDataColumnEx(..) requires to prepare data in a temporary buffer first<br>
 *      to use DMA, resulting in a slower process than using CPU. However DMA_QUEUE is very useful as it wil prepare the data<br>
 *      and transfer the data as fast as possible during VBlank.
 *
 *  Set a column of tilemap data (tilemap wrapping supported) with given tile attributes values.<br>
 *  Unlike #VDP_setTileMapDataColumn(..) this method let you to override the base tile attributes (priority, palette and base index)<br>
 *  at the expense of more computation time. If you want faster tilemap processing (using #VDP_setTileMapDataColumn(..)), you can use<br>
 *  the 'mapbase' parameter when declaring your IMAGE resource to set base tile attributes but then you have fixed/static tile allocation.
 *  <b>WARNING:</b> this function doesn't work when plane width is set to 128 (see #VDP_setPlaneSize(..) method)
 *
 *  \see VDP_setTileMapDataColumnPart()
 *  \see VDP_setTileMapDataColumnEx()
 *  \see VDP_setTileMapData()
 */
void VDP_setTileMapDataColumnEx(VDPPlane plane, const u16 *data, u16 basetile, u16 column, u16 y, u16 h, u16 wm, TransferMethod tm);

/**
 *  \brief
 *      Load tilemap region.
 *
 *  \param plane
 *      Plane where we want to load tilemap.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param tilemap
 *      Source tilemap to load.<br>
 *      The TileMap is unpacked "on-the-fly" if needed (require some memory).<br>
 *      Using DMA_QUEUE for packed resource is unsafe as the resource will be released and eventually
 *      can be overwritten before DMA operation so use DMA_QUEUE_COPY in that case or unpack the resource first.
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *  \param w
 *      Region Width (in tile).
 *  \param h
 *      Region Heigh (in tile).
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *
 *  Load the specified tilemap region at equivalent plane position (tilemap wrapping supported).<br>
 *  You can use this method when you are using the 'mapbase' parameter on your resource definition to set the base attributes<br>
 *  (palette, priority and base tile index) so you don't need to provide them here.<br>
 *  This method is faster than using #VDP_setTileMapEx(..) which allow to override base tile attributes though the 'basetile' parameter.
 *
 *  \see VDP_setTileMapData()
 *  \see VDP_setTileMapDataEx()
 */
bool VDP_setTileMap(VDPPlane plane, const TileMap *tilemap, u16 x, u16 y, u16 w, u16 h, TransferMethod tm);
/**
 *  \brief
 *      Load tilemap region at specified plane position.
 *
 *  \param plane
 *      Plane where we want to load tilemap.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param tilemap
 *      Source tilemap to load.<br>
 *      The TileMap is unpacked "on-the-fly" if needed (require some memory).<br>
 *      Using DMA_QUEUE for packed resource is unsafe as the resource will be released and eventually
 *      can be overwritten before DMA operation so use DMA_QUEUE_COPY in that case or unpack the resource first.
 *  \param basetile
 *      Base index and flag for tile attributes (see TILE_ATTR_FULL() macro).
 *  \param xp
 *      Plane X destination position (in tile).
 *  \param yp
 *      Plane Y destination position (in tile).
 *  \param x
 *      Region X start position (in tile).
 *  \param y
 *      Region Y start position (in tile).
 *  \param w
 *      Region Width (in tile).
 *  \param h
 *      Region Heigh (in tile).
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY<br>
 *      But i highly discourage of using DMA here as VDP_setTileMapEx(..) requires to prepare data in a temporary buffer first<br>
 *      to use DMA, resulting in a slower process than using CPU. However DMA_QUEUE is very useful as it wil prepare the data<br>
 *      and transfer the data as fast as possible during VBlank.
 *
 *  Load the specified tilemap region at specified plane position (tilemap wrapping supported).<br>
 *  Unlike #VDP_setTileMap(..) this method let you to override the base tile attributes (priority, palette and base index)<br>
 *  at the expense of more computation time. If you want faster tilemap processing (using #VDP_setTileMap(..)), you can use<br>
 *  the 'mapbase' parameter when declaring your IMAGE resource to set base tile attributes but then you have fixed/static tile allocation.
 *
 *  \see VDP_setTileMapDataRect()
 *  \see VDP_setTileMapDataRectEx()
 */
bool VDP_setTileMapEx(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 xp, u16 yp, u16 x, u16 y, u16 w, u16 h, TransferMethod tm);
/**
 *  \brief
 *      Load tilemap row.
 *
 *  \param plane
 *      Plane where we want to load tilemap.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param tilemap
 *      Source tilemap to set row from.<br>
 *      The TileMap is unpacked "on-the-fly" if needed (require some memory).<br>
 *      Using DMA_QUEUE for packed resource is unsafe as the resource will be released and eventually
 *      can be overwritten before DMA operation so use DMA_QUEUE_COPY in that case or unpack the resource first.
 *  \param row
 *      Plane row we want to set data
 *  \param x
 *      Source tilemap X start position (in tile).
 *  \param w
 *      Row width to update (in tile)
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *
 *  Load a complete row of data from tilemap at equivalent plane position (wrapped around if needed).<br>
 *  You can use this method when you are using the 'mapbase' parameter on your resource definition to set the base attributes<br>
 *  (palette, priority and base tile index) so you don't need to provide them here.<br>
 *  This method is faster than using #VDP_setTileMapRowEx(..) which allow to override base tile attributes though the 'basetile' parameter.
 *
 *  \see VDP_setTileMapRowEx()
 *  \see VDP_setMapColumn()
 *  \see VDP_setTileMapDataRow()
 */
bool VDP_setTileMapRow(VDPPlane plane, const TileMap *tilemap, u16 row, u16 x, u16 w, TransferMethod tm);
/**
 *  \brief
 *      Load tilemap row (extended version).
 *
 *  \param plane
 *      Plane where we want to load tilemap.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param tilemap
 *      Source tilemap to set row from.<br>
 *      The TileMap is unpacked "on-the-fly" if needed (require some memory).<br>
 *      Using DMA_QUEUE for packed resource is unsafe as the resource will be released and eventually
 *      can be overwritten before DMA operation so use DMA_QUEUE_COPY in that case or unpack the resource first.
 *  \param basetile
 *      Base index and flag for tile attributes (see TILE_ATTR_FULL() macro).
 *  \param row
 *      Plane row we want to set data
 *  \param x
 *      Source tilemap X start position (in tile)
 *  \param y
 *      Source tilemap Y / row position (in tile), can be different that plane row if desired.
 *  \param w
 *      Row width to update (in tile)
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY<br>
 *      But i highly discourage of using DMA here as VDP_setTileMapRowEx(..) requires to prepare data in a temporary buffer first<br>
 *      to use DMA, resulting in a slower process than using CPU. However DMA_QUEUE is very useful as it wil prepare the data<br>
 *      and transfer the data as fast as possible during VBlank.
 *
 *  Load a complete row of data from tilemap at equivalent plane position (wrapped around if needed).<br>
 *  Unlike #VDP_setTileMapRow(..) this method let you to override the base tile attributes (priority, palette and base index)<br>
 *  at the expense of more computation time. If you want faster tilemap processing (using #VDP_setTileMapRow(..)), you can use<br>
 *  the 'mapbase' parameter when declaring your IMAGE resource to set base tile attributes but then you have fixed/static tile allocation.
 *
 *  \see VDP_setTileMapRow()
 *  \see VDP_setMapColumnEx()
 *  \see VDP_setTileMapDataRowEx()
 */
bool VDP_setTileMapRowEx(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 row, u16 x, u16 y, u16 w, TransferMethod tm);
/**
 *  \brief
 *      Load tilemap column (not supported when plane width is set to 128).
 *
 *  \param plane
 *      Plane where we want to load tilemap.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param tilemap
 *      Source tilemap to set column from.<br>
 *      The TileMap is unpacked "on-the-fly" if needed (require some memory).<br>
 *      Using DMA_QUEUE for packed resource is unsafe as the resource will be released and eventually
 *      can be overwritten before DMA operation so use DMA_QUEUE_COPY in that case or unpack the resource first.
 *  \param column
 *      Plane column we want to set data
 *  \param y
 *      Source tilemap Y start position (in tile).
 *  \param h
 *      Column height to update (in tile)
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *
 *  Load a complete column of data from tilemap at equivalent plane position (wrapped around if needed).<br>
 *  You can use this method when you are using the 'mapbase' parameter on your resource definition to set the base attributes<br>
 *  (palette, priority and base tile index) so you don't need to provide them here.<br>
 *  This method is faster than using #VDP_setTileMapColumnEx(..) which allow to override base tile attributes though the 'basetile' parameter.<br>
 *  <b>WARNING:</b> this function doesn't work when plane width is set to 128 (see #VDP_setPlaneSize(..) method)
 *
 *  \see VDP_setTileMapColumnEx()
 *  \see VDP_setMapRow()
 *  \see VDP_setTileMapDataColumn()
 */
bool VDP_setTileMapColumn(VDPPlane plane, const TileMap *tilemap, u16 column, u16 y, u16 h, TransferMethod tm);
/**
 *  \brief
 *      Load tilemap column - extended version (not supported when plane width is set to 128).
 *
 *  \param plane
 *      Plane where we want to load tilemap.<br>
 *      Accepted values are:<br>
 *      - BG_A<br>
 *      - BG_B<br>
 *      - WINDOW<br>
 *  \param tilemap
 *      Source tilemap to set column from.<br>
 *      The TileMap is unpacked "on-the-fly" if needed (require some memory).<br>
 *      Using DMA_QUEUE for packed resource is unsafe as the resource will be released and eventually
 *      can be overwritten before DMA operation so use DMA_QUEUE_COPY in that case or unpack the resource first.
 *  \param basetile
 *      Base index and flag for tile attributes (see TILE_ATTR_FULL() macro).
 *  \param column
 *      Plane column we want to set data
 *  \param x
 *      Source tilemap X / column position (in tile), can be different than plane column if desired.
 *  \param y
 *      Source tilemap Y start position (in tile).
 *  \param h
 *      Column height to update (in tile)
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY<br>
 *      But i highly discourage of using DMA here as VDP_setTileMapColumnEx(..) requires to prepare data in a temporary buffer first<br>
 *      to use DMA, resulting in a slower process than using CPU. However DMA_QUEUE is very useful as it wil prepare the data<br>
 *      and transfer the data as fast as possible during VBlank.
 *
 *  Load a complete column of data from tilemap at equivalent plane position (wrapped around if needed).<br>
 *  Unlike #VDP_setTileMapColumn(..) this method let you to override the base tile attributes (priority, palette and base index)<br>
 *  at the expense of more computation time. If you want faster tilemap processing (using #VDP_setTileMapColumn(..)), you can use<br>
 *  the 'mapbase' parameter when declaring your IMAGE resource to set base tile attributes but then you have fixed/static tile allocation.<br>
 *  <b>WARNING:</b> this function doesn't work when plane width is set to 128 (see #VDP_setPlaneSize(..) method)
 *
 *  \see VDP_setTileMapColumn()
 *  \see VDP_setMapRowEx()
 *  \see VDP_setTileMapDataColumnEx()
 */
bool VDP_setTileMapColumnEx(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 column, u16 x, u16 y, u16 h, TransferMethod tm);

/**
 *  \deprecated
 *      Use #VDP_setTileMap() instead.
 */
bool VDP_setMap(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 x, u16 y);
/**
 *  \deprecated
 *      Use #VDP_setTileMapEx() instead.
 */
bool VDP_setMapEx(VDPPlane plane, const TileMap *tilemap, u16 basetile, u16 x, u16 y, u16 xm, u16 ym, u16 wm, u16 hm);


#endif // _VDP_TILE_H_
