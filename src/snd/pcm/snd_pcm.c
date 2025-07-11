#include "config.h"
#include "types.h"

#include "z80_ctrl.h"
#include "snd/sound.h"
#include "snd/pcm/snd_pcm.h"
#include "src/snd/pcm/drv_pcm.h"
#include "snd/smp_null.h"

#include "timer.h"
#include "vdp.h"
#include "sys.h"


// we don't want to share them
extern s16 currentDriver;
extern void Z80_loadDriverInternal(const u8 *drv, u16 size);

// Z80_DRIVER_PCM
// single channel 8 bits signed sample driver
///////////////////////////////////////////////////////////////

NO_INLINE void SND_PCM_loadDriver(const bool waitReady)
{
    // already loaded
    if (currentDriver == Z80_DRIVER_PCM) return;

    Z80_loadDriverInternal(drv_pcm, sizeof(drv_pcm));

    SYS_disableInts();

    // misc parameters initialisation
    Z80_requestBus(TRUE);
    // point to Z80 null sample parameters
    vu8 *pb = (u8*) (Z80_DRV_PARAMS + 0x20);

    u32 addr = (u32) smp_null;
    // null sample address (256 bytes aligned)
    pb[0] = addr >> 8;
    pb[1] = addr >> 16;
    // null sample length (256 bytes aligned)
    pb[2] = sizeof(smp_null) >> 8;
    pb[3] = sizeof(smp_null) >> 16;
    Z80_releaseBus();

    // wait driver for being ready
    if (waitReady)
    {
        while(!Z80_isDriverReady())
            waitMs(1);
    }

    SYS_enableInts();

    // driver loaded
    currentDriver = Z80_DRIVER_PCM;
}

NO_INLINE void SND_PCM_unloadDriver(void)
{
    // nothing to do here
}

NO_INLINE bool SND_PCM_isPlaying(void)
{
    vu8 *pb;
    u8 ret;

    // load the appropriate driver if not already done
    SND_PCM_loadDriver(TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & Z80_DRV_STAT_PLAYING;

    Z80_releaseBus();

    return ret;
}

NO_INLINE void SND_PCM_startPlay(const u8 *sample, const u32 len, const SoundPcmSampleRate rate, const SoundPanning pan, const u8 loop)
{
    vu8 *pb;
    u32 addr;

    // load the appropriate driver if not already done
    SND_PCM_loadDriver(TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 base parameters
    pb = (u8 *) Z80_DRV_PARAMS;

    addr = (u32) sample;
    // sample address (256 bytes aligned)
    pb[0] = addr >> 8;
    pb[1] = addr >> 16;
    // sample length (256 bytes aligned)
    pb[2] = len >> 8;
    pb[3] = len >> 16;
    // rate
    pb[4] = rate;
    // pan (left / right / center)
    pb[6] = pan;

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

NO_INLINE void SND_PCM_stopPlay(void)
{
    vu8 *pb;
    u32 addr;

    // load the appropriate driver if not already done
    SND_PCM_loadDriver(TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 internal parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x10);

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
    pb[0] &= ~Z80_DRV_STAT_PLAYING;
    pb[1] &= ~Z80_DRV_STAT_PLAYING;

    Z80_releaseBus();
}
