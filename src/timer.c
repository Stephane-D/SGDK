#include "config.h"
#include "types.h"

#include "timer.h"

#include "maths.h"
#include "vdp.h"


u32 vtimer;

static u32 timer[MAXTIMER];


// return elapsed time from console reset (1/76800 second based)
// WARNING : this function isn't accurate because of the VCounter rollback
u32 getSubTick()
{
    u32 vcnt;

    vcnt = GET_VCOUNTER;
    const u32 scrh = VDP_getScreenHeight();
    // as VCounter roolback in blank area we use a "medium" value
    if (vcnt >= scrh) vcnt = 16;
    else vcnt += (256 - scrh);
    const u32 current = (vtimer << 8) + vcnt;

    if (IS_PALSYSTEM) return current * 6;
    else return current * 5;
}

// return elapsed time from console reset (1/300 second based)
u32 getTick()
{
    if (IS_PALSYSTEM) return vtimer * 6;
    else return vtimer * 5;
}

// return elapsed time from console reset (1/256 second based)
u32 getTime(u16 fromTick)
{
    if (fromTick) return (getTick() * TICKPERSECOND) >> 8;
    else return getSubTick() / TICKPERSECOND;
}

fix32 getTimeAsFix32(u16 fromTick)
{
#if (FIX32_FRAC_BITS > 8)
    return getTime(fromTick) << (FIX32_FRAC_BITS - 8);
#else
    return getTime(fromTick) >> (8 - FIX32_FRAC_BITS);
#endif
}


void startTimer(u16 numTimer)
{
    timer[numTimer & (MAXTIMER - 1)] = getSubTick();
}

u32 getTimer(u16 numTimer, u16 restart)
{
    const u32 res = getSubTick() - timer[numTimer & (MAXTIMER - 1)];
    if (restart) startTimer(numTimer);
    return res;
}


// wait for a certain amount of subtick
// WARNING : this function isn't accurate because of the VCounter rollback
void waitSubTick(u32 subtick)
{
    u32 start;
    u32 current;

    start = getSubTick();
    // wait until we reached subtick
    do
    {
        current = getSubTick();
        // error du to the VCounter roolback, ignore...
        if (current < start) current = start;
    } while ((current - start) < subtick);
}

// wait for a certain amount of tick
void waitTick(u32 tick)
{
    u32 start;

    start = getTick();
    // wait until we reached tick
    while ((getTick() - start) < tick);
}

// wait for a certain amount of millisecond (~3.33 ms based timer so use 4 ms at least)
void waitMs(u32 ms)
{
    waitTick((ms * TICKPERSECOND) / 1000);
}
