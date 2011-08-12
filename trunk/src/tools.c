/**
 * \file tools.c
 * \brief Misc tools methods
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides some misc tools methods as getFPS()
 */

#include "config.h"
#include "types.h"

#include "tools.h"

#include "maths.h"
#include "string.h"
#include "timer.h"
#include "vdp_bg.h"


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


void showFPS(u16 float_display)
{
    char str[16];

    if (float_display)
    {
        fix32ToStr(getFPS_f(), str, 1);
        VDP_clearText(2, 1, 5);
    }
    else
    {
        uintToStr(getFPS(), str, 1);
        VDP_clearText(2, 1, 2);
    }

    // display FPS
    VDP_drawText(str, 1, 1);
}
