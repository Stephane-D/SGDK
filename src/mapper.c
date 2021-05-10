#include "config.h"
#include "types.h"

#include "vdp_bg.h"
#include "sprite_eng.h"
#include "bmp.h"

#include "tools.h"

#include "mapper.h"


#define NUM_BANK        8


static u16 banks[NUM_BANK] = {0, 1, 2, 3, 4, 5, 6, 7};
static u16 reg = 0;


u16 SYS_getBank(u16 regionIndex)
{
    return banks[regionIndex];
}

void SYS_setBank(u16 regionIndex, u16 bankIndex)
{
    // check we are in valid region (region 0 is not switchable)
    if ((regionIndex > 0) && (regionIndex < 8))
    {
        // set bank
        *(vu8*)(MAPPER_BASE + (regionIndex * 2)) = bankIndex;
        // store it so we can read it later
        banks[regionIndex] = bankIndex;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
        KLog_U2("Region #", regionIndex, " set to bank ", bankIndex);
#endif
    }
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    else
        KLog_U1("Cannot set bank in region #", regionIndex);
#endif
}


static bool needBankSwitch(u32 addr)
{
    const u16 mask = (addr >> 16) & 0xFFF0;
    return (mask >= 0x0030) && (mask < 0xE0E0);
}

static u32 setBank(u32 addr)
{
    // get 512 KB bank index
    const u16 bankIndex = (addr >> 19) & 0x3F;

    // check if bank is already set ?
    if (banks[6] == bankIndex)
    {
        reg = 1;
        return 0x300000;
    }
    if (banks[7] == bankIndex)
    {
        reg = 0;
        return 0x380000;
    }

    if (reg)    // use region 7
    {
        // set bank
        SYS_setBank(7, bankIndex);
        // next time we will use other region
        reg = 0;

        // return bank address
        return 0x380000;
    }
    else        // use region 6
    {
        // set bank
        SYS_setBank(6, bankIndex);
        // next time we will use other region
        reg = 1;

        // return bank address
        return 0x300000;
    }
}

static u32 setBankEx(u32 addr, u16 bank)
{
    // get 512 KB bank index
    const u16 bankIndex = (addr >> 19) & 0x3F;

    // check if bank is already set ?
    if (banks[6 + bank] == bankIndex)
    {
        reg = 1 - bank;
        return bank?0x380000:0x300000;
    }

    if (bank)    // use region 7
    {
        // set bank
        SYS_setBank(7, bankIndex);
        // next time we will use other region
        reg = 0;
        // bank 6 point on same bank ? --> force reset so we will stay on bank 7 for this region
        if (banks[6] == bankIndex) banks[6] = 0;

        // return bank address
        return 0x380000;
    }
    else        // use region 6
    {
        // set bank
        SYS_setBank(6, bankIndex);
        // next time we will use other region
        reg = 1;

        // return bank address
        return 0x300000;
    }
}

static u32 setBanks(u32 addr)
{
    // get 512 KB bank index
    const u16 bankIndex = (addr >> 19) & 0x3F;

    // special case of 0x28xxxx-0x3xxxxx range ?
    if (bankIndex == 5)
    {
        // only set second bank
        SYS_setBank(6, 6);
        // return bank address
        return 0x280000;
    }

    // set the 2 banks as we have data crossing banks
    SYS_setBank(6, bankIndex + 0);
    SYS_setBank(7, bankIndex + 1);

    // return bank address
    return 0x300000;
}

void* SYS_getFarData(void* data)
{
    // convert to address
    const u32 addr = (u32) data;

    // don't require bank switch --> return direct pointer
    if (!needBankSwitch(addr)) return data;

    // set bank and get mapped address
    const u32 mappedAddr = setBank(addr) + (addr & BANK_IN_MASK);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
//    {
//        char str[64];
//
//        sprintf(str, "Data at %8X accessed through bank switch from %8X", addr, mappedAddr);
//        KLog(str);
//    }
#endif

    // return it
    return (void*) mappedAddr;
}

void* SYS_getFarDataEx(void* data, u16 bank)
{
    // convert to address
    const u32 addr = (u32) data;

    // don't require bank switch --> return direct pointer
    if (!needBankSwitch(addr)) return data;

    // set bank and get mapped address
    const u32 mappedAddr = setBankEx(addr, bank & 1) + (addr & BANK_IN_MASK);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
//    {
//        char str[64];
//
//        sprintf(str, "Data at %8X accessed through bank switch from %8X", addr, mappedAddr);
//        KLog(str);
//    }
#endif

    // return it
    return (void*) mappedAddr;
}

static bool isCrossingBank(u32 start, u32 end)
{
    // return TRUE if crossing bank
    return ((start ^ end) & BANK_OUT_MASK)?TRUE:FALSE;
}

bool SYS_isCrossingBank(void* data, u32 size)
{
    u32 start = (u32) data;
    u32 end = start + (size - 1);

    return isCrossingBank(start, end);
}

void* SYS_getFarDataSafe(void* data, u32 size)
{
    u32 start = (u32) data;
    u32 end = start + (size - 1);

    // crossing bank ? --> need to use 2 banks
    if (isCrossingBank(start, end))
    {
        // don't require bank switch (better to test on end address) --> return direct pointer
        if (!needBankSwitch(end)) return data;

        // set bank and get mapped address
        const u32 mappedAddr = setBanks(start) + (start & BANK_IN_MASK);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
//    {
//        char str[64];
//
//        sprintf(str, "Data at %8X:%8X accessed through bank switch from %8X", start, end, mappedAddr);
//        KLog(str);
//    }
#endif

        // return it
        return (void*) mappedAddr;
    }

    // use the simpler method
    return SYS_getFarData(data);
}

void* SYS_getFarDataSafeEx(void* data, u32 size, u16 bank)
{
    u32 start = (u32) data;
    u32 end = start + (size - 1);

    // crossing bank ? --> need to use 2 banks
    if (isCrossingBank(start, end))
    {
        // don't require bank switch (better to test on end address) --> return direct pointer
        if (!needBankSwitch(end)) return data;

        // set bank and get mapped address
        const u32 mappedAddr = setBanks(start) + (start & BANK_IN_MASK);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
//    {
//        char str[64];
//
//        sprintf(str, "Data at %8X:%8X accessed through bank switch from %8X", start, end, mappedAddr);
//        KLog(str);
//    }
#endif

        // return it
        return (void*) mappedAddr;
    }

    // use the simpler method
    return SYS_getFarDataEx(data, bank);
}
