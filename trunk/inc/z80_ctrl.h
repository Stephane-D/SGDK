/**
 *  \file z80_ctrl.h
 *  \brief Z80 control
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides Z80 access from the YM2612 :<br/>
 * - enable / disable Z80<br/>
 * - request / release Z80 BUS<br/>
 * - upload / download data to / from Z80 memory<br/>
 * - set Z80 external Bank<br/>
 * - Z80 driver handling<br/>
 */

#ifndef _Z80_CTRL_H_
#define _Z80_CTRL_H_


#define Z80_HALT_PORT                   0xA11100
#define Z80_RESET_PORT                  0xA11200

/**
 *  \def Z80_RAM_START
 *
 * Z80 RAM start address.
 */
#define Z80_RAM_START                   0xA00000
/**
 *  \def Z80_RAM_END
 *
 * Z80 RAM end address.
 */
#define Z80_RAM_END                     0xA01FFF
/**
 *  \def Z80_RAM
 *
 * Z80 RAM address.
 */
#define Z80_RAM                         Z80_RAM_START
/**
 *  \def Z80_RAM_LEN
 *
 * Z80 RAM length in byte.
 */
#define Z80_RAM_LEN                     ((Z80_RAM_END - Z80_RAM_START) + 1)
/**
 *  \def Z80_YM2612
 *
 * Z80 YM2612 port address.
 */
#define Z80_YM2612                      0xA04000
/**
 *  \def Z80_BANK_REGISTER
 *
 * Z80 Bank register address.
 */
#define Z80_BANK_REGISTER               0xA06000

/**
 *  \def Z80_DRV_COMMAND
 *
 * Z80 default driver command address.
 */
#define Z80_DRV_COMMAND                 0xA00100
/**
 *  \def Z80_DRV_STATUS
 *
 * Z80 default driver status address.
 */
#define Z80_DRV_STATUS                  0xA00102
/**
 *  \def Z80_DRV_PARAMS
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
 *  \def Z80_DRV_COM_PLAY
 *
 * Z80 default driver play command.
 */
#define Z80_DRV_COM_PLAY                (1 << Z80_DRV_COM_PLAY_SFT)
/**
 *  \def Z80_DRV_COM_STOP
 *
 * Z80 default driver stop command.
 */
#define Z80_DRV_COM_STOP                (1 << Z80_DRV_COM_STOP_SFT)
/**
 *  \def Z80_DRV_STAT_PLAYING
 *
 * Z80 default driver play status.
 */
#define Z80_DRV_STAT_PLAYING            (1 << Z80_DRV_STAT_PLAYING_SFT)
/**
 *  \def Z80_DRV_STAT_READY
 *
 * Z80 default driver ready status.
 */
#define Z80_DRV_STAT_READY              (1 << Z80_DRV_STAT_READY_SFT)

// channel definition
#define Z80_DRV_CH0_SFT                 0
#define Z80_DRV_CH1_SFT                 1
#define Z80_DRV_CH2_SFT                 2
#define Z80_DRV_CH3_SFT                 3

/**
 *  \def Z80_DRV_CH0
 *
 * Z80 default driver channel 0 id.
 */
#define Z80_DRV_CH0                     (1 << Z80_DRV_CH0_SFT)
/**
 *  \def Z80_DRV_CH1
 *
 * Z80 default driver channel 1 id.
 */
#define Z80_DRV_CH1                     (1 << Z80_DRV_CH1_SFT)
/**
 *  \def Z80_DRV_CH2
 *
 * Z80 default driver channel 2 id.
 */
#define Z80_DRV_CH2                     (1 << Z80_DRV_CH2_SFT)
/**
 *  \def Z80_DRV_CH3
 *
 * Z80 default driver channel 3 id.
 */
#define Z80_DRV_CH3                     (1 << Z80_DRV_CH3_SFT)


/**
 *  \def Z80_DRIVER_NULL
 *
 * NULL Z80 driver.
 */
#define Z80_DRIVER_NULL                 0
/**
 *  \def Z80_DRIVER_PCM
 *
 *  Variable rate sample player Z80 driver.<br/>
 *  It can play a sample (8 bit signed) from 8 Khz up to 32 Khz rate.
 */
#define Z80_DRIVER_PCM                  1
/**
 *  \def Z80_DRIVER_2ADPCM
 *
 *  2 channels PCM sample player Z80 driver.<br/>
 *  It can mix 2 samples (4 bit PCM) at a fixed 22 Khz rate.
 */
#define Z80_DRIVER_2ADPCM               2
/**
 *  \def Z80_DRIVER_4PCM
 *
 *  4 channels sample player Z80 driver.<br/>
 *  It can mix 4 samples (8 bit signed) at a fixed 16 Khz rate.
 */
#define Z80_DRIVER_4PCM                 3
/**
 *  \def Z80_DRIVER_4PCM_ENV
 *
 *  4 channels sample player Z80 driver with envelop control.<br/>
 *  It can mix 4 samples (8 bit signed) at a fixed 16 Khz rate<br/>
 *  and handle volume (16 levels) for each channel.
 */
#define Z80_DRIVER_4PCM_ENV             4
/**
 *  \def Z80_DRIVER_MVS
 *
 *  MVS tracker Z80 driver (updated to last version with PCM and PSG support).
 */
#define Z80_DRIVER_MVS                  5
/**
 *  \def Z80_DRIVER_TFM
 *
 *  TFM tracker Z80 driver.
 */
#define Z80_DRIVER_TFM                  6
/**
 *  \def Z80_DRIVER_VGM
 *
 *  VGM Z80 driver.
 */
#define Z80_DRIVER_VGM                  7
/**
 *  \def Z80_DRIVER_CUSTOM
 *
 * CUSTOM Z80 driver.
 */
#define Z80_DRIVER_CUSTOM               -1

#define Z80_DRIVER_DEFAULT              Z80_DRIVER_PCM


/**
 *  \brief
 *      Initialize Z80 sub system.
 *
 *  Request Z80 BUS and reset bank number.
 */
void Z80_init();

/**
 *  \brief
 *      Return Z80 BUS taken state.
 */
u16  Z80_isBusTaken();
/**
 *  \brief
 *      Request Z80 BUS.
 *  \param wait
 *      Wait for BUS request operation to complete.
 */
void Z80_requestBus(u16 wait);
/**
 *  \brief
 *      Release Z80 BUS.
 */
void Z80_releaseBus();

/**
 *  \brief
 *      Start Z80 reset.
 */
void Z80_startReset();
/**
 *  \brief
 *      End Z80 reset.
 */
void Z80_endReset();

/**
 *  \brief
 *      Set Z80 memory bank.
 *  \param bank
 *      Bank number to set (0x000-0x1FF)
 */
void Z80_setBank(const u16 bank);

/**
 *  \brief
 *      Clear Z80 memory.
 *  \param dest
 *      Destination address (Z80 memory).
 *  \param size
 *      Size in byte of region to clear.
 *  \param resetz80
 *      Reset Z80 if set to TRUE.
 */
void Z80_clear(const u16 dest, const u16 size, const u16 resetz80);
/**
 *  \brief
 *      Upload data in Z80 memory.
 *  \param dest
 *      Destination address (Z80 memory).
 *  \param data
 *      Data to upload.
 *  \param size
 *      Size in byte of data to upload.
 *  \param resetz80
 *      Reset Z80 if set to TRUE.
 */
void Z80_upload(const u16 dest, const u8 *data, const u16 size, const u16 resetz80);
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
 *  Possible returned values are:<br/>
 *  - Z80_DRIVER_NULL<br/>
 *  - Z80_DRIVER_PCM<br/>
 *  - Z80_DRIVER_2ADPCM<br/>
 *  - Z80_DRIVER_4PCM<br/>
 *  - Z80_DRIVER_4PCM_ENV<br/>
 *  - Z80_DRIVER_MVS<br/>
 *  - Z80_DRIVER_TFM<br/>
 *  - Z80_DRIVER_CUSTOM<br/>
 */
u16  Z80_getLoadedDriver();
/**
 *  \brief
 *      Unload Z80 driver (actually clear Z80 ram).
 */
void Z80_unloadDriver();
/**
 *  \brief
 *      Load a Z80 driver.
 *
 *  \param driver
 *      Driver to load, possible values are:<br/>
 *      - Z80_DRIVER_NULL<br/>
 *      - Z80_DRIVER_PCM<br/>
 *      - Z80_DRIVER_2ADPCM<br/>
 *      - Z80_DRIVER_4PCM<br/>
 *      - Z80_DRIVER_4PCM_ENV<br/>
 *      - Z80_DRIVER_MVS<br/>
 *      - Z80_DRIVER_TFM<br/>
 *  \param waitReady
 *      Wait for driver to be ready.
 */
void Z80_loadDriver(const u16 driver, const u16 waitReady);
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
u16  Z80_isDriverReady();


#endif // _Z80_CTRL_H_
