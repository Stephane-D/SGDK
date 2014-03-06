#include "config.h"
#include "types.h"

#include "timer.h"

#include "maths.h"
#include "vdp.h"
#include "sys.h"


// TODO : use H counter for more accurate timer
//
// H Counter values
//
// Description            32-cell         40-cell
// ----------------------------------------------------------------------------
// Range                  00-93, E9-FF    00-B6, E4-FF
// Display area           00-7F           00-9F
// V counter increment    84, 85          A4, A5
// V-blanking in          86, 87          A7, A8
// V-blanking out         86, 87          A7, A8
// H-blanking in          93, E9          B2, E4
// H-blanking out         06, 07          06, 07


u32 vtimer;

static u32 timer[MAXTIMER];


// return elapsed time from console reset (1/76800 second based)
// WARNING : this function isn't accurate because of the VCounter rollback
u32 getSubTick()
{
    u32 vcnt;

    vcnt = GET_VCOUNTER;
    const u32 scrh = screenHeight;

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
    u32 result;

    if (fromTick) result = getTick() << 8;
    else result = getSubTick();

    return result / TICKPERSECOND;
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
    const u32 t = getSubTick();
    u32* time = &timer[numTimer & (MAXTIMER - 1)];
    const u32 res = t - *time;

    if (restart) *time = t;

    return res;
}


// wait for a certain amount of subtick
// WARNING : this function isn't accurate because of the VCounter rollback
void waitSubTick(u32 subtick)
{
    u32 start;
    u32 current;
    u32 i;

    // waitSubTick(...) can not be called from V-Int callback or when V-Int is disabled
    if (SYS_getInterruptMaskLevel() >= 6)
    {
        i = subtick;

        // TODO: use cycle accurate wait loop in asm
        while(i--)
        {
            asm("nop");
            asm("nop");
            asm("nop");
            asm("nop");
        }

        return;
    }

    start = getSubTick();

    // wait until we reached subtick
    do
    {
        current = getSubTick();

        // error du to the VCounter roolback, ignore...
        if (current < start) current = start;
    }
    while ((current - start) < subtick);
}

// wait for a certain amount of tick
void waitTick(u32 tick)
{
    u32 start;
    u32 i;

    // waitTick(...) can not be called from V-Int callback or when V-Int is disabled
    if (SYS_getInterruptMaskLevel() >= 6)
    {
        i = tick;

        while(i--) waitSubTick(256);

        return;
    }

    start = getTick();

    // wait until we reached tick
    while ((getTick() - start) < tick);
}

// wait for a certain amount of millisecond (~3.33 ms based timer so use 4 ms at least)
void waitMs(u32 ms)
{
    waitTick((ms * TICKPERSECOND) / 1000);
}
