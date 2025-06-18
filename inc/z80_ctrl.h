/**
 *  \file z80_ctrl.h
 *  \brief Z80 control
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides Z80 access from the YM2612:<br>
 * - enable / disable Z80<br>
 * - request / release Z80 BUS<br>
 * - upload / download data to / from Z80 memory<br>
 * - set Z80 external Bank<br>
 * - Z80 driver handling<br>
 */

#ifndef _Z80_CTRL_H_
#define _Z80_CTRL_H_


#define Z80_HALT_PORT                   0xA11100
#define Z80_RESET_PORT                  0xA11200

/**
 *  \brief
 *
 * Z80 RAM start address.
 */
#define Z80_RAM_START                   0xA00000
/**
 *  \brief
 *
 * Z80 RAM end address.
 */
#define Z80_RAM_END                     0xA01FFF
/**
 *  \brief
 *
 * Z80 RAM address.
 */
#define Z80_RAM                         Z80_RAM_START
/**
 *  \brief
 *
 * Z80 RAM length in byte.
 */
#define Z80_RAM_LEN                     ((Z80_RAM_END - Z80_RAM_START) + 1)
/**
 *  \brief
 *
 * Z80 YM2612 port address.
 */
#define Z80_YM2612                      0xA04000
/**
 *  \brief
 *
 * Z80 Bank register address.
 */
#define Z80_BANK_REGISTER               0xA06000

/**
 *  \brief
 *
 * Z80 default driver command address.
 */
#define Z80_DRV_COMMAND                 0xA00100
/**
 *  \brief
 *
 * Z80 default driver status address.
 */
#define Z80_DRV_STATUS                  0xA00102
/**
 *  \brief
 *
 * Z80 default driver parameters address.
 */
#define Z80_DRV_PARAMS                  0xA00104

// default command and status value
#define Z80_DRV_COM_PLAY_SFT            0
#define Z80_DRV_COM_STOP_SFT            4
#define Z80_DRV_STAT_PLAYING_SFT        0
#define Z80_DRV_STAT_READY_SFT          7

/**
 *  \brief
 *      Z80 default driver play command.
 */
#define Z80_DRV_COM_PLAY                (1 << Z80_DRV_COM_PLAY_SFT)
/**
 *  \brief
 *      Z80 default driver stop command.
 */
#define Z80_DRV_COM_STOP                (1 << Z80_DRV_COM_STOP_SFT)
/**
 *  \brief
 *      Z80 default driver play status.
 */
#define Z80_DRV_STAT_PLAYING            (1 << Z80_DRV_STAT_PLAYING_SFT)
/**
 *  \brief
 *      Z80 default driver ready status.
 */
#define Z80_DRV_STAT_READY              (1 << Z80_DRV_STAT_READY_SFT)


#define Z80_DRV_CH0_SFT                 _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_CH1 instead.\"")
#define Z80_DRV_CH1_SFT                 _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_CH2 instead.\"")
#define Z80_DRV_CH2_SFT                 _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_CH3 instead.\"")
#define Z80_DRV_CH3_SFT                 _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_CH4 instead.\"")

#define Z80_DRV_CH0                     _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_CH1_MSK instead.\"")
#define Z80_DRV_CH1                     _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_CH2_MSK instead.\"")
#define Z80_DRV_CH2                     _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_CH3_MSK instead.\"")
#define Z80_DRV_CH3                     _Pragma("GCC error \"This method is deprecated, use SOUND_PCM_CH4_MSK instead.\"")


/**
 *  \brief
 *      NULL Z80 driver.
 */
#define Z80_DRIVER_NULL                 0
/**
 *  \brief
 *      Variable rate sample player Z80 driver.<br>
 *      It can play a sample (8 bit signed) from 8 Khz up to 32 Khz rate.
 */
#define Z80_DRIVER_PCM                  1
/**
 *  \brief
 *      2 channels PCM sample player Z80 driver.<br>
 *      It can mix 2 samples (4 bit PCM) at a fixed 22 Khz rate.
 */
#define Z80_DRIVER_DPCM2                2
/**
 *  \brief
 *      4 channels sample player Z80 driver with envelop control.<br>
 *      It can mix 4 samples (8 bit signed) at a fixed 16 Khz rate<br>
 *      and handle volume (16 levels) for each channel.
 */
#define Z80_DRIVER_PCM4                 3
/**
 *  \brief
 *      eXtended VGM music player driver.<br>
 *      This driver takes VGM (or XGM) file as input to play music.<br>
 *      It supports 4 PCM channels at a fixed 14 Khz and allows to play SFX through PCM with 16 level of priority.<br>
 *      The driver is designed to avoid DMA contention when possible (depending CPU load).
 */
#define Z80_DRIVER_XGM                  4
/**
 *  \brief
 *      eXtended VGM music player driver version 2.<br>
 *      This driver takes VGM (or XGM2) file as input to play music.<br>
 *      It supports 3 PCM channels at either 13.3 Khz or 6.65 Khz and envelop control for both FM and PSG.<br>
 *      It allows to play SFX through PCM with 16 level of priority.<br>
 *      The driver supports renforced protection against DMA contention.
 */
#define Z80_DRIVER_XGM2                 5
/**
 *  \brief
 *      CUSTOM Z80 driver.
 */
#define Z80_DRIVER_CUSTOM               -1


#define Z80_DRIVER_4PCM_ENV             _Pragma("GCC error \"This definition is deprecated, use Z80_DRIVER_PCM4 instead.\"")
#define Z80_DRIVER_2ADPCM               _Pragma("GCC error \"This definition is deprecated, use Z80_DRIVER_DPCM2 instead.\"")


/**
 *  \brief
 *      Initialize Z80 sub system.
 *
 *  Request Z80 BUS and reset bank number.
 */
void Z80_init(void);

/**
 *  \brief
 *      Return Z80 BUS taken state.
 */
bool Z80_isBusTaken(void);
/**
 *  \brief
 *      Request Z80 BUS.
 *  \param wait
 *      Wait for BUS request operation to complete.
 */
void Z80_requestBus(bool wait);
/**
 *  \brief
 *      Request Z80 BUS if not yet taken.
 *  \param wait
 *      Wait for BUS request operation to complete.
 *  \return
 *      Z80 BUS taken state before calling the function.
 */
bool Z80_getAndRequestBus(bool wait);

/**
 *  \brief
 *      Release Z80 BUS.
 */
void Z80_releaseBus(void);

/**
 *  \brief
 *      Start Z80 reset.
 */
void Z80_startReset(void);
/**
 *  \brief
 *      End Z80 reset.
 */
void Z80_endReset(void);

/**
 *  \brief
 *      Set Z80 memory bank.
 *  \param bank
 *      Bank number to set (0x000-0x1FF)
 */
void Z80_setBank(const u16 bank);

/**
 *  \brief
 *      Read Z80 memory (Z80_RAM).
 *  \param addr
 *      Address to read (relative to start of Z80_RAM).
 *
 *  You need to request Z80 BUS before accessing Z80 memory.
 *
 *  \see Z80_requestBus(bool)
 */
u8 Z80_read(const u16 addr);
/**
 *  \brief
 *      Write to Z80 memory (Z80_RAM).
 *  \param addr
 *      Address to write (relative to start of Z80_RAM).
 *  \param value
 *      Value to write.
 *
 *  You need to request Z80 BUS before accessing Z80 memory.
 *
 *  \see Z80_requestBus(bool)
 */
void Z80_write(const u16 addr, const u8 value);

/**
 *  \brief
 *      Clear Z80 memory.
 *
 *  You need to request Z80 BUS before accessing Z80 memory.
 *
 *  \see Z80_requestBus(bool)
 */
void Z80_clear(void);
/**
 *  \brief
 *      Upload data in Z80 memory.
 *  \param dest
 *      Destination address (Z80 memory).
 *  \param data
 *      Data to upload.
 *  \param size
 *      Size in byte of data to upload.
 */
void Z80_upload(const u16 dest, const u8 *data, const u16 size);
/**
 *  \brief
 *      Read data from Z80 memory.
 *
 *  \param from
 *      Source address (Z80 memory).
 *  \param dest
 *      Destination where to write data.
 *  \param size
 *      Size in byte of data to read.
 */
void Z80_download(const u16 from, u8 *dest, const u16 size);

/**
 *  \brief
 *      Return currently loaded Z80 driver.
 *
 *  Possible returned values are:<br>
 *  - #Z80_DRIVER_NULL<br>
 *  - #Z80_DRIVER_PCM<br>
 *  - #Z80_DRIVER_DPCM2<br>
 *  - #Z80_DRIVER_PCM4<br>
 *  - #Z80_DRIVER_XGM<br>
 *  - #Z80_DRIVER_XGM2<br>
 *  - #Z80_DRIVER_CUSTOM<br>
 */
s16  Z80_getLoadedDriver(void);
/**
 *  \brief
 *      Unload Z80 driver (set the NULL driver).
 */
void Z80_unloadDriver(void);

/**
 *  \deprecated Use the dedicated loadDriver(..) method (as XGM_loadDriver(..) for instance)
 */
#define Z80_loadDriver(driver, waitReady)   _Pragma("GCC error \"This method is deprecated, directly use the dedicated driver 'loadDriver' method (as XGM_loadDriver(..) for instance).\"")

/**
 *  \brief
 *      Load a custom Z80 driver.
 *
 *  \param drv
 *      Pointer to the driver binary to load.
 *  \param size
 *      Size (in bytes) of the driver binary.
 */
void Z80_loadCustomDriver(const u8 *drv, u16 size);

/**
 *  \brief
 *      Return driver ready state (only for non custom driver).
 */
bool Z80_isDriverReady(void);

/**
 *  \brief
 *      Get the Z80 task 'Vertical interrupt' callback method.
 *
 *  \return the pointer of the method called on Vertical interrupt period or NULL if no method are set
 *
 * \see Z80_setVIntCallback(VoidCallback *CB);
 */
VoidCallback* Z80_getVIntCallback(void);
/**
 *  \brief
 *      Set the Z80 task 'Vertical interrupt' callback method.
 *
 *  \param CB
 *      Pointer to the method to call on Vertical interrupt period.<br>
 *      You can remove current callback by passing a <i>NULL</i> pointer here.
 *
 * Vertical interrupt happen at the end of display period at the start of the vertical blank period.<br>
 * The only things that SGDK always handle from the vint callback is sound driver task as music tempo or Bitmap engine phase reset.<br>
 * It's recommended to keep your code as fast as possible as it will eat precious VBlank time, nor you should touch the VDP from your Vint callback
 * otherwise you will need to protect any VDP accesses from your main loop (which is painful).
 *
 * \see SYS_setVIntCallback(VoidCallback* CB);
 */
void Z80_setVIntCallback(VoidCallback* CB);

/**
 *  \brief
 *      Enable/disable 68K bus access protection from Z80 (can be used by any sound driver).
 *
 *  \param signalAddress
 *      Z80 RAM address used (relative to the start of Z80 RAM) to set the BUS protection signal.<br>
 *      Signal is set to 1 when main BUS should not be accesssed from Z80 (DMA operation in progess), set to 0 otherwise.
 */
void Z80_useBusProtection(u16 signalAddress);

/**
 *  \brief
 *      Set temporary 68K BUS protection from Z80 (for sound driver supporting it).<br>
 *      You should protect BUS Access during DMA and restore it after. Ex:<br>
 *      Z80_setBusProtection(TRUE);
 *      DMA_doDma(VRAM, data, 0x1000, 0x100, 2);
 *      Z80_setBusProtection(FALSE);
 *
 *      This way the sound driver will *try* to avoid accessing the 68K BUS during DMA to
 *      avoid execution interruption and so preserve PCM playback quality.<br>
 *      Note that depending the sound driver, the success of the operation is not 100% garantee and can fails in some conditions
 *      (heavy Z80 load, lot of PSG data in XGM music).<br>
 *      In that case you can also try to use the #Z80_setForceDelayDMA() method to help improving the PCM playblack.
 *
 *  \see Z80_useBusProtection(..)
 *  \see Z80_enableBusProtection(..)
 *  \see Z80_disableBusProtection(..)
 */
void Z80_setBusProtection(bool value);
/**
 *  \brief
 *      Enable temporary 68K BUS protection from Z80 (for sound driver supporting it). See #Z80_setBusProtection(..) for more info.
 *
 *  \see Z80_setBusProtection(..)
 *  \see Z80_enableBusProtection(..)
 *  \see Z80_disableBusProtection(..)
 */
void Z80_enableBusProtection();
/**
 *  \brief
 *      Disable temporary 68K BUS protection from Z80 (for sound driver supporting it). See #Z80_setBusProtection(..) for more info.
 *
 *  \see Z80_setBusProtection(..)
 *  \see Z80_enableBusProtection(..)
 *  \see Z80_disableBusProtection(..)
 */
void Z80_disableBusProtection();

/**
 *  \brief
 *      Returns #TRUE if DMA delay is enabled to improve PCM playback.
 *
 *  \see Z80_setForceDelayDMA(bool)
 */
bool Z80_getForceDelayDMA(void);
/**
 *  \brief
 *      This method can be used to improve the PCM playback during music play and while DMA queue is used.<br>
 *      Even using the BUS protection with #Z80_setBusProtection you may still experience altered PCM playback.
 *      With the XGM driver for instance this happens when music contains PSG data (Z80 requires the main BUS to access PSG).<br>
 *      By delaying a bit the DMA execution from the DMA queue we let the Z80 to access main bus for a bit of time thus avoiding any stall.
 *      The delay is about 3 scanlines so using the force delay DMA will reduce the DMA bandwidth for about 3 vblank lines.
 *
 *  \param value TRUE or FALSE
 *
 *  \see Z80_getForceDelayDMA()
 *  \see Z80_setBusProtection()
 */
void Z80_setForceDelayDMA(bool value);

#endif // _Z80_CTRL_H_
