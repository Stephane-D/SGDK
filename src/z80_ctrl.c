#include "config.h"
#include "types.h"

#include "z80_ctrl.h"

#include "ym2612.h"
#include "psg.h"
#include "memory.h"
#include "timer.h"
#include "sys.h"
#include "vdp.h"
#include "tools.h"


// driver(s) flags
#define DRIVER_FLAG_DELAY_DMA    (1 << 0)


// we don't want to share it
extern vu16 VBlankProcess;

u16 currentDriver;
u16 driverFlags;
u16 busProtectSignalAddress;


// we don't want to share them
extern void SND_NULL_unloadDriver(void);
extern void SND_PCM_unloadDriver(void);
extern void SND_DPCM2_unloadDriver(void);
extern void SND_PCM4_unloadDriver(void);
extern void XGM_unloadDriver(void);
extern void XGM2_unloadDriver(void);

extern void SND_NULL_loadDriver(void);
extern void SND_PCM_loadDriver(const bool waitReady);
extern void SND_DPCM2_loadDriver(const bool waitReady);
extern void SND_PCM4_loadDriver(const bool waitReady);
extern void XGM_loadDriver(const bool waitReady);
extern void XGM2_loadDriver(const bool waitReady);


NO_INLINE void Z80_init()
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


NO_INLINE void Z80_clear()
{
    SYS_disableInts();
    bool busTaken = Z80_getAndRequestBus(TRUE);

    const u8 zero = getZeroU8();
    vu8* dst = (u8*) Z80_RAM;
    u16 len = Z80_RAM_LEN;

    while(len--) *dst++ = zero;

    // release bus
    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

NO_INLINE void Z80_upload(const u16 to, const u8 *from, const u16 size)
{
    SYS_disableInts();
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // copy data to Z80 RAM (need to use byte copy here)
    u8* src = (u8*) from;
    vu8* dst = (u8*) (Z80_RAM + to);
    u16 len = size;

    while(len--) *dst++ = *src++;

    // release bus
    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}

NO_INLINE void Z80_download(const u16 from, u8 *to, const u16 size)
{
    SYS_disableInts();
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // copy data from Z80 RAM (need to use byte copy here)
    vu8* src = (u8*) (Z80_RAM + from);
    u8* dst = (u8*) to;
    u16 len = size;

    while(len--) *dst++ = *src++;

    // release bus
    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();
}


u16 Z80_getLoadedDriver()
{
    return currentDriver;
}

void Z80_unloadDriver()
{
    switch(currentDriver)
    {
        case Z80_DRIVER_PCM:
            SND_PCM_unloadDriver();
            break;

        case Z80_DRIVER_DPCM2:
            SND_DPCM2_unloadDriver();
            break;

        case Z80_DRIVER_PCM4:
            SND_PCM4_unloadDriver();
            break;

        case Z80_DRIVER_XGM:
            XGM_unloadDriver();
            break;

        case Z80_DRIVER_XGM2:
            XGM2_unloadDriver();
            break;
    }

    // load NULL driver
    SND_NULL_loadDriver();
    currentDriver = Z80_DRIVER_NULL;
}

NO_INLINE void Z80_loadDriverInternal(const u8 *drv, u16 size)
{
    SYS_disableInts();
    Z80_requestBus(TRUE);

    // reset sound chips
    YM2612_reset();
    PSG_reset();

    // clear z80 memory
    Z80_clear();
    // upload Z80 driver
    Z80_upload(0, drv, size);

    // reset Z80
    Z80_startReset();
    Z80_releaseBus();
    // wait a bit so Z80 reset completed
    waitSubTick(50);
    Z80_endReset();
    SYS_enableInts();
}

NO_INLINE void Z80_loadCustomDriver(const u8 *drv, u16 size)
{
    Z80_unloadDriver();
    Z80_loadDriverInternal(drv, size);

    // custom driver set
    currentDriver = Z80_DRIVER_CUSTOM;
}

NO_INLINE void Z80_loadDriver(const u16 driver, const bool waitReady)
{
    // already loaded
    if (currentDriver == driver) return;

    Z80_unloadDriver();

    switch(driver)
    {
        case Z80_DRIVER_NULL:
            SND_NULL_loadDriver();
            break;

        case Z80_DRIVER_PCM:
            SND_PCM_loadDriver(waitReady);
            break;

        case Z80_DRIVER_DPCM2:
            SND_DPCM2_loadDriver(waitReady);
            break;

        case Z80_DRIVER_PCM4:
            SND_PCM4_loadDriver(waitReady);
            break;

        case Z80_DRIVER_XGM:
            XGM_loadDriver(waitReady);
            break;

        case Z80_DRIVER_XGM2:
            XGM2_loadDriver(waitReady);
            break;
    }

    // new driver set
    currentDriver = driver;
}


bool Z80_isDriverReady()
{
    // point to Z80 status
    vu8* pb = (vu8*) Z80_DRV_STATUS;

    SYS_disableInts();
    // request Z80 BUS
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // ready status
    bool ret = (*pb & Z80_DRV_STAT_READY)?TRUE:FALSE;

    if (!busTaken) Z80_releaseBus();
    SYS_enableInts();

    return ret;
}


void Z80_useBusProtection(u16 signalAddress)
{
    busProtectSignalAddress = signalAddress;
}

void Z80_setBusProtection(bool value)
{
    // bus protection not defined ? --> exit
    if (!busProtectSignalAddress)
        return;

    // point to Z80 PROTECT parameter
    vu8* pb = (vu8*) (Z80_RAM + busProtectSignalAddress);

    SYS_disableInts();
    bool busTaken = Z80_getAndRequestBus(TRUE);

    *pb = value?1:0;

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
    return driverFlags & DRIVER_FLAG_DELAY_DMA;
}

void Z80_setForceDelayDMA(bool value)
{
    if (value) driverFlags |= DRIVER_FLAG_DELAY_DMA;
    else driverFlags &= ~DRIVER_FLAG_DELAY_DMA;
}
