/**
 *  \file vdp_bg.h
 *  \brief VDP background plan support
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides plan A & plan B facilities :
 * - set scrolling
 * - clear plan
 * - draw text in plan
 */

#ifndef _VDP_BG_H_
#define _VDP_BG_H_

#include "bmp.h"
#include "vdp.h"
#include "vdp_tile.h"


/**
 *  \brief
 *      Image structure which contains all data to define an image in a background plan.<br>
 *      Use the unpackImage() method to unpack if compression is enabled in TileSet or Map structure.
 *
 *  \param palette
 *      Palette data.
 *  \param tileset
 *      TileSet data structure (contains tiles definition for the image).
 *  \param map
 *      Map data structure (contains tilemap definition for the image).
 */
typedef struct
{
    Palette *palette;
    TileSet *tileset;
    Map *map;
} Image;


/**
 *  Contains current VRAM tile position where we will upload next tile data.
 *
 *  \see VDP_drawBitmap()
 *  \see VDP_drawImage()
 */
extern u16 curTileInd;

/**
 *  \brief
 *      Set plan horizontal scroll (plain scroll mode).<br>
 *      3 horizontal scroll modes are supported:<br>
 *      - Plain (whole plan)<br>
 *      - Tile (8 pixels bloc)<br>
 *      - Line (per pixel scroll)<br>
 *
 *  \param plan
 *      Plan we want to set the horizontal scroll.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *  \param value
 *      H scroll offset.<br>
 *      Negative value will move the plan to the left while positive
 *      value will move it to the right.
 *
 *  \see VDP_setScrollingMode() function to change scroll mode.
 */
void VDP_setHorizontalScroll(VDPPlan plan, s16 value);
/**
 *  \brief
 *      Set plan horizontal scroll (tile scroll mode).<br>
 *      3 horizontal scroll modes are supported:<br>
 *      - Plain (whole plan)<br>
 *      - Tile (8 pixels bloc)<br>
 *      - Line (per pixel scroll)<br>
 *
 *  \param plan
 *      Plan we want to set the horizontal scroll.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *  \param tile
 *      First tile we want to set the horizontal scroll.
 *  \param values
 *      H scroll offsets.<br>
 *      Negative values will move the plan to the left while positive
 *      values will move it to the right.
 *  \param len
 *      Number of tile to set.
 *  \param use_dma
 *      Use DMA flag (faster for large transfer).
 *
 *  \see VDP_setScrollingMode() function to change scroll mode.
 */
void VDP_setHorizontalScrollTile(VDPPlan plan, u16 tile, s16* values, u16 len, u16 use_dma);
/**
 *  \brief
 *      Set plan horizontal scroll (line scroll mode).<br>
 *      3 horizontal scroll modes are supported:<br>
 *      - Plain (whole plan)<br>
 *      - Tile (8 pixels bloc)<br>
 *      - Line (per pixel scroll)<br>
 *
 *  \param plan
 *      Plan we want to set the horizontal scroll.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B
 *  \param line
 *      First line we want to set the horizontal scroll.
 *  \param values
 *      H scroll offsets.<br>
 *      Negative values will move the plan to the left while positive values will move it to the right.
 *  \param len
 *      Number of line to set.
 *  \param use_dma
 *      Use DMA flag (faster for large transfer).
 *
 *  \see VDP_setScrollingMode()
 */
void VDP_setHorizontalScrollLine(VDPPlan plan, u16 line, s16* values, u16 len, u16 use_dma);

/**
 *  \brief
 *      Set plan vertical scroll (plain scroll mode).
 *      2 vertical scroll modes are supported:<br>
 *      - Plain (whole plan)<br>
 *      - 2-Tiles (16 pixels bloc)<br>
 *
 *  \param plan
 *      Plan we want to set the vertical scroll.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *  \param value
 *      V scroll offset.<br>
 *      Negative value will move the plan down while positive value will move it up.
 *
 *  \see VDP_setScrollingMode()
 */
void VDP_setVerticalScroll(VDPPlan plan, s16 value);
/**
 *  \brief
 *      Set plan vertical scroll (2-Tiles scroll mode).
 *      2 vertical scroll modes are supported:<br>
 *      - Plain (whole plan)<br>
 *      - 2-Tiles (16 pixels bloc)<br>
 *
 *  \param plan
 *      Plan we want to set the vertical scroll.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *  \param tile
 *      First tile we want to set the vertical scroll.
 *  \param values
 *      V scroll offsets.<br>
 *      Negative values will move the plan down while positive values will move it up.
 *  \param len
 *      Number of tile to set.
 *  \param use_dma
 *      Use DMA flag (faster for large transfer).
 *
 *  \see VDP_setScrollingMode()
 */
void VDP_setVerticalScrollTile(VDPPlan plan, u16 tile, s16* values, u16 len, u16 use_dma);

/**
 *  \brief
 *      Clear specified plan.
 *
 *  \param plan
 *      Plan we want to clear.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *      - PLAN_WINDOW<br>
 *  \param wait
 *      Wait the operation to complete when set to TRUE otherwise it returns immediately
 *      but then you will require to wait for DMA completion (#DMA_waitCompletion()) before accessing the VDP.
 */
void VDP_clearPlan(VDPPlan plan, u16 wait);

/**
 *  \brief
 *      Returns the plan used to display text.
 *
 *  Returned value should be either equals to PLAN_A, PLAN_B or PLAN_WINDOW.
 *
 *  \see VDP_drawText(..)
 *  \see VDP_clearText(..)
 */
VDPPlan VDP_getTextPlan();
/**
 *  \brief
 *      Returns the palette number used to display text.
 *
 *  \see VDP_drawText(..)
 *  \see VDP_clearText(..)
 */
u16 VDP_getTextPalette();
/**
 *  \brief
 *      Returns the priority used to display text.
 *
 *  \see VDP_drawText(..)
 *  \see VDP_clearText(..)
 */
u16 VDP_getTextPriority();

/**
 *  \brief
 *      Define the plan to use to display text.
 *
 *  \param plan
 *      Plan where to display text.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *      - PLAN_WINDOW<br>
 *
 *  \see VDP_drawText(..)
 *  \see VDP_clearText(..)
 */
void VDP_setTextPlan(VDPPlan plan);
/**
 *  \brief
 *      Define the palette to use to display text.
 *
 *  \param palette
 *      Palette number.
 *
 *  \see VDP_drawText(..)
 *  \see VDP_clearText(..)
 */
void VDP_setTextPalette(u16 palette);
/**
 *  \brief
 *      Define the priority to use to display text.
 *
 *  \param prio
 *      Priority:<br>
 *      1 = HIGH PRIORITY TILE.<br>
 *      0 = LOW PRIORITY TILE.<br>
 *
 *  \see VDP_drawText(..)
 *  \see VDP_clearText(..)
 */
void VDP_setTextPriority(u16 prio);

/**
 *  \brief
 *      Draw text in specified plan.
 *
 *  \param plan
 *      Plan where we want to draw text.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *      - PLAN_WINDOW<br>
 *  \param str
 *      String to draw.
 *  \param x
 *      X position (in tile).
 *  \param y
 *      y position (in tile).
 *
 *  \see VDP_clearText(..)
 *  \see VDP_setTextPalette(..)
 *  \see VDP_setTextPriority(..)
 *  \see VDP_setTextPlan(..)
 */
void VDP_drawTextBG(VDPPlan plan, const char *str, u16 x, u16 y);
/**
 *  \brief
 *      Clear a single line portion of text.
 *
 *  \param plan
 *      Plan where we want to clear text.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *      - PLAN_WINDOW<br>
 *  \param x
 *      X position (in tile).
 *  \param y
 *      y position (in tile).
 *  \param w
 *      width to clear (in tile).
 *
 *  \see VDP_drawText(..)
 *  \see VDP_clearTextArea(..)
 *  \see VDP_clearTextLine(..)
 */
void VDP_clearTextBG(VDPPlan plan, u16 x, u16 y, u16 w);
/**
 *  \brief
 *      Clear a specific area of text.
 *
 *  \param plan
 *      Plan where we want to clear text.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *      - PLAN_WINDOW<br>
 *  \param x
 *      X position (in tile).
 *  \param y
 *      y position (in tile).
 *  \param w
 *      width to clear (in tile).
 *  \param h
 *      heigth to clear (in tile).
 *
 *  \see VDP_drawText(..)
 *  \see VDP_clearText(..)
 *  \see VDP_clearTextLine(..)
 */
void VDP_clearTextAreaBG(VDPPlan plan, u16 x, u16 y, u16 w, u16 h);
/**
 *  \brief
 *      Clear a complete line of text.
 *
 *  \param plan
 *      Plan where we want to clear text.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *      - PLAN_WINDOW<br>
 *  \param y
 *      y position (in tile).
 *
 *  \see VDP_drawText(..)
 *  \see VDP_clearText(..)
 *  \see VDP_clearTextArea(..)
 */
void VDP_clearTextLineBG(VDPPlan plan, u16 y);

/**
 *  \brief
 *      Draw text.
 *
 *  \param str
 *      String to draw.
 *  \param x
 *      X position (in tile).
 *  \param y
 *      y position (in tile).
 *
 *  \see VDP_clearText(..)
 *  \see VDP_setTextPalette(..)
 *  \see VDP_setTextPriority(..)
 *  \see VDP_setTextPlan(..)
 */
void VDP_drawText(const char *str, u16 x, u16 y);
/**
 *  \brief
 *      Clear a single line portion of text.
 *
 *  \param x
 *      X position (in tile).
 *  \param y
 *      y position (in tile).
 *  \param w
 *      width to clear (in tile).
 *
 *  \see VDP_drawText(..)
 *  \see VDP_clearTextArea(..)
 *  \see VDP_clearTextLine(..)
 */
void VDP_clearText(u16 x, u16 y, u16 w);
/**
 *  \brief
 *      Clear a specific area of text.
 *
 *  \param x
 *      X position (in tile).
 *  \param y
 *      y position (in tile).
 *  \param w
 *      width to clear (in tile).
 *  \param h
 *      heigth to clear (in tile).
 *
 *  \see VDP_drawText(..)
 *  \see VDP_clearText(..)
 *  \see VDP_clearTextLine(..)
 */
void VDP_clearTextArea(u16 x, u16 y, u16 w, u16 h);
/**
 *  \brief
 *      Clear a complete line of text.
 *
 *  \param y
 *      y position (in tile).
 *
 *  \see VDP_drawText(..)
 *  \see VDP_clearText(..)
 *  \see VDP_clearTextArea(..)
 */
void VDP_clearTextLine(u16 y);

/**
 *  \brief
 *      Draw Bitmap in specified background plan and at given position.
 *
 *  \param plan
 *      Plan where we want to draw the bitmap.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *      - PLAN_WINDOW<br>
 *  \param bitmap
 *      Genesis bitmap (the width and height should be aligned to 8).<br>
 *      The Bitmap is unpacked "on the fly" if needed (require some memory).
 *  \param x
 *      Plan X position (in tile).
 *  \param y
 *      Plan Y position (in tile).
 *  \return
 *      FALSE if there is not enough memory to unpack the specified Bitmap (only if compression was enabled).
 *
 *  This function does "on the fly" 4bpp bitmap conversion to tile data and transfert them to VRAM.<br>
 *  It's very helpful when you use bitmap images but the conversion eats sometime so you should use it only for static screen only.<br>
 *  For "in-game" condition you should use VDP_loadTileData() method with prepared tile data.
 *
 *  \see VDP_loadBMPTileData()
 */
u16 VDP_drawBitmap(VDPPlan plan, const Bitmap *bitmap, u16 x, u16 y);
/**
 *  \brief
 *      Draw Bitmap in specified background plan and at given position.
 *
 *  \param plan
 *      Plan where we want to draw the bitmap.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *      - PLAN_WINDOW<br>
 *  \param bitmap
 *      Genesis bitmap (the width and height should be aligned to 8).<br>
 *      The Bitmap is unpacked "on the fly" if needed (require some memory).
 *  \param basetile
 *      Base tile attributes data (see TILE_ATTR_FULL() macro).
 *  \param x
 *      Plan X position (in tile).
 *  \param y
 *      Plan Y position (in tile).
 *  \param loadpal
 *      Load the bitmap palette information when non zero.
 *  \return
 *      FALSE if there is not enough memory to unpack the specified Bitmap (only if compression was enabled).
 *
 *  This function does "on the fly" 4bpp bitmap conversion to tile data and transfert them to VRAM.<br>
 *  It's very helpful when you use bitmap images but the conversion eats sometime so you should use it only for static screen only.<br>
 *  For "in-game" condition you should use VDP_loadTileData() method with prepared tile data.<br>
 *
 *  \see VDP_loadBMPTileData()
 */
u16 VDP_drawBitmapEx(VDPPlan plan, const Bitmap *bitmap, u16 basetile, u16 x, u16 y, u16 loadpal);

/**
 *  \brief
 *      Draw Map in specified background plan and at given position.
 *
 *  \param plan
 *      Plan where we want to draw the map.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *      - PLAN_WINDOW<br>
 *  \param image
 *      Image structure to draw.<br>
 *      The Image is unpacked "on the fly" if needed (require some memory).
 *  \param x
 *      Plan X position (in tile).
 *  \param y
 *      Plan Y position (in tile).
 *  \return
 *      FALSE if there is not enough memory to unpack the specified Image (only if compression was enabled).
 *
 *  Load the image tiles data in VRAM and display it at specified tilemap region.
 *
 *  \see VDP_drawImageEx()
 */
u16 VDP_drawImage(VDPPlan plan, const Image *image, u16 x, u16 y);
/**
 *  \brief
 *      Draw Map at the specified plan position.
 *
 *  \param plan
 *      Plan where we want to load map.<br>
 *      Accepted values are:<br>
 *      - PLAN_A<br>
 *      - PLAN_B<br>
 *      - PLAN_WINDOW<br>
 *  \param image
 *      Image structure to draw.<br>
 *      The Image is unpacked "on the fly" if needed (require some memory).
 *  \param basetile
 *      Base tile attributes data (see TILE_ATTR_FULL() macro).
 *  \param x
 *      Plan X position (in tile).
 *  \param y
 *      Plan Y position (in tile).
 *  \param loadpal
 *      Load the bitmap palette information when non zero (can be TRUE or FALSE)
 *  \param use_dma
 *      Use DMA transfert (faster but can lock Z80 execution).
 *  \return
 *      FALSE if there is not enough memory to unpack the specified Image (only if image was packed).
 *
 *  Load the image tiles data in VRAM and display it at specified tilemap region.
 *
 *  \see VDP_drawImage()
 */
u16 VDP_drawImageEx(VDPPlan plan, const Image *image, u16 basetile, u16 x, u16 y, u16 loadpal, u16 use_dma);


#endif // _VDP_BG_H_
