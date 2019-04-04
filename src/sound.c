#include "config.h"
#include "types.h"

#include "sound.h"

#include "z80_ctrl.h"
#include "z80_mvsc.h"
#include "smp_null.h"
#include "smp_null_pcm.h"
#include "ym2612.h"
#include "psg.h"
#include "timer.h"
#include "vdp.h"
#include "sys.h"
#include "xgm.h"


// Z80_DRIVER_PCM
// single channel 8 bits signed sample driver
///////////////////////////////////////////////////////////////

u8 SND_isPlaying_PCM()
{
    vu8 *pb;
    u8 ret;

    // disable ints when requesting Z80 bus
    SYS_disableInts();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_PCM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & Z80_DRV_STAT_PLAYING;

    Z80_releaseBus();

    // re-enable ints
    SYS_enableInts();

    return ret;
}

void SND_startPlay_PCM(const u8 *sample, const u32 len, const u8 rate, const u8 pan, const u8 loop)
{
    vu8 *pb;
    u32 addr;

    // disable ints when requesting Z80 bus
    SYS_disableInts();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_PCM, TRUE);

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

    // re-enable ints
    SYS_enableInts();
}

void SND_stopPlay_PCM()
{
    vu8 *pb;
    u32 addr;

    // disable ints when requesting Z80 bus
    SYS_disableInts();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_PCM, TRUE);

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

    // re-enable ints
    SYS_enableInts();
}


// Z80_DRIVER_2ADPCM
// 2 channels 4 bits ADPCM sample driver
///////////////////////////////////////////////////////////////

u8 SND_isPlaying_2ADPCM(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // disable ints when requesting Z80 bus
    SYS_disableInts();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_2ADPCM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    // re-enable ints
    SYS_enableInts();

    return ret;
}

void SND_startPlay_2ADPCM(const u8 *sample, const u32 len, const u16 channel, const u8 loop)
{
    vu8 *pb;
    u8 status;
    u16 ch;
    u32 addr;

    // disable ints when requesting Z80 bus
    SYS_disableInts();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_2ADPCM, TRUE);

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

    // re-enable ints
    SYS_enableInts();
}

void SND_stopPlay_2ADPCM(const u16 channel)
{
    vu8 *pb;
    u32 addr;

    // disable ints when requesting Z80 bus
    SYS_disableInts();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_2ADPCM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 internal parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x10 + (channel * 4));

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
    pb[0] &= ~(Z80_DRV_STAT_PLAYING << channel);
    pb[1] &= ~(Z80_DRV_STAT_PLAYING << channel);

    Z80_releaseBus();

    // re-enable ints
    SYS_enableInts();
}


// Z80_DRIVER_4PCM
// 4 channels 8 bits signed sample driver with volume support
///////////////////////////////////////////////////////////////

u8 SND_isPlaying_4PCM(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // disable ints when requesting Z80 bus
    SYS_disableInts();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    // re-enable ints
    SYS_enableInts();

    return ret;
}

void SND_startPlay_4PCM(const u8 *sample, const u32 len, const u16 channel, const u8 loop)
{
    vu8 *pb;
    u8 status;
    u16 ch;
    u32 addr;

    // disable ints when requesting Z80 bus
    SYS_disableInts();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM, TRUE);

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

    // re-enable ints
    SYS_enableInts();
}

void SND_stopPlay_4PCM(const u16 channel)
{
    vu8 *pb;
    u32 addr;

    // disable ints when requesting Z80 bus
    SYS_disableInts();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM, TRUE);

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

    // re-enable ints
    SYS_enableInts();
}

void SND_setVolume_4PCM(const u16 channel, const u8 volume)
{
    vu8 *pb;

    // disable ints when requesting Z80 bus
    SYS_disableInts();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 volume parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x20) + channel;
    // set volume (16 levels)
    *pb = volume & 0x0F;

    Z80_releaseBus();

    // re-enable ints
    SYS_enableInts();
}

u8 SND_getVolume_4PCM(const u16 channel)
{
    vu8 *pb;
    u8 volume;

    // disable ints when requesting Z80 bus
    SYS_disableInts();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 volume parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x20) + channel;
    // get volume (16 levels)
    volume = *pb & 0x0F;

    Z80_releaseBus();

    // re-enable ints
    SYS_enableInts();

    return volume;
}


// Z80_DRIVER_XGM
// XGM driver
///////////////////////////////////////////////////////////////

u8 SND_isPlaying_XGM()
{
    return XGM_isPlaying();
}

void SND_startPlay_XGM(const u8 *song)
{
    XGM_startPlay(song);
}

void SND_stopPlay_XGM()
{
    XGM_stopPlay();
}

void SND_pausePlay_XGM()
{
    XGM_pausePlay();
}

void SND_resumePlay_XGM()
{
    XGM_resumePlay();
}

u8 SND_isPlayingPCM_XGM(const u16 channel_mask)
{
    return XGM_isPlayingPCM(channel_mask);
}

void SND_setPCM_XGM(const u8 id, const u8 *sample, const u32 len)
{
    XGM_setPCM(id, sample, len);
}

void SND_setPCMFast_XGM(const u8 id, const u8 *sample, const u32 len)
{
    XGM_setPCMFast(id, sample, len);
}

void SND_startPlayPCM_XGM(const u8 id, const u8 priority, const u16 channel)
{
    XGM_startPlayPCM(id, priority, channel);
}

void SND_stopPlayPCM_XGM(const u16 channel)
{
    XGM_stopPlayPCM(channel);
}

void SND_setLoopNumber_XGM(u8 value)
{
    XGM_setLoopNumber(value);
}

void SND_set68KBUSProtection_XGM(u8 value)
{
    XGM_set68KBUSProtection(value);
}

void SND_nextXFrame_XGM(u16 num)
{
    XGM_nextXFrame(num);
}

u16 SND_getManualSync_XGM()
{
    return XGM_getManualSync();
}

void SND_setManualSync_XGM(u16 value)
{
    XGM_setManualSync(value);
}

u16 SND_getForceDelayDMA_XGM()
{
    return XGM_getForceDelayDMA();
}

void SND_setForceDelayDMA_XGM(u16 value)
{
    XGM_setForceDelayDMA(value);
}

u16 SND_getMusicTempo_XGM()
{
    return XGM_getMusicTempo();
}

void SND_setMusicTempo_XGM(u16 value)
{
    XGM_setMusicTempo(value);
}

u32 SND_getCPULoad_XGM()
{
    return XGM_getCPULoad();
}
