/**
 * \file vdp_tile.h
 * \brief VDP General Tile / Tilemap operations
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides methods to manipulate VDP tiles and tilemap :<br/>
 * - upload tiles to VDP memory<br/>
 * - upload tiles to VDP memory from bitmap data<br/>
 * - clear / fill / set tile map data<br/>
 */

#ifndef _VDP_TILE_H_
#define _VDP_TILE_H_

/**
 * \def TILE_ATTR
 *      Encode tile attributes for tilemap data.
 *
 * \param pal
 *      Palette index
 * \param prio
 *      Tile priority
 * \param flipV
 *      Vertical flip
 * \param flipH
 *      Horizontal flip
 */
#define TILE_ATTR(pal, prio, flipV, flipH)               (((flipH) << 11) + ((flipV) << 12) + ((pal) << 13) + ((prio) << 15))
/**
 * \def TILE_ATTR_FULL
 *      Encode tile attributes for tilemap data.
 *
 * \param pal
 *      Palette index
 * \param prio
 *      Tile priority
 * \param flipV
 *      Vertical flip
 * \param flipH
 *      Horizontal flip
 * \param index
 *      Tile index
 */
#define TILE_ATTR_FULL(pal, prio, flipV, flipH, index)   (((flipH) << 11) + ((flipV) << 12) + ((pal) << 13) + ((prio) << 15) + (index))


/**
 * \struct GenResTiles
 *      GenRes tile structure
 */
typedef struct
{
    u16 *pal;               // pointer to pal data
    u32 *tiles;             // pointer to tiles data
    u16 width;              // width in tiles
    u16 height;             // height in tiles
    u16 compressedSize;     // 0
} GenResTiles;


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
 *      Load font tile data in VRAM.
 *
 *  \param font
 *      Pointer to font tile data.
 *  \param use_dma
 *      Use DMA transfert (faster but can lock Z80 execution).
 *
 *  This fonction permits to replace system font by user font.<br/>
 *  Font tile data are loaded to TILE_FONTINDEX and the font should contains FONT_LEN characters.
 *  See also VDP_loadTileData().
 */
void VDP_loadFont(const u32 *font, u8 use_dma);
/**
 *  \brief
 *      Load 4bpp bitmap tile data in VRAM.
 *
 *  \param data
 *      Pointer to tile data.
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
 *      Pointer to tile data.
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
 *      Use DMA transfert (faster and so recommended).
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
 *      tile index.
 */
void VDP_setTileMapByIndex(u16 plan, u16 tile, u16 ind);
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
void VDP_setTileMap(u16 plan, u16 tile, u16 x, u16 y);
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
 *  \see VDP_fillTileMapRectIncByIndex()
 */
void VDP_fillTileMapRectByIndex(u16 plan, u16 tile, u16 ind, u16 num);
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
 *  \see VDP_fillTileMapRectByIndex() (faster method)
 *  \see VDP_fillTileMapRectInc()
 */
void VDP_fillTileMapRect(u16 plan, u16 tile, u16 x, u16 y, u16 w, u16 h);
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
 *      Use DMA transfert (faster and so recommended).
 */
void VDP_clearTileMapRectByIndex(u16 plan, u16 ind, u16 num, u8 use_dma);
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
 */
void VDP_clearTileMapRect(u16 plan, u16 x, u16 y, u16 w, u16 h);
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
 *  \see also VDP_fillTileMapRectByIndex()
 *  \see also VDP_fillTileMapRectInc()
 */
void VDP_fillTileMapRectIncByIndex(u16 plan, u16 basetile, u16 ind, u16 num);
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
 *  \see also VDP_fillTileMapRectIncByIndex()
 *  \see also VDP_fillTileMapRect()
 */
void VDP_fillTileMapRectInc(u16 plan, u16 basetile, u16 x, u16 y, u16 w, u16 h);
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
 *  \see VDP_setTileMapRectExByIndex().
 *  \see VDP_setTileMapRect().
 */
void VDP_setTileMapRectByIndex(u16 plan, const u16 *data, u16 ind, u16 num, u8 use_dma);
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
 *  Set the specified tilemap region with specified tile attributes values.<br/>
 *
 *  \see VDP_setTileMapRectEx().
 *  \see VDP_setTileMapRectByIndex().
 */
void VDP_setTileMapRect(u16 plan, const u16 *data, u16 x, u16 y, u16 w, u16 h);
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
 *  \param baseindex
 *      Base index for tile attributes.
 *  \param baseflags
 *      Base flags for tile attributes.
 *  \param ind
 *      Tile index where to start to set tilemap.
 *  \param num
 *      Number of tile to set.
 *
 *  Set the specified tilemap with specified tile attributes values.<br/>
 *  Values written in tilemap are calculated this way: <code>*tilemap = baseflags | (*data + baseindex);</code><br/>
 *
 *  \see VDP_setTileMapRectByIndex()
 *  \see VDP_setTileMapRectEx()
 */
void VDP_setTileMapRectExByIndex(u16 plan, const u16 *data, u16 baseindex, u16 baseflags, u16 ind, u16 num);
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
 *      tile attributes data pointer (see TILE_ATTR_FULL() and TILE_ATTR() macros).
 *  \param baseindex
 *      Base index for tile attributes.
 *  \param baseflags
 *      Base flags for tile attributes.
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
 *  Values written in tilemap are calculated this way: <code>*tilemap = baseflags | (*data + baseindex);</code><br/>
 *
 *  \see VDP_setTileMapRect()
 *  \see VDP_setTileMapRectExByIndex()
 */
void VDP_setTileMapRectEx(u16 plan, const u16 *data, u16 baseindex, u16 baseflags, u16 x, u16 y, u16 w, u16 h);


#endif // _VDP_TILE_H_
