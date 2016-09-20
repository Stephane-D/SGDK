#include "config.h"
#include "types.h"

#include "maths.h"

#include "tab_cnv.h"
#include "vdp.h"


u32 intToBCD(u32 value)
{
    if (value > 99999999) return 0x99999999;

    if (value < 100) return cnv_bcd_tab[value];

    u32 v;
    u32 res;
    u16 carry;
    u16 cnt;

    v = value;
    res = 0;
    carry = 0;
    cnt = 4;

    while(cnt--)
    {
        u16 bcd = (v & 0xFF) + carry;

        if (bcd > 199)
        {
            carry = 2;
            bcd -= 200;
        }
        else if (bcd > 99)
        {
            carry = 1;
            bcd -= 100;
        }
        else carry = 0;

        if (bcd)
            res |= cnv_bcd_tab[bcd] << ((3 - cnt) * 8);

        // next byte
        v >>= 8;
    }

    return res;
}

u32 distance_approx(s32 dx, s32 dy)
{
    return getApproximatedDistance(dx, dy);
}

u32 getApproximatedDistance(s32 dx, s32 dy)
{
    u32 min, max;

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    if (dx < dy )
    {
        min = dx;
        max = dy;
    }
    else
    {
        min = dy;
        max = dx;
    }

    // coefficients equivalent to ( 123/128 * max ) and ( 51/128 * min )
    return ((max << 8) + (max << 3) - (max << 4) - (max << 1) +
             (min << 7) - (min << 5) + (min << 3) - (min << 1)) >> 8;
}

s32 getApproximatedLog2(s32 value)
{
    s32 x, t, y;

    x = value;
    y = 0xa65af;

    if (x < 0x00008000)
    {
        x <<= 16;
        y -= 0xb1721;
    }
    if (x < 0x00800000)
    {
        x <<= 8;
        y -= 0x58b91;
    }
    if (x < 0x08000000)
    {
        x <<= 4;
        y -= 0x2c5c8;
    }
    if (x < 0x20000000)
    {
        x <<= 2;
        y -= 0x162e4;
    }
    if (x < 0x40000000)
    {
        x <<= 1;
        y -= 0x0b172;
    }

    t = x + (x >> 1);
    if (t >= 0)
    {
        x = t;
        y -= 0x067cd;
    }
    t = x + (x >> 2);
    if (t >= 0)
    {
        x = t;
        y -= 0x03920;
    }
    t = x + (x >> 3);
    if (t >= 0)
    {
        x = t;
        y -= 0x01e27;
    }
    t = x + (x >> 4);
    if (t >= 0)
    {
        x = t;
        y -= 0x00f85;
    }
    t = x + (x >> 5);
    if (t >= 0)
    {
        x = t;
        y -= 0x007e1;
    }
    t = x + (x >> 6);
    if (t >= 0)
    {
        x = t;
        y -= 0x003f8;
    }
    t = x + (x >> 7);
    if (t >= 0)
    {
        x = t;
        y -= 0x001fe;
    }

    x = 0x80000000 - x;
    y -= x >> 15;

    return y;
}

u16 getLog2Int(u32 value)
{
    u16 v;
    u16 result;

    result = 0;
    if (value > 0xFFFF) result = 16;

    v = value; // keep only low 16 bit
    if (v > 0xFF) result += 8;
    v >>= 8;
    if (v > 0xF) result += 4;
    v >>= 4;
    if (v > 0x3) result += 2;

    return result | (v >> 3);
}
