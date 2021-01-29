#include "config.h"
#include "types.h"

#include "xgm.h"

#include "z80_ctrl.h"
#include "smp_null.h"
#include "sys.h"
#include "mapper.h"

//// just to get xgmstop resource
#include "vdp.h"
#include "bmp.h"
#include "vdp_tile.h"
#include "libres.h"


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


// Z80_DRIVER_XGM
// XGM driver
///////////////////////////////////////////////////////////////

u8 XGM_isPlaying()
{
    vu8 *pb;
    u8 ret;
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;

    Z80_requestBus(TRUE);
    // play status
    ret = *pb & (1 << 6);
    if (!busTaken) Z80_releaseBus();

    return ret;
}

void XGM_startPlay(const u8 *song)
{
    u8 ids[0x100-4];
    u32 addr;
    u16 i;
    vu8 *pb;
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
    Z80_upload(0x1C00 + 4, ids, 0x100 - 4, FALSE);

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
    // set play XGM command
    *pb |= (1 << 6);

    // point to PENDING_FRM parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0F);
    // clear pending frame
    *pb = 0;

    if (!busTaken) Z80_releaseBus();
}

void XGM_startPlay_FAR(const u8 *song, u32 size)
{
    XGM_startPlay(FAR_SAFE(song, size));
}

void XGM_stopPlay()
{
    vu8 *pb;
    u32 addr;
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
    // set play XGM command
    *pb |= (1 << 6);

    // point to PENDING_FRM parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0F);
    // clear pending frame
    *pb = 0;

    if (!busTaken) Z80_releaseBus();
}

void XGM_pausePlay()
{
    vu8 *pb;
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 command
    pb = (u8 *) Z80_DRV_COMMAND;
    // set pause XGM command
    *pb |= (1 << 4);

    if (!busTaken) Z80_releaseBus();
}

void XGM_resumePlay()
{
    vu8 *pb;
    bool busTaken = Z80_isBusTaken();

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

    if (!busTaken) Z80_releaseBus();
}

u8 XGM_isPlayingPCM(const u16 channel_mask)
{
    vu8 *pb;
    u8 ret;
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;
    // play status
    ret = *pb & (channel_mask << Z80_DRV_STAT_PLAYING_SFT);

    if (!busTaken) Z80_releaseBus();

    return ret;
}

void XGM_setPCM(const u8 id, const u8 *sample, const u32 len)
{
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);
    XGM_setPCMFast(id, sample, len);
    if (!busTaken) Z80_releaseBus();
}

void XGM_setPCMFast(const u8 id, const u8 *sample, const u32 len)
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

void XGM_startPlayPCM(const u8 id, const u8 priority, const u16 channel)
{
    vu8 *pb;
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
}

void XGM_stopPlayPCM(const u16 channel)
{
    vu8 *pb;
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
}

void XGM_setLoopNumber(s8 value)
{
    vu8 *pb;
    bool busTaken = Z80_isBusTaken();

    // load the appropriate driver if not already done
    Z80_loadDriver(Z80_DRIVER_XGM, TRUE);

    Z80_requestBus(TRUE);

    // point to Z80 PCM parameters
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0C);

    // set loop argument (+1 as internally 0 = infinite)
    *pb = value + 1;

    if (!busTaken) Z80_releaseBus();
}

void XGM_set68KBUSProtection(u8 value)
{
    vu8 *pb;

    // nothing to do (driver should be loaded here)
    if (currentDriver != Z80_DRIVER_XGM)
        return;

    bool busTaken = Z80_getAndRequestBus(TRUE);

    // point to Z80 PROTECT parameter
    pb = (u8 *) (Z80_DRV_PARAMS + 0x0D);
    *pb = value;

    // release bus
    if (!busTaken) Z80_releaseBus();
}


u16 XGM_getManualSync()
{
    return driverFlags & DRIVER_FLAG_MANUALSYNC_XGM;
}

void XGM_setManualSync(u16 value)
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

u16 XGM_getForceDelayDMA()
{
    return driverFlags & DRIVER_FLAG_DELAYDMA_XGM;
}

void XGM_setForceDelayDMA(u16 value)
{
    // nothing to do
    if (currentDriver != Z80_DRIVER_XGM)
        return;

    if (value)
        driverFlags |= DRIVER_FLAG_DELAYDMA_XGM;
    else
        driverFlags &= ~DRIVER_FLAG_DELAYDMA_XGM;
}

u16 XGM_getMusicTempo()
{
    return xgmTempo;
}

void XGM_setMusicTempo(u16 value)
{
    xgmTempo = value;
    if (IS_PALSYSTEM) xgmTempoDef = 50;
    else xgmTempoDef = 60;
}

u32 XGM_getElapsed()
{
    vu8 *pb;
    u8 *dst;
    u8 values[3];
    u32 result;

    // nothing to do (driver should be loaded here)
    if (currentDriver != Z80_DRIVER_XGM)
        return 0;

    // point to ELAPSED value
    pb = (u8 *) (Z80_DRV_PARAMS + 0x90);
    dst = values;

    bool busTaken = Z80_getAndRequestBus(TRUE);

    // copy quickly elapsed time
    *dst++ = *pb++;
    *dst++ = *pb++;
    *dst = *pb;

    if (!busTaken) Z80_releaseBus();

    result = (values[0] << 0) | (values[1] << 8) | ((u32) values[2] << 16);

    // fix possible 24 bit negative value (parsing first extra frame)
    if (result >= 0xFFFFF0) return 0;

    return result;
}

u32 XGM_getCPULoad()
{
    vu8 *pb;
    u16 idle;
    u16 wait;
    u16 ind;
    s16 load;

    // nothing to do (driver should be loaded here)
    if (currentDriver != Z80_DRIVER_XGM)
        return 0;

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

void XGM_nextXFrame(u16 num)
{
    vu16 *pw_bus;
    vu16 *pw_reset;
    vu8 *pb;

    // nothing to do (driver should be loaded here)
    if (currentDriver != Z80_DRIVER_XGM)
        return;

    bool busTaken = Z80_isBusTaken();

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
    // increment frame to process
    *pb += num;

    // release bus
    if (!busTaken) *pw_bus = 0x0000;
}

// VInt processing for XGM driver
void XGM_doVBlankProcess()
{
    vu16 *pw_bus;
    vu16 *pw_reset;
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
    // driver should be loaded here
    bool busTaken = Z80_isBusTaken();

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
    if (!busTaken) *pw_bus = 0x0000;
}
