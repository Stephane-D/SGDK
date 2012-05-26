/**
 * \file sys.h
 * \brief Entry point unit / Interrupt callback / System
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit contains SGDK initialization / reset methods, IRQ callbacks and others system stuff.
 */

#ifndef _SYS_H_
#define _SYS_H_


#define PROCESS_PALETTE_FADING      (1 << 0)
#define PROCESS_BITMAP_TASK         (1 << 1)


/**
 * \brief
 *      Bus error interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *busErrorCB;
/**
 * \brief
 *      Address error interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *addressErrorCB;
/**
 * \brief
 *      Illegal instruction exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *illegalInstCB;
/**
 * \brief
 *      Division by zero exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *zeroDivideCB;
/**
 * \brief
 *      CHK instruction interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *chkInstCB;
/**
 * \brief
 *      TRAPV instruction interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *trapvInstCB;
/**
 * \brief
 *      Privilege violation exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *privilegeViolationCB;
/**
 * \brief
 *      Trace interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *traceCB;
/**
 * \brief
 *      Line 1x1x exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *line1x1xCB;
/**
 * \brief
 *      Error exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *errorExceptionCB;
/**
 * \brief
 *      Level interrupt callback.
 *
 * You can modify it to use your own callback.
 */
extern _voidCallback *intCB;
/**
 * \brief
 *      Internal Vertical interrupt callback.
 *
 * You can modify it to use your own callback.<br/>
 * Be careful: by doing that you disable SGDK default V-Int code and related features !<br/>
 * You should use it only for very low level process and if you don't care about SGDK facilities.<br/>
 * In all others cases you would use the SYS_setVIntCallback() method.
 */
extern _voidCallback *internalVIntCB;
/**
 * \brief
 *      Internal Horizontal interrupt callback.
 *
 * You can modify it to use your own callback.<br/>
 * Be careful: by doing that you disable SGDK default H-Int code and related features !<br/>
 * You should use it only for very low level process and if you don't care about SGDK facilities.<br/>
 * In all others cases you would use the SYS_setHIntCallback() method.
 */
extern _voidCallback *internalHIntCB;
/**
 * \brief
 *      Internal External interrupt callback.
 *
 * You can modify it to use your own callback.<br/>
 * Be careful: by doing that you disable SGDK default Ext-Int code and related features !<br/>
 * You should use it only for very low level process and if you don't care about SGDK facilities.<br/>
 * In all others cases you would use the SYS_setExtIntCallback() method.
 */
extern _voidCallback *internalExtIntCB;


/**
 * \brief
 *      Assert reset
 *
 * Assert reset pin on the 68000 CPU.
 * This is needed to reset some attached hardware.
 */
void SYS_assertReset();
/**
 * \brief
 *      Soft reset
 *
 * Software reset
 */
void SYS_reset();

/**
 * \brief
 *      Return interrupt mask level.
 *
 * See SYS_setInterruptMaskLevel() for more informations about interrupt mask level.
 */
u16 SYS_getInterruptMaskLevel();
/**
 * \brief
 *      Set interrupt mask level.
 *
 * You can disable interrupt depending their level.<br>
 * Interrupt with level <= interrupt mask level are ignored.<br>
 * We have 3 different interrupts:<br>
 * <b>Vertical interrupt (V-INT): level 6</b>
 * <b>Horizontal interrupt (H-INT): level 4</b>
 * <b>External interrupt (EX-INT): level 2</b>
 * Vertical interrupt has the highest level (and so priority) where external interrupt has lowest one.<br>
 * See also SYS_getInterruptMaskLevel(), SYS_setVIntCallback() and SYS_setHIntCallback() methods.
 */
void SYS_setInterruptMaskLevel(u16 value);


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
void SYS_setVIntCallback(_voidCallback *CB);
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
void SYS_setHIntCallback(_voidCallback *CB);
/**
 * \brief
 *      Set External interrupt callback method.
 *
 * \param CB
 *      Pointer to the method to call on External Interrupt.<br>
 *      You can remove current callback by passing a null pointer here.
 *
 * External interrupt happen on Light Gun trigger (HVCounter is locked).
 */
void SYS_setExtIntCallback(_voidCallback *CB);

/**
 * \brief
 *      Return != 0 if we are in the V-Int callback method.
 *
 * You can test if we are currently in the V-Int callback with this function.
 */
u16 SYS_isInVIntCallback();

/**
 * \brief
 *      Return != 0 if we are in the H-Int callback method.
 *
 * You can test if we are currently in the H-Int callback with this function.
 */
u16 SYS_isInHIntCallback();

/**
 * \brief
 *      Return != 0 if we are in the Ext-Int callback method.
 *
 * You can test if we are currently in the Ext-Int callback with this function.
 */
u16 SYS_isInExtIntCallback();


#endif // _SYS_H_
