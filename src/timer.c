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

// PAL           |V28              |V30
//|VCounter      |[1]0x00-0xFF,    |[1]0x00-0xFF,
//|progression   |[2]0x00-0x02,    |[2]0x00-0x0A,
//|              |[3]0xCA-0xFF     |[3]0xD2-0xFF
// NTSC           |V28             |V30
//|VCounter      |[1]0x00-0xEA     |[1]0x00-0xFF
//|progression   |[2]0xE5-0xFF     |[2]0x00-0xFF

// we don't want to share them outside
extern u16 getAdjustedVCounterInternal(u16 blank, u16 vcnt);

vu32 vtimer;

static u32 timer[MAXTIMER];
static u32 lastTick = 0;


// return elapsed time from console reset (1/76800 second based)
// WARNING : this function isn't accurate because of the VCounter rollback during VBlank
u32 getSubTickInternal(u16 blank, u16 vcnt, u32 vt)
{
    u16 vc = getAdjustedVCounterInternal(blank, vcnt);
    u32 current = (vt << 8) + vc;

    // possible only if vtimer not yet increase while in vblank --> fix
    if (current < lastTick) current += 256;
    lastTick = current;

    if (IS_PALSYSTEM) return current * 6;
    else return current * 5;
}

u32 getSubTick()
{
    return getSubTickInternal(GET_VDPSTATUS(VDP_VBLANK_FLAG), GET_VCOUNTER, vtimer);
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
// WARNING: this function isn't accurate during VBlank (always wait until the end of VBlank) because of the VCounter rollback
void waitSubTick(u32 subtick)
{
    // waitSubTick(...) can not be used from V-Int callback or when HV counter is latched
    // also it doesn't work during VBlank so always use alternative method for small wait
    if ((subtick < 150) || (SYS_getInterruptMaskLevel() >= 6) || VDP_getHVLatching())
    {
        u32 i = subtick;

        while(i--)
        {
            u32 tmp;

            // TODO: use cycle accurate wait loop in asm (about 100 cycles for 1 subtick)
            asm volatile ("moveq #7,%0\n"
                "1:\n\t"
                "dbra %0,1b\n\t"
                : "=d" (tmp) : : "cc"
            );
        }

        return;
    }

    const u32 start = getSubTick();
    u32 max = start + subtick;
    u32 current;

    // need to check for overflow
    if (max < start) max = 0xFFFFFFFF;

    // wait until we reached subtick
    do
    {
        s32 remain;

        current = getSubTick();

        // shouldn't happen anymore (we take care of VCounter rollback in getSubTick()) but just for safety
        if (current < start) current = start;

        // still one frame to wait ?
        remain = max - current;
        if (remain >= 1280)
            // do vblank process (maintain compatiblity with previous SGDK)
            SYS_doVBlankProcess();
    }
    while (current < max);
}

// wait for a certain amount of tick
void waitTick(u32 tick)
{
    // waitTick(...) can not be called from V-Int callback or when V-Int is disabled
    if (SYS_getInterruptMaskLevel() >= 6)
    {
        // cannot wait more than that using sub tick
        if (tick >= 0xFFFFFF) waitSubTick(0xFFFFFFFF);
        else waitSubTick(tick * 256);
        return;
    }

    const u32 start = getTick();
    u32 max = start + tick;
    u32 current;

    // need to check for overflow
    if (max < start) max = 0xFFFFFFFF;

    // wait until we reached subtick
    do
    {
        s32 remain;

        current = getTick();

        // still one frame to wait ?
        remain = max - current;
        if (remain >= 5)
            // do vblank process (maintain compatiblity with previous SGDK)
            SYS_doVBlankProcess();
    }
    while (current < max);
}

// wait for a certain amount of millisecond (~3.33 ms based timer when wait is >= 100ms)
void waitMs(u32 ms)
{
    // try "accurate" wait for small amount of time
    if (ms < 100)
        waitSubTick((ms * SUBTICKPERSECOND) / 1000);
    else
        waitTick((ms * TICKPERSECOND) / 1000);
}
