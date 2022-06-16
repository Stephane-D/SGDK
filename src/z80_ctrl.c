#include "config.h"
#include "types.h"

#include "z80_ctrl.h"

#include "ym2612.h"
#include "psg.h"
#include "memory.h"
#include "timer.h"
#include "sys.h"
#include "vdp.h"
#include "sound.h"
#include "xgm.h"

// Z80 drivers
#include "z80_drv0.h"
#include "z80_drv1.h"
#include "z80_drv2.h"
#include "z80_drv3.h"
#include "z80_xgm.h"

#include "tab_vol.h"
#include "smp_null.h"
#include "smp_null_pcm.h"


// driver(s) flags
#define DRIVER_FLAG_DELAYDMA    (1 << 0)


// we don't want to share it
extern vu16 VBlankProcess;

u16 currentDriver;
u16 driverFlags;
u16 busProtectSignalAddress;


// we don't want to share it
extern void XGM_resetLoadCalculation();


void Z80_init()
{
    // request Z80 bus
    Z80_requestBus(TRUE);
    // set bank to 0
    Z80_setBank(0);

    // no loaded driver
    currentDriver = -1;
    driverFlags = 0;
    busProtectSignalAddress = 0;

    // load null/dummy driver as it's important to have Z80 active (state is preserved)
    Z80_loadDriver(Z80_DRIVER_NULL, FALSE);
}


bool Z80_isBusTaken()
{
    vu16 *pw;

    pw = (u16 *) Z80_HALT_PORT;
    if (*pw & 0x0100) return FALSE;
    else return TRUE;
}

void Z80_requestBus(bool wait)
{
    vu16 *pw_bus;
    vu16 *pw_reset;

    // request bus (need to end reset)
    pw_bus = (u16 *) Z80_HALT_PORT;
    pw_reset = (u16 *) Z80_RESET_PORT;

    // take bus and end reset
    *pw_bus = 0x0100;
    *pw_reset = 0x0100;

    if (wait)
    {
        // wait for bus taken
        while (*pw_bus & 0x0100);
    }
}

bool Z80_getAndRequestBus(bool wait)
{
    vu16 *pw_bus;
    vu16 *pw_reset;

    pw_bus = (u16 *) Z80_HALT_PORT;

    // already requested ? just return TRUE
    if (!(*pw_bus & 0x0100)) return TRUE;

    pw_reset = (u16 *) Z80_RESET_PORT;

    // take bus and end reset
    *pw_bus = 0x0100;
    *pw_reset = 0x0100;

    if (wait)
    {
        // wait for bus taken
        while (*pw_bus & 0x0100);
    }

    return FALSE;
}

void Z80_releaseBus()
{
    vu16 *pw;

    pw = (u16 *) Z80_HALT_PORT;
    *pw = 0x0000;
}


void Z80_startReset()
{
    vu16 *pw;

    pw = (u16 *) Z80_RESET_PORT;
    *pw = 0x0000;
}

void Z80_endReset()
{
    vu16 *pw;

    pw = (u16 *) Z80_RESET_PORT;
    *pw = 0x0100;
}


void Z80_setBank(const u16 bank)
{
    vu8 *pb;
    u16 i, value;

    pb = (u8 *) Z80_BANK_REGISTER;

    i = 9;
    value = bank;
    while (i--)
    {
        *pb = value;
        value >>= 1;
    }
}

u8 Z80_read(const u16 addr)
{
    return ((vu8*) Z80_RAM)[addr];
}

void Z80_write(const u16 addr, const u8 value)
{
    ((vu8*) Z80_RAM)[addr] = value;
}


void Z80_clear(const u16 to, const u16 size, const bool resetz80)
{
    Z80_requestBus(TRUE);

    const u8 zero = getZeroU8();
    vu8* dst = (u8*) (Z80_RAM + to);
    u16 len = size;

    while(len--) *dst++ = zero;

    if (resetz80) Z80_startReset();
    Z80_releaseBus();
    // wait bus released
    while(Z80_isBusTaken());
    if (resetz80) Z80_endReset();
}

void Z80_upload(const u16 to, const u8 *from, const u16 size, const bool resetz80)
{
    Z80_requestBus(TRUE);

    // copy data to Z80 RAM (need to use byte copy here)
    u8* src = (u8*) from;
    vu8* dst = (u8*) (Z80_RAM + to);
    u16 len = size;

    while(len--) *dst++ = *src++;

    if (resetz80) Z80_startReset();
    Z80_releaseBus();
    // wait bus released
    while(Z80_isBusTaken());
    if (resetz80) Z80_endReset();
}

void Z80_download(const u16 from, u8 *to, const u16 size)
{
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // copy data from Z80 RAM (need to use byte copy here)
    vu8* src = (u8*) (Z80_RAM + from);
    u8* dst = (u8*) to;
    u16 len = size;

    while(len--) *dst++ = *src++;

    if (!busTaken)
        Z80_releaseBus();
}


u16 Z80_getLoadedDriver()
{
    return currentDriver;
}

void Z80_unloadDriver()
{
    // already unloaded
    if (currentDriver == Z80_DRIVER_NULL) return;

    // clear Z80 RAM
    Z80_clear(0, Z80_RAM_LEN, TRUE);

    currentDriver = Z80_DRIVER_NULL;

    // remove XGM task if present
    VBlankProcess &= ~PROCESS_XGM_TASK;
}

void Z80_loadDriver(const u16 driver, const bool waitReady)
{
    const u8 *drv;
    u16 len;

    // already loaded
    if (currentDriver == driver) return;

    switch(driver)
    {
        case Z80_DRIVER_NULL:
            drv = z80_drv0;
            len = sizeof(z80_drv0);
            break;

        case Z80_DRIVER_PCM:
            drv = z80_drv1;
            len = sizeof(z80_drv1);
            break;

        case Z80_DRIVER_2ADPCM:
            drv = z80_drv2;
            len = sizeof(z80_drv2);
            break;

        case Z80_DRIVER_4PCM:
            drv = z80_drv3;
            len = sizeof(z80_drv3);
            break;

        case Z80_DRIVER_XGM:
            drv = z80_xgm;
            len = sizeof(z80_xgm);
            break;

        default:
            // no valid driver to load
            return;
    }

    // clear z80 memory
    Z80_clear(0, Z80_RAM_LEN, FALSE);
    // upload Z80 driver and reset Z80
    Z80_upload(0, drv, len, TRUE);

    // driver initialisation
    switch(driver)
    {
        vu8 *pb;
        u32 addr;

        case Z80_DRIVER_2ADPCM:
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
            break;

        case Z80_DRIVER_PCM:
            // misc parameters initialisation
            Z80_requestBus(TRUE);
            // point to Z80 null sample parameters
            pb = (u8 *) (Z80_DRV_PARAMS + 0x20);

            addr = (u32) smp_null;
            // null sample address (256 bytes aligned)
            pb[0] = addr >> 8;
            pb[1] = addr >> 16;
            // null sample length (256 bytes aligned)
            pb[2] = sizeof(smp_null) >> 8;
            pb[3] = sizeof(smp_null) >> 16;
            Z80_releaseBus();
            break;

        case Z80_DRIVER_4PCM:
            // load volume table
            Z80_upload(0x1000, tab_vol, 0x1000, 0);

            // misc parameters initialisation
            Z80_requestBus(TRUE);
            // point to Z80 null sample parameters
            pb = (u8 *) (Z80_DRV_PARAMS + 0x20);

            addr = (u32) smp_null;
            // null sample address (256 bytes aligned)
            pb[4] = addr >> 8;
            pb[5] = addr >> 16;
            // null sample length (256 bytes aligned)
            pb[6] = sizeof(smp_null) >> 8;
            pb[7] = sizeof(smp_null) >> 16;
            Z80_releaseBus();
            break;

        case Z80_DRIVER_XGM:
            // reset sound chips
            YM2612_reset();
            PSG_init();

            // misc parameters initialisation
            Z80_requestBus(TRUE);
            // point to Z80 sample id table (first entry = silent sample)
            pb = (u8 *) (0xA01C00);

            addr = (u32) smp_null;
            // null sample address (256 bytes aligned)
            pb[0] = addr >> 8;
            pb[1] = addr >> 16;
            // null sample length (256 bytes aligned)
            pb[2] = sizeof(smp_null) >> 8;
            pb[3] = sizeof(smp_null) >> 16;
            Z80_releaseBus();
            break;
    }

    // wait driver for being ready
    if (waitReady)
    {
        switch(driver)
        {
            // drivers supporting ready status
            case Z80_DRIVER_2ADPCM:
            case Z80_DRIVER_PCM:
            case Z80_DRIVER_4PCM:
            case Z80_DRIVER_XGM:
                Z80_releaseBus();
                // wait bus released
                while(Z80_isBusTaken());

                // just wait for it
                while(!Z80_isDriverReady())
                    waitMs(1);
                break;
        }
    }

    // new driver set
    currentDriver = driver;

    // post init stuff
    switch(driver)
    {
        // XGM driver
        case Z80_DRIVER_XGM:
            // using auto sync --> enable XGM task on VInt
            if (!XGM_getManualSync())
                VBlankProcess |= PROCESS_XGM_TASK;
            // define default XGM tempo (always based on NTSC timing)
            XGM_setMusicTempo(60);
            // reset load calculation
            XGM_resetLoadCalculation();
            // set bus protection signal address
            Z80_useBusProtection((Z80_DRV_PARAMS + 0x0D) & 0xFFFF);
            break;

        default:
            VBlankProcess &= ~PROCESS_XGM_TASK;
            // no bus protection (signal address set to 0)
            Z80_useBusProtection(0);
            break;
    }
}

void Z80_loadCustomDriver(const u8 *drv, u16 size)
{
    // clear z80 memory
    Z80_clear(0, Z80_RAM_LEN, FALSE);
    // upload Z80 driver and reset Z80
    Z80_upload(0, drv, size, TRUE);

    // custom driver set
    currentDriver = Z80_DRIVER_CUSTOM;

    // remove XGM task if present
    VBlankProcess &= ~PROCESS_XGM_TASK;
}

u16 Z80_isDriverReady()
{
    vu8 *pb;
    u8 ret;

    // point to Z80 status
    pb = (u8 *) Z80_DRV_STATUS;

    // bus already taken ? just check status
    if (Z80_isBusTaken())
        ret = *pb & Z80_DRV_STAT_READY;
    else
    {
        // take the bus, check status and release bus
        Z80_requestBus(TRUE);
        ret = *pb & Z80_DRV_STAT_READY;
        Z80_releaseBus();
    }

    return ret;
}


void Z80_useBusProtection(u16 signalAddress)
{
    busProtectSignalAddress = signalAddress;
}

void Z80_setBusProtection(bool value)
{
    vu8 *pb;

    // bus protection not defined ? --> exist
    if (busProtectSignalAddress == 0)
        return;

    SYS_disableInts();
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // point to Z80 PROTECT parameter
    pb = (u8 *) (Z80_RAM + busProtectSignalAddress);
    *pb = value;

    // release bus
    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

void Z80_enableBusProtection()
{
    Z80_setBusProtection(TRUE);
}

void Z80_disableBusProtection()
{
    Z80_setBusProtection(FALSE);
}

bool Z80_getForceDelayDMA()
{
    return driverFlags & DRIVER_FLAG_DELAYDMA;
}

void Z80_setForceDelayDMA(bool value)
{
    if (value) driverFlags |= DRIVER_FLAG_DELAYDMA;
    else driverFlags &= ~DRIVER_FLAG_DELAYDMA;
}

