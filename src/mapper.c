#include "config.h"
#include "types.h"

#include "vdp_bg.h"
#include "sprite_eng.h"
#include "bmp.h"

#include "mapper.h"

#define NUM_BANK        8


static u16 banks[NUM_BANK] = {0, 0, 0, 0, 0, 0, 0, 0};
static u16 reg = 0;


u16 BANK_getBank(u16 regionIndex)
{
    return banks[regionIndex];
}

void BANK_setBank(u16 regionIndex, u16 bankIndex)
{
    // check we are in valid region
    if ((regionIndex > 0) && (regionIndex < 8))
    {
        // set bank
        *(vu8*)(MAPPER_BASE + (regionIndex * 2)) = bankIndex;
        // store it so we can read it later
        banks[regionIndex] = bankIndex;
    }
}


static bool needBankSwitch(u32 addr)
{
    return (addr & 0x00700000) >= 0x00300000;
}

static u32 setBank(u32 addr)
{
    // get 512 KB bank index
    const u16 bankIndex = addr >> 11;

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

void* BANK_getFarData(void* data)
{
    // convert to address
    const u32 addr = (u32) data;

    // don't require bank switch --> return direct pointer
    if (!needBankSwitch(addr)) return data;

    // set bank and return adjusted address
    return (void*) (setBank(addr) + (addr & 0x07FFFF));
}
