#include "config.h"
#include "types.h"

#include "z80_ctrl.h"

#include "snd/sound.h"
#include "snd/xgm2.h"
#include "snd/xgm2/drv_xgm2.h"

#include "sys.h"
#include "timer.h"
#include "mapper.h"
#include "maths.h"
#include "memory.h"
#include "vdp.h"

#include "tools.h"


#define Z80_DRV_VARS                (Z80_RAM + 0x110)


#define XGM2_PLAY_ARG_FM_ADDR       (Z80_DRV_PARAMS + 0x00)     // FM stream address (b8-b23)
#define XGM2_PLAY_ARG_PSG_ADDR      (Z80_DRV_PARAMS + 0x02)     // PSG stream address (b8-b23)
#define XGM2_PLAY_ARG_LOOP          (Z80_DRV_PARAMS + 0x04)     // number of loop (0 = no loop, 255 = infinite)

#define XGM2_PCM_ARG_BASE           (Z80_DRV_PARAMS + 0x05)     // PCM base args

#define XGM2_PCM0_ARG               (XGM2_PCM_ARGS_BASE + 0)    // PCM0: b0-b3 = priority (0 to 15); b6 = half speed; b7 = loop
#define XGM2_PCM1_ARG               (XGM2_PCM_ARGS_BASE + 1)    // PCM1: b0-b3 = priority (0 to 15); b6 = half speed; b7 = loop
#define XGM2_PCM2_ARG               (XGM2_PCM_ARGS_BASE + 2)    // PCM2: b0-b3 = priority (0 to 15); b6 = half speed; b7 = loop

#define XGM2_FM_ARG_VOLUME          (Z80_DRV_PARAMS + 0x08)     // FM volume: 0x00 to 0x7F where 0x00 is maximum volume and 0x7F is silent (attenuation)
#define XGM2_PSG_ARG_VOLUME         (Z80_DRV_PARAMS + 0x09)     // PSG volume: 0x0 to 0xF where 0x0 is maximum volume and 0xF is silent (attenuation)

#define XGM2_TEMPO_ARG_INC_FRAC     (Z80_DRV_PARAMS + 0x0A)     // tempo: frame increment at each vsync (fractional part)
#define XGM2_TEMPO_ARG_INC          (Z80_DRV_PARAMS + 0x0B)     // tempo: frame increment at each vsync


#define XGM2_ACCESS_CMD_SFT                 0
#define XGM2_ACCESS_PCM_ARG_SFT             1
#define XGM2_ACCESS_WRITE_ELAPSED_SFT       2
#define XGM2_ACCESS_READ_TEMPO_SFT          3

#define XGM2_ACCESS_CMD_MSK                 (1 << XGM2_ACCESS_CMD_SFT)
#define XGM2_ACCESS_PCM_ARG_MSK             (1 << XGM2_ACCESS_PCM_ARG_SFT)
#define XGM2_ACCESS_WRITE_ELAPSED_MSK       (1 << XGM2_ACCESS_WRITE_ELAPSED_SFT)
#define XGM2_ACCESS_READ_TEMPO_MSK          (1 << XGM2_ACCESS_READ_TEMPO_SFT)


#define XGM2_ACCESS                         (Z80_DRV_VARS + 0x50)       // b0 = Z80 is processing 68k commands (reading params / clearing commands / writing status)
                                                                        //      68k should wait before issuing / modifying commands / reading status
                                                                        // b1 = Z80 is writing ELAPSED_FRAME timestamp (multi byte write)
                                                                        //      68k should wait before reading it
                                                                        // b2 = Z80 is reading TEMPO parameter (multi byte param)
                                                                        //      68k should wait before writing it
#define XGM2_IN_DMA                         (Z80_DRV_VARS + 0x51)       // b0 = DMA operation in progress - read only from z80
                                                                        //      Z80 cannot access 68k BUS when the bit is set

#define XGM2_ELAPSED_FRAME          (Z80_DRV_VARS + 0x55)       // elapsed frames since music start play (in frames), encoded on 24 bit
#define XGM2_MISSED_FRAME           (Z80_DRV_VARS + 0x58)       // missed frames since music start play (in frames), encoded on 8 bit

#define XGM2_FM_START               (Z80_DRV_VARS + 0x5C)
#define XGM2_FM_CUR                 (Z80_DRV_VARS + 0x5E)
#define XGM2_FM_BUF_RD              (Z80_DRV_VARS + 0x62)
#define XGM2_FM_BUF_WR              (Z80_DRV_VARS + 0x63)
#define XGM2_FM_WAIT_FRM            (Z80_DRV_VARS + 0x64)

#define XGM2_PSG_START              (Z80_DRV_VARS + 0x66)
#define XGM2_PSG_CUR                (Z80_DRV_VARS + 0x68)
#define XGM2_PSG_BUF_RD             (Z80_DRV_VARS + 0x6C)
#define XGM2_PSG_BUF_WR             (Z80_DRV_VARS + 0x6D)
#define XGM2_PSG_WAIT_FRM           (Z80_DRV_VARS + 0x6E)

#define XGM2_PCM_PARAM_BASE         (Z80_DRV_VARS + 0xD0)
#define XGM2_PCM_PARAM_LEN          8

#define XGM2_PCM_ADDR_INT           (XGM2_PCM_PARAM_BASE + 0)
#define XGM2_PCM_LEN_INT            (XGM2_PCM_PARAM_BASE + 3)
#define XGM2_PCM_PRIO_EXT_INT       (XGM2_PCM_PARAM_BASE + 5)

#define XGM2_IDLE_TIME              (Z80_DRV_VARS + 0xEB)       // idle loop counter (idle wait in number of PCM sample)
#define XGM2_DMA_WAIT_TIME          (Z80_DRV_VARS + 0xEC)       // wait loop counter (IN_DMA wait in number of PCM sample)

#define XGM2_DEBUG0                 (Z80_DRV_VARS + 0x48)       // first debug space (reserve 8 bytes for that)
#define XGM2_DEBUG1                 (Z80_DRV_VARS + 0xC8)       // second debug space (reserve 8 bytes for that)
#define XGM2_PLAYED_SAMPLES         (XGM2_DEBUG0 + 1)           // number of samples played on last frame
#define XGM2_FRAME_COUNTER          (XGM2_DEBUG0 + 2)           // number of elapsed frames since driver reset
#define XGM2_PROCESS_TIME           (XGM2_DEBUG1 + 0)           // last vint process time in number of frame


#define XGM2_PCM_BUFFER             (Z80_RAM + 0x1900)          // PCM ring buffer (4 x 64 bytes)
#define XGM2_SID_TABLE              (Z80_RAM + 0x1C00)          // Sample id table (size = $200)


#define XGM2_PCM_ADDR_ARG_BASE      (XGM2_SID_TABLE + 0x1F4)
#define XGM2_PCM_LEN_ARG_BASE       (XGM2_SID_TABLE + 0x1F6)


#define XGM2_COM_PLAY_PCM_BASE      (1 << 0)
#define XGM2_COM_PLAY_PCM0          (XGM2_COM_PLAY_PCM_BASE << 0)
#define XGM2_COM_PLAY_PCM1          (XGM2_COM_PLAY_PCM_BASE << 1)
#define XGM2_COM_PLAY_PCM2          (XGM2_COM_PLAY_PCM_BASE << 2)
#define XGM2_COM_RESUME_PLAY        (1 << 3)
#define XGM2_COM_START_PLAY         (1 << 4)
#define XGM2_COM_PAUSE_PLAY         (1 << 5)
#define XGM2_COM_SET_VOLUME_FM      (1 << 6)
#define XGM2_COM_SET_VOLUME_PSG     (1 << 7)

#define XGM2_STATUS_PLAYING_PCM1    (1 << 0)
#define XGM2_STATUS_PLAYING_PCM2    (1 << 1)
#define XGM2_STATUS_PLAYING_PCM3    (1 << 2)
#define XGM2_STATUS_PLAYING_PCM_ALL (XGM2_STATUS_PLAYING_PCM1 | XGM2_STATUS_PLAYING_PCM2 | XGM2_STATUS_PLAYING_PCM3)
#define XGM2_STATUS_PLAYING         (1 << 4)
#define XGM2_STATUS_READY           (1 << 7)


#define XGM2_PAL_FLAG               (1 << 0)
#define XGM2_MULTI_TRACK_FLAG       (1 << 1)
#define XGM2_GD3_TAGS_FLAG          (1 << 2)
#define XGM2_PACKED_FLAG            (1 << 3)


typedef enum
{
    DO_NOTHING,
    DO_STOP,
    DO_PAUSE
} FadeEndProcess;


// FM volume conversion table
const u8 fmVolTable[100] =
{
    127, 121, 115, 110, 104, 99, 95, 90, 86, 82,
    78, 74, 71, 67, 64, 61, 58, 55, 52, 50,
    48, 45, 43, 41, 39, 37, 35, 34, 32, 30,
    29, 27, 26, 25, 24, 22, 21, 20, 19, 18,
    17, 17, 16, 15, 14, 13, 13, 12, 11, 11,
    10, 10, 9, 9, 8, 8, 7, 7, 7, 6,
    6, 6, 5, 5, 5, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 2, 2, 2, 2, 2,
    2, 2, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0
};

// PSG volume conversion table
const u8 psgVolTable[100] =
{
    15, 15, 14, 14, 13, 13, 13, 12, 12, 11,
    11, 11, 10, 10, 10, 10, 9, 9, 9, 8,
    8, 8, 8, 7, 7, 7, 7, 7, 6, 6,
    6, 6, 6, 5, 5, 5, 5, 5, 5, 4,
    4, 4, 4, 4, 4, 4, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


// allow to access it without "public" share
extern vu16 VBlankProcess;

// current loaded XGM track
static const u8* currentXGM;

// tempo
static u16 xgm2Tempo;

// volume
static u16 fmVol;
static u16 psgVol;
static bool restoreVolume;

// fade vars
static f16 fadeFMVol;
static f16 fadePSGVol;
static f16 fadeFMVolStep;
static f16 fadePSGVolStep;
static u16 fadeCount;
static FadeEndProcess fadeEndProcess;

// Z80 cpu load calculation for XGM2 driver
static u8 xgm2IdleTab[8];
static u8 xgm2WaitTab[8];
static u16 xgm2IdleTabInd;
static u16 xgm2WaitTabInd;
static u16 xgm2IdleMean;
static u16 xgm2WaitMean;

// forward
static bool getAccess(const u8 flag);
static void releaseAccess(const bool busTaken);
static void initLoadCalculation(void);
static s16 getPCMChannel(const u8 priority);
static void setMusicTempo(const u16 value);
static void setLoopNumber(const s8 value);
static void setFMVolume(const u16 value);
static void setPSGVolume(const u16 value);
static void doFade(const u16 fmVolStart, const u16 fmVolEnd, const u16 psgVolStart, const u16 psgVolEnd, const u16 frame, const FadeEndProcess fep);

// we don't want to share it
extern void Z80_loadDriverInternal(const u8 *drv, const u16 size);

// Z80_DRIVER_XGM2
// XGM2 driver
///////////////////////////////////////////////////////////////

void NO_INLINE XGM2_loadDriver(bool waitReady)
{
    Z80_loadDriverInternal(drv_xgm2, sizeof(drv_xgm2));

    SYS_disableInts();

    // wait driver for being ready
    if (waitReady)
    {
        // wait driver ready
        while(!Z80_isDriverReady())
            waitMs(1);
    }

    // default
    fmVol = 100;
    psgVol = 100;
    restoreVolume = FALSE;

    // set infinite loop
    setLoopNumber(0xFF);
    // set default tempo
    setMusicTempo(60);
    // init load calculations
    initLoadCalculation();
    // set bus protection signal address
    Z80_useBusProtection(XGM2_IN_DMA & 0xFFFF);

    SYS_enableInts();
}

void NO_INLINE XGM2_unloadDriver(void)
{
    // remove bus protection (signal address set to 0)
    Z80_useBusProtection(0);
}


bool XGM2_isPlaying(void)
{
    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);

    // point to Z80 status
    vu8* pb = (vu8*) Z80_DRV_STATUS;

    SYS_disableInts();
    // request Z80 BUS
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // play status
    bool ret = (*pb & XGM2_STATUS_PLAYING)?TRUE:FALSE;

    releaseAccess(busTaken);

    return ret;
}

void NO_INLINE XGM2_load(const u8 *song)
{
    u8 ids[249*2];
    u16 i;

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);

    // set current XGM
    currentXGM = song;

    // sample offset
    u32 sampleOff = (u32) song;
    // num max sample
    u16 maxSample;

    // PAL timing ?
    if (song[1] & XGM2_PAL_FLAG) setMusicTempo(50);
    else setMusicTempo(60);

    // multi tracks ?
    if (song[1] & XGM2_MULTI_TRACK_FLAG)
    {
        sampleOff += 0x400;
        // only 500 bytes of the SID table can be used as the last 12 bytes are used for SFX
        maxSample = (500 / 2) - 1;

    }
    // single track
    else
    {
        sampleOff += 0x100;
        maxSample = 124;
    }

    // prepare sample id table
    for(i = 0; i < maxSample; i++)
    {
        u32 addr;

        // sample address in sample id table data
        addr = song[8 + (i * 2) + 0] << 0;
        addr |= song[8 + (i * 2) + 1] << 8;

        // no more sample ? stop here
        if (addr == 0xFFFF) break;

        // adjust sample address (make it absolute)
        addr = (addr << 8) + sampleOff;

        // write adjusted addr
        ids[(i * 2) + 0] = addr >> 8;
        ids[(i * 2) + 1] = addr >> 16;
    }

    // upload sample id table
    Z80_upload(XGM2_SID_TABLE & 0xFFFF, ids, i * 2);
}

void XGM2_load_FAR(const u8 *song, const u32 len)
{
    XGM2_load(FAR_SAFE(song, len));
}

void NO_INLINE XGM2_playTrack(const u16 track)
{
    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    // trying to play no existing track ?
    if (!(currentXGM[1] & XGM2_MULTI_TRACK_FLAG) && (track > 0))
        kprintf("XGM2_playTrack() error: cannot play track %d on a single track XGM file", track);
#endif

    u32 offset;
    u32 sampleLen;
    u32 fmLen;

    // base offset (bypass setting + base sample id table)
    offset = (u32) &currentXGM[0x100];
    // sample data len
    sampleLen = currentXGM[2] << 8;
    sampleLen +=  currentXGM[3] << 16;
    // FM data len
    fmLen = currentXGM[4] << 8;
    fmLen +=  currentXGM[5] << 16;

    u32 fmAddr;
    u32 psgAddr;

    // multi tracks ?
    if (currentXGM[1] & XGM2_MULTI_TRACK_FLAG)
    {
        // bypass sample id table extension (0x100) + FM id table (0x100) + PSG id table (0x100)
        offset += 0x300;
        // get base offset for FM and PSG
        fmAddr = currentXGM[0x200 + (track * 2) + 0] << 8;
        fmAddr += currentXGM[0x200 + (track * 2) + 1] << 16;
        psgAddr = currentXGM[0x300 + (track * 2) + 0] << 8;
        psgAddr += currentXGM[0x300 + (track * 2) + 1] << 16;
    }
    else
    {
        fmAddr = 0;
        psgAddr = 0;
    }

    // compute final FM and PSG streams address
    fmAddr += offset + sampleLen;
    psgAddr += offset + sampleLen + fmLen;

    // request Z80 bus access
    const bool busTaken = getAccess(XGM2_ACCESS_CMD_MSK);

    // point to Z80 FM stream address parameter
    vu8* pb = (vu8*) XGM2_PLAY_ARG_FM_ADDR;
    // set FM stream address (aligned on 256 bytes)
    *pb++ = fmAddr >> 8;
    *pb++ = fmAddr >> 16;
    // set PSG stream address (aligned on 256 bytes)
    *pb++ = psgAddr >> 8;
    *pb++ = psgAddr >> 16;

    // point to Z80 command
    pb = (vu8*) Z80_DRV_COMMAND;
    // set play XGM2 command (and clear pause/resume if any)
    *pb = (*pb & ~(XGM2_COM_RESUME_PLAY | XGM2_COM_PAUSE_PLAY)) | XGM2_COM_START_PLAY;

    releaseAccess(busTaken);

    // need to restore FM and PSG volumes
    if (restoreVolume)
    {
        setFMVolume(fmVol);
        setPSGVolume(psgVol);
        restoreVolume = FALSE;
    }
}

void XGM2_play(const u8* song)
{
    XGM2_load(song);
    XGM2_playTrack(0);
}

void XGM2_play_FAR(const u8* song, const u32 len)
{
    XGM2_load_FAR(song, len);
    XGM2_playTrack(0);
}

void NO_INLINE XGM2_stop(void)
{
    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);

    // request Z80 bus access
    const bool busTaken = getAccess(XGM2_ACCESS_CMD_MSK);

    // point to Z80 FM stream address parameter
    vu8* pb = (vu8*) XGM2_PLAY_ARG_FM_ADDR;
    // set FM stream address to NULL
    *pb++ = 0;
    *pb++ = 0;
    // set PSG stream address to NULL
    *pb++ = 0;
    *pb++ = 0;

    // point to Z80 command
    pb = (vu8*) Z80_DRV_COMMAND;
    // set play XGM2 command (and clear pause/resume if any)
    *pb = (*pb & ~(XGM2_COM_RESUME_PLAY | XGM2_COM_PAUSE_PLAY)) | XGM2_COM_START_PLAY;

    releaseAccess(busTaken);
}

void NO_INLINE XGM2_pause(void)
{
    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);

    // request Z80 bus access
    const bool busTaken = getAccess(XGM2_ACCESS_CMD_MSK);

    // point to Z80 status
    vu8* pb = (vu8*) Z80_DRV_STATUS;
    // playing ?
    if (*pb & XGM2_STATUS_PLAYING)
    {
        // point to Z80 command
        pb = (vu8*) Z80_DRV_COMMAND;
        // set pause XGM2 command (and clear resume/play if any)
        *pb = (*pb & ~(XGM2_COM_RESUME_PLAY | XGM2_COM_START_PLAY)) | XGM2_COM_PAUSE_PLAY;
    }

    releaseAccess(busTaken);
}

void NO_INLINE XGM2_resume(void)
{
    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);

    // request Z80 bus access
    const bool busTaken = getAccess(XGM2_ACCESS_CMD_MSK);

    // point to Z80 status
    vu8* pb = (vu8*) Z80_DRV_STATUS;
    // not playing ?
    if (!(*pb & XGM2_STATUS_PLAYING))
    {
        // point to Z80 FM stream start address
        pb = (vu8*) XGM2_FM_START;
        // FM stream start address not NULL ?
        if (pb[0] | pb[1])
        {
            // point to Z80 command
            pb = (vu8*) Z80_DRV_COMMAND;
            // set command only if we don't have a play command pending
            if (!(*pb & XGM2_COM_START_PLAY))
                // set resume XGM2 command (and clear pause if any)
                *pb = (*pb & ~XGM2_COM_PAUSE_PLAY) | XGM2_COM_RESUME_PLAY;
        }
    }

    releaseAccess(busTaken);

    // need to restore FM and PSG volumes
    if (restoreVolume)
    {
        setFMVolume(fmVol);
        setPSGVolume(psgVol);
        restoreVolume = FALSE;
    }
}

u8 XGM2_isPlayingPCM(const u16 channel_mask)
{
    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);

    // point to Z80 status
    vu8* pb = (vu8*) Z80_DRV_STATUS;

    SYS_disableInts();
    // request Z80 BUS
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // play status
    u8 ret = *pb & channel_mask;

    releaseAccess(busTaken);

    return ret;
}

static s16 getPCMChannel(const u8 priority)
{
    SYS_disableInts();
    // request Z80 BUS
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // play status
    vu8* pb = (vu8*) Z80_DRV_STATUS;
    u8 status = *pb & XGM2_STATUS_PLAYING_PCM_ALL;
    u8 prios[3];

    // play priorities
    pb = (vu8*) (XGM2_PCM_PRIO_EXT_INT + (XGM2_PCM_PARAM_LEN * 0));
    prios[0] = *pb & 0xF;
    pb = (vu8*) (XGM2_PCM_PRIO_EXT_INT + (XGM2_PCM_PARAM_LEN * 1));
    prios[1] = *pb & 0xF;
    pb = (vu8*) (XGM2_PCM_PRIO_EXT_INT + (XGM2_PCM_PARAM_LEN * 2));
    prios[2] = *pb & 0xF;

    releaseAccess(busTaken);

    // try channel 3 first if free (lower CPU usage)
    if (!(status & XGM2_STATUS_PLAYING_PCM3)) return SOUND_PCM_CH3;
    // the try channel 2 as channel 1 can be used for music
    if (!(status & XGM2_STATUS_PLAYING_PCM2)) return SOUND_PCM_CH2;

    // then compare channel 3 priority
    if (prios[2] <= priority) return SOUND_PCM_CH3;
    // then compare channel 2 priority
    if (prios[1] <= priority) return SOUND_PCM_CH2;

    // try channel 1 in last (can be used for music)
    if (!(status & XGM2_STATUS_PLAYING_PCM2)) return SOUND_PCM_CH1;
    if (prios[0] <= priority) return SOUND_PCM_CH1;

    // can't play
    return -1;
}

bool NO_INLINE XGM2_playPCMEx(const u8 *sample, const u32 len, const SoundPCMChannel channel, const u8 priority, const bool halfRate, const bool loop)
{

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);

    // get channel
    const s16 ch = (channel == SOUND_PCM_CH_AUTO)?getPCMChannel(priority):channel;
    // no available channel ? --> exit
    if (ch == -1) return FALSE;

    // request Z80 bus access
    const bool busTaken = getAccess(XGM2_ACCESS_CMD_MSK | XGM2_ACCESS_PCM_ARG_MSK);
    vu8* pb;

    // manual channel selection ? --> test if available
    if (channel != SOUND_PCM_CH_AUTO)
    {
        // get channel play status
        pb = (vu8*) Z80_DRV_STATUS;
        u8 playing = *pb & (1 << ch);
        // get channel priority
        pb = (vu8*) (XGM2_PCM_PRIO_EXT_INT + (XGM2_PCM_PARAM_LEN * ch));
        u8 prio = *pb & 0xF;

        // channel playing and prio > new prio ? --> cannot play on this channel
        if (playing && (prio > priority))
        {
            releaseAccess(busTaken);
            return FALSE;
        }
    }

    // get slot address in sample id table (use the 3 last slots which aren't used by music)
    pb = (vu8*) (XGM2_PCM_ADDR_ARG_BASE + (ch * 4));
    // write sample addr
    *pb++ = ((u32) sample) >> 8;
    *pb++ = ((u32) sample) >> 16;
    // write sample len
    if (halfRate)
    {
        // len x2 for half rate (as we play both sample twice)
        *pb++ = len >> 5;
        *pb   = len >> 13;
    }
    else
    {
        *pb++ = len >> 6;
        *pb   = len >> 14;
    }

    // point to Z80 PCM parameter
    pb = (vu8*) (XGM2_PCM_ARG_BASE + ch);
    // b0-b3 = priority (0 to 15); b4 = 1; b6 = half speed; b7 = loop
    *pb = (priority & 0xF) | (halfRate?0x40:0) | (loop?0x80:0) | 0x10;

    // point to Z80 command
    pb = (vu8*) Z80_DRV_COMMAND;
    // set play PCM channel command
    *pb |= (XGM2_COM_PLAY_PCM_BASE << ch);

    releaseAccess(busTaken);

    return TRUE;
}

bool XGM2_playPCM(const u8 *sample, const u32 len, const SoundPCMChannel channel)
{
    return XGM2_playPCMEx(sample, len, channel, 6, FALSE, FALSE);
}

void XGM2_stopPCM(const SoundPCMChannel channel)
{
    // use play NULL PCM as stop PCM commad
    XGM2_playPCMEx(NULL, 0, channel, 15, FALSE, FALSE);
}


static void doFade(const u16 fmVolStart, const u16 fmVolEnd, const u16 psgVolStart, const u16 psgVolEnd, const u16 frame, const FadeEndProcess fep)
{
    if (frame == 0) return;

    f16 fmVolStartF = intToFix16(fmVolStart);
    f16 psgVolStartF = intToFix16(psgVolStart);
    f16 fmVolEndF = intToFix16(fmVolEnd);
    f16 psgVolEndF = intToFix16(psgVolEnd);
    // we use "/ 2" as we alternate PSG and FM volume update on fading
    f16 frameF = intToFix16(frame) / 2;

    // set fade process variables
    fadeFMVol = fmVolStartF;
    fadePSGVol = psgVolStartF;
    fadeFMVolStep = fix16Div(fmVolEndF - fmVolStartF, frameF);
    fadePSGVolStep = fix16Div(psgVolEndF - psgVolStartF, frameF);
    fadeCount = frame;
    fadeEndProcess = fep;

    // init fade
    setFMVolume(fix16ToInt(fadeFMVol));
    setPSGVolume(fix16ToInt(fadePSGVol));

    // add task for vblank process
    VBlankProcess |= PROCESS_XGM2_FADE_TASK;
}


bool XGM2_isProcessingFade(void)
{
    return (VBlankProcess & PROCESS_XGM2_FADE_TASK)?TRUE:FALSE;
}

void XGM2_fadeIn(const u16 frame)
{
    doFade(0, fmVol, 0, psgVol, frame, DO_NOTHING);
}

void XGM2_fadeOut(const u16 frame)
{
    doFade(fmVol, 0, psgVol, 0, frame, DO_NOTHING);
}

void XGM2_fadeOutAndStop(const u16 frame)
{
    doFade(fmVol, 0, psgVol, 0, frame, DO_STOP);
}

void XGM2_fadeOutAndPause(const u16 frame)
{
    doFade(fmVol, 0, psgVol, 0, frame, DO_PAUSE);
}

void XGM2_fadeTo(const u16 toFMVolume, const u16 toPSGVolume, const u16 frame)
{
    doFade(fmVol, toFMVolume, psgVol, toPSGVolume, frame, DO_NOTHING);
}

bool XGM2_doVBlankFadeProcess(void)
{
    fadeCount--;

    // we do that to lower a bit Z80 CPU processing for volume fade effect
    if (fadeCount & 1)
    {
        fadeFMVol += fadeFMVolStep;
        setFMVolume(fix16ToInt(fadeFMVol));
    }
    else
    {
        fadePSGVol += fadePSGVolStep;
        setPSGVolume(fix16ToInt(fadePSGVol));
    }

    // mark volume need to be restored
    restoreVolume = TRUE;

    // end of fade ?
    if (fadeCount == 0)
    {
        // end process to do
        switch(fadeEndProcess)
        {
            case DO_STOP:
                XGM2_stop();
                break;

            case DO_PAUSE:
                XGM2_pause();
                break;

            default:
            case DO_NOTHING:
                break;
        }

        // done
        return FALSE;
    }

    return TRUE;
}


static void NO_INLINE setLoopNumber(const s8 value)
{
    // point to Z80 play loop parameter
    vu8* pb = (vu8*) XGM2_PLAY_ARG_LOOP;

    // request Z80 bus access
    const bool busTaken = getAccess(XGM2_ACCESS_CMD_MSK);

    // set loop argument (0 = no loop, 0xFF = infinite)
    *pb = value;

    releaseAccess(busTaken);
}

void XGM2_setLoopNumber(s8 value)
{
    if (Z80_getLoadedDriver() != Z80_DRIVER_XGM2) return;

    setLoopNumber(value);
}

u16 XGM2_getMusicTempo(void)
{
    return xgm2Tempo;
}

static void NO_INLINE setMusicTempo(const u16 value)
{
    xgm2Tempo = value;
    // compute tempo
    u16 adjTempo = divu(value << 8, IS_PAL_SYSTEM?50:60);
    // point to Z80 tempo parameter
    vu8* pb = (vu8*) XGM2_TEMPO_ARG_INC_FRAC;

    // request Z80 bus access
    const bool busTaken = getAccess(XGM2_ACCESS_READ_TEMPO_MSK);

    // set tempo fractional part first
    *pb++ = adjTempo & 0xFF;
    // set tempo integer part first
    *pb = adjTempo >> 8;

    releaseAccess(busTaken);
}

void XGM2_setMusicTempo(const u16 value)
{
    if (Z80_getLoadedDriver() != Z80_DRIVER_XGM2) return;

    setMusicTempo(value);
}

u32 NO_INLINE XGM2_getElapsed(void)
{
    if (Z80_getLoadedDriver() != Z80_DRIVER_XGM2) return 0;

    u8 values[4];
    u8 *dst = values;

    // point to ELAPSED_FRAME value
    vu8* pb = (vu8*) XGM2_ELAPSED_FRAME;

    // request Z80 bus access
    const bool busTaken = getAccess(XGM2_ACCESS_WRITE_ELAPSED_MSK);

    // copy quickly elapsed time
    *dst++ = *pb++;
    *dst++ = *pb++;
    *dst = *pb;

    releaseAccess(busTaken);

    u32 result = (values[0] << 0) | (values[1] << 8) | ((u32) values[2] << 16);

    // fix possible 24 bit negative value (parsing first extra frame)
    if (result >= 0xFFFFF0) return 0;

    return result;
}


static void NO_INLINE setFMVolume(u16 value)
{
    // request Z80 bus access
    const bool busTaken = getAccess(XGM2_ACCESS_CMD_MSK);

    // point to Z80 FM volume parameter
    vu8* pb = (vu8*) XGM2_FM_ARG_VOLUME;
    // set FM volume (attenuation)
    *pb = (value >= 100)?0:fmVolTable[value];

    // point to Z80 command
    pb = (vu8*) Z80_DRV_COMMAND;
    // set FM volume XGM2 command
    *pb |= XGM2_COM_SET_VOLUME_FM;

    releaseAccess(busTaken);
}

static void NO_INLINE setPSGVolume(u16 value)
{
    // request Z80 bus access
    const bool busTaken = getAccess(XGM2_ACCESS_CMD_MSK);

    // point to Z80 FM volume parameter
    vu8* pb = (vu8*) XGM2_PSG_ARG_VOLUME;
    // set PSG volume (attenuation)
    *pb = (value >= 100)?0:psgVolTable[value];

    // point to Z80 command
    pb = (vu8*) Z80_DRV_COMMAND;
    // set PSG volume XGM2 command
    *pb |= XGM2_COM_SET_VOLUME_PSG;

    releaseAccess(busTaken);
}

void XGM2_setFMVolume(const u16 value)
{
    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);

    setFMVolume(value);
    // store it
    fmVol = value;
}

void XGM2_setPSGVolume(const u16 value)
{
    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM2, TRUE);

    setPSGVolume(value);
    // store it
    psgVol = value;
}


bool XGM2_isPAL(const u8 *xgm2)
{
    return (xgm2[1] & XGM2_PAL_FLAG)?TRUE:FALSE;
}


u16 NO_INLINE XGM2_getCPULoad(const bool mean)
{
    if (Z80_getLoadedDriver() != Z80_DRIVER_XGM2) return 0;

    // point to Z80 'idle time' value (in frame)
    vu8* pb = (vu8*) XGM2_IDLE_TIME;

    SYS_disableInts();
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // get idle
    u16 idle = *pb;

    releaseAccess(busTaken);

    if (mean)
    {
        // compute mean
        u16 ind = xgm2IdleTabInd;

        xgm2IdleMean -= xgm2IdleTab[ind];
        xgm2IdleMean += idle;
        xgm2IdleTab[ind] = idle;

        xgm2IdleTabInd = (ind + 1) & 7;

        // use mean value
        idle = xgm2IdleMean >> 3;
    }

    const u16 samplesPerFrame = 13300 / (IS_PAL_SYSTEM?50:60);
    return 100 - (mulu(idle, 100) / samplesPerFrame);
}

u16 NO_INLINE XGM2_getDMAWaitTime(const bool mean)
{
    if (Z80_getLoadedDriver() != Z80_DRIVER_XGM2) return 0;

    // point to Z80 'DMA wait time' value (in frame)
    vu8* pb = (vu8*) XGM2_DMA_WAIT_TIME;

    SYS_disableInts();
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // get dma wait
    u16 dmaWait = *pb;

    releaseAccess(busTaken);

    if (mean)
    {
        // compute mean
        u16 ind = xgm2WaitTabInd;

        xgm2WaitMean -= xgm2WaitTab[ind];
        xgm2WaitMean += dmaWait;
        xgm2WaitTab[ind] = dmaWait;

        xgm2WaitTabInd = (ind + 1) & 7;

        // use mean value
        dmaWait = xgm2WaitMean >> 3;
    }

    const u16 samplesPerFrame = 13300 / (IS_PAL_SYSTEM?50:60);
    return mulu(dmaWait, 100) / samplesPerFrame;
}


u16 NO_INLINE XGM2_getDebugFrameCounter(void)
{
    if (Z80_getLoadedDriver() != Z80_DRIVER_XGM2) return 0;

    vu8* pb = (vu8*) XGM2_FRAME_COUNTER;

    SYS_disableInts();
    bool busTaken = Z80_getAndRequestBus(TRUE);

    u16 frameCounter = pb[0] | (pb[1] << 8);

    releaseAccess(busTaken);

    return frameCounter;
}

u16 NO_INLINE XGM2_getDebugPCMRate(void)
{
    if (Z80_getLoadedDriver() != Z80_DRIVER_XGM2) return 0;

    vu8* pb = (vu8*) XGM2_PLAYED_SAMPLES;

    SYS_disableInts();
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // the value give the number of played sample per frame
    u8 playedSamplesPerFrame = *pb;

    releaseAccess(busTaken);

    // compute playback rate
    return mulu(playedSamplesPerFrame, IS_PAL_SYSTEM?50:60);
}

u8 NO_INLINE XGM2_getDebugMissedFrames(void)
{
    if (Z80_getLoadedDriver() != Z80_DRIVER_XGM2) return 0;

    vu8* pb = (vu8*) XGM2_MISSED_FRAME;

    SYS_disableInts();
    // request Z80 BUS
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // get missed frames (8 bit value, not really important)
    u8 result = *pb;

    releaseAccess(busTaken);

    return result;
}

u8 NO_INLINE XGM2_getDebugProcessDuration(const u16 ind)
{
    if (Z80_getLoadedDriver() != Z80_DRIVER_XGM2) return 0;

    vu8* pb = (vu8*) (XGM2_PROCESS_TIME + ind);

    SYS_disableInts();
    bool busTaken = Z80_getAndRequestBus(TRUE);

    u8 processDuration = *pb;

    releaseAccess(busTaken);

    return processDuration;
}


static bool getAccess(const u8 flag)
{
    // point to Z80 access state
    vu8* pb = (vu8*) XGM2_ACCESS;

    SYS_disableInts();
    bool busTaken = Z80_getAndRequestBus(TRUE);

    u8 access = *pb;

    // wait until access is granted
    while(access & flag)
    {
        Z80_releaseBus();
        SYS_enableInts();

        // wait a bit
        waitSubTick(5);

        SYS_disableInts();
        Z80_requestBus(TRUE);

        access = *pb;
    }

    return busTaken;
}

static void releaseAccess(const bool busTaken)
{
    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

static void initLoadCalculation(void)
{
    memset(xgm2IdleTab, 0, 8);
    memset(xgm2WaitTab, 0, 8);

    xgm2IdleTabInd = 0;
    xgm2WaitTabInd = 0;
    xgm2IdleMean = 0;
    xgm2WaitMean = 0;
}
