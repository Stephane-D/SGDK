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
//    if (regionIndex < 8)
    if ((regionIndex > 0) && (regionIndex < 8))
    {
        // set bank
        *(vu8*)(MAPPER_BASE + (regionIndex * 2)) = bankIndex;
        // store it so we can read it later
        banks[regionIndex] = bankIndex;

#if (LIB_DEBUG != 0)
        KLog_U2("Region #", regionIndex, " set to bank #", bankIndex);
#endif
    }
#if (LIB_DEBUG != 0)
    else
        KLog_U1("Cannot set bank in region #", regionIndex);
#endif
}


static bool needBankSwitch(u32 addr)
{
    const u16 mask = (addr >> 16) & 0xF0;
    return (mask >= 0x30) && (mask < 0xC0);
}

static u32 setBank(u32 addr)
{
    // get 512 KB bank index
    const u16 bankIndex = (addr >> 19) & 0x3F;

    // check if bank is already set ?
    if (banks[6] == bankIndex) return 0x300000;
    if (banks[7] == bankIndex) return 0x380000;

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

void* SYS_getFarData(void* data)
{
    // convert to address
    const u32 addr = (u32) data;

    // don't require bank switch --> return direct pointer
    if (!needBankSwitch(addr)) return data;

    // set bank and get mapped address
    const u32 mappedAddr = setBank(addr) + (addr & BANK_MASK);

#if (LIB_DEBUG != 0)
//     KLog_U2("Data at ", addr, " accessed through bank switch from ", mappedAddr);
#endif

    // return it
    return (void*) mappedAddr;
}
