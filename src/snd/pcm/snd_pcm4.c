#include "config.h"
#include "types.h"

#include "z80_ctrl.h"

#include "snd/sound.h"
#include "snd/pcm/snd_pcm4.h"
#include "src/snd/pcm/drv_pcm4.h"
#include "snd/pcm/tab_vol.h"
#include "snd/smp_null.h"

#include "timer.h"
#include "vdp.h"
#include "sys.h"


// we don't want to share them
extern s16 currentDriver;
extern void Z80_loadDriverInternal(const u8 *drv, u16 size);

// Z80_DRIVER_PCM4
// 4 channels 8 bits signed sample driver with volume support
///////////////////////////////////////////////////////////////

NO_INLINE void SND_PCM4_loadDriver(const bool waitReady)
{
    // already loaded
    if (currentDriver == Z80_DRIVER_PCM4) return;

    Z80_loadDriverInternal(drv_pcm4, sizeof(drv_pcm4));

    SYS_disableInts();

    // load volume table
    Z80_upload(0x1000, tab_vol, 0x1000);

    // misc parameters initialisation
    Z80_requestBus(TRUE);
    // point to Z80 null sample parameters
    vu8 *pb = (u8 *) (Z80_DRV_PARAMS + 0x20);

    u32 addr = (u32) smp_null;
    // null sample address (256 bytes aligned)
    pb[4] = addr >> 8;
    pb[5] = addr >> 16;
    // null sample length (256 bytes aligned)
    pb[6] = sizeof(smp_null) >> 8;
    pb[7] = sizeof(smp_null) >> 16;
    Z80_releaseBus();

    // wait driver for being ready
    if (waitReady)
    {
        // just wait for it (the function does request Z80 bus if needed)
        while(!Z80_isDriverReady())
            waitMs(1);
    }

    SYS_enableInts();

    // driver loaded
    currentDriver = Z80_DRIVER_PCM4;
}

NO_INLINE void SND_PCM4_unloadDriver(void)
{
    // nothing to do here
}


NO_INLINE bool SND_PCM4_isPlaying(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // load the appropriate driver if not already done
    SND_PCM4_loadDriver(TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    return ret;
}

NO_INLINE void SND_PCM4_startPlay(const u8 *sample, const u32 len, const SoundPCMChannel channel, const bool loop)
{
    vu8 *pb;
    u8 status;
    u16 ch;
    u32 addr;

    // load the appropriate driver if not already done
    SND_PCM4_loadDriver(TRUE);

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

        while ((ch < 4) && (status & (Z80_DRV_STAT_PLAYING << ch))) ch++;

        // if all channel busy we use the first
        if (ch == 4) ch = 0;
    }
    // we use specified channel
    else ch = channel;

    // point to Z80 base parameters
    pb = (u8 *) (Z80_DRV_PARAMS + (ch * 4));

    addr = (u32) sample;
    // sample address (256 bytes aligned)
    pb[0] = addr >> 8;
    pb[1] = addr >> 16;
    // sample length (256 bytes aligned)
    pb[2] = len >> 8;
    pb[3] = len >> 16;

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

NO_INLINE void SND_PCM4_stopPlay(const SoundPCMChannel channel)
{
    vu8 *pb;
    u32 addr;

    // load the appropriate driver if not already done
    SND_PCM4_loadDriver(TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 internal parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x10 + (channel * 4));

    addr = (u32) smp_null;
    // sample address (256 bytes aligned)
    pb[0] = addr >> 8;
    pb[1] = addr >> 16;
    // sample length (256 bytes aligned)
    pb[2] = sizeof(smp_null) >> 8;
    pb[3] = sizeof(smp_null) >> 16;

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // remove play and loop status
    pb[0] &= ~(Z80_DRV_STAT_PLAYING << channel);
    pb[1] &= ~(Z80_DRV_STAT_PLAYING << channel);

    Z80_releaseBus();
}

NO_INLINE void SND_PCM4_setVolume(const SoundPCMChannel channel, const u8 volume)
{
    vu8 *pb;

    // load the appropriate driver if not already done
    SND_PCM4_loadDriver(TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 volume parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x20) + channel;
    // set volume (16 levels)
    *pb = volume & 0x0F;

    Z80_releaseBus();
}

NO_INLINE u8 SND_PCM4_getVolume(const SoundPCMChannel channel)
{
    vu8 *pb;
    u8 volume;

    // load the appropriate driver if not already done
    SND_PCM4_loadDriver(TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 volume parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x20) + channel;
    // get volume (16 levels)
    volume = *pb & 0x0F;

    Z80_releaseBus();

    return volume;
}
