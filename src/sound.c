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


// allow to access it without "public" share
extern vu32 VIntProcess;
extern s16 currentDriver;
extern u16 driverFlags;

// specific for the XGM driver
static u16 xgmTempo;
static u16 xgmTempoDef;
static s16 xgmTempoCnt = 0;

// Z80 cpu load calculation for XGM driver
static u16 xgmIdleTab[32];
static u16 xgmWaitTab[32];
static u16 xgmTabInd;
static u16 xgmIdleMean;
static u16 xgmWaitMean;

// Z80_DRIVER_PCM
// single channel 8 bits signed sample driver
///////////////////////////////////////////////////////////////

u8 SND_isPlaying_PCM()
{
    vu8 *pb;
    u8 ret;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_PCM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & Z80_DRV_STAT_PLAYING;

    Z80_releaseBus();

    return ret;
}

void SND_startPlay_PCM(const u8 *sample, const u32 len, const u8 rate, const u8 pan, const u8 loop)
{
    vu8 *pb;
    u32 addr;

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
}

void SND_stopPlay_PCM()
{
    vu8 *pb;
    u32 addr;

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
}


// Z80_DRIVER_2ADPCM
// 2 channels 4 bits ADPCM sample driver
///////////////////////////////////////////////////////////////

u8 SND_isPlaying_2ADPCM(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_2ADPCM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    return ret;
}

void SND_startPlay_2ADPCM(const u8 *sample, const u32 len, const u16 channel, const u8 loop)
{
    vu8 *pb;
    u8 status;
    u16 ch;
    u32 addr;

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
}

void SND_stopPlay_2ADPCM(const u16 channel)
{
    vu8 *pb;
    u32 addr;

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
}


// Z80_DRIVER_4PCM_ENV
// 4 channels 8 bits signed sample driver with volume support
///////////////////////////////////////////////////////////////

u8 SND_isPlaying_4PCM_ENV(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    return ret;
}

void SND_startPlay_4PCM_ENV(const u8 *sample, const u32 len, const u16 channel, const u8 loop)
{
    vu8 *pb;
    u8 status;
    u16 ch;
    u32 addr;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, TRUE);

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

void SND_stopPlay_4PCM_ENV(const u16 channel)
{
    vu8 *pb;
    u32 addr;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, TRUE);

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

void SND_setVolume_4PCM_ENV(const u16 channel, const u8 volume)
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 volume parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x20) + channel;
    // set volume (16 levels)
    *pb = volume & 0x0F;

    Z80_releaseBus();
}

u8 SND_getVolume_4PCM_ENV(const u16 channel)
{
    vu8 *pb;
    u8 volume;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_4PCM_ENV, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 volume parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x20) + channel;
    // get volume (16 levels)
    volume = *pb & 0x0F;

    Z80_releaseBus();

    return volume;
}


// Z80_DRIVER_MVS
// MVS Tracker driver (Kaneda version)
///////////////////////////////////////////////////////////////

u8 SND_isPlaying_MVS()
{
    vu8 *pb;
    u8 ret;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(TRUE);

    // point to Z80 FM command
    pb = (u8 *) MVS_FM_CMD;

    // status
    // 0 :silence   1: play once   2: loop play
    ret = *pb & 3;

    Z80_releaseBus();

    return ret;
}

void SND_startPlay_MVS(const u8 *music, const u8 loop)
{
    vu8 *pb;
    u32 addr;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(TRUE);

    addr = (u32) music;

    // point to Z80 FM address
    pb = (u8 *) MVS_FM_ADR;

    // set song address
    *pb++ = addr >> 0;
    *pb++ = addr >> 8;
    *pb = addr >> 16;

    // point to Z80 FM command
    pb = (u8 *) MVS_FM_CMD;

    // command
    if (loop)
        *pb++ = MVS_FM_LOOP;
    else
        *pb++ = MVS_FM_ONCE;
    // reset previous command
    *pb = MVS_FM_RESET;

    Z80_releaseBus();
}

void SND_stopPlay_MVS()
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(TRUE);

    // point to Z80 FM command
    pb = (u8 *) MVS_FM_CMD;

    // command
    *pb++ = MVS_FM_STOP;
    // reset previous command
    *pb = MVS_FM_RESET;

    Z80_releaseBus();
}

void SND_setTempo_MVS(u8 tempo)
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(TRUE);

    // point to Z80 FM tempo
    pb = (u8 *) MVS_FM_TEMPO;

    // set tempo
    *pb = tempo;

    Z80_releaseBus();
}


void SND_startDAC_MVS(const u8 *sample, u16 size)
{
    vu8 *pb;
    u32 addr;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(TRUE);

    addr = (u32) sample;

    // point to Z80 DAC address
    pb = (u8 *) MVS_DAC_ADR;

    // set sample address
    *pb++ = addr >> 0;
    *pb++ = addr >> 8;
    *pb = addr >> 16;

    // point to Z80 DAC command
    pb = (u8 *) MVS_DAC_CMD;

    // command
    *pb = MVS_DAC_PLAY;

    // point to Z80 DAC size
    pb = (u8 *) MVS_DAC_SIZE;

    *pb++ = size >> 0;
    *pb = size >> 8;

    Z80_releaseBus();
}

void SND_stopDAC_MVS()
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(TRUE);

    // point to Z80 DAC command
    pb = (u8 *) MVS_DAC_CMD;

    // command
    *pb = MVS_DAC_STOP;

    Z80_releaseBus();
}


u8 SND_isPlayingPSG_MVS()
{
    vu8 *pb;
    u8 ret;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(TRUE);

    // point to Z80 PSG status
    pb = (u8 *) MVS_PSG_STAT;

    // status
    // 0 :silence   1: playing
    ret = *pb;

    Z80_releaseBus();

    return ret;
}

void SND_startPSG_MVS(const u8 *music)
{
    vu8 *pb;
    u32 addr;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(TRUE);

    addr = (u32) music;

    // point to Z80 PSG address
    pb = (u8 *) MVS_PSG_ADR;

    // set music address
    *pb++ = addr >> 0;
    *pb++ = addr >> 8;
    *pb = addr >> 16;

    // point to Z80 PSG command
    pb = (u8 *) MVS_PSG_CMD;

    // command
    *pb = MVS_PSG_PLAY;

    Z80_releaseBus();
}

void SND_stopPSG_MVS()
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(TRUE);

    // point to Z80 PSG command
    pb = (u8 *) MVS_PSG_CMD;

    // command
    *pb = MVS_PSG_STOP;

    Z80_releaseBus();
}

void SND_enablePSG_MVS(u8 chan)
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(TRUE);

    // point to Z80 PSG channel
    pb = (u8 *) MVS_PSG_CHAN;

    // enable channel
    *pb |= 1 << chan;

    Z80_releaseBus();
}

void SND_disablePSG_MVS(u8 chan)
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_MVS, 0);

    Z80_requestBus(TRUE);

    // point to Z80 PSG channel
    pb = (u8 *) MVS_PSG_CHAN;

    // disable channel
    *pb &= ~(1 << chan);

    Z80_releaseBus();
}


// Z80_DRIVER_TFM
// TFM Tracker driver
///////////////////////////////////////////////////////////////

void SND_startPlay_TFM(const u8 *song)
{
    vu8 *pb;
    u32 addr;

    // force driver reload to clear memory
    Z80_unloadDriver();
    Z80_loadDriver(Z80_DRIVER_TFM, 0);

    Z80_requestBus(TRUE);

    addr = (u32) song;

    // point to Z80 base parameters for TFM
    pb = (u8 *) 0xA01FFC;

    // song address
    pb[0x00] = addr >> 0;
    pb[0x01] = addr >> 8;
    pb[0x02] = addr >> 16;
    pb[0x03] = addr >> 24;

    // release BUS & reset Z80
    Z80_startReset();
    Z80_releaseBus();
    // wait bus released
    while(Z80_isBusTaken());
    Z80_endReset();
}

void SND_stopPlay_TFM()
{
    // just unload driver
    Z80_unloadDriver();
}


// Z80_DRIVER_VGM
// VGM driver
///////////////////////////////////////////////////////////////

void SND_startPlay_VGM(const u8 *song)
{
    vu8 *pb;
    u32 addr;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_VGM, TRUE);

    // stop current music
    if (SND_isPlaying_VGM())
    {
        SND_stopPlay_VGM();
        waitMs(10);
    }

    Z80_requestBus(TRUE);

    addr = (u32) song;

    // point to Z80 VGM address parameter
    pb = (u8 *) 0xA00FFF;

    // song address
    pb[0x00] = addr >> 0;
    pb[0x01] = addr >> 8;
    pb[0x02] = addr >> 16;
    pb[0x03] = addr >> 24;

    // point to Z80 VGM init parameter
    pb = (u8 *) 0xA01003;
    *pb = 0x01;
   // point to Z80 VGM play parameter
    pb = (u8 *) 0xA01004;
    *pb = 0x01;

    Z80_releaseBus();
}

void SND_stopPlay_VGM()
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_VGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 VGM play parameter
    pb = (u8 *) 0xA01004;
    *pb = 0x00;

    Z80_releaseBus();
}

void SND_resumePlay_VGM()
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_VGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 VGM play parameter
    pb = (u8 *) 0xA01004;
    *pb = 0x01;

    Z80_releaseBus();
}

u8 SND_isPlaying_VGM()
{
    u8 result;
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_VGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 VGM play parameter
    pb = (u8 *) 0xA01004;
    result = *pb;

    Z80_releaseBus();

    return result;
}

void SND_playSfx_VGM(const u8 *sfx, u16 len)
{
    vu8 *pb;
    u16 z80addr;
    u8 z80bank;
    u32 addr = (u32) sfx;

    z80bank = ((addr & 0xFF8000) >> 15);
    z80addr = ((addr & ~0xFF8000) + 0x8000);

    char *p = (char *) 0xA01027;
    char *q = (char *)&z80addr;
    char *r = (char *) 0xA01029;
    char *s = (char *)&len;
    char *t = (char *) 0xA01026;
    char *u = (char *)&z80bank;

    Z80_requestBus(TRUE);
    p[0] = q[1];
    p[1] = q[0];
    r[0] = s[1];
    r[1] = s[0];
    t[0] = u[0];
    pb = (u8 *) 0xA01025;
    *pb = 0x01;
    Z80_releaseBus();
}


// Z80_DRIVER_XGM
// XGM driver
///////////////////////////////////////////////////////////////

u8 SND_isPlaying_XGM()
{
    vu8 *pb;
    u8 ret;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (1 << 6);

    Z80_releaseBus();

    return ret;
}

void SND_startPlay_XGM(const u8 *song)
{
    u8 ids[0x100-4];
    u32 addr;
    u16 i;
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // prepare sample id table
    for(i = 0; i < 0x3F; i++)
    {
        // sample address in sample bank data
        addr = song[(i * 4) + 0] << 8;
        addr |= song[(i * 4) + 1] << 16;

        // silent sample ? use null sample address
        if (addr == 0xFFFF00) addr = (u32) smp_null;
        // adjust sample address (make it absolute)
        else addr += ((u32) song) + 0x100;

        // write adjusted addr
        ids[(i * 4) + 0] = addr >> 8;
        ids[(i * 4) + 1] = addr >> 16;
        // and recopy len
        ids[(i * 4) + 2] = song[(i * 4) + 2];
        ids[(i * 4) + 3] = song[(i * 4) + 3];
    }

    // upload sample id table (first entry is silent sample, we don't transfer it)
    Z80_upload(0x1C00 + 4, ids, 0x100 - 4, FALSE);

    // Z80 upload release bus after operation
    Z80_requestBus(TRUE);

    // point to Z80 XGM address parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x00);

    // get song address and bypass sample id table
    addr = ((u32) song) + 0x100;

    // bypass sample data (use the sample data size)
    addr += song[0xFC] << 8;
    addr += song[0xFD] << 16;

    // and bypass the music data size field
    addr += 4;

    // set XGM music data address
    pb[0x00] = addr >> 0;
    pb[0x01] = addr >> 8;
    pb[0x02] = addr >> 16;
    pb[0x03] = addr >> 24;

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // set play XGM command
    *pb |= (1 << 6);

    // point to PENDING_FRM parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0F);
    // clear pending frame
    *pb = 0;

    Z80_releaseBus();
}

void SND_stopPlay_XGM()
{
    vu8 *pb;
    u32 addr;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // special xgm sequence to stop any sound
    addr = ((u32) stop_xgm);

    // point to Z80 XGM address parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x00);

    // set XGM music data address
    pb[0x00] = addr >> 0;
    pb[0x01] = addr >> 8;
    pb[0x02] = addr >> 16;
    pb[0x03] = addr >> 24;

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // set play XGM command
    *pb |= (1 << 6);

    // point to PENDING_FRM parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0F);
    // clear pending frame
    *pb = 0;

    Z80_releaseBus();
}

void SND_pausePlay_XGM()
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // set pause XGM command
    *pb |= (1 << 4);

    Z80_releaseBus();
}

void SND_resumePlay_XGM()
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // set resume XGM command
    *pb |= (1 << 5);

    // point to PENDING_FRM parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0F);
    // clear pending frame
    *pb = 0;

    Z80_releaseBus();
}

u8 SND_isPlayingPCM_XGM(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    Z80_releaseBus();

    return ret;
}

void SND_setPCM_XGM(const u8 id, const u8 *sample, const u32 len)
{
    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    SND_setPCMFast_XGM(id, sample, len);

    Z80_releaseBus();
}

void SND_setPCMFast_XGM(const u8 id, const u8 *sample, const u32 len)
{
    vu8 *pb;

    // point to sample id table
    pb = (u8 *) (0xA01C00 + (id * 4));

    // write sample addr
    pb[0x00] = ((u32) sample) >> 8;
    pb[0x01] = ((u32) sample) >> 16;
    pb[0x02] = len >> 8;
    pb[0x03] = len >> 16;
}

void SND_startPlayPCM_XGM(const u8 id, const u8 priority, const u16 channel)
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 PCM parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x04 + (channel * 2));

    // set PCM priority and id
    pb[0x00] = priority & 0xF;
    pb[0x01] = id;

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // set play PCM channel command
    *pb |= (Z80_DRV_COM_PLAY << channel);

    Z80_releaseBus();
}

void SND_stopPlayPCM_XGM(const u16 channel)
{
    vu8 *pb;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 PCM parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x04 + (channel * 2));

    // use silent PCM (id = 0) with maximum priority
    pb[0x00] = 0xF;
    pb[0x01] = 0;

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // set play PCM channel command
    *pb |= (Z80_DRV_COM_PLAY << channel);

    Z80_releaseBus();
}

void SND_set68KBUSProtection_XGM(u8 value)
{
    vu16 *pw_bus;
    vu8 *pb;

    // driver should be loaded here

    // request bus (need to end reset)
    pw_bus = (u16 *) Z80_HALT_PORT;

    // take bus
    *pw_bus = 0x0100;
    // wait for bus taken
    while (*pw_bus & 0x0100);

    // point to Z80 PROTECT parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0D);

    *pb = value;

    // release bus
    *pw_bus = 0x0000;
}

void SND_nextXFrame_XGM(u16 num)
{
    vu16 *pw_bus;
    vu8 *pb;

    // driver should be loaded here

    // request bus
    pw_bus = (u16 *) Z80_HALT_PORT;
    // point to MODIFYING_F parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0E);

    while(TRUE)
    {
        // take bus
        *pw_bus = 0x0100;
        // wait for bus taken
        while (*pw_bus & 0x0100);

        // Z80 not accessing ?
        if (!*pb) break;

        // release bus
        *pw_bus = 0x0000;

        // wait a bit (about 80 cycles)
        asm volatile ("\t\tmovm.l %d0-%d3,-(%sp)\n");
        asm volatile ("\t\tmovm.l (%sp)+,%d0-%d3\n");
    }

    // point to PENDING_FRM parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0F);
    // increment frame to process
    *pb += num;

    // release bus
    *pw_bus = 0x0000;
}


u16 SND_getManualSync_XGM()
{
    return driverFlags & DRIVER_FLAG_MANUALSYNC_XGM;
}

void SND_setManualSync_XGM(u16 value)
{
    // nothing to do
    if (currentDriver != Z80_DRIVER_XGM)
        return;

    if (value)
    {
        driverFlags |= DRIVER_FLAG_MANUALSYNC_XGM;
        // remove VInt XGM process
        VIntProcess &= ~PROCESS_XGM_TASK;
    }
    else
    {
        driverFlags &= ~DRIVER_FLAG_MANUALSYNC_XGM;
        // set VInt XGM process
        VIntProcess |= PROCESS_XGM_TASK;
    }
}

u16 SND_getForceDelayDMA_XGM()
{
    return driverFlags & DRIVER_FLAG_DELAYDMA_XGM;
}

void SND_setForceDelayDMA_XGM(u16 value)
{
    // nothing to do
    if (currentDriver != Z80_DRIVER_XGM)
        return;

    if (value)
        driverFlags |= DRIVER_FLAG_DELAYDMA_XGM;
    else
        driverFlags &= ~DRIVER_FLAG_DELAYDMA_XGM;
}

u16 SND_getMusicTempo_XGM()
{
    return xgmTempo;
}

void SND_setMusicTempo_XGM(u16 value)
{
    xgmTempo = value;
    if (IS_PALSYSTEM) xgmTempoDef = 50;
    else xgmTempoDef = 60;
}

u32 SND_getCPULoad_XGM()
{
    vu8 *pb;
    u16 idle;
    u16 wait;
    u16 ind;
    s16 load;

    // driver should be loaded here

    // point to Z80 'idle wait loop' value
    pb = (u8 *) (Z80_DRV_PARAMS + 0x7C);

    Z80_requestBus(TRUE);

    // get idle
    idle = pb[0] + (pb[1] << 8);
    // reset it and point on 'dma wait loop'
    *pb++ = 0;
    *pb++ = 0;

    // get dma wait
    wait = pb[0] + (pb[1] << 8);
    // and reset it
    *pb++ = 0;
    *pb = 0;

    Z80_releaseBus();

    ind = xgmTabInd;

    xgmIdleMean -= xgmIdleTab[ind];
    xgmIdleMean += idle;
    xgmIdleTab[ind] = idle;

    xgmWaitMean -= xgmWaitTab[ind];
    xgmWaitMean += wait;
    xgmWaitTab[ind] = wait;

    xgmTabInd = (ind + 1) & 0x1F;

    load = 105 - (xgmIdleMean >> 5);

    return load | ((xgmWaitMean >> 5) << 16);
}

void XGM_resetLoadCalculation()
{
    u16 i;
    u16 *s1;
    u16 *s2;

    s1 = xgmIdleTab;
    s2 = xgmWaitTab;
    i = 32;
    while(i--)
    {
        *s1++ = 0;
        *s2++ = 0;
    }

    xgmTabInd = 0;
    xgmIdleMean = 0;
    xgmWaitMean = 0;
}

// VInt processing for XGM driver
void XGM_doVBlankProcess()
{
    vu16 *pw_bus;
    vu8 *pb;
    s16 cnt = xgmTempoCnt;
    u16 step = xgmTempoDef;
    u16 num = 0;

    while(cnt <= 0)
    {
        num++;
        cnt += step;
    }

    xgmTempoCnt = cnt - xgmTempo;

    // directly do the frame here as we want this code to be as fast as possible (to not waste vint time)

    // request bus (need to end reset)
    pw_bus = (u16 *) Z80_HALT_PORT;
    // point to MODIFYING_F parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0E);

    while(TRUE)
    {
        // take bus
        *pw_bus = 0x100;
        // wait for bus taken
        while (*pw_bus & 0x100);

        // Z80 not accessing ?
        if (!*pb) break;

        // release bus
        *pw_bus = 0x0000;

        // wait a bit (about 80 cycles)
        asm volatile ("\t\tmovm.l %d0-%d3,-(%sp)\n");
        asm volatile ("\t\tmovm.l (%sp)+,%d0-%d3\n");
    }

    // point to PENDING_FRM parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0F);
    // increment frame to process
    *pb += num;

    // release bus
    *pw_bus = 0x0000;
}
