#include "config.h"
#include "types.h"

#include "maths.h"

#include "tab_cnv.h"
#include "vdp.h"


FORCE_INLINE u32 mulu(u16 op1, u16 op2)
{
    return op1 * op2;
}

FORCE_INLINE s32 muls(s16 op1, s16 op2)
{
    return op1 * op2;
}

//FORCE_INLINE u32 mulu(u16 op1, u16 op2)
//{
//    u32 result = op1;
//    asm ("mulu.w %1, %0"
//         : "+d" (result)
//         : "d" (op2)
//         : "cc");
//    return result;
//}
//
//FORCE_INLINE s32 muls(s16 op1, s16 op2)
//{
//    s32 result = op1;
//    asm ("muls.w %1, %0"
//         : "+d" (result)
//         : "d" (op2)
//         : "cc");
//    return result;
//}

FORCE_INLINE u16 divu(u32 op1, u16 op2)
{
    u32 result = op1;
    asm ("divu.w %1, %0"
         : "+d" (result)
         : "d" (op2)
         : "cc");
    return result;
}

FORCE_INLINE s16 divs(s32 op1, s16 op2)
{
    s32 result = op1;
    asm ("divs.w %1, %0"
         : "+d" (result)
         : "d" (op2)
         : "cc");
    return result;
}

FORCE_INLINE u16 modu(u32 op1, u16 op2)
{
    u32 result = op1;
    asm ("divu.w %1, %0\n"
         "swap %0"
         : "+d" (result)
         : "d" (op2)
         : "cc");
    return result;
}

FORCE_INLINE s16 mods(s32 op1, s16 op2)
{
    s32 result = op1;
    asm ("divs.w %1, %0\n"
         "swap %0"
         : "+d" (result)
         : "d" (op2)
         : "cc");
    return result;
}

FORCE_INLINE u32 divmodu(u32 op1, u16 op2)
{
    u32 result = op1;
    asm ("divu.w %1, %0"
         : "+d" (result)
         : "d" (op2)
         : "cc");
    return result;
}

FORCE_INLINE s32 divmods(s32 op1, s16 op2)
{
    s32 result = op1;
    asm ("divs.w %1, %0"
         : "+d" (result)
         : "d" (op2)
         : "cc");
    return result;
}


FORCE_INLINE fix16 intToFix16(s16 value)
{
    return value << FIX16_FRAC_BITS;
}

FORCE_INLINE s16 fix16ToInt(fix16 value)
{
    return value >> FIX16_FRAC_BITS;
}

FORCE_INLINE fix16 fix16Round(fix16 value)
{
    return fix16Int(value + (FIX16(0.5) - 1));
}

FORCE_INLINE s16 fix16ToRoundedInt(fix16 value)
{
    return fix16ToInt(value + (FIX16(0.5) - 1));
}

FORCE_INLINE fix16 fix16Frac(fix16 value)
{
    return value & FIX16_FRAC_MASK;
}

FORCE_INLINE fix16 fix16Int(fix16 value)
{
    return value & FIX16_INT_MASK;
}


FORCE_INLINE fix16 fix16Mul(fix16 val1, fix16 val2)
{
    return muls(val1, val2) >> FIX16_FRAC_BITS;
}

FORCE_INLINE fix16 fix16Div(fix16 val1, fix16 val2)
{
    return divs(val1 << FIX16_FRAC_BITS, val2);
}

FORCE_INLINE fix16 fix16Avg(fix16 val1, fix16 val2)
{
    return (val1 + val2) >> 1;
}


FORCE_INLINE fix16 fix16Log2(fix16 value)
{
    return log2tab_f16[value];
}

FORCE_INLINE fix16 fix16Log10(fix16 value)
{
    return log10tab_f16[value];
}

FORCE_INLINE fix16 fix16Sqrt(fix16 value)
{
    return sqrttab_f16[value];
}

FORCE_INLINE fix16 sinFix16(u16 value)
{
    return sintab_f16[value & 1023];
}

FORCE_INLINE fix16 cosFix16(u16 value)
{
    return sintab_f16[(value + 256) & 1023];
}


FORCE_INLINE fix32 intToFix32(s32 value)
{
    return value << FIX32_FRAC_BITS;
}

FORCE_INLINE s32 fix32ToInt(fix32 value)
{
    return value >> FIX32_FRAC_BITS;
}

FORCE_INLINE fix32 fix32Round(fix32 value)
{
    return fix32Int(value + (FIX32(0.5) - 1));
}

FORCE_INLINE s32 fix32ToRoundedInt(fix32 value)
{
    return fix32ToInt(value + (FIX32(0.5) - 1));
}

FORCE_INLINE fix32 fix32Frac(fix32 value)
{
    return value & FIX32_FRAC_MASK;
}

FORCE_INLINE fix32 fix32Int(fix32 value)
{
    return value & FIX32_INT_MASK;
}


FORCE_INLINE fix32 fix32Mul(fix32 val1, fix32 val2)
{
    fix32 v1 = val1 >> (FIX32_FRAC_BITS / 2);
    fix32 v2 = val2 >> (FIX32_FRAC_BITS / 2);

    return v1 * v2;
}

FORCE_INLINE fix32 fix32Div(fix32 val1, fix32 val2)
{
    fix32 v1 = val1 << (FIX32_FRAC_BITS / 2);
    fix32 v2 = val2 >> (FIX32_FRAC_BITS / 2);

    return v1 / v2;
}

FORCE_INLINE fix32 fix32Avg(fix32 val1, fix32 val2)
{
    return (val1 + val2) >> 1;
}


FORCE_INLINE fix16 fix32ToFix16(fix32 value)
{
    return value >> (FIX32_FRAC_BITS - FIX16_FRAC_BITS);
}

FORCE_INLINE fix32 fix16ToFix32(fix16 value)
{
    return value << (FIX32_FRAC_BITS - FIX16_FRAC_BITS);
}


FORCE_INLINE fix32 sinFix32(u16 value)
{
    return sintab_f32[value & 1023];
}

FORCE_INLINE fix32 cosFix32(u16 value)
{
    return sintab_f32[(value + 256) & 1023];
}


FORCE_INLINE fastfix16 intToFastFix16(s16 value)
{
    return value << FASTFIX16_FRAC_BITS;
}

FORCE_INLINE s16 fastFix16ToInt(fastfix16 value)
{
    return value >> FASTFIX16_FRAC_BITS;
}

FORCE_INLINE fastfix16 fastFix16Round(fastfix16 value)
{
    return fastFix16Int(value + (FASTFIX16(0.5) - 1));
}

FORCE_INLINE s16 fastFix16ToRoundedInt(fastfix16 value)
{
    return fastFix16ToInt(value + (FASTFIX16(0.5) - 1));
}

FORCE_INLINE fastfix16 fastFix16Frac(fastfix16 value)
{
    return value & FASTFIX16_FRAC_MASK;
}

FORCE_INLINE fastfix16 fastFix16Int(fastfix16 value)
{
    return value & FASTFIX16_INT_MASK;
}


FORCE_INLINE fastfix16 fastFix16Mul(fastfix16 val1, fastfix16 val2)
{
     return muls(val1, val2) >> FASTFIX16_FRAC_BITS;
}

FORCE_INLINE fastfix16 fastFix16Div(fastfix16 val1, fastfix16 val2)
{
     return divs(val1 << FASTFIX16_FRAC_BITS, val2);
}


FORCE_INLINE fastfix32 intToFastFix32(s16 value)
{
    return value << FASTFIX32_FRAC_BITS;
}

FORCE_INLINE s16 fastFix32ToInt(fastfix32 value)
{
    return value >> FASTFIX32_FRAC_BITS;
}

FORCE_INLINE fastfix32 fastFix32Round(fastfix32 value)
{
    return fastFix32Int(value + (FASTFIX32(0.5) - 1));
}

FORCE_INLINE s32 fastFix32ToRoundedInt(fastfix32 value)
{
    return fastFix32ToInt(value + (FASTFIX32(0.5) - 1));
}

FORCE_INLINE fastfix32 fastFix32Frac(fastfix32 value)
{
    return value & FASTFIX32_FRAC_MASK;
}

FORCE_INLINE fastfix32 fastFix32Int(fastfix32 value)
{
    return value & FASTFIX32_INT_MASK;
}

FORCE_INLINE fastfix32 fastFix32Mul(fastfix32 val1, fastfix32 val2)
{
    fastfix32 v1 = val1 >> (FASTFIX32_FRAC_BITS / 2);
    fastfix32 v2 = val2 >> (FASTFIX32_FRAC_BITS / 2);

    return v1 * v2;
}

FORCE_INLINE fastfix32 fastFix32Div(fastfix32 val1, fastfix32 val2)
{
    fastfix32 v1 = val1 << (FASTFIX32_FRAC_BITS / 2);
    fastfix32 v2 = val2 >> (FASTFIX32_FRAC_BITS / 2);

    return v1 / v2;
}

u32 u16ToBCD(u16 value)
{
    u16 v = value;
    u32 res;

    if (v >= 100)
    {
        const u32 dm = divmodu(v, 100);
        const u16 m = dm >> 16;
        v = dm;
        res = cnv_bcd_tab[m];
    }
    else return cnv_bcd_tab[v];

    if (v >= 100)
    {
        const u32 dm = divmodu(v, 100);
        const u16 m = dm >> 16;
        v = dm;
        res |= cnv_bcd_tab[m] << 8;
    }
    else return (res | (cnv_bcd_tab[v] << 8));

    return (res | (cnv_bcd_tab[v] << 16));
}

u32 u32ToBCD(u32 value)
{
    if (value > 99999999) return 0x99999999;

    if (value >= 65536)
    {
        const u16 m = value % 10000;
        const u16 d = value / 10000;
        return (u16ToBCD(d) << 16) | u16ToBCD(m);
    }

    return u16ToBCD(value);
}

u32 getApproximatedDistance(s32 dx, s32 dy)
{
    u32 min, max;

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    if (dx < dy)
    {
        min = dx;
        max = dy;
    }
    else
    {
        min = dy;
        max = dx;
    }

    // coefficients equivalent to ((123/128) * max) and ((51/128) * min)
    return ((max << 8) + (max << 3) - (max << 4) - (max << 1) +
             (min << 7) - (min << 5) + (min << 3) - (min << 1)) >> 8;
}

u32 getApproximatedDistanceV(V2s32* v2)
{
    return getApproximatedDistance(v2->x, v2->y);

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

    // keep only low 16 bit
    v = value;

    if (v >= 0x0100)
    {
        result += 8;
        v >>= 8;
    }
    if (v >= 0x0010)
    {
        result += 4;
        v >>= 4;
    }
    if (v >= 0x0004)
    {
        result += 2;
        v >>= 2;
    }

    return result | (v >> 1);
}

u32 getNextPow2(u32 value)
{
    u32 result = value - 1;

    result |= result >> 1;
    result |= result >> 2;
    result |= result >> 4;
    result |= result >> 8;
    result |= result >> 16;

    return result + 1;
}
