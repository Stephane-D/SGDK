/**
 * \file vdp_bg.h
 * \brief VDP background plan support
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides plan A & plan B facilities :
 * - set scrolling
 * - clear plan
 * - draw text in plan
 */

#ifndef _VDP_BG_H_
#define _VDP_BG_H_


/**
 * \brief
 *      Set plan horizontal scroll.
 *
 * \param plan
 *      Plan we want to set the horizontal scroll.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 * \param line
 *      Line (scanline) we want to set the horizontal scroll if current scroll mode supports it.<br/>
 *      3 horizontal scroll modes are supported:<br/>
 *      - Plain (whole plan)<br/>
 *      - Tile (bloc of 8 pixels)<br/>
 *      - Scanline (per pixel scroll)<br/>
 * \param value
 *      H scroll offset.
 *
 *  See VDP_setReg() function to change scroll mode.
 */
void VDP_setHorizontalScroll(u16 plan, u16 line, u16 value);
/**
 * \brief
 *      Set plan vertical scroll.
 *
 * \param plan
 *      Plan we want to set the vertical scroll.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 * \param cell
 *      Cell we want to set the vertical scroll if current scroll mode supports it.
 *      2 vertical scroll modes are supported:<br/>
 *      - Plain (whole plan)<br/>
 *      - Cell (bloc of 16 pixels)<br/>
 * \param value
 *      V scroll offset.
 *
 *  See VDP_setReg() function to change scroll mode.
 */
void VDP_setVerticalScroll(u16 plan, u16 cell, u16 value);

/**
 * \brief
 *      Clear specified plan.
 *
 * \param plan
 *      Plan we want to clear.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 * \param use_dma
 *      Use DMA or software clear.
 *
 *  Using DMA permit faster clear operation but can lock Z80 execution.
 */
void VDP_clearPlan(u16 plan, u8 use_dma);

/**
 * \brief
 *      Define the palette to use to display text.
 *
 * \param palette
 *      Palette number.
 */
void VDP_setTextPalette(u16 palette);
/**
 * \brief
 *      Define the priority to use to display text.
 *
 * \param prio
 *      Priority:<br/>
 *      1 = HIGH PRIORITY TILE.<br/>
 *      0 = LOW PRIORITY TILE.<br/>
 */
void VDP_setTextPriority(u16 prio);

/**
 * \brief
 *      Returns the palette number used to display text.
 */
u16 VDP_getTextPalette();
/**
 * \brief
 *      Returns the priority used to display text.
 */
u16 VDP_getTextPriority();

/**
 * \brief
 *      Draw text.
 *
 * \param str
 *      String to draw.
 * \param x
 *      X position (in tile).
 * \param y
 *      y position (in tile).
 *
 *  This method uses VDP_PLAN_A to draw the text.<br/>
 *  See VDP_setTextPalette() and VDP_setTextPriority().
 */
void VDP_drawText(const char *str, u16 x, u16 y);
/**
 * \brief
 *      Clear text.
 *
 * \param x
 *      X position (in tile).
 * \param y
 *      y position (in tile).
 * \param w
 *      width to clear (in tile).
 *
 *  This method uses VDP_PLAN_A to draw/clear the text.
 */
void VDP_clearText(u16 x, u16 y, u16 w);
/**
 * \brief
 *      Clear a complete line of text.
 *
 * \param y
 *      y position (in tile).
 *
 *  This method uses VDP_PLAN_A to draw/clear the text.
 */
void VDP_clearTextLine(u16 y);

/**
 * \brief
 *      Draw text in backgound plan.
 *
 * \param plan
 *      Plan we want to use to draw text.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 * \param str
 *      String to draw.
 * \param flags
 *      tile flags (see TILE_ATTR macro).
 * \param x
 *      X position (in tile).
 * \param y
 *      y position (in tile).
 *
 *  This method uses the specified plan to draw the text.<br/>
 *  Each character fit in one tile (8x8 pixels).
 */
void VDP_drawTextBG(u16 plan, const char *str, u16 flags, u16 x, u16 y);
/**
 * \brief
 *      Clear text from background plan.
 *
 * \param plan
 *      Plan where we want to clear text.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 * \param x
 *      X position (in tile).
 * \param y
 *      y position (in tile).
 * \param w
 *      width to clear (in tile).
 */
void VDP_clearTextBG(u16 plan, u16 x, u16 y, u16 w);
/**
 * \brief
 *      Clear a complete line of text from background plan.
 *
 * \param plan
 *      Plan where we want to clear text.<br/>
 *      Accepted values are:<br/>
 *      - VDP_PLAN_A<br/>
 *      - VDP_PLAN_B<br/>
 * \param y
 *      y position (in tile).
 */
void VDP_clearTextLineBG(u16 plan, u16 y);


#endif // _VDP_BG_H_
