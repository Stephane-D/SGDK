#include "genesis.h"
#include "smp_null_pcm.h"

#include "pcm.h"
#include "z80_drv.h"


void loadDriver()
{
    vu8 *pb;
    u32 addr;

    // upload Z80 driver and reset Z80
    Z80_upload(0, z80_drv, sizeof(z80_drv), TRUE);

    // misc parameters initialisation
    Z80_requestBus(TRUE);

    // point to Z80 null sample parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x20);

    addr = (u32) smp_null_pcm;
    // null sample address (128 bytes aligned)
    pb[0] = addr >> 7;
    pb[1] = addr >> 15;
    // null sample length (128 bytes aligned)
    pb[2] = sizeof(smp_null_pcm) >> 7;
    pb[3] = sizeof(smp_null_pcm) >> 15;

    Z80_releaseBus();
    // wait bus released
    while(Z80_isBusTaken());

    // just wait for driver to be ready
    while(!Z80_isDriverReady())
        while(Z80_isBusTaken());
}

//u8 isPlaying()
//{
//    vu8 *pb;
//    u8 ret;
//
//    Z80_requestBus(TRUE);
//
//    // point to Z80 status
//    pb = (u8 *) Z80_DRV_STATUS;
//    // play status
//    ret = *pb & (SOUND_PCM_CH1_MSK << Z80_DRV_STAT_PLAYING_SFT);
//
//    Z80_releaseBus();
//
//    return ret;
//}

void startPlay(const u8 *sample, const u32 len, const u8 loop)
{
    vu8 *pb;
    u8 status;
    u32 addr;

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // get status
    status = *pb;

    // point to Z80 base parameters
    pb = (u8 *) Z80_DRV_PARAMS;

    addr = (u32) sample;
    // sample address (128 bytes aligned)
    pb[0] = addr >> 7;
    pb[1] = addr >> 15;
    // sample length (128 bytes aligned)
    pb[2] = len >> 7;
    pb[3] = len >> 15;

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // play command
    *pb |= Z80_DRV_COM_PLAY;

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;

    // loop flag in status
    if (loop) pb[1] |= Z80_DRV_STAT_PLAYING;
    else pb[1] &= ~Z80_DRV_STAT_PLAYING;

    Z80_releaseBus();
}

void stopPlay()
{
    vu8 *pb;
    u32 addr;

    Z80_requestBus(TRUE);

    // point to Z80 internal parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x10);

    addr = (u32) smp_null_pcm;
    // sample address (128 bytes aligned)
    pb[0] = addr >> 7;
    pb[1] = addr >> 15;
    // sample length (128 bytes aligned)
    pb[2] = sizeof(smp_null_pcm) >> 7;
    pb[3] = sizeof(smp_null_pcm) >> 15;

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // remove play and loop status
    pb[0] &= ~Z80_DRV_STAT_PLAYING;
    pb[1] &= ~Z80_DRV_STAT_PLAYING;

    Z80_releaseBus();
}
