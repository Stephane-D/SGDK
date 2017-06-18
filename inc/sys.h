/**
 *  \file sys.h
 *  \brief Entry point unit / Interrupt callback / System
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit contains SGDK initialization / reset methods, IRQ callbacks and others system stuff.
 */

#ifndef _SYS_H_
#define _SYS_H_


#define PROCESS_PALETTE_FADING      (1 << 0)
#define PROCESS_BITMAP_TASK         (1 << 1)
#define PROCESS_TILECACHE_TASK      (1 << 2)
#define PROCESS_DMA_TASK            (1 << 3)
#define PROCESS_XGM_TASK            (1 << 4)


/**
 *  \brief
 *      Bus error interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *busErrorCB;
/**
 *  \brief
 *      Address error interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *addressErrorCB;
/**
 *  \brief
 *      Illegal instruction exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *illegalInstCB;
/**
 *  \brief
 *      Division by zero exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *zeroDivideCB;
/**
 *  \brief
 *      CHK instruction interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *chkInstCB;
/**
 *  \brief
 *      TRAPV instruction interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *trapvInstCB;
/**
 *  \brief
 *      Privilege violation exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *privilegeViolationCB;
/**
 *  \brief
 *      Trace interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *traceCB;
/**
 *  \brief
 *      Line 1x1x exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *line1x1xCB;
/**
 *  \brief
 *      Error exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern _voidCallback *errorExceptionCB;
/**
 *  \brief
 *      Level interrupt callback.
 *
 * You can modify it to use your own callback.
 */
extern _voidCallback *intCB;
/**
 *  \brief
 *      Internal Vertical interrupt callback.
 *
 * You can modify it to use your own callback.<br>
 * Be careful: by doing that you disable SGDK default V-Int code and related features !<br>
 * You should use it only for very low level process and if you don't care about SGDK facilities.<br>
 * In all others cases you would use the SYS_setVIntCallback() method.
 */
extern _voidCallback *internalVIntCB;
/**
 *  \brief
 *      Internal Horizontal interrupt callback.
 *
 * You can modify it to use your own callback.<br>
 * Be careful: by doing that you disable SGDK default H-Int code and related features !<br>
 * You should use it only for very low level process and if you don't care about SGDK facilities.<br>
 * In all others cases you would use the SYS_setHIntCallback() method.
 */
extern _voidCallback *internalHIntCB;
/**
 *  \brief
 *      Internal External interrupt callback.
 *
 * You can modify it to use your own callback.<br>
 * Be careful: by doing that you disable SGDK default Ext-Int code and related features !<br>
 * You should use it only for very low level process and if you don't care about SGDK facilities.<br>
 * In all others cases you would use the SYS_setExtIntCallback() method.
 */
extern _voidCallback *internalExtIntCB;


/**
 *  \brief
 *      Assert reset
 *
 * Assert reset pin on the 68000 CPU.
 * This is needed to reset some attached hardware.
 */
void SYS_assertReset();
/**
 *  \brief
 *      Soft reset
 *
 * Software reset
 */
void SYS_reset();
/**
 *  \brief
 *      Hard reset
 *
 * Reset with forced hardware init and memory clear / reset operation.
 */
void SYS_hardReset();

/**
 *  \brief
 *      Return current interrupt mask level.
 *
 * See SYS_setInterruptMaskLevel() for more informations about interrupt mask level.
 */
u16 SYS_getInterruptMaskLevel();
/**
 *  \brief
 *      Set interrupt mask level.
 *
 * You can disable interrupt depending their level.<br>
 * Interrupt with level <= interrupt mask level are ignored.<br>
 * We have 3 different interrupts:<br>
 * <b>Vertical interrupt (V-INT): level 6</b><br>
 * <b>Horizontal interrupt (H-INT): level 4</b><br>
 * <b>External interrupt (EX-INT): level 2</b><br>
 * Vertical interrupt has the highest level (and so priority) where external interrupt has lowest one.<br>
 * For instance to disable Vertical interrupt just use SYS_setInterruptMaskLevel(6).<br>
 *
 * \see SYS_getInterruptMaskLevel()
 * \see SYS_getAndSetInterruptMaskLevel()
 * \see SYS_setVIntCallback()
 * \see SYS_setHIntCallback()
 */
void SYS_setInterruptMaskLevel(u16 value);

/**
 *  \brief
 *      Set the interrupt mask level to given value and return previous level.
 *
 * You can disable interrupt depending their level.<br>
 * Interrupt with level <= interrupt mask level are ignored.<br>
 * We have 3 different interrupts:<br>
 * <b>Vertical interrupt (V-INT): level 6</b><br>
 * <b>Horizontal interrupt (H-INT): level 4</b><br>
 * <b>External interrupt (EX-INT): level 2</b><br>
 * Vertical interrupt has the highest level (and so priority) where external interrupt has lowest one.<br>
 * For instance to disable Vertical interrupt just use SYS_setInterruptMaskLevel(6).<br>
 *
 * \see SYS_getInterruptMaskLevel()
 * \see SYS_setInterruptMaskLevel()
 * \see SYS_setVIntCallback()
 * \see SYS_setHIntCallback()
 */
u16 SYS_getAndSetInterruptMaskLevel(u16 value);

/**
 *  \brief
 *      Disable interrupts (Vertical, Horizontal and External).
 *
 *
 * This method is used to temporary disable interrupt (to protect some VDP accesses for instance)
 * and should always be followed by SYS_enableInts().<br>
 * Be careful, this method can't be used if you are currently processing an interrupt !
 *
 * \see SYS_enableInts()
 */
void SYS_disableInts();
/**
 *  \brief
 *      Reenable interrupts (Vertical, Horizontal and External).
 *
 * This method is used to reenable interrupt after a call to SYS_disableInts().<br>
 * Has no effect if called without a prior SYS_disableInts() call.
 *
 * \see SYS_disableInts()
 */
void SYS_enableInts();

/**
 *  \brief
 *      Set start of 'Vertical interrupt' callback method.
 *
 *  \param CB
 *      Pointer to the method to call when the Vertical Interrupt just started.<br>
 *      You can remove current callback by passing a null pointer here.
 *
 * Vertical interrupt happen at the end of display period right before vertical blank.<br>
 * This period is usually used to prepare next frame data (refresh sprites, scrolling ...).<br>
 * The difference with the SYS_setVIntCallback(..) method is this one is called right after
 * the Vertical Interrupt happened and before any internals SGDK V-Int processes.<br>
 * This is useful when you really need to do something right at the beginning of the V-Blank area.
 *
 * \see SYS_setVIntCallback(_voidCallback *CB);
 * \see SYS_setHIntCallback(_voidCallback *CB);
 */
void SYS_setVIntPreCallback(_voidCallback *CB);
/**
 *  \brief
 *      Set 'Vertical Interrupt' callback method.
 *
 *  \param CB
 *      Pointer to the method to call on Vertical Interrupt.<br>
 *      You can remove current callback by passing a null pointer here.
 *
 * Vertical interrupt happen at the end of display period right before vertical blank.<br>
 * This period is usually used to prepare next frame data (refresh sprites, scrolling ...).<br>
 * Note that the callback will be called after some internal SGDK V-Int processes and so probably
 * not right at the start of the V-Blank area.<br>
 * For that you can use the SYS_setPreVIntCallback(..) method instead.
 *
 * \see SYS_setHIntCallback(_voidCallback *CB);
 * \see SYS_setVIntPreCallback(_voidCallback *CB);
 */
void SYS_setVIntCallback(_voidCallback *CB);
/**
 *  \brief
 *      Set 'Horizontal Interrupt' callback method.
 *
 *  \param CB
 *      Pointer to the method to call on Horizontal Interrupt.<br>
 *      You can remove current callback by passing a null pointer here.
 *
 * Horizontal interrupt happen at the end of scanline display period right before Horizontal blank.<br>
 * This period is usually used to do mid frame changes (palette, scrolling or others raster effect)
 */
void SYS_setHIntCallback(_voidCallback *CB);
/**
 *  \brief
 *      Set External interrupt callback method.
 *
 *  \param CB
 *      Pointer to the method to call on External Interrupt.<br>
 *      You can remove current callback by passing a null pointer here.
 *
 * External interrupt happen on Light Gun trigger (HVCounter is locked).
 */
void SYS_setExtIntCallback(_voidCallback *CB);

/**
 *  \brief
 *      Return != 0 if we are in the V-Int callback method.
 *
 * This method tests if we are currently processing a Vertical retrace interrupt.
 */
u16 SYS_isInVIntCallback();
/**
 *  \brief
 *      Return != 0 if we are in the H-Int callback method.
 *
 * This method tests if we are currently processing a Horizontal retrace interrupt.
 */
u16 SYS_isInHIntCallback();
/**
 *  \brief
 *      Return != 0 if we are in the Ext-Int callback method.
 *
 * This method tests if we are currently processing an External interrupt.
 */
u16 SYS_isInExtIntCallback();
/**
 *  \brief
 *      Return != 0 if we are in an interrupt callback method (Vertical, Horizontal or External)
 *
 * This method tests if we are currently processing an interrupt.
 */
u16 SYS_isInInterrupt();

/**
 *  \brief
 *      Return != 0 if we are on a NTSC system.
 *
 * Better to use the IS_PALSYSTEM
 */
u16 SYS_isNTSC();
/**
 *  \brief
 *      Return != 0 if we are on a PAL system.
 *
 * Better to use the IS_PALSYSTEM
 */
u16 SYS_isPAL();

/**
 *  \brief
 *      Die with the specified error message.<br>
 *      Program execution is interrupted.
 *
 * This actually display an error message and program ends execution.
 */
void SYS_die(char *err);


#endif // _SYS_H_
