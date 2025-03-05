#include "config.h"
#include "types.h"
#include "sys.h"

#include "maths.h"

#include "tab_cnv.h"
#include "vdp.h"


const fix16 trigtab_f16[90 + 1] =
{
    FIX16( 0.0000), FIX16( 0.0175), FIX16( 0.0349), FIX16( 0.0523), FIX16( 0.0698), FIX16( 0.0872), FIX16( 0.1045), FIX16( 0.1219), FIX16( 0.1392), FIX16( 0.1564),
	FIX16( 0.1736), FIX16( 0.1908), FIX16( 0.2079), FIX16( 0.2250), FIX16( 0.2419), FIX16( 0.2588), FIX16( 0.2756), FIX16( 0.2924), FIX16( 0.3090), FIX16( 0.3256),
	FIX16( 0.3420), FIX16( 0.3584), FIX16( 0.3746), FIX16( 0.3907), FIX16( 0.4067), FIX16( 0.4226), FIX16( 0.4384), FIX16( 0.4540), FIX16( 0.4695), FIX16( 0.4848),
	FIX16( 0.5000), FIX16( 0.5150), FIX16( 0.5299), FIX16( 0.5446), FIX16( 0.5592), FIX16( 0.5736), FIX16( 0.5878), FIX16( 0.6018), FIX16( 0.6157), FIX16( 0.6293),
	FIX16( 0.6428), FIX16( 0.6561), FIX16( 0.6691), FIX16( 0.6820), FIX16( 0.6947), FIX16( 0.7071), FIX16( 0.7193), FIX16( 0.7314), FIX16( 0.7431), FIX16( 0.7547),
	FIX16( 0.7660), FIX16( 0.7771), FIX16( 0.7880), FIX16( 0.7986), FIX16( 0.8090), FIX16( 0.8192), FIX16( 0.8290), FIX16( 0.8387), FIX16( 0.8480), FIX16( 0.8572),
	FIX16( 0.8660), FIX16( 0.8746), FIX16( 0.8829), FIX16( 0.8910), FIX16( 0.8988), FIX16( 0.9063), FIX16( 0.9135), FIX16( 0.9205), FIX16( 0.9272), FIX16( 0.9336),
	FIX16( 0.9397), FIX16( 0.9455), FIX16( 0.9511), FIX16( 0.9563), FIX16( 0.9613), FIX16( 0.9659), FIX16( 0.9703), FIX16( 0.9744), FIX16( 0.9781), FIX16( 0.9816),
	FIX16( 0.9848), FIX16( 0.9877), FIX16( 0.9903), FIX16( 0.9925), FIX16( 0.9945), FIX16( 0.9962), FIX16( 0.9976), FIX16( 0.9986), FIX16( 0.9994), FIX16( 0.9998),
	FIX16( 1.0000)
};

const fix32 trigtab_f32[(90 * 4) + 1] =
{
    FIX32(0.0), FIX32(0.0043), FIX32(0.0087), FIX32(0.013), FIX32(0.0174), FIX32(0.0218), FIX32(0.0261), FIX32(0.0305), 
    FIX32(0.0348), FIX32(0.0392), FIX32(0.0436), FIX32(0.0479), FIX32(0.0523), FIX32(0.0566), FIX32(0.061), FIX32(0.0654), 
    FIX32(0.0697), FIX32(0.0741), FIX32(0.0784), FIX32(0.0828), FIX32(0.0871), FIX32(0.0915), FIX32(0.0958), FIX32(0.1001), 
    FIX32(0.1045), FIX32(0.1088), FIX32(0.1132), FIX32(0.1175), FIX32(0.1218), FIX32(0.1261), FIX32(0.1305), FIX32(0.1348), 
    FIX32(0.1391), FIX32(0.1434), FIX32(0.1478), FIX32(0.1521), FIX32(0.1564), FIX32(0.1607), FIX32(0.165), FIX32(0.1693), 
    FIX32(0.1736), FIX32(0.1779), FIX32(0.1822), FIX32(0.1865), FIX32(0.1908), FIX32(0.195), FIX32(0.1993), FIX32(0.2036), 
    FIX32(0.2079), FIX32(0.2121), FIX32(0.2164), FIX32(0.2206), FIX32(0.2249), FIX32(0.2292), FIX32(0.2334), FIX32(0.2376), 
    FIX32(0.2419), FIX32(0.2461), FIX32(0.2503), FIX32(0.2546), FIX32(0.2588), FIX32(0.263), FIX32(0.2672), FIX32(0.2714), 
    FIX32(0.2756), FIX32(0.2798), FIX32(0.284), FIX32(0.2881), FIX32(0.2923), FIX32(0.2965), FIX32(0.3007), FIX32(0.3048), 
    FIX32(0.309), FIX32(0.3131), FIX32(0.3173), FIX32(0.3214), FIX32(0.3255), FIX32(0.3296), FIX32(0.3338), FIX32(0.3379), 
    FIX32(0.342), FIX32(0.3461), FIX32(0.3502), FIX32(0.3542), FIX32(0.3583), FIX32(0.3624), FIX32(0.3665), FIX32(0.3705), 
    FIX32(0.3746), FIX32(0.3786), FIX32(0.3826), FIX32(0.3867), FIX32(0.3907), FIX32(0.3947), FIX32(0.3987), FIX32(0.4027), 
    FIX32(0.4067), FIX32(0.4107), FIX32(0.4146), FIX32(0.4186), FIX32(0.4226), FIX32(0.4265), FIX32(0.4305), FIX32(0.4344), 
    FIX32(0.4383), FIX32(0.4422), FIX32(0.4461), FIX32(0.45), FIX32(0.4539), FIX32(0.4578), FIX32(0.4617), FIX32(0.4656), 
    FIX32(0.4694), FIX32(0.4733), FIX32(0.4771), FIX32(0.4809), FIX32(0.4848), FIX32(0.4886), FIX32(0.4924), FIX32(0.4962), 
    FIX32(0.4999), FIX32(0.5037), FIX32(0.5075), FIX32(0.5112), FIX32(0.515), FIX32(0.5187), FIX32(0.5224), FIX32(0.5262), 
    FIX32(0.5299), FIX32(0.5336), FIX32(0.5372), FIX32(0.5409), FIX32(0.5446), FIX32(0.5482), FIX32(0.5519), FIX32(0.5555), 
    FIX32(0.5591), FIX32(0.5628), FIX32(0.5664), FIX32(0.5699), FIX32(0.5735), FIX32(0.5771), FIX32(0.5807), FIX32(0.5842), 
    FIX32(0.5877), FIX32(0.5913), FIX32(0.5948), FIX32(0.5983), FIX32(0.6018), FIX32(0.6052), FIX32(0.6087), FIX32(0.6122), 
    FIX32(0.6156), FIX32(0.619), FIX32(0.6225), FIX32(0.6259), FIX32(0.6293), FIX32(0.6327), FIX32(0.636), FIX32(0.6394), 
    FIX32(0.6427), FIX32(0.6461), FIX32(0.6494), FIX32(0.6527), FIX32(0.656), FIX32(0.6593), FIX32(0.6626), FIX32(0.6658), 
    FIX32(0.6691), FIX32(0.6723), FIX32(0.6755), FIX32(0.6788), FIX32(0.6819), FIX32(0.6851), FIX32(0.6883), FIX32(0.6915), 
    FIX32(0.6946), FIX32(0.6977), FIX32(0.7009), FIX32(0.704), FIX32(0.7071), FIX32(0.7101), FIX32(0.7132), FIX32(0.7163), 
    FIX32(0.7193), FIX32(0.7223), FIX32(0.7253), FIX32(0.7283), FIX32(0.7313), FIX32(0.7343), FIX32(0.7372), FIX32(0.7402), 
    FIX32(0.7431), FIX32(0.746), FIX32(0.7489), FIX32(0.7518), FIX32(0.7547), FIX32(0.7575), FIX32(0.7604), FIX32(0.7632), 
    FIX32(0.766), FIX32(0.7688), FIX32(0.7716), FIX32(0.7743), FIX32(0.7771), FIX32(0.7798), FIX32(0.7826), FIX32(0.7853), 
    FIX32(0.788), FIX32(0.7906), FIX32(0.7933), FIX32(0.796), FIX32(0.7986), FIX32(0.8012), FIX32(0.8038), FIX32(0.8064), 
    FIX32(0.809), FIX32(0.8115), FIX32(0.8141), FIX32(0.8166), FIX32(0.8191), FIX32(0.8216), FIX32(0.8241), FIX32(0.8265), 
    FIX32(0.829), FIX32(0.8314), FIX32(0.8338), FIX32(0.8362), FIX32(0.8386), FIX32(0.841), FIX32(0.8433), FIX32(0.8457), 
    FIX32(0.848), FIX32(0.8503), FIX32(0.8526), FIX32(0.8549), FIX32(0.8571), FIX32(0.8594), FIX32(0.8616), FIX32(0.8638), 
    FIX32(0.866), FIX32(0.8681), FIX32(0.8703), FIX32(0.8724), FIX32(0.8746), FIX32(0.8767), FIX32(0.8788), FIX32(0.8808), 
    FIX32(0.8829), FIX32(0.8849), FIX32(0.887), FIX32(0.889), FIX32(0.891), FIX32(0.8929), FIX32(0.8949), FIX32(0.8968), 
    FIX32(0.8987), FIX32(0.9006), FIX32(0.9025), FIX32(0.9044), FIX32(0.9063), FIX32(0.9081), FIX32(0.9099), FIX32(0.9117), 
    FIX32(0.9135), FIX32(0.9153), FIX32(0.917), FIX32(0.9187), FIX32(0.9205), FIX32(0.9222), FIX32(0.9238), FIX32(0.9255), 
    FIX32(0.9271), FIX32(0.9288), FIX32(0.9304), FIX32(0.932), FIX32(0.9335), FIX32(0.9351), FIX32(0.9366), FIX32(0.9381), 
    FIX32(0.9396), FIX32(0.9411), FIX32(0.9426), FIX32(0.944), FIX32(0.9455), FIX32(0.9469), FIX32(0.9483), FIX32(0.9496), 
    FIX32(0.951), FIX32(0.9523), FIX32(0.9537), FIX32(0.955), FIX32(0.9563), FIX32(0.9575), FIX32(0.9588), FIX32(0.96), 
    FIX32(0.9612), FIX32(0.9624), FIX32(0.9636), FIX32(0.9647), FIX32(0.9659), FIX32(0.967), FIX32(0.9681), FIX32(0.9692), 
    FIX32(0.9702), FIX32(0.9713), FIX32(0.9723), FIX32(0.9733), FIX32(0.9743), FIX32(0.9753), FIX32(0.9762), FIX32(0.9772), 
    FIX32(0.9781), FIX32(0.979), FIX32(0.9799), FIX32(0.9807), FIX32(0.9816), FIX32(0.9824), FIX32(0.9832), FIX32(0.984), 
    FIX32(0.9848), FIX32(0.9855), FIX32(0.9862), FIX32(0.9869), FIX32(0.9876), FIX32(0.9883), FIX32(0.989), FIX32(0.9896), 
    FIX32(0.9902), FIX32(0.9908), FIX32(0.9914), FIX32(0.992), FIX32(0.9925), FIX32(0.993), FIX32(0.9935), FIX32(0.994), 
    FIX32(0.9945), FIX32(0.9949), FIX32(0.9953), FIX32(0.9958), FIX32(0.9961), FIX32(0.9965), FIX32(0.9969), FIX32(0.9972), 
    FIX32(0.9975), FIX32(0.9978), FIX32(0.9981), FIX32(0.9983), FIX32(0.9986), FIX32(0.9988), FIX32(0.999), FIX32(0.9992), 
    FIX32(0.9993), FIX32(0.9995), FIX32(0.9996), FIX32(0.9997), FIX32(0.9998), FIX32(0.9999), FIX32(0.9999), FIX32(0.9999), 
    FIX32(1.0)
};


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

u16 getLog2(u32 value)
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


///////////////////////////////////////////////
//  Fix16 Fixed point math functions
///////////////////////////////////////////////

FORCE_INLINE fix16 F16_fromInt(s16 value)
{
    return value << FIX16_FRAC_BITS;
}

FORCE_INLINE s16 F16_toInt(fix16 value)
{
    return value >> FIX16_FRAC_BITS;
}

FORCE_INLINE fix32 F16_toFix32(fix16 value)
{
    return value << (FIX32_FRAC_BITS - FIX16_FRAC_BITS);
}

FORCE_INLINE fastfix16 F16_toFastFix16(fix16 value)
{
    return value << (FASTFIX16_FRAC_BITS - FIX16_FRAC_BITS);
}

FORCE_INLINE fastfix32 F16_toFastFix32(fix16 value)
{
    return value << (FASTFIX32_FRAC_BITS - FIX16_FRAC_BITS);
}

FORCE_INLINE fix16 F16_frac(fix16 value)
{
    return value & FIX16_FRAC_MASK;
}

FORCE_INLINE fix16 F16_int(fix16 value)
{
    return value & FIX16_INT_MASK;
}

FORCE_INLINE fix16 F16_abs(fix16 x)
{
    return abs(x);
}

FORCE_INLINE fix16 F16_round(fix16 value)
{
    return F16_int(value + (FIX16(0.5) - 1));
}

FORCE_INLINE s16 F16_toRoundedInt(fix16 value)
{
    return F16_toInt(value + (FIX16(0.5) - 1));
}


FORCE_INLINE fix16 F16_mul(fix16 val1, fix16 val2)
{
    return muls(val1, val2) >> FIX16_FRAC_BITS;
}

FORCE_INLINE fix16 F16_div(fix16 val1, fix16 val2)
{
    return divs(val1 << FIX16_FRAC_BITS, val2);
}

FORCE_INLINE fix16 F16_avg(fix16 val1, fix16 val2)
{
    return (val1 + val2) >> 1;
}


FORCE_INLINE fix16 F16_Log2(fix16 value)
{
    return log2tab_f16[value];
}

FORCE_INLINE fix16 F16_log10(fix16 value)
{
    return log10tab_f16[value];
}

FORCE_INLINE fix16 F16_sqrt(fix16 value)
{
    return sqrttab_f16[value];
}


FORCE_INLINE fix16 F16_normalizeAngle(fix16 angle)
{
    // nothing to do
    if ((angle >= FIX16(0)) && (angle < FIX16(360))) return angle;

    s16 result = angle % FIX16(360);
    // want angle into [FIX16(0)..FIX16(360)[ range
    if (result < FIX16(0)) result += FIX16(360);

    return result;
}

FORCE_INLINE fix16 F16_sin(fix16 angle)
{
    fix16 normAngle = F16_normalizeAngle(angle);
    
    // trigtab_f16 is [0..90°] over 90+1 entries
    if (normAngle <= FIX16(90)) return trigtab_f16[normAngle >> (FIX16_FRAC_BITS - 0)];
    if (normAngle <= FIX16(180)) return trigtab_f16[(FIX16(180) - normAngle) >> (FIX16_FRAC_BITS - 0)];
    if (normAngle <= FIX16(270)) return -trigtab_f16[(normAngle - FIX16(180)) >> (FIX16_FRAC_BITS - 0)];
    return -trigtab_f16[(FIX16(360) - normAngle) >> (FIX16_FRAC_BITS - 0)];
}

FORCE_INLINE fix16 F16_cos(fix16 angle)
{
    return F16_sin(angle + FIX16(90));
}

FORCE_INLINE fix16 F16_tan(fix16 angle)
{
    return F16_div(F16_sin(angle), F16_cos(angle));
}

FORCE_INLINE fix16 F16_atan(fix16 x)
{
    const fix16 a1 = FIX16( 0.999999999999999);
    const fix16 a3 = FIX16(-0.333333333333196);
    const fix16 a5 = FIX16( 0.199999975760886);
    const fix16 a7 = FIX16(-0.142356622678549);

    fix16 x2 = F16_mul(x, x);
    fix16 x3 = F16_mul(x2, x);
    fix16 x5 = F16_mul(x3, x2);
    fix16 x7 = F16_mul(x5, x2);

    // atan result approximation (radian)
    fix16 result = F16_mul(a1, x) + F16_mul(a3, x3) + F16_mul(a5, x5) + F16_mul(a7, x7);

    // return result in degree
    return F16_radianToDegree(result);
}

fix16 F16_atan2(fix16 y, fix16 x)
{
    if ((x == 0) && (y == 0)) return 0;

    fix16 angle;
    if (F16_abs(x) >= F16_abs(y))
    {
        angle = F16_atan(F16_div(y, x));
        if (x < 0)
        {
            if (y >= 0) angle += FIX16(180);
            else angle -= FIX16(180);
        }
    }
    else
    {
        angle = F16_atan(F16_div(x, y));
        if (y > 0) angle = FIX16(90) - angle;
        else angle = FIX16(-90) - angle;
    }

    // keep it inside [FIX16(0)..FIX16(360)[ range
    if (angle < FIX16(0)) angle += FIX16(360);
    else if (angle >= FIX16(360)) angle -= FIX16(360);

    return angle;
}

FORCE_INLINE fix16 F16_degreeToRadian(fix16 degree)
{
    return F16_div(degree, F16_RAD_TO_DEG);
}

FORCE_INLINE fix16 F16_radianToDegree(fix16 radian)
{
    return F16_mul(radian, F16_RAD_TO_DEG);
}

fix16 F16_getAngle(fix16 x1, fix16 y1, fix16 x2, fix16 y2)
{
    fix16 dx = x2 - x1;
    fix16 dy = y2 - y1;

    if (dx == 0) return (dy > 0) ? FIX16(90) : FIX16(270);
    if (dy == 0) return (dx > 0) ? 0 : FIX16(180);
    if (F16_abs(dx) == F16_abs(dy))
    {
        if (dx > 0) return (dy > 0) ? FIX16(45) : FIX16(315);
        return (dy > 0) ? FIX16(135) : FIX16(225);
    }
    
    return F16_atan2(dy, dx);
}

static void f16_movePointEx(fix16 *x2, fix16 *y2, fix16 x1, fix16 y1, fix16 ang, fix16 dist, fix16 cosAdj, fix16 sinAdj, bool useAdj)
{
    fix16 sinVal = F16_sin(ang);
    fix16 cosVal = F16_cos(ang);

    if (useAdj)
    {
        sinVal = F16_mul(sinVal, sinAdj);
        cosVal = F16_mul(cosVal, cosAdj);
    }

    fix16 moveX = F16_mul(dist, cosVal);
    fix16 moveY = F16_mul(dist, sinVal);

    *x2 = x1 + moveX;
    *y2 = y1 + moveY;
}

void F16_movePoint(fix16 *x2, fix16 *y2, fix16 x1, fix16 y1, fix16 ang, fix16 dist)
{
    f16_movePointEx(x2, y2, x1, y1, ang, dist, FIX16(1), FIX16(1), FALSE);
}

void F16_movePointEx(fix16 *x2, fix16 *y2, fix16 x1, fix16 y1, fix16 ang, fix16 dist, fix16 cosAdj, fix16 sinAdj)
{
    f16_movePointEx(x2, y2, x1, y1, ang, dist, cosAdj, sinAdj, TRUE);
}


// to keep backward compatibility
FORCE_INLINE fix16 sinFix16(u16 value)
{
    // convert [0..1024[ to [0..360[
    return F16_sin(FIX16(mulu(value & 1023, 360) >> 10));
}

FORCE_INLINE fix16 cosFix16(u16 value)
{
    // convert [0..1024[ to [0..360[
    return F16_cos(FIX16(mulu(value & 1023, 360) >> 10));
}


///////////////////////////////////////////////
//  Fix32 Fixed point math functions
///////////////////////////////////////////////

FORCE_INLINE fix32 F32_fromInt(s32 value)
{
    return value << FIX32_FRAC_BITS;
}

FORCE_INLINE s32 F32_toInt(fix32 value)
{
    return value >> FIX32_FRAC_BITS;
}

FORCE_INLINE fix16 F32_toFix16(fix32 value)
{
    return value >> (FIX32_FRAC_BITS - FIX16_FRAC_BITS);
}

FORCE_INLINE fastfix16 F32_toFastFix16(fix32 value)
{
    return value >> (FIX32_FRAC_BITS - FASTFIX16_FRAC_BITS);
}

FORCE_INLINE fastfix32 F32_toFastFix32(fix32 value)
{
    return value << (FASTFIX32_FRAC_BITS - FIX32_FRAC_BITS);
}

FORCE_INLINE fix32 F32_frac(fix32 value)
{
    return value & FIX32_FRAC_MASK;
}

FORCE_INLINE fix32 F32_int(fix32 value)
{
    return value & FIX32_INT_MASK;
}

FORCE_INLINE fix32 F32_round(fix32 value)
{
    return F32_int(value + (FIX32(0.5) - 1));
}

FORCE_INLINE s32 F32_toRoundedInt(fix32 value)
{
    return F32_toInt(value + (FIX32(0.5) - 1));
}


FORCE_INLINE fix32 F32_mul(fix32 val1, fix32 val2)
{
    fix32 v1 = val1 >> (FIX32_FRAC_BITS / 2);
    fix32 v2 = val2 >> (FIX32_FRAC_BITS / 2);

    return v1 * v2;
}

FORCE_INLINE fix32 F32_div(fix32 val1, fix32 val2)
{
    fix32 v1 = val1 << (FIX32_FRAC_BITS / 2);
    fix32 v2 = val2 >> (FIX32_FRAC_BITS / 2);

    return v1 / v2;
}

FORCE_INLINE fix32 F32_avg(fix32 val1, fix32 val2)
{
    return (val1 + val2) >> 1;
}


FORCE_INLINE fix32 F32_sin(fix16 angle)
{
    fix16 normAngle = F16_normalizeAngle(angle);
    
    // trigtab_f32 is [0..90°] over 360+1 entries
    if (normAngle <= FIX16(90)) return trigtab_f32[normAngle >> (FIX16_FRAC_BITS - 2)];
    if (normAngle <= FIX16(180)) return trigtab_f32[(FIX16(180) - normAngle) >> (FIX16_FRAC_BITS - 2)];
    if (normAngle <= FIX16(270)) return -trigtab_f32[(normAngle - FIX16(180)) >> (FIX16_FRAC_BITS - 2)];
    return -trigtab_f32[(FIX16(360) - normAngle) >> (FIX16_FRAC_BITS - 2)];
}

FORCE_INLINE fix32 F32_cos(fix16 angle)
{
    return F32_sin(angle + FIX16(90));
}


// to keep backward compatibility
FORCE_INLINE fix32 sinFix32(u16 value)
{
    // convert [0..1024[ to [0..360[
    return F32_sin(FIX16(divu(mulu(value & 1023, 360), 1024)));
}

FORCE_INLINE fix32 cosFix32(u16 value)
{
    // convert [0..1024[ to [0..360[
    return F32_cos(FIX16(divu(mulu(value & 1023, 360), 1024)));
}


///////////////////////////////////////////////
//  Fast Fix16 Fixed point math functions
///////////////////////////////////////////////

FORCE_INLINE fastfix16 FF16_fromInt(s16 value)
{
    return value << FASTFIX16_FRAC_BITS;
}

FORCE_INLINE s16 FF16_toInt(fastfix16 value)
{
    return value >> FASTFIX16_FRAC_BITS;
}

FORCE_INLINE fix16 FF16_toFix16(fastfix16 value)
{
    return value >> (FASTFIX16_FRAC_BITS - FIX16_FRAC_BITS);
}

FORCE_INLINE fix32 FF16_toFix32(fastfix16 value)
{
    return value << (FIX32_FRAC_BITS - FASTFIX16_FRAC_BITS);
}

FORCE_INLINE fastfix32 FF16_toFastFix32(fastfix16 value)
{
    return value << (FASTFIX32_FRAC_BITS - FASTFIX16_FRAC_BITS);
}

FORCE_INLINE fastfix16 FF16_frac(fastfix16 value)
{
    return value & FASTFIX16_FRAC_MASK;
}

FORCE_INLINE fastfix16 FF16_int(fastfix16 value)
{
    return value & FASTFIX16_INT_MASK;
}

FORCE_INLINE fastfix16 FF16_round(fastfix16 value)
{
    return FF16_int(value + (FASTFIX16(0.5) - 1));
}

FORCE_INLINE s16 FF16_toRoundedInt(fastfix16 value)
{
    return FF16_toInt(value + (FASTFIX16(0.5) - 1));
}


FORCE_INLINE fastfix16 FF16_mul(fastfix16 val1, fastfix16 val2)
{
     return muls(val1, val2) >> FASTFIX16_FRAC_BITS;
}

FORCE_INLINE fastfix16 FF16_div(fastfix16 val1, fastfix16 val2)
{
     return divs(val1 << FASTFIX16_FRAC_BITS, val2);
}


///////////////////////////////////////////////
//  Fast Fix32 Fixed point math functions
///////////////////////////////////////////////

FORCE_INLINE fastfix32 FF32_fromInt(s16 value)
{
    return value << FASTFIX32_FRAC_BITS;
}

FORCE_INLINE s16 FF32_toInt(fastfix32 value)
{
    return value >> FASTFIX32_FRAC_BITS;
}

FORCE_INLINE fix16 FF32_toFix16(fastfix32 value)
{
    return value >> (FASTFIX32_FRAC_BITS - FIX16_FRAC_BITS);
}

FORCE_INLINE fix32 FF32_toFix32(fastfix32 value)
{
    return value >> (FASTFIX32_FRAC_BITS - FIX32_FRAC_BITS);
}

FORCE_INLINE fastfix16 FF32_toFastFix16(fastfix32 value)
{
    return value >> (FASTFIX32_FRAC_BITS - FASTFIX16_FRAC_BITS);
}

FORCE_INLINE fastfix32 FF32_frac(fastfix32 value)
{
    return value & FASTFIX32_FRAC_MASK;
}

FORCE_INLINE fastfix32 FF32_int(fastfix32 value)
{
    return value & FASTFIX32_INT_MASK;
}

FORCE_INLINE fastfix32 FF32_round(fastfix32 value)
{
    return FF32_int(value + (FASTFIX32(0.5) - 1));
}

FORCE_INLINE s32 FF32_toRoundedInt(fastfix32 value)
{
    return FF32_toInt(value + (FASTFIX32(0.5) - 1));
}

FORCE_INLINE fastfix32 FF32_mul(fastfix32 val1, fastfix32 val2)
{
    fastfix32 v1 = val1 >> (FASTFIX32_FRAC_BITS / 2);
    fastfix32 v2 = val2 >> (FASTFIX32_FRAC_BITS / 2);

    return v1 * v2;
}

FORCE_INLINE fastfix32 FF32_div(fastfix32 val1, fastfix32 val2)
{
    fastfix32 v1 = val1 << (FASTFIX32_FRAC_BITS / 2);
    fastfix32 v2 = val2 >> (FASTFIX32_FRAC_BITS / 2);

    return v1 / v2;
}


fastfix32 FF32_getLog2Fast(fastfix32 value)
{
    s32 x = value;
    s32 y = 0xa65af;

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

    s32 t = x + (x >> 1);
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


///////////////////////////////////////////////
//  Vector2D base math functions
///////////////////////////////////////////////

u32 V2D_S32_getApproximatedDistance(V2s32* v)
{
    return getApproximatedDistance(v->x, v->y);
}


fix16 V2D_F16_getAngle(V2f16* pt1, V2f16* pt2)
{
    return F16_getAngle(pt1->x, pt1->y, pt2->x, pt2->y);
}

void V2D_F16_movePoint(V2f16* pt, fix16 ang, fix16 dist)
{
    f16_movePointEx(&pt->x, &pt->y, pt->x, pt->y, ang, dist, FIX16(1), FIX16(1), FALSE);
}

void V2D_F16_movePointEx(V2f16* pt, fix16 ang, fix16 dist, fix16 cosAdj, fix16 sinAdj)
{
    f16_movePointEx(&pt->x, &pt->y, pt->x, pt->y, ang, dist, cosAdj, sinAdj, TRUE);
}
