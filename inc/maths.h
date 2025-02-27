/**
 *  \file maths.h
 *  \brief Mathematical methods.
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides basic maths methods.<br>
 * You can find a tutorial about how use maths with SGDK <a href="https://github.com/Stephane-D/SGDK/wiki/Tuto-Maths">here</a>.<br>
 */

#ifndef _MATHS_H_
#define _MATHS_H_



// 1° step is enough for FIX16
extern const fix16 trigtab_f16[90 + 1];
// 0.25° step is ok for fix32
extern const fix32 trigtab_f32[(90 * 4) + 1];

extern const fix16 log2tab_f16[0x10000];
extern const fix16 log10tab_f16[0x10000];
extern const fix16 sqrttab_f16[0x10000];


#ifndef PI
/**
 *  \brief
 *      PI number (3,1415..)
 */
#define PI                          3.14159265358979323846
#endif


#define FIX16_INT_BITS              10
#define FIX16_FRAC_BITS             (16 - FIX16_INT_BITS)
#define FIX16_INT_MASK              (((1 << FIX16_INT_BITS) - 1) << FIX16_FRAC_BITS)
#define FIX16_FRAC_MASK             ((1 << FIX16_FRAC_BITS) - 1)

#define FIX32_INT_BITS              22
#define FIX32_FRAC_BITS             (32 - FIX32_INT_BITS)
#define FIX32_INT_MASK              (((1 << FIX32_INT_BITS) - 1) << FIX32_FRAC_BITS)
#define FIX32_FRAC_MASK             ((1 << FIX32_FRAC_BITS) - 1)

#define FASTFIX16_INT_BITS          8
#define FASTFIX16_FRAC_BITS         (16 - FASTFIX16_INT_BITS)
#define FASTFIX16_INT_MASK          (((1 << FASTFIX16_INT_BITS) - 1) << FASTFIX16_FRAC_BITS)
#define FASTFIX16_FRAC_MASK         ((1 << FASTFIX16_FRAC_BITS) - 1)

#define FASTFIX32_INT_BITS          16
#define FASTFIX32_FRAC_BITS         (32 - FASTFIX32_INT_BITS)
#define FASTFIX32_INT_MASK          (((1 << FASTFIX32_INT_BITS) - 1) << FASTFIX32_FRAC_BITS)
#define FASTFIX32_FRAC_MASK         ((1 << FASTFIX32_FRAC_BITS) - 1)

/**
 *  \brief
 *      Convert specified value to fix16
 *
 *  Ex:<br>
 *      f16 v = FIX16(-27.12);
 */
#define FIX16(value)                ((fix16) ((value) * (1 << FIX16_FRAC_BITS)))
/**
 *  \brief
 *      Convert specified value to fix32.
 *
 *  Ex:<br>
 *      f32 v = FIX32(34.567);
 */
#define FIX32(value)                ((fix32) ((value) * (1 << FIX32_FRAC_BITS)))

/**
 *  \brief
 *      Convert specified value to "fast" fix16
 *
 *  Ex:<br>
 *      ff16 v = FASTFIX16(-27.12);
 */
#define FASTFIX16(value)            ((fastfix16) ((value) * (1 << FASTFIX16_FRAC_BITS)))
/**
 *  \brief
 *      Convert specified value to "fast" fix32.
 *
 *  Ex:<br>
 *      ff32 v = FASTFIX32(34.567);
 */
#define FASTFIX32(value)            ((fastfix32) ((value) * (1 << FASTFIX32_FRAC_BITS)))

/**
 *  \brief
 *      Convert specified value to fix16 (short version)
 *  \see FIX16
 */
#define F16(value)                  FIX16(value)
/**
 *  \brief
 *      Convert specified value to fix32 (short version)
 *  \see FIX32
 */
#define F32(value)                  FIX32(value)
/**
 *  \brief
 *      Convert specified value to "fast" fix16 (short version)
 *  \see FASTFIX16
 */
#define FF16(value)                 FASTFIX16(value)
/**
 *  \brief
 *      Convert specified value to "fast" fix32 (short version)
 *  \see FASTFIX32
 */
#define FF32(value)                 FASTFIX32(value)


#define F16_PI                      ((fix16) F16(PI))
#define F16_RAD_TO_DEG              F16(57.29577951308232)

#define F32_PI                      ((fix32) F32(PI))


// 2D base structures

/**
 *  \brief
 *      2D Vector structure - u16 type.
 */
typedef struct
{
    u16 x;
    u16 y;
} Vect2D_u16;

/**
 *  \brief
 *      2D Vector structure - s16 type.
 */
typedef struct
{
    s16 x;
    s16 y;
} Vect2D_s16;

/**
 *  \brief
 *      2D Vector structure - u32 type.
 */
typedef struct
{
    u32 x;
    u32 y;
} Vect2D_u32;

/**
 *  \brief
 *      2D Vector structure - s32 type.
 */
typedef struct
{
    s32 x;
    s32 y;
} Vect2D_s32;

/**
 *  \brief
 *      2D Vector structure - f16 (fix16) type.
 */
typedef struct
{
    fix16 x;
    fix16 y;
} Vect2D_f16;

/**
 *  \brief
 *      2D Vector structure - f32 (fix32) type.
 */
typedef struct
{
    fix32 x;
    fix32 y;
} Vect2D_f32;

/**
 *  \brief
 *      2x2 Matrice structure - f16 (fix16) type.<br>
 *      Internally uses 2 2D vectors.
 */
typedef struct
{
    Vect2D_f16 a;
    Vect2D_f16 b;
} Mat2D_f16;

/**
 *  \brief
 *      2x2 Matrice structure - f32 (fix32) type.<br>
 *      Internally uses 2 2D vectors.
 */
typedef struct
{
    Vect2D_f32 a;
    Vect2D_f32 b;
} Mat2D_f32;


// short alias

/**
 *  \brief alias for Vect2D_u16
 */
typedef Vect2D_u16 V2u16;
/**
 *  \brief alias for Vect2D_s16
 */
typedef Vect2D_s16 V2s16;
/**
 *  \brief alias for Vect2D_u32
 */
typedef Vect2D_u32 V2u32;
/**
 *  \brief alias for Vect2D_s32
 */
typedef Vect2D_s32 V2s32;
/**
 *  \brief alias for Vect2D_f16
 */
typedef Vect2D_f16 V2f16;
/**
 *  \brief alias for Vect2D_f32
 */
typedef Vect2D_f32 V2f32;

/**
 *  \brief alias for Mat2D_f16
 */
typedef Mat2D_f16 M2f16;
/**
 *  \brief alias for Mat2D_f32
 */
typedef Mat2D_f32 M2f32;


///////////////////////////////////////////////
//  Basic math functions
///////////////////////////////////////////////

/**
 *  \brief
 *      Returns the lowest value between X an Y.
 */
#define min(X, Y)       (((X) < (Y))?(X):(Y))

/**
 *  \brief
 *      Returns the highest value between X an Y.
 */
#define max(X, Y)       (((X) > (Y))?(X):(Y))

/**
 *  \brief
 *      Returns L if X is less than L, H if X is greater than H or X if in between L and H.
 */
#define clamp(X, L, H)  (min(max((X), (L)), (H)))

#if (ENABLE_NEWLIB == 0)
/**
 *  \brief
 *      Returns the absolute value of X.
 */
#define abs(X)          (((X) < 0)?-(X):(X))
#endif  // ENABLE_NEWLIB


/**
 *  \brief
 *      16x16=32 unsigned multiplication. Force GCC to use proper 68000 <i>mulu</i> instruction.
 *
 *  \param op1
 *      first operand
 *  \param op2
 *      second operand
 *  \return 32 bit (unsigned) result of multiply
 */
u32 mulu(u16 op1, u16 op2);
/**
 *  \brief
 *      16x16=32 signed multiplication. Force GCC to use proper 68000 <i>muls</i> instruction.
 *
 *  \param op1
 *      first operand
 *  \param op2
 *      second operand
 *  \return 32 bit (signed) result of multiply
 */
s32 muls(s16 op1, s16 op2);
/**
 *  \brief
 *      Direct divu instruction (unsigned 32/16=16:16) access using inline assembly
 *      to process op1/op2 operation.
 *
 *  \param op1
 *      first operand - dividende (32 bit)
 *  \param op2
 *      second operand - divisor (16 bit)
 *  \return 16 bit (unsigned) result of the division
 */
u16 divu(u32 op1, u16 op2);
/**
 *  \brief
 *      Direct divs instruction (signed 32/16=16:16) access using inline assembly
 *      to process op1/op2 operation.
 *
 *  \param op1
 *      first operand (32 bit)
 *  \param op2
 *      second operand (16 bit)
 *  \return 16 bit (signed) result of the division
 */
s16 divs(s32 op1, s16 op2);
/**
 *  \brief
 *      Direct divu instruction (unsigned 32/16=16:16) access using inline assembly
 *
 *  \param op1
 *      first operand (32 bit)
 *  \param op2
 *      second operand (16 bit)
 *  \return 16 bit (unsigned) modulo result of the division
 */
u16 modu(u32 op1, u16 op2);
/**
 *  \brief
 *      Direct divs instruction (signed 32/16=16:16) access using inline assembly
 *
 *  \param op1
 *      first operand (32 bit)
 *  \param op2
 *      second operand (16 bit)
 *  \return 16 bit (signed) modulo result of the division
 */
s16 mods(s32 op1, s16 op2);

/**
 *  \brief
 *      Direct divu instruction (unsigned 32/16=16:16) access using inline assembly
 *      to process op1/op2 operation and op1%op2 at same time.
 *
 *  \param op1
 *      first operand - dividende (32 bit)
 *  \param op2
 *      second operand - divisor (16 bit)
 *  \return 16 bit (unsigned) result of the division in low 16 bit (0-15) and
 *      16 bit (unsigned) result of the modulo operation in high 16 bit (16-31)
 */
u32 divmodu(u32 op1, u16 op2);
/**
 *  \brief
 *      Direct divs instruction (signed 32/16=16:16) access using inline assembly
 *      to process op1/op2 operation and op1%op2 at same time.
 *
 *  \param op1
 *      first operand - dividende (32 bit)
 *  \param op2
 *      second operand - divisor (16 bit)
 *  \return 16 bit (signed) result of the division in low 16 bit (0-15) and
 *      16 bit (signed) result of the modulo operation in high 16 bit (16-31)
 */
s32 divmods(s32 op1, s16 op2);

/**
 *  \brief
 *      Convert u16 to BCD.
 *
 *  \param value
 *      u16 value to convert.
 */
u32 u16ToBCD(u16 value);
/**
 *  \brief
 *      Convert u32 to BCD.
 *
 *  \param value
 *      u32 value to convert.
 */
u32 u32ToBCD(u32 value);

/**
 *  \brief
 *      Return next pow2 value which is greater than specified 32 bits unsigned value.
 *      Ex:<br>
 *      getNextPow2(700) = 1024<br>
 *      getNextPow2(18) = 32<br>
 */
u32 getNextPow2(u32 value);
/**
 *  \brief
 *      Return integer log2 of specified 32 bits unsigned value.
 *      Ex:<br>
 *      getLog2Int(1024) = 10<br>
 *      getLog2Int(12345) = 13<br>
 *
 *  \param value
 *      value to return log2 of
 */
u16 getLog2(u32 value);

/**
 *  \brief
 *      Return euclidean distance approximation for specified vector.<br>
 *      The returned distance is not 100% perfect but calculation is fast.
 *
 *  \param dx
 *      delta X.
 *  \param dy
 *      delta Y.
 */
u32 getApproximatedDistance(s32 dx, s32 dy);
/**
 *  \brief
 *      Return euclidean distance approximation for specified vector.<br>
 *      The returned distance is not 100% perfect but calculation is fast.
 *
 *  \param v
 *      2D vector.
 */
u32 getApproximatedDistanceV(V2s32* v);


#define distance_approx(dx, dy)     _Pragma("GCC error \"This method is deprecated, use getApproximatedDistance(..) instead.\"")
#define getApproximatedLog2(value)  _Pragma("GCC error \"This method is deprecated, use FF32_getLog2Fast(..) instead.\"")
#define getLog2Int(value)           _Pragma("GCC error \"This method is deprecated, use getLog2(..) instead.\"")


///////////////////////////////////////////////
//  Fix16 Fixed point math functions
///////////////////////////////////////////////

/**
 *  \brief
 *      Convert fix16 to integer.
 */
s16 F16_toInt(fix16 value);
/**
 *  \brief
 *      Convert specified fix16 value to fix32.
 */
fix32 F16_toFix32(fix16 value);
/**
 *  \brief
 *      Return fractional part of the specified value (fix16).
 */
fix16 F16_frac(fix16 value);
/**
 *  \brief
 *      Return integer part of the specified value (fix16).
 */
fix16 F16_int(fix16 value);
/**
 *  \brief
 *      Return the absolute value of the specified value (fix16).
 */
fix16 F16_abs(fix16 x);
/**
 *  \brief
 *      Round the specified value to nearest integer (fix16).
 */
fix16 F16_round(fix16 value);
/**
 *  \brief
 *      Round and convert the specified fix16 to integer.
 */
s16 F16_toRoundedInt(fix16 value);

/**
 *  \brief
 *      Compute and return the result of the multiplication of val1 and val2 (fix16).
 */
fix16 F16_mul(fix16 val1, fix16 val2);
/**
 *  \brief
 *      Compute and return the result of the division of val1 by val2 (fix16).
 */
fix16 F16_div(fix16 val1, fix16 val2);
/**
 *  \brief
 *      Compute and return the result of the average of val1 by val2 (fix16).
 */
fix16 F16_avg(fix16 val1, fix16 val2);

/**
 *  \brief
 *      Compute and return the result of the Log2 of specified value (fix16).
 */
fix16 F16_log2(fix16 value);
/**
 *  \brief
 *      Compute and return the result of the Log10 of specified value (fix16).
 */
fix16 F16_log10(fix16 value);
/**
 *  \brief
 *      Compute and return the result of the root square of specified value (fix16).
 */
fix16 F16_sqrt(fix16 value);

/**
 *  \brief
 *      Return a normalized form of the input angle degree:<br>
 *      Output value is guaranteed to be in [FIX16(0)..FIX16(360)[ range.
 */
fix16 F16_normalizeAngle(fix16 angle);
/**
 *  \brief
 *      Compute sinus of specified angle (in degree) and return it (fix16).
 */
fix16 F16_sin(fix16 angle);
/**
 *  \brief
 *      Compute cosinus of specified angle (in degree) and return it (fix16).
 */
fix16 F16_cos(fix16 angle);
/**
 *  \brief
 *      Compute the tangent of specified angle (in degree) and return it (fix16).
 */
fix16 F16_tan(fix16 angle);
/**
 *  \brief
 *      Compute the arctangent of specified value and return it in degree (fix16).
 */
fix16 F16_atan(fix16 x);
/**
 *  \brief
 *      Compute the arctangent of y/x. i.e: return the angle (in degree) for the (0,0)-(x,y) vector.
 */
fix16 F16_atan2(fix16 y, fix16 x);

/**
 *  \deprecated Use F16_sin(..) instead
 *  \brief
 *      Compute sinus of specified value and return it as fix16.<br>
 *      The input value is an integer defined as [0..1024] range corresponding to radian [0..2PI] range.
 */
fix16 sinFix16(u16 value);
/**
 *  \deprecated Use F16_cos(..) instead
 *  \brief
 *      Compute cosinus of specified value and return it as fix16.<br>
 *      The input value is an integer defined as [0..1024] range corresponding to radian [0..2PI] range.
 */
fix16 cosFix16(u16 value);

/**
 *  \brief
 *      Convert degree to radian (fix16)
 */
fix16 F16_degreeToRadian(fix16 degree);
/**
 *  \brief
 *      Convert radian to degree (fix16)
 */
fix16 F16_radianToDegree(fix16 radian);
/**
 *  \brief
 *      Compute and return the angle in degree between the 2 points defined by (x1,y1) and (x2,y2).
 */
fix16 F16_getAngle(fix16 x1, fix16 y1, fix16 x2, fix16 y2);
/**
 *  \brief
 *      Compute the new position of the point defined by (x1, y1) by moving it by the given 'distance' in the 'angle' direction
 *      and store the result in (x2, y2).
 * 
 *  \param x2
 *      new X position
 *  \param y2
 *      new Y position
 *  \param x1
 *      current X position
 *  \param y1
 *      current Y position
 *  \param ang
 *      angle in degree
 *  \param dist
 *      distance to move
 */
void F16_computePosition(fix16 *x2, fix16 *y2, fix16 x1, fix16 y1, fix16 ang, fix16 dist);
/**
 *  \brief
 *      Compute the new position of the point defined by (x1, y1) by moving it by the given 'distance' in the 'angle' direction
 *      and store the result in (x2, y2).
* 
 *  \param x2
 *      new X position
 *  \param y2
 *      new Y position
 *  \param x1
 *      current X position
 *  \param y1
 *      current Y position
 *  \param ang
 *      angle in degree
 *  \param dist
 *      distance to move
 *  \param cosAdj
 *      cosine adjustment
 *  \param sinAdj
 *      sine adjustment
*/
void F16_computePositionEx(fix16 *x2, fix16 *y2, fix16 x1, fix16 y1, fix16 ang, fix16 dist, fix16 cosAdj, fix16 sinAdj);


// Deprecated functions
#define intToFix16(a)           _Pragma("GCC error \"This method is deprecated, use FIX16(..) instead.\"")
#define fix16ToInt(a)           _Pragma("GCC error \"This method is deprecated, use F16_toInt(..) instead.\"")
#define fix16ToFix32(a)         _Pragma("GCC error \"This method is deprecated, use F16_toFix32(..) instead.\"")
#define fix16Frac(a)            _Pragma("GCC error \"This method is deprecated, use F16_frac(..) instead.\"")
#define fix16Int(a)             _Pragma("GCC error \"This method is deprecated, use F16_int(..) instead.\"")
#define fix16Round(a)           _Pragma("GCC error \"This method is deprecated, use F16_round(..) instead.\"")
#define fix16ToRoundedInt(a)    _Pragma("GCC error \"This method is deprecated, use F16_toRoundedInt(..) instead.\"")
#define fix16Add(a, b)          _Pragma("GCC error \"This method is deprecated, simply use '+' operator to add fix16 values together.\"")
#define fix16Sub(a, b)          _Pragma("GCC error \"This method is deprecated, simply use '-' operator to subtract fix16 values.\"")
#define fix16Neg(a)             _Pragma("GCC error \"This method is deprecated, simply use '0 - value' to get the negative fix16 value.\"")
#define fix16Mul(a, b)          _Pragma("GCC error \"This method is deprecated, use F16_mul(..) instead.\"")
#define fix16Div(a, b)          _Pragma("GCC error \"This method is deprecated, use F16_div(..) instead.\"")
#define fix16Avg(a, b)          _Pragma("GCC error \"This method is deprecated, use F16_avg(..) instead.\"")
#define fix16Log2(a)            _Pragma("GCC error \"This method is deprecated, use F16_log2(..) instead.\"")
#define fix16Log10(a)           _Pragma("GCC error \"This method is deprecated, use F16_log10(..) instead.\"")
#define fix16Sqrt(a)            _Pragma("GCC error \"This method is deprecated, use F16_sqrt(..) instead.\"")


///////////////////////////////////////////////
//  Fix32 Fixed point math functions
///////////////////////////////////////////////

/**
 *  \brief
 *      Convert fix32 to integer.
 */
s32 F32_toInt(fix32 value);
/**
 *  \brief
 *      Convert specified fix32 value to fix16.
 */
fix16 F32_toFix16(fix32 value);
/**
 *  \brief
 *      Return fractional part of the specified value (fix32).
 */
fix32 F32_frac(fix32 value);
/**
 *  \brief
 *      Return integer part of the specified value (fix32).
 */
fix32 F32_int(fix32 value);
/**
 *  \brief
 *      Round the specified value to nearest integer (fix32).
 */
fix32 F32_round(fix32 value);
/**
 *  \brief
 *      Round and convert the specified fix32 value to integer.
 */
s32 F32_toRoundedInt(fix32 value);

/**
 *  \brief
 *      Compute and return the result of the multiplication of val1 and val2 (fix32).<br>
 *      WARNING: result can easily overflow so its recommended to stick with fix16 type for mul and div operations.
 */
fix32 F32_mul(fix32 val1, fix32 val2);
/**
 *  \brief
 *      Compute and return the result of the division of val1 by val2 (fix32).<br>
 *      WARNING: result can easily overflow so its recommended to stick with fix16 type for mul and div operations.
 */
fix32 F32_div(fix32 val1, fix32 val2);
/**
 *  \brief
 *      Compute and return the result of the average of val1 by val2 (fix32).
 */
fix32 F32_avg(fix32 val1, fix32 val2);

/**
 *  \brief
 *      Compute sinus of specified angle (in degree) and return it as fix32.
 */
fix32 F32_sin(fix16 angle);
/**
 *  \brief
 *      Compute cosinus of specified angle (in degree) and return it as fix32.
 */
fix32 F32_cos(fix16 angle);

/**
 *  \deprecated Use F32_sin(..) instead
 *  \brief
 *      Compute sinus of specified value and return it as fix32.<br>
 *      The input value is an integer defined as [0..1024] range corresponding to radian [0..2PI] range.
 */
fix32 sinFix32(u16 value);
/**
 *  \deprecated Use F32_cos(..) instead
 *  \brief
 *      Compute cosinus of specified value and return it as fix32.<br>
 *      The input value is an integer defined as [0..1024] range corresponding to radian [0..2PI] range.
 */
fix32 cosFix32(u16 value);


// Deprecated functions
#define intToFix32(a)           _Pragma("GCC error \"This method is deprecated, use FIX32(..) instead.\"")
#define fix32ToInt(a)           _Pragma("GCC error \"This method is deprecated, use F32_toInt(..) instead.\"")
#define fix32ToFix16(a)         _Pragma("GCC error \"This method is deprecated, use F32_toFix16(..) instead.\"")
#define fix32Frac(a)            _Pragma("GCC error \"This method is deprecated, use F32_frac(..) instead.\"")
#define fix32Int(a)             _Pragma("GCC error \"This method is deprecated, use F32_int(..) instead.\"")
#define fix32Round(a)           _Pragma("GCC error \"This method is deprecated, use F32_round(..) instead.\"")
#define fix32ToRoundedInt(a)    _Pragma("GCC error \"This method is deprecated, use F32_toRoundedInt(..) instead.\"")
#define fix32Add(a, b)          _Pragma("GCC error \"This method is deprecated, simply use '+' operator to add fix32 values together.\"")
#define fix32Sub(a, b)          _Pragma("GCC error \"This method is deprecated, simply use '-' operator to subtract fix32 values.\"")
#define fix32Neg(a)             _Pragma("GCC error \"This method is deprecated, simply use '0 - value' to get the negative fix32 value.\"")
#define fix32Mul(a, b)          _Pragma("GCC error \"This method is deprecated, use F32_mul(..) instead.\"")
#define fix32Div(a, b)          _Pragma("GCC error \"This method is deprecated, use F32_div(..) instead.\"")
#define fix32Avg(a, b)          _Pragma("GCC error \"This method is deprecated, use F32_avg(..) instead.\"")


///////////////////////////////////////////////
//  Fast Fix16 Fixed point math functions
///////////////////////////////////////////////

/**
 *  \brief
 *      Convert fastfix16 to integer.
 */
s16 FF16_toInt(fastfix16 value);

/**
 *  \brief
 *      Return fractional part of the specified value (fastfix16).
 */
fastfix16 FF16_frac(fastfix16 value);
/**
 *  \brief
 *      Return integer part of the specified value (fastfix16).
 */
fastfix16 FF16_int(fastfix16 value);
/**
 *  \brief
 *      Round the specified value to nearest integer (fastfix16).
 */
fastfix16 FF16_round(fastfix16 value);
/**
 *  \brief
 *      Round and convert the specified fastfix16 value to integer (fastfix16).
 */
s16 FF16_toRoundedInt(fastfix16 value);

/**
 *  \brief
 *      Compute and return the result of the multiplication of val1 and val2 (fastfix16).
 */
fastfix16 FF16_mul(fastfix16 val1, fastfix16 val2);
/**
 *  \brief
 *      Compute and return the result of the division of val1 by val2 (fastfix16).
 */
fastfix16 FF16_div(fastfix16 val1, fastfix16 val2);


// Deprecated functions
#define intToFastFix16(a)           _Pragma("GCC error \"This method is deprecated, use FASTFIX16(..) instead.\"")
#define fastFix16ToInt(a)           _Pragma("GCC error \"This method is deprecated, use FF16_toInt(..) instead.\"")
#define fastFix16Frac(a)            _Pragma("GCC error \"This method is deprecated, use FF16_frac(..) instead.\"")
#define fastFix16Int(a)             _Pragma("GCC error \"This method is deprecated, use FF16_int(..) instead.\"")
#define fastFix16Round(a)           _Pragma("GCC error \"This method is deprecated, use FF16_round(..) instead.\"")
#define fastFix16ToRoundedInt(a)    _Pragma("GCC error \"This method is deprecated, use FF16_toRoundedInt(..) instead.\"")
#define fastFix16Mul(a, b)          _Pragma("GCC error \"This method is deprecated, use FF16_mul(..) instead.\"")
#define fastFix16Div(a, b)          _Pragma("GCC error \"This method is deprecated, use FF16_div(..) instead.\"")


///////////////////////////////////////////////
//  Fast Fix32 Fixed point math functions
///////////////////////////////////////////////

/**
 *  \brief
 *      Convert integer to fastfix32.
 */
fastfix32 FF32_fromInt(s16 value);
/**
 *  \brief
 *      Convert fastfix32 to integer.
 */
s16 FF32_toInt(fastfix32 value);
/**
 *  \brief
 *      Return fractional part of the specified value (fastfix32).
 */
fastfix32 FF32_frac(fastfix32 value);
/**
 *  \brief
 *      Return integer part of the specified value (fastfix32).
 */
fastfix32 FF32_int(fastfix32 value);
/**
 *  \brief
 *      Round the specified value to nearest integer (fastfix32).
 */
fastfix32 FF32_round(fastfix32 value);
/**
 *  \brief
 *      Round and convert the specified fastfix32 value to integer.
 */
s32 FF32_toRoundedInt(fastfix32 value);

/**
 *  \brief
 *      Compute and return the result of the multiplication of val1 and val2 (fastfix32).<br>
 *      WARNING: result can easily overflow so its recommended to stick with fix16 type for mul and div operations.
 */
fastfix32 FF32_mul(fastfix32 val1, fastfix32 val2);
/**
 *  \brief
 *      Compute and return the result of the division of val1 by val2 (fastfix32).<br>
 *      WARNING: result can easily overflow so its recommended to stick with fix16 type for mul and div operations.
 */
fastfix32 FF32_div(fastfix32 val1, fastfix32 val2);

/**
 *  \brief
 *      Return a not accurate but fast log2 *approximation* of the specified value (fastfixed32)
 *      Ex:<br>
 *      getLog2(FASTFIX32(1)) = 0<br>
 *      getLog2(12345 << 16) = ~9.5 (real value = ~13.6)<br>
 *
 *  \param value
 *      fastfixed32 value to return log2 of
 */
fastfix32 FF32_getLog2Fast(fastfix32 value);


// Deprecated functions
#define intToFastFix32(a)           _Pragma("GCC error \"This method is deprecated, use FASTFIX32(..) instead.\"")
#define fastFix32ToInt(a)           _Pragma("GCC error \"This method is deprecated, use FF32_toInt(..) instead.\"")
#define fastFix32Frac(a)            _Pragma("GCC error \"This method is deprecated, use FF32_frac(..) instead.\"")
#define fastFix32Int(a)             _Pragma("GCC error \"This method is deprecated, use FF32_int(..) instead.\"")
#define fastFix32Round(a)           _Pragma("GCC error \"This method is deprecated, use FF32_round(..) instead.\"")
#define fastFix32ToRoundedInt(a)    _Pragma("GCC error \"This method is deprecated, use FF32_toRoundedInt(..) instead.\"")
#define fastFix32Mul(a, b)          _Pragma("GCC error \"This method is deprecated, use FF32_mul(..) instead.\"")
#define fastFix32Div(a, b)          _Pragma("GCC error \"This method is deprecated, use FF32_div(..) instead.\"")


///////////////////////////////////////////////
//  Vector2D base math functions
///////////////////////////////////////////////

/**
 *  \brief
 *      Return euclidean distance approximation for specified vector.<br>
 *      The returned distance is not 100% perfect but calculation is fast.
 *
 *  \param v
 *      2D vector.
 */
u32 V2D_S32_getApproximatedDistance(V2s32* v);

/**
 *  \brief
 *      Compute and return the angle in degree (fix16) between the 2 points pt1 and pt2.
 */
fix16 V2D_F16_getAngle(V2f16* pt1, V2f16* pt2);

/**
 *  \brief
 *      Move the given point by the given 'distance' in the 'angle' direction.
 * 
 *  \param pt
 *      point to move
 *  \param ang
 *      angle in degree
 *  \param dist
 *      distance to move
 */
void V2D_F16_movePoint(V2f16* pt, fix16 ang, fix16 dist);
/**
 *  \brief
 *      Compute the new position of the point defined by (x1, y1) by moving it by the given 'distance' in the 'angle' direction
 *      and store the resultin (x2, y2).
* 
 *  \param pt
 *      point to move
 *  \param ang
 *      angle in degree
 *  \param dist
 *      distance to move
 *  \param cosAdj
 *      cosine adjustment
 *  \param sinAdj
 *      sine adjustment
*/
void V2D_F16_movePointEx(V2f16* pt, fix16 ang, fix16 dist, fix16 cosAdj, fix16 sinAdj);


// Deprecated functions
#define getApproximatedDistanceV(dx, dy)     _Pragma("GCC error \"This method is deprecated, use V2D_S32_getApproximatedDistance(..) instead.\"")

#endif // _MATHS_H_
