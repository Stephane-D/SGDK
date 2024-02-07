#include "config.h"
#include "types.h"

#include "z80_ctrl.h"

#include "snd/sound.h"
#include "snd/xgm.h"
#include "snd/drv_xgm.h"
#include "snd/smp_null.h"

#include "sys.h"
#include "timer.h"
#include "mapper.h"

#include "vdp.h"
#include "bmp.h"
#include "vdp_tile.h"
#include "libres.h"


// custom XGM flag (start at index 8)
#define DRIVER_FLAG_MANUALSYNC_XGM  (1 << 8)


// allow to access it without "public" share
extern vu16 VBlankProcess;
extern s16 currentDriver;
extern u16 driverFlags;

// specific for the XGM driver
static u16 xgmTempo;
static u16 xgmTempoDef;
// can be nice to alter it from external
s16 xgmTempoCnt = 0;

// Z80 cpu load calculation for XGM driver
static u16 xgmIdleTab[32];
static u16 xgmWaitTab[32];
static u16 xgmTabInd;
static u16 xgmIdleMean;
static u16 xgmWaitMean;

// set next frame helper
static void setNextXFrame(u16 num, bool set);
static void resetLoadCalculation();

// we don't want to share it
extern void Z80_loadDriverInternal(const u8 *drv, u16 size);

// Z80_DRIVER_XGM
// XGM driver
///////////////////////////////////////////////////////////////

void NO_INLINE XGM_loadDriver(const bool waitReady)
{
    Z80_loadDriverInternal(drv_xgm, sizeof(drv_xgm));

    SYS_disableInts();

    // misc parameters initialisation
    Z80_requestBus(TRUE);
    // point to Z80 sample id table (first entry = silent sample)
    vu8 *pb = (u8 *) (0xA01C00);

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

    // using auto sync --> enable XGM task on VInt
    if (!XGM_getManualSync())
        VBlankProcess |= PROCESS_XGM_TASK;
    // define default XGM tempo (always based on NTSC timing)
    XGM_setMusicTempo(60);
    // reset load calculation
    resetLoadCalculation();
    // set bus protection signal address
    Z80_useBusProtection((Z80_DRV_PARAMS + 0x0D) & 0xFFFF);

    SYS_enableInts();
}

void NO_INLINE XGM_unloadDriver(void)
{
    // remove XGM vblank / vint task
    VBlankProcess &= ~PROCESS_XGM_TASK;
    // remove bus protection (signal address set to 0)
    Z80_useBusProtection(0);
}


bool NO_INLINE XGM_isPlaying(void)
{
    vu8 *pb;
    u8 ret;

    SYS_disableInts();
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;

    Z80_requestBus(TRUE);
    // play status
    ret = *pb & (1 << 6);

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();

    return ret;
}

void NO_INLINE XGM_startPlay(const u8 *song)
{
    u8 ids[0x100-4];
    u32 addr;
    u16 i;
    vu8 *pb;

    SYS_disableInts();
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    // prepare sample id table
    for(i = 0; i < 0x3F; i++)
    {
        // sample address in sample bank data
        addr = song[(i * 4) + 0] << 8;
        addr |= ((u32) song[(i * 4) + 1]) << 16;

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
    Z80_upload(0x1C00 + 4, ids, 0x100 - 4);

    // get song address and bypass sample id table
    addr = ((u32) song) + 0x100;
    // bypass sample data (use the sample data size)
    addr += song[0xFC] << 8;
    addr += ((u32) song[0xFD]) << 16;
    // and bypass the music data size field
    addr += 4;

    // request Z80 BUS
    Z80_requestBus(TRUE);

    // point to Z80 XGM address parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x00);
    // set XGM music data address
    pb[0x00] = addr >> 0;
    pb[0x01] = addr >> 8;
    pb[0x02] = addr >> 16;
    pb[0x03] = addr >> 24;

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // set play XGM command (clear pause/resume if any)
    *pb = (*pb & 0x0F) | (1 << 6);

    // clear pending frame
    setNextXFrame(0, TRUE);

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

void XGM_startPlay_FAR(const u8 *song, u32 size)
{
    XGM_startPlay(FAR_SAFE(song, size));
}

void NO_INLINE XGM_stopPlay()
{
    vu8 *pb;
    u32 addr;

    SYS_disableInts();
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // special xgm sequence to stop any sound
    addr = (u32) stop_xgm;

    // point to Z80 XGM address parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x00);

    // set XGM music data address
    pb[0x00] = addr >> 0;
    pb[0x01] = addr >> 8;
    pb[0x02] = addr >> 16;
    pb[0x03] = addr >> 24;

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // set play XGM command (clear pause/resume if any)
    *pb = (*pb & 0x0F) | (1 << 6);

    // force immediate music process
    setNextXFrame(3, TRUE);

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

void NO_INLINE XGM_pausePlay()
{
    vu8 *pb;

    SYS_disableInts();
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // set pause XGM command (clear play/resume if any)
    *pb = (*pb & 0x0F) | (1 << 4);

    // clear pending frame
    setNextXFrame(0, TRUE);

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

void NO_INLINE XGM_resumePlay()
{
    vu8 *pb;

    SYS_disableInts();
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // does not contains a play command ?
    if ((*pb & (1 << 6)) == 0)
    {
        // set resume XGM command (clear pause if any)
        *pb = (*pb & 0x0F) | (1 << 5);
        // clear pending frame
        setNextXFrame(0, TRUE);
    }

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

u8 NO_INLINE XGM_isPlayingPCM(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;

    SYS_disableInts();
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();

    return ret;
}

void NO_INLINE XGM_setPCM(const u8 id, const u8 *sample, const u32 len)
{
    SYS_disableInts();
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    XGM_setPCMFast(id, sample, len);

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

void XGM_setPCMFast(const u8 id, const u8 *sample, const u32 len)
{
    // point to sample id table
    vu8 *pb = (u8 *) (0xA01C00 + (id * 4));

    // write sample addr
    pb[0x00] = ((u32) sample) >> 8;
    pb[0x01] = ((u32) sample) >> 16;
    pb[0x02] = len >> 8;
    pb[0x03] = len >> 16;
}

void NO_INLINE XGM_setPCM_FAR(const u8 id, const u8 *sample, const u32 len)
{
    SYS_disableInts();
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    XGM_setPCMFast_FAR(id, sample, len);

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

void XGM_setPCMFast_FAR(const u8 id, const u8 *sample, const u32 len)
{
    // point to sample id table
    vu8 *pb = (u8 *) (0xA01C00 + (id * 4));
    // sample address (with bank switch support)
    u32 addr = (u32) FAR_SAFE(sample, len);

    // write sample addr
    pb[0x00] = addr >> 8;
    pb[0x01] = addr >> 16;
    pb[0x02] = len >> 8;
    pb[0x03] = len >> 16;
}

void NO_INLINE XGM_startPlayPCM(const u8 id, const u8 priority, const SoundPCMChannel channel)
{
    vu8 *pb;

    SYS_disableInts();
    bool busTaken = Z80_isBusTaken();

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

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

void NO_INLINE XGM_stopPlayPCM(const SoundPCMChannel channel)
{
    vu8 *pb;

    SYS_disableInts();
    bool busTaken = Z80_isBusTaken();

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

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

void NO_INLINE XGM_setLoopNumber(s8 value)
{
    vu8 *pb;

    SYS_disableInts();
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 PCM parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0C);

    // set loop argument (+1 as internally 0 = infinite)
    *pb = value + 1;

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

void XGM_set68KBUSProtection(bool value)
{
    Z80_setBusProtection(value);
}


u16 XGM_getManualSync()
{
    return driverFlags & DRIVER_FLAG_MANUALSYNC_XGM;
}

void XGM_setManualSync(const bool value)
{
    // nothing to do
    if (currentDriver != Z80_DRIVER_XGM)
        return;

    if (value)
    {
        driverFlags |= DRIVER_FLAG_MANUALSYNC_XGM;
        // remove VInt XGM process
        VBlankProcess &= ~PROCESS_XGM_TASK;
    }
    else
    {
        driverFlags &= ~DRIVER_FLAG_MANUALSYNC_XGM;
        // set VInt XGM process
        VBlankProcess |= PROCESS_XGM_TASK;
    }
}

bool XGM_getForceDelayDMA()
{
    return Z80_getForceDelayDMA();
}

void XGM_setForceDelayDMA(const bool value)
{
    Z80_setForceDelayDMA(value);
}

u16 XGM_getMusicTempo()
{
    return xgmTempo;
}

void XGM_setMusicTempo(const u16 value)
{
    xgmTempo = value;
    if (IS_PAL_SYSTEM) xgmTempoDef = 50;
    else xgmTempoDef = 60;
}

u32 NO_INLINE XGM_getElapsed()
{
    vu8 *pb;
    u8 *dst;
    u8 values[3];
    u32 result;

    // nothing to do (driver should be loaded here)
    if (currentDriver != Z80_DRIVER_XGM)
        return 0;

    SYS_disableInts();

    // point to ELAPSED value
    pb = (u8 *) (Z80_DRV_PARAMS + 0x90);
    dst = values;

    bool busTaken = Z80_getAndRequestBus(TRUE);

    // copy quickly elapsed time
    *dst++ = *pb++;
    *dst++ = *pb++;
    *dst = *pb;

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();

    result = (values[0] << 0) | (values[1] << 8) | ((u32) values[2] << 16);

    // fix possible 24 bit negative value (parsing first extra frame)
    if (result >= 0xFFFFF0) return 0;

    return result;
}

u32 NO_INLINE XGM_getCPULoad()
{
    vu8 *pb;
    u16 idle;
    u16 wait;
    u16 ind;
    s16 load;

    // nothing to do (driver should be loaded here)
    if (currentDriver != Z80_DRIVER_XGM)
        return 0;

    SYS_disableInts();
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // point to Z80 'idle wait loop' value
    pb = (u8 *) (Z80_DRV_PARAMS + 0x7C);

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

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();

    ind = xgmTabInd;

    xgmIdleMean -= xgmIdleTab[ind];
    xgmIdleMean += idle;
    xgmIdleTab[ind] = idle;

    xgmWaitMean -= xgmWaitTab[ind];
    xgmWaitMean += wait;
    xgmWaitTab[ind] = wait;

    xgmTabInd = (ind + 1) & 0x1F;

    load = 105 - (xgmIdleMean >> 5);

    return load | ((u32) (xgmWaitMean >> 5) << 16);
}

static void resetLoadCalculation()
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

static void NO_INLINE setNextXFrame(u16 num, bool set)
{
    vu16 *pw_bus;
    vu16 *pw_reset;
    vu8 *pb;

    // point on bus req and reset ports
    pw_bus = (u16 *) Z80_HALT_PORT;
    pw_reset = (u16 *) Z80_RESET_PORT;
    // point to MODIFYING_F parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0E);

    while(TRUE)
    {
        // take bus and end reset (fast method)
        *pw_bus = 0x0100;
        *pw_reset = 0x0100;
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
    pb++;
    // set num frame to process
    if (set) *pb = num;
    // increment num frame to process
    else *pb += num;
}

void XGM_nextXFrame(const u16 num)
{
    // nothing to do (driver should be loaded here)
    if (currentDriver != Z80_DRIVER_XGM)
        return;

    SYS_disableInts();
    bool busTaken = Z80_isBusTaken();

    // add num frame to process
    setNextXFrame(num, FALSE);

    // release bus
    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

// VInt processing for XGM driver
void NO_INLINE XGM_doVBlankProcess()
{
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
    // driver should be loaded here
    bool busTaken = Z80_isBusTaken();

    // add num frame to process
    setNextXFrame(num, FALSE);

    // release bus
    if (!busTaken) Z80_releaseBus();
}
