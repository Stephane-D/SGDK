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


#include "snd/z80_driver.h"


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
 *      CUSTOM Z80 driver.
 */
#define Z80_DRIVER_CUSTOM               _Pragma("GCC error \"This definition is deprecated, use a Z80Driver struct instead.\"")


#define Z80_DRIVER_4PCM_ENV             _Pragma("GCC error \"This definition is deprecated, use Z80_DRIVER_PCM4 instead.\"")
#define Z80_DRIVER_2ADPCM               _Pragma("GCC error \"This definition is deprecated, use Z80_DRIVER_DPCM2 instead.\"")


#define Z80_loadCustomDriver(drv, size) _Pragma("GCC error \"This method is deprecated, use a Z80Driver struct instead.\"")


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
 *      Return currently loaded Z80 driver.<br>
 *      Useful to check the identity of the loaded driver.
 */
const Z80Driver* Z80_getLoadedDriver(void);
/**
 *  \brief
 *      Unload Z80 driver (set NULL driver).
 */
void Z80_unloadDriver(void);
/**
 *  \brief
 *      Load a Z80 driver.<br>
 *      If the requested driver is already loaded, the function returns immediately.<br>
 *      Otherwise, the current driver is unloaded before loading the new one.
 *
 *  \param driver
 *      Pointer to the Z80Driver to load, or NULL to load the null driver.
 *  \param waitReady
 *      Wait for driver to be ready.
 */
void Z80_loadDriver(const Z80Driver* driver, const bool waitReady);

/**
 *  \brief
 *      Return driver ready state.
 */
bool Z80_isDriverReady(void);

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
