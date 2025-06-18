#include "config.h"
#include "types.h"

#include "z80_ctrl.h"

#include "snd/sound.h"
#include "snd/pcm/snd_dpcm2.h"
#include "src/snd/pcm/drv_dpcm2.h"
#include "snd/smp_null_dpcm.h"

#include "timer.h"
#include "vdp.h"
#include "sys.h"


// we don't want to share it
extern void Z80_loadDriverInternal(const u8 *drv, u16 size);

// Z80_DRIVER_DPCM2
// 2 channels 4 bits DPCM sample driver
///////////////////////////////////////////////////////////////

NO_INLINE void SND_DPCM2_loadDriver(const bool waitReady)
{
    Z80_loadDriverInternal(drv_dpcm2, sizeof(drv_dpcm2));

    SYS_disableInts();

    // misc parameters initialisation
    Z80_requestBus(TRUE);
    // point to Z80 null sample parameters
    vu8 *pb = (u8 *) (Z80_DRV_PARAMS + 0x20);

    u32 addr = (u32) smp_null_dpcm;
    // null sample address (128 bytes aligned)
    pb[0] = addr >> 7;
    pb[1] = addr >> 15;
    // null sample length (128 bytes aligned)
    pb[2] = sizeof(smp_null_dpcm) >> 7;
    pb[3] = sizeof(smp_null_dpcm) >> 15;
    Z80_releaseBus();

    // wait driver for being ready
    if (waitReady)
    {
        while(!Z80_isDriverReady())
            waitMs(1);
    }

    SYS_enableInts();
}

NO_INLINE void SND_DPCM2_unloadDriver(void)
{
    // nothing to do here
}


NO_INLINE bool SND_DPCM2_isPlaying(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_DPCM2, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    return ret;
}

NO_INLINE void SND_DPCM2_startPlay(const u8 *sample, const u32 len, const SoundPCMChannel channel, const bool loop)
{
    vu8 *pb;
    u8 status;
    u16 ch;
    u32 addr;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_DPCM2, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // get status
    status = *pb;

    // auto channel ?
    if (channel == SOUND_PCM_CH_AUTO)
    {
        // scan for first free channel
        ch = 0;

        while ((ch < 2) && (status & (Z80_DRV_STAT_PLAYING << ch))) ch++;

        // if all channel busy we use the first
        if (ch == 2) ch = 0;
    }
    // we use specified channel
    else ch = channel;

    // point to Z80 base parameters
    pb = (u8 *) (Z80_DRV_PARAMS + (ch * 4));

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
    *pb |= (Z80_DRV_COM_PLAY << ch);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;

    // loop flag in status
    if (loop) pb[1] |= (Z80_DRV_STAT_PLAYING << ch);
    else pb[1] &= ~(Z80_DRV_STAT_PLAYING << ch);

    Z80_releaseBus();
}

NO_INLINE void SND_DPCM2_stopPlay(const SoundPCMChannel channel)
{
    vu8 *pb;
    u32 addr;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_DPCM2, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 internal parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x10 + (channel * 4));

    addr = (u32) smp_null_dpcm;
    // sample address (128 bytes aligned)
    pb[0] = addr >> 7;
    pb[1] = addr >> 15;
    // sample length (128 bytes aligned)
    pb[2] = sizeof(smp_null_dpcm) >> 7;
    pb[3] = sizeof(smp_null_dpcm) >> 15;

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // remove play and loop status
    pb[0] &= ~(Z80_DRV_STAT_PLAYING << channel);
    pb[1] &= ~(Z80_DRV_STAT_PLAYING << channel);

    Z80_releaseBus();
}

