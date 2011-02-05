#include "config.h"
#include "types.h"

#include "z80_ctrl.h"

#include "tools.h"
#include "timer.h"

// Z80 drivers
#include "z80_drv1.h"
#include "z80_drv2.h"
#include "z80_drv3.h"
#include "z80_drv4.h"
#include "z80_mvst.h"
#include "z80_tfm.h"

#include "tab_vol.h"
#include "smp_null.h"


static u16 currentDriver;


void Z80_init()
{
    // request Z80 bus
    Z80_requestBus(1);
    // set bank to 0
    Z80_setBank(0);

    // no loaded driver
    currentDriver = Z80_DRIVER_NULL;
}


u16 Z80_isBusTaken()
{
    vu16 *pw;

    pw = (u16 *) Z80_HALT_PORT;
    if (*pw & 0x0100) return 0;
    else return 1;
}

void Z80_requestBus(u16 wait)
{
    vu16 *pw_bus;
    vu16 *pw_reset;

    // request bus (need to end reset)
    pw_bus = (u16 *) Z80_HALT_PORT;
    pw_reset = (u16 *) Z80_RESET_PORT;

    // bus not yet taken ?
    if (*pw_bus & 0x100)
    {
        *pw_bus = 0x0100;
        *pw_reset = 0x0100;

        if (wait)
        {
            // wait for bus taken
            while (*pw_bus & 0x0100);
        }
    }
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


void Z80_upload(const u16 dest, const u8 *data, const u16 size, const u16 resetz80)
{
    Z80_requestBus(1);

    // copy data to Z80 RAM
    memcpy((u8 *) (Z80_RAM + dest), data, size);

    if (resetz80) Z80_startReset();
    Z80_releaseBus();
    if (resetz80) Z80_endReset();
}

void Z80_download(const u16 from, u8 *dest, const u16 size)
{
    Z80_requestBus(1);

    // copy data from Z80 RAM
    memcpy(dest, (u8 *) (Z80_RAM + from), size);

    Z80_releaseBus();
}


u16 Z80_getLoadedDriver()
{
    return currentDriver;
}

void Z80_loadDriver(const u16 driver, const u16 waitReady)
{
    const u8 *drv;
    u16 len;

    // already loaded
    if (currentDriver == driver) return;

    switch(driver)
    {
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

        case Z80_DRIVER_4PCM_VOL:
            drv = z80_drv4;
            len = sizeof(z80_drv4);
            break;

        case Z80_DRIVER_MVS:
            drv = z80_mvst;
            len = sizeof(z80_mvst);
            break;

        case Z80_DRIVER_TFM:
            drv = z80_tfm;
            len = sizeof(z80_tfm);
            break;

        default:
            // no valid driver to load
            return;
    }

    // upload Z80 driver and reset Z80
    Z80_upload(0, drv, len, 1);

    // driver initialisation
    switch(driver)
    {
        vu8 *pb;
        u32 addr;

        case Z80_DRIVER_2ADPCM:
        case Z80_DRIVER_4PCM:
            // misc parameters initialisation
            Z80_requestBus(1);
            // point to Z80 null sample parameters
            pb = (u8 *) (Z80_DRV_PARAMS + 0x20);

            addr = (u32) smp_null;
            // null sample address (128 bytes aligned)
            pb[0] = addr >> 7;
            pb[1] = addr >> 15;
            // null sample length (128 bytes aligned)
            pb[2] = sizeof(smp_null) >> 7;
            pb[3] = sizeof(smp_null) >> 15;
            Z80_releaseBus();
            break;

        case Z80_DRIVER_4PCM_VOL:
            // load volume table
            Z80_upload(0x1000, tab_vol, 0x1000, 0);

            // misc parameters initialisation
            Z80_requestBus(1);
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
    }

    // wait driver for being ready ?
    if (waitReady)
    {
        Z80_releaseBus();

        // just wait for it
        while (!Z80_isDriverReady())
            waitSubTick(1);
    }

    // new driver set
    currentDriver = driver;
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
        Z80_requestBus(1);
        ret = *pb & Z80_DRV_STAT_READY;
        Z80_releaseBus();
    }

    return ret;
}
