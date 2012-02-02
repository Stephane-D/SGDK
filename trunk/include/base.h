/**
 * \file base.h
 * \brief Entry point unit / Interrupt callback
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit contains SGDK initialization routines, reset methods and IRQ callbacks
 */

#ifndef _BASE_H_
#define _BASE_H_


#define PROCESS_PALETTE_FADING      (1 << 0)
#define PROCESS_BITMAP_TASK         (1 << 1)


/**
 * \brief
 *      Assert reset
 *
 * Assert reset pin on the 68000 CPU.
 * This is needed to reset some attached hardware.
 */
void assert_reset();
/**
 * \brief
 *      Soft reset
 *
 * Software reset
 */
void reset();

/**
 * \brief
 *      Set Vertical interrupt callback method.
 *
 * \param CB
 *      Pointer to the method to call on Vertical Interrupt.<br>
 *      You can remove current callback by passing a null pointer here.
 *
 * Vertical interrupt happen at the end of display period right before vertical blank.<br>
 * This period is usually used to prepare next frame data (refresh sprites, scrolling ...)
 */
void setVBlankCallback(_voidCallback *CB);
/**
 * \brief
 *      Set Horizontal interrupt callback method.
 *
 * \param CB
 *      Pointer to the method to call on Horizontal Interrupt.<br>
 *      You can remove current callback by passing a null pointer here.
 *
 * Horizontal interrupt happen at the end of scanline display period right before Horizontal blank.<br>
 * This period is usually used to do mid frame changes (palette, scrolling or others raster effect)
 */
void setHBlankCallback(_voidCallback *CB);


#endif // _BASE_H_
