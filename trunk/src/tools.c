#include "config.h"
#include "types.h"

#include "tools.h"

#include "timer.h"
#include "maths.h"


static u32 framecnt;
static u32 last;

u32 getFPS()
{
    static u32 result;

    const u32 current = getSubTick();
    const u32 delta = current - last;

	if (delta > 19200)
    {
        result = framecnt / delta;
        if (result > 999) result = 999;
        last = current;
        framecnt = 76800;
    }
	else framecnt += 76800;

	return result;
}

fix32 getFPS_f()
{
    static fix32 result;

    const u32 current = getSubTick();
    const u32 delta = current - last;

	if (delta > 19200)
    {
        if (framecnt > (250 * 76800)) result = FIX32(999);
        else
        {
            result = (framecnt << FIX16_FRAC_BITS) / delta;
            if (result > (999 << FIX16_FRAC_BITS)) result = FIX32(999);
            else result <<= (FIX32_FRAC_BITS - FIX16_FRAC_BITS);
        }
        last = current;
        framecnt = 76800;
    }
	else framecnt += 76800;

	return result;
}
