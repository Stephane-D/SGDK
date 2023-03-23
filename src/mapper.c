#include "config.h"
#include "types.h"

#include "mapper.h"

#include "tools.h"


#define NUM_BANK        8


static u16 banks[NUM_BANK] = {0, 1, 2, 3, 4, 5, 6, 7};
// next region access (FALSE = low region; TRUE = high region)
static bool region;


void SYS_resetBanks()
{
    u16 len = 8;
    while(--len) SYS_setBank(len, len);
    region = FALSE;
}


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

    // low region set last time ?
    if (region)
    {
        // check if bank is already set on low region first
        if (banks[6] == bankIndex)
        {
            // use high region next time
            region = TRUE;
            return 0x300000;
        }
        if (banks[7] == bankIndex)
        {
            // use low region next time
            region = FALSE;
            return 0x380000;
        }

        // set bank through high region
        SYS_setBank(7, bankIndex);
        // use low region next time
        region = FALSE;

        // return bank address
        return 0x380000;
    }
    // high region set last time ?
    else
    {
        // check if bank is already set on high region first
        if (banks[7] == bankIndex)
        {
            // use low region next time
            region = FALSE;
            return 0x380000;
        }
        if (banks[6] == bankIndex)
        {
            // use high region next time
            region = TRUE;
            return 0x300000;
        }

        // set bank through low region
        SYS_setBank(6, bankIndex);
        // use high region next time
        region = TRUE;

        // return bank address
        return 0x300000;
    }
}

static u32 setBankEx(u32 addr, bool high)
{
    // get 512 KB bank index
    const u16 bankIndex = (addr >> 19) & 0x3F;

    // check if bank is already set ?
    if (high)
    {
        // set bank through high region if needed
        if (banks[7] != bankIndex)
            SYS_setBank(7, bankIndex);

        // use low region next time
        region = FALSE;

        // return bank address
        return 0x380000;
    }
    else
    {
        // set bank through low region if needed
        if (banks[6] != bankIndex)
            SYS_setBank(6, bankIndex);

        // use high region next time
        region = TRUE;

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
    kprintf("Data at %8lX accessed through bank switch from %8lX", addr, mappedAddr);
#endif

    // return it
    return (void*) mappedAddr;
}

void* SYS_getFarDataEx(void* data, bool high)
{
    // convert to address
    const u32 addr = (u32) data;

    // don't require bank switch --> return direct pointer
    if (!needBankSwitch(addr)) return data;

    // set bank and get mapped address
    const u32 mappedAddr = setBankEx(addr, high) + (addr & BANK_IN_MASK);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    kprintf("Data at %8lX accessed through bank switch from %8lX", addr, mappedAddr);
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
    const u32 start = (u32) data;
    const u32 end = start + (size - 1);

    return isCrossingBank(start, end);
}

void* SYS_getFarDataSafe(void* data, u32 size)
{
    const u32 start = (u32) data;
    const u32 end = start + (size - 1);

    // crossing bank ? --> need to use 2 banks
    if (isCrossingBank(start, end))
    {
        // don't require bank switch (better to test on end address) --> return direct pointer
        if (!needBankSwitch(end)) return data;

        // set bank and get mapped address
        const u32 mappedAddr = setBanks(start) + (start & BANK_IN_MASK);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
        kprintf("Data at %8lX:%8lX accessed through bank switch from %8lX", start, end, mappedAddr);
#endif

        // return it
        return (void*) mappedAddr;
    }

    // use the simpler method
    return SYS_getFarData(data);
}

void* SYS_getFarDataSafeEx(void* data, u32 size, bool high)
{
    const u32 start = (u32) data;
    const u32 end = start + (size - 1);

    // crossing bank ? --> need to use 2 banks
    if (isCrossingBank(start, end))
    {
        // don't require bank switch (better to test on end address) --> return direct pointer
        if (!needBankSwitch(end)) return data;

        // set bank and get mapped address
        const u32 mappedAddr = setBanks(start) + (start & BANK_IN_MASK);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
        kprintf("Data at %8lX:%8lX accessed through bank switch from %8lX", start, end, mappedAddr);
#endif

        // return it
        return (void*) mappedAddr;
    }

    // use the simpler method
    return SYS_getFarDataEx(data, high);
}

bool SYS_getNextFarAccessRegion()
{
    return region;
}

void SYS_setNextFarAccessRegion(bool high)
{
    region = high;
}
