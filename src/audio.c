#include "config.h"
#include "types.h"

#include "audio.h"

#include "z80_ctrl.h"
#include "smp_null.h"
#include "smp_null_pcm.h"


// Z80_DRIVER_PCM
// single channel 8 bits signed sample driver
///////////////////////////////////////////////////////////////
u8 isPlaying_PCM()
{
    vu8 *pb;
    u8 ret;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_PCM, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & Z80_DRV_STAT_PLAYING;

    Z80_releaseBus();

    return ret;
}

void startPlay_PCM(const u8 *sample, const u32 len, const u8 rate, const u8 pan, const u8 loop)
{
    vu8 *pb;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_PCM, 1);

    Z80_requestBus(1);

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

void stopPlay_PCM()
{
    vu8 *pb;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_PCM, 1);

    Z80_requestBus(1);

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


// Z80_DRIVER_2ADPCM
// 2 channels 4 bits ADPCM sample driver
///////////////////////////////////////////////////////////////
u8 isPlaying_2ADPCM(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_2ADPCM, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    return ret;
}

void startPlay_2ADPCM(const u8 *sample, const u32 len, const u16 channel, const u8 loop)
{
    vu8 *pb;
    u8 status;
    u16 ch;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_2ADPCM, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // get status
    status = *pb;

    // auto channel ?
    if (channel == AUDIO_PCM_CH_AUTO)
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

void stopPlay_2ADPCM(const u16 channel)
{
    vu8 *pb;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_2ADPCM, 1);

    Z80_requestBus(1);

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
}


// Z80_DRIVER_4PCM
// 4 channels 8 bits signed sample driver
///////////////////////////////////////////////////////////////
u8 isPlaying_4PCM(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    return ret;
}

void startPlay_4PCM(const u8 *sample, const u32 len, const u16 channel, const u8 loop)
{
    vu8 *pb;
    u8 status;
    u16 ch;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // get status
    status = *pb;

    // auto channel ?
    if (channel == AUDIO_PCM_CH_AUTO)
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

void stopPlay_4PCM(const u16 channel)
{
    vu8 *pb;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM, 1);

    Z80_requestBus(1);

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


// Z80_DRIVER_4PCM_ENV
// 4 channels 8 bits signed sample driver with volume support
///////////////////////////////////////////////////////////////
u8 isPlaying_4PCM_ENV(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    return ret;
}

void startPlay_4PCM_ENV(const u8 *sample, const u32 len, const u16 channel, const u8 loop)
{
    vu8 *pb;
    u8 status;
    u16 ch;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, 1);

    Z80_requestBus(1);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // get status
    status = *pb;

    // auto channel ?
    if (channel == AUDIO_PCM_CH_AUTO)
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

void stopPlay_4PCM_ENV(const u16 channel)
{
    vu8 *pb;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, 1);

    Z80_requestBus(1);

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

void setVolume_4PCM_ENV(const u16 channel, const u8 volume)
{
    vu8 *pb;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, 1);

    Z80_requestBus(1);

    // point to Z80 volume parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x20) + channel;
    // set volume (16 levels)
    *pb = (volume & 0x0F) | 0x10;

    Z80_releaseBus();
}

u8 getVolume_4PCM_ENV(const u16 channel)
{
    vu8 *pb;
    u8 volume;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, 1);

    Z80_requestBus(1);

    // point to Z80 volume parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x20) + channel;
    // get volume (16 levels)
    volume = *pb & 0x0F;

    Z80_releaseBus();

    return volume;
}


// Z80_DRIVER_MVS
// MVS Tracker driver
///////////////////////////////////////////////////////////////
u8 isPlaying_MVS()
{
    vu8 *pb;
    u8 ret;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(1);

    // point to Z80 play status for MVS
    pb = (u8 *) 0xA0151D;

    // status
    // 0 :silence 1: loop play 2: play once
    ret = *pb & 3;

    Z80_releaseBus();

    return ret;
}

void startPlay_MVS(const u8 *song, const u8 cmd)
{
    vu8 *pb;
    u32 addr;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(1);

    addr = (u32) song;

    // point to Z80 base parameters for MVS
    pb = (u8 *) 0xA0151A;

    // song address
    pb[0x00] = addr >> 0;
    pb[0x01] = addr >> 8;
    pb[0x02] = addr >> 16;

    // command
    pb[0x03] = cmd;

    Z80_releaseBus();
}

void stopPlay_MVS()
{
    vu8 *pb;

    // load the appropried driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(1);

    // point to Z80 command for MVS
    pb = (u8 *) 0xA0151D;
    // stop command for MVS
    *pb = 0;

    Z80_releaseBus();
}

// Z80_DRIVER_TFM
// TFM Tracker driver
///////////////////////////////////////////////////////////////
void startPlay_TFM(const u8 *song)
{
    vu8 *pb;
    u32 addr;

    Z80_requestBus(1);

    addr = (u32) song;

    // point to Z80 base parameters for TFM
    pb = (u8 *) 0xA01FFC;

    // song address
    pb[0x00] = addr >> 0;
    pb[0x01] = addr >> 8;
    pb[0x02] = addr >> 16;
    pb[0x00] = addr >> 24;

    Z80_releaseBus();

    // load the driver efter we set the song adress
    Z80_loadDriver(Z80_DRIVER_TFM, 0);

    // reset Z80 (in case driver was already loaded)
    Z80_startReset();
    Z80_endReset();
}
