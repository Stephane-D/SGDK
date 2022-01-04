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
#define PROCESS_DMA_TASK            (1 << 2)
#define PROCESS_XGM_TASK            (1 << 3)
#define PROCESS_VDP_SCROLL_TASK     (1 << 4)


#define ROM_ALIGN_BIT               17
#define ROM_ALIGN                   (1 << ROM_ALIGN_BIT)
#define ROM_ALIGN_MASK              (ROM_ALIGN - 1)

#define ROM_START                   0
#define ROM_END                     (((u32) &_stext) + ((u32) &_sdata))
#define ROM_SIZE                    ((ROM_END + ROM_ALIGN_MASK) & (~ROM_ALIGN_MASK))


// exist through rom_head.c
typedef struct
{
    char console[16];               /* Console Name (16) */
    char copyright[16];             /* Copyright Information (16) */
    char title_local[48];           /* Domestic Name (48) */
    char title_int[48];             /* Overseas Name (48) */
    char serial[14];                /* Serial Number (2, 12) */
    u16 checksum;                   /* Checksum (2) */
    char IOSupport[16];             /* I/O Support (16) */
    u32 rom_start;                  /* ROM Start Address (4) */
    u32 rom_end;                    /* ROM End Address (4) */
    u32 ram_start;                  /* Start of Backup RAM (4) */
    u32 ram_end;                    /* End of Backup RAM (4) */
    char sram_sig[2];               /* "RA" for save ram (2) */
    u16 sram_type;                  /* 0xF820 for save ram on odd bytes (2) */
    u32 sram_start;                 /* SRAM start address - normally 0x200001 (4) */
    u32 sram_end;                   /* SRAM end address - start + 2*sram_size (4) */
    char modem_support[12];         /* Modem Support (24) */
    char notes[40];                 /* Memo (40) */
    char region[16];                /* Country Support (16) */
} ROMHeader;

extern const ROMHeader rom_header;

// size of text segment --> start of initialized data (RO)
extern u32 _stext;
// size of initialized data segment
extern u32 _sdata;

/**
 *  \brief
 *      Define at which period to do VBlank process (see #SYS_doVBlankProcess() method)
 */
typedef enum
{
    IMMEDIATELY,        /** Start VBlank process immediately whatever we are in blanking period or not */
    ON_VBLANK ,         /** Start VBlank process on VBlank period, start immediatly in we are already in VBlank */
    ON_VBLANK_START     /** Start VBlank process on VBlank *start* period, means that we wait the next *start* of VBlank period if we missed it */
} VBlankProcessTime;

/**
 *  \brief
 *      Bus error interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern VoidCallback *busErrorCB;
/**
 *  \brief
 *      Address error interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern VoidCallback *addressErrorCB;
/**
 *  \brief
 *      Illegal instruction exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern VoidCallback *illegalInstCB;
/**
 *  \brief
 *      Division by zero exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern VoidCallback *zeroDivideCB;
/**
 *  \brief
 *      CHK instruction interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern VoidCallback *chkInstCB;
/**
 *  \brief
 *      TRAPV instruction interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern VoidCallback *trapvInstCB;
/**
 *  \brief
 *      Privilege violation exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern VoidCallback *privilegeViolationCB;
/**
 *  \brief
 *      Trace interrupt callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern VoidCallback *traceCB;
/**
 *  \brief
 *      Line 1x1x exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern VoidCallback *line1x1xCB;
/**
 *  \brief
 *      Error exception callback.
 *
 * You can modify it to use your own callback (for debug purpose).
 */
extern VoidCallback *errorExceptionCB;
/**
 *  \brief
 *      Level interrupt callback.
 *
 * You can modify it to use your own callback.
 */
extern VoidCallback *intCB;


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
 *      Wait for start of VBlank and do all the VBlank processing (DMA transfers, XGM driver tempo, Joypad pooling..)
 *  \return FALSE if process was canceled because the method was called from V-Int (vertical interrupt) callback
 *      in which case we exit the function as V-Int will be triggered immediately.<br>
 *
 * Do all the SGDK VBlank process.<br>
 * Some specific processing should be done during the Vertical Blank period as the VDP is idle at this time.
 * This is always where we should do all VDP data transfer (using the DMA preferably) but we can also do the processes which
 * has to be done at a frame basis (joypad polling, sound driver sync/update..)<br>
 * In the case of SGDK, calling this method will actually do the following tasks:<br>
 * - flush the DMA queue<br>
 * - process asynchronous palette fading operation<br>
 * - joypad polling<br>
 * <br>
 * Note that VBlank process may be delayed to next VBlank if we missed the start of the VBlank period so that will cause a frame miss.
 */
bool SYS_doVBlankProcess();
/**
 *  \brief
 *      Do all the VBlank processing (DMA transfers, XGM driver tempo, Joypad pooling..)
 *  \param processTime
 *      Define at which period we start VBlank process, accepted values are:<br>
 *      <b>IMMEDIATELY</b>      Start VBlank process immediatly whatever we are in blanking period or not
 *                              (*highly discouraged* unless you really know what you're doing !)<br>
 *      <b>ON_VBLANK</b>        Start VBlank process on VBlank period, if we already are in VBlank period
 *                              it starts immediately (discouraged as VBlank period may be shortened and all
 *                              processes cannot be completed in time)<br>
 *      <b>ON_VBLANK_START</b>  Start VBlank process on VBlank *start* period (recommanded as default value).
 *                              That means that if #SYS_doVBlankProcess() is called too late (after the start
 *                              of VBlank) then we force a passive wait for the next start of VBlank so we can
 *                              align the processing with the beggining of VBlank period to ensure fast DMA
 *                              transfert and avoid possible graphical glitches due to VRAM update during active display.<br>
 *  \return FALSE if process was canceled because we forced Start VBlank process (<i>time = ON_VBLANK_START</i>)
 *      and the method was called from V-Int (vertical interrupt) callback in which case we exit the function
 *      as V-Int will be triggered immediately.<br>
 *
 * Do all the SGDK VBlank process.<br>
 * Some specific processing should be done during the Vertical Blank period as the VDP is idle at this time.
 * This is always where we should do all VDP data transfer (using the DMA preferably) but we can also do the processes which
 * has to be done at a frame basis (joypad polling, sound driver sync/update..)<br>
 * In the case of SGDK, calling this method will actually do the following tasks:<br>
 * - flush the DMA queue<br>
 * - process asynchronous palette fading operation<br>
 * - joypad polling<br>
 * <br>
 * Note that depending the used <i>time</i> parameter, VBlank process may be delayed to next VBlank so that will wause a frame miss.
 */
bool SYS_doVBlankProcessEx(VBlankProcessTime processTime);

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
 * This method is used to temporary disable interrupts to protect some processes and should always be followed by SYS_enableInts().<br>
 * You need to protect against interrupts any processes than can be perturbed / corrupted by the interrupt callback code (IO ports access in general but not only).<br>
 * Now by default SGDK doesn't do anything armful in its interrupts handlers (except with the Bitmap engine) so it's not necessary to protect from interrupts by default
 * but you may need it if your interrupts callback code does mess with VDP for instance.<br>
 * Note that you can nest #SYS_disableInts / #SYS_enableInts() calls.
 *
 * \see SYS_enableInts()
 */
void SYS_disableInts();
/**
 *  \brief
 *      Re-enable interrupts (Vertical, Horizontal and External).
 *
 * This method is used to reenable interrupts after a call to #SYS_disableInts().<br>
 * Note that you can nest #SYS_disableInts / #SYS_enableInts() calls.
 *
 * \see SYS_disableInts()
 */
void SYS_enableInts();

/**
 *  \brief
 *      Set user 'Vertical Blank' callback method.
 *
 *  \param CB
 *      Pointer to the method to call on Vertical Blank period.<br>
 *      You can remove current callback by passing a <i>NULL</i> pointer here.
 *
 * Vertical blank period starts right at the end of display period.<br>
 * This period is usually used to prepare next frame data (refresh sprites, scrolling ...).<br>
 * SGDK handle that in the #SYS_doVBlankProcess() method and will call the user 'Vertical Blank' from this method after all major tasks.<br>
 * It's recommended to use the 'Vertical Blank' callback instead of the 'VInt' callback if you need to do some VDP accesses.
 *
 * \see SYS_setVIntCallback(VoidCallback *CB);
 */
void SYS_setVBlankCallback(VoidCallback *CB);

/**
 *  \brief
 *      Set 'Vertical Interrupt' callback method.
 *
 *  \param CB
 *      Pointer to the method to call on Vertical Interrupt.<br>
 *      You can remove current callback by passing a <i>NULL</i> pointer here.
 *
 * Vertical interrupt happen at the end of display period at the start of the vertical blank period.<br>
 * This period is usually used to prepare next frame data (refresh sprites, scrolling ...) though now
 * SGDK handle most of these process using #SYS_doVBlankProcess() so you can control it manually (do it from main loop or put it in Vint callback).<br>
 * The only things that SGDK always handle from the vint callback is the XGM sound driver music tempo and Bitmap engine phase reset.<br>
 * It's recommended to keep your code as fast as possible as it will eat precious VBlank time, nor you should touch the VDP from your Vint callback
 * otherwise you will need to protect any VDP accesses from your main loop (which is painful), use the SYS_setVIntCallback(..) instead for that.
 *
 * \see SYS_setVBlankCallback(VoidCallback *CB);
 * \see SYS_setHIntCallback(VoidCallback *CB);
 */
void SYS_setVIntCallback(VoidCallback *CB);
/**
 *  \brief
 *      Set 'Horizontal Interrupt' callback method.
 *
 *  \param CB
 *      Pointer to the method to call on Horizontal Interrupt.<br>
 *      You can remove current callback by passing a null pointer here.
 *
 * Horizontal interrupt happen at the end of scanline display period right before Horizontal blank.<br>
 * This period is usually used to do mid frame changes (palette, scrolling or others raster effect).<br>
 * When you do that, don't forget to protect your VDP access from your main loop using
 * #SYS_disableInts() / #SYS_enableInts() otherwise you may corrupt your VDP writes.
 */
void SYS_setHIntCallback(VoidCallback *CB);
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
void SYS_setExtIntCallback(VoidCallback *CB);

/**
 *  \deprecated
 *      Use #SYS_isInVInt() instead
 */
u16 SYS_isInVIntCallback();
/**
 *  \deprecated
 *      Always return 0 now, you need to use your own flag to detect if you are processing a Horizontal interrupt
 */
u16 SYS_isInHIntCallback();
/**
 *  \deprecated
 *      Always return 0 now, you need to use your own flag to detect if you are processing an External interrupt
 */
u16 SYS_isInExtIntCallback();
/**
 *  \deprecated
 *      Use #SYS_isInVInt() instead, only vertical interrupt supported now
 */
u16 SYS_isInInterrupt();

/**
 *  \brief
 *      Return TRUE if we are in the V-Interrupt process.
 *
 * This method tests if we are currently processing a Vertical retrace interrupt (V-Int callback).
 */
bool SYS_isInVInt();

/**
 *  \deprecated
 *      Not anymore useful as #SYS_doVBlankProcess() handle that directly now
 */
void SYS_setVIntAligned(bool value);
/**
 *  \deprecated
 *      Not anymore useful as #SYS_doVBlankProcess() handle that directly now
 */
bool SYS_isVIntAligned();

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
 *      Returns number of Frame Per Second.
 *
 * This function actually returns the number of time it was called in the last second.<br>
 * i.e: for benchmarking you should call this method only once per frame update.
 */
u32 SYS_getFPS();
/**
 *  \brief
 *      Returns number of Frame Per Second (fix32 form).
 *
 * This function actually returns the number of time it was called in the last second.<br>
 * i.e: for benchmarking you should call this method only once per frame update.
 */
fix32 SYS_getFPSAsFloat();
/**
 *  \brief
 *      Return an estimation of CPU frame load (in %)
 *
 * Return an estimation of CPU load (in %, mean value computed on 8 frames) based of idle time spent in #VDP_waitVSync() / #VDP_waitVInt() methods.<br>
 * The method can return value above 100% you CPU load is higher than 1 frame.
 *
 * \see VDP_waitVSync()
 * \see VDP_waitVInt()
 */
u16 SYS_getCPULoad();
/**
 *  \brief
 *      Show a cursor indicating current frame load level in scanline (top = 0% load, bottom = 100% load)
 *
 *  \param mean
 *      frame load level display is averaged on 8 frames (mean load)
 *
 *  Show current frame load using a cursor indicating the scanline reached when #VDP_waitVSync() / #VDP_waitVInt() method was called.<br>
 *  Note that internally sprite 0 is used to display to cursor (palette 0 and color 15) as it is not directly used by the Sprite Engine but
 *  if you're using the low level VDP sprite methods then you should know that sprite 0 will be used here.
 *
 * \see SYS_hideFrameLoad()
 */
void SYS_showFrameLoad(bool mean);
/**
 *  \brief
 *      Hide the frame load cursor previously enabled using #SYS_showFrameLoad() method.

 * \see SYS_showFrameLoad()
 */
void SYS_hideFrameLoad();


/**
 *  \brief
 *      Computes full ROM checksum and return it.<br>
 *      The checksum is a custom fast 32 bit checksum converted to 16 bit at end
 */
u16 SYS_computeChecksum();
/**
 *  \brief
 *      Returns TRUE if ROM checksum is ok (correspond to rom_head.checksum field)
 */
bool SYS_isChecksumOk();

/**
 *  \brief
 *      Die with the specified error message.<br>
 *      Program execution is interrupted.
 *
 * This actually display an error message and program ends execution.
 */
void SYS_die(char *err);

#endif // _SYS_H_
