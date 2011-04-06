#ifndef _Z80_CTRL_H_
#define _Z80_CTRL_H_


#define Z80_HALT_PORT                   0xA11100
#define Z80_RESET_PORT                  0xA11200

#define Z80_RAM                         0xA00000
#define Z80_YM2612                      0xA04000
#define Z80_BANK_REGISTER               0xA06000

// default command, status and parameter address
#define Z80_DRV_COMMAND                 0xA00100
#define Z80_DRV_STATUS                  0xA00102
#define Z80_DRV_PARAMS                  0xA00104

// default command and status value
#define Z80_DRV_COM_PLAY_SFT            0
#define Z80_DRV_COM_STOP_SFT            4
#define Z80_DRV_STAT_PLAYING_SFT        0
#define Z80_DRV_STAT_READY_SFT          7

#define Z80_DRV_COM_PLAY                (1 << Z80_DRV_COM_PLAY_SFT)
#define Z80_DRV_COM_STOP                (1 << Z80_DRV_COM_STOP_SFT)
#define Z80_DRV_STAT_PLAYING            (1 << Z80_DRV_STAT_PLAYING_SFT)
#define Z80_DRV_STAT_READY              (1 << Z80_DRV_STAT_READY_SFT)

// channel definition
#define Z80_DRV_CH0_SFT                 0
#define Z80_DRV_CH1_SFT                 1
#define Z80_DRV_CH2_SFT                 2
#define Z80_DRV_CH3_SFT                 3

#define Z80_DRV_CH0                     (1 << Z80_DRV_CH0_SFT)
#define Z80_DRV_CH1                     (1 << Z80_DRV_CH1_SFT)
#define Z80_DRV_CH2                     (1 << Z80_DRV_CH2_SFT)
#define Z80_DRV_CH3                     (1 << Z80_DRV_CH3_SFT)


#define Z80_DRIVER_NULL                 0
// variable rate sample player Z80 driver
// it can play a sample (8 bit unsigned) from 4 Khz up to 52 Khz rate
#define Z80_DRIVER_PCM                  1
// 2 channels PCM sample player Z80 driver
// it can mix 2 samples (4 bit PCM) at a fixed 22 Khz rate.
#define Z80_DRIVER_2ADPCM               2
// 4 channels sample player Z80 driver
// it can mix 4 samples (8 bit signed) at a fixed 16 Khz rate.
#define Z80_DRIVER_4PCM                 3
// 4 channels sample player Z80 driver with envelop control
// it can mix 4 samples (8 bit signed) at a fixed 16 Khz rate
// and handle volume (16 levels) for each channel
#define Z80_DRIVER_4PCM_ENV             4
// MVS tracker Z80 driver
#define Z80_DRIVER_MVS                  5
// TFM tracker Z80 driver
#define Z80_DRIVER_TFM                  6

#define Z80_DRIVER_DEFAULT              Z80_DRIVER_PCM


void Z80_init();

u16  Z80_isBusTaken();
void Z80_requestBus(u16 wait);
void Z80_releaseBus();

void Z80_startReset();
void Z80_endReset();

void Z80_setBank(const u16 bank);

void Z80_upload(const u16 dest, const u8 *data, const u16 size, const u16 resetz80);
void Z80_download(const u16 from, u8 *dest, const u16 size);

u16  Z80_getLoadedDriver();
void Z80_loadDriver(const u16 driver, const u16 waitReady);

u16  Z80_isDriverReady();


#endif // _Z80_CTRL_H_
