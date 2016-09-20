/**
 *  \file maths.h
 *  \brief Mathematical methods.
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides basic maths methods.<br>
 * You can find a tutorial about how use maths with SGDK <a href="http://code.google.com/p/sgdk/wiki/SGDK_Math">here</a>.<br>
 */

#ifndef _MATHS_H_
#define _MATHS_H_


extern const fix32 sintab32[1024];
extern const fix16 sintab16[1024];

#if (MATH_BIG_TABLES != 0)
extern const fix16 log2tab16[0x10000];
extern const fix16 log10tab16[0x10000];
extern const fix16 sqrttab16[0x10000];
#endif


/**
 *  \brief
 *      Returns the lowest value between X an Y.
 */
#define min(X, Y)   (((X) < (Y))?(X):(Y))

/**
 *  \brief
 *      Returns the highest value between X an Y.
 */
#define max(X, Y)   (((X) > (Y))?(X):(Y))

/**
 *  \brief
 *      Returns the absolute value of X.
 */
#define abs(X)      (((X) < 0)?-(X):(X))

#ifndef PI
/**
 *  \brief
 *      PI number (3,1415..)
 */
#define PI 3.14159265358979323846
#endif


#define FIX32_INT_BITS              22
#define FIX32_FRAC_BITS             (32 - FIX32_INT_BITS)

#define FIX32_INT_MASK              (((1 << FIX32_INT_BITS) - 1) << FIX32_FRAC_BITS)
#define FIX32_FRAC_MASK             ((1 << FIX32_FRAC_BITS) - 1)

/**
 *  \brief
 *      Convert specified value to fix32.
 *
 *  EX:<br>
 *      f32 v = FIX32(34.567);
 */
#define FIX32(value)                ((fix32) ((value) * (1 << FIX32_FRAC_BITS)))

/**
 *  \brief
 *      Convert integer to fix32.
 */
#define intToFix32(value)           ((value) << FIX32_FRAC_BITS)
/**
 *  \brief
 *      Convert fix32 to integer.
 */
#define fix32ToInt(value)           ((value) >> FIX32_FRAC_BITS)

/**
 *  \brief
 *      Round the specified value to nearest integer (fix32).
 */
#define fix32Round(value)           \
    ((fix32Frac(value) > FIX32(0.5))?fix32Int(value + FIX32(1)) + 1:fix32Int(value))

/**
 *  \brief
 *      Round and convert the specified fix32 value to integer.
 */
#define fix32ToRoundedInt(value)    \
    ((fix32Frac(value) > FIX32(0.5))?fix32ToInt(value) + 1:fix32ToInt(value))

/**
 *  \brief
 *      Return fractional part of the specified value (fix32).
 */
#define fix32Frac(value)            ((value) & FIX32_FRAC_MASK)
/**
 *  \brief
 *      Return integer part of the specified value (fix32).
 */
#define fix32Int(value)             ((value) & FIX32_INT_MASK)

/**
 *  \brief
 *      Compute and return the result of the addition of val1 and val2 (fix32).
 */
#define fix32Add(val1, val2)        ((val1) + (val2))
/**
 *  \brief
 *      Compute and return the result of the substraction of val2 from val1 (fix32).
 */
#define fix32Sub(val1, val2)        ((val1) - (val2))
/**
 *  \brief
 *      Return negate of specified value (fix32).
 */
#define fix32Neg(value)             (0 - (value))

/**
 *  \brief
 *      Compute and return the result of the multiplication of val1 and val2 (fix32).
 */
#define fix32Mul(val1, val2)        (((val1) * (val2)) >> FIX32_FRAC_BITS)
/**
 *  \brief
 *      Compute and return the result of the division of val1 by val2 (fix32).
 */
#define fix32Div(val1, val2)        (((val1) << FIX32_FRAC_BITS) / (val2))


#define FIX16_INT_BITS              10
#define FIX16_FRAC_BITS             (16 - FIX16_INT_BITS)

#define FIX16_INT_MASK              (((1 << FIX16_INT_BITS) - 1) << FIX16_FRAC_BITS)
#define FIX16_FRAC_MASK             ((1 << FIX16_FRAC_BITS) - 1)

/**
 *  \brief
 *      Convert specified value to fix16
 *
 *  EX:<br>
 *      f16 v = FIX16(-27.12);
 */
#define FIX16(value)                ((fix16) ((value) * (1 << FIX16_FRAC_BITS)))

/**
 *  \brief
 *      Convert integer to fix16.
 */
#define intToFix16(value)           ((value) << FIX16_FRAC_BITS)
/**
 *  \brief
 *      Convert fix16 to integer.
 */
#define fix16ToInt(value)           ((value) >> FIX16_FRAC_BITS)

/**
 *  \brief
 *      Round the specified value to nearest integer (fix16).
 */
#define fix16Round(value)           \
    (fix16Frac(value) > FIX16(0.5))?fix16Int(value + FIX16(1)) + 1:fix16Int(value)

/**
 *  \brief
 *      Round and convert the specified fix16 value to integer.
 */
#define fix16ToRoundedInt(value)    \
    (fix16Frac(value) > FIX16(0.5))?fix16ToInt(value) + 1:fix16ToInt(value)

/**
 *  \brief
 *      Return fractional part of the specified value (fix16).
 */
#define fix16Frac(value)            ((value) & FIX16_FRAC_MASK)
/**
 *  \brief
 *      Return integer part of the specified value (fix16).
 */
#define fix16Int(value)             ((value) & FIX16_INT_MASK)

/**
 *  \brief
 *      Compute and return the result of the addition of val1 and val2 (fix16).
 */
#define fix16Add(val1, val2)        ((val1) + (val2))
/**
 *  \brief
 *      Compute and return the result of the substraction of val2 from val1 (fix16).
 */
#define fix16Sub(val1, val2)        ((val1) - (val2))
/**
 *  \brief
 *      Return negate of specified value (fix16).
 */
#define fix16Neg(value)             (0 - (value))

/**
 *  \brief
 *      Compute and return the result of the multiplication of val1 and val2 (fix16).
 */
#define fix16Mul(val1, val2)        (((val1) * (val2)) >> FIX16_FRAC_BITS)
/**
 *  \brief
 *      Compute and return the result of the division of val1 by val2 (fix16).
 */
#define fix16Div(val1, val2)        (((val1) << FIX16_FRAC_BITS) / (val2))


#if (MATH_BIG_TABLES != 0)

/**
 *  \brief
 *      Compute and return the result of the Log2 of specified value (fix16).
 */
#define fix16Log2(v)                log2tab16[v]
/**
 *  \brief
 *      Compute and return the result of the Log10 of specified value (fix16).
 */
#define fix16Log10(v)               log10tab16[v]
/**
 *  \brief
 *      Compute and return the result of the root square of specified value (fix16).
 */
#define fix16Sqrt(v)                sqrttab16[v]

#endif


/**
 *  \brief
 *      Convert specified fix32 value to fix16.
 */
#define fix32ToFix16(value)         (((value) << FIX16_FRAC_BITS) >> FIX32_FRAC_BITS)
/**
 *  \brief
 *      Convert specified fix16 value to fix32.
 */
#define fix16ToFix32(value)         (((value) << FIX32_FRAC_BITS) >> FIX16_FRAC_BITS)

/**
 *  \brief
 *      Compute sinus of specified value and return it as fix32.<br>
 *      The input value is an integer defined as [0..1024] range corresponding to radian [0..2PI] range.
 */
#define sinFix32(value)             sintab32[(value) & 1023]
/**
 *  \brief
 *      Compute cosinus of specified value and return it as fix32.<br>
 *      The input value is an integer defined as [0..1024] range corresponding to radian [0..2PI] range.
 */
#define cosFix32(value)             sintab32[((value) + 256) & 1023]

/**
 *  \brief
 *      Compute sinus of specified value and return it as fix16.<br>
 *      The input value is an integer defined as [0..1024] range corresponding to radian [0..2PI] range.
 */
#define sinFix16(value)             sintab16[(value) & 1023]
/**
 *  \brief
 *      Compute cosinus of specified value and return it as fix16.<br>
 *      The input value is an integer defined as [0..1024] range corresponding to radian [0..2PI] range.
 */
#define cosFix16(value)             sintab16[((value) + 256) & 1023]


// 2D STUFF

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
 *      2x2 Matrice structure - f16 (fix16) type.<br>
 *      Internally uses 2 2D vectors.
 */
typedef struct
{
    Vect2D_f16 a;
    Vect2D_f16 b;
} Mat2D_f16;


// 3D STUFF

/**
 *  \brief
 *      3D Vector structure - f16 (fix16) type.
 */
typedef struct
{
    fix16 x;
    fix16 y;
    fix16 z;
} Vect3D_f16;

/**
 *  \brief
 *      3x3 Matrice structure - f16 (fix16) type.<br>
 *      Internally uses 3 3D vectors.
 */
typedef struct
{
    Vect3D_f16 a;
    Vect3D_f16 b;
    Vect3D_f16 c;
} Mat3D_f16;


// 4D STUFF

/**
 *  \brief
 *      4D Vector structure - f16 (fix16) type.
 */
typedef struct
{
    fix16 x;
    fix16 y;
    fix16 z;
    fix16 w;
} Vect4D_f16;

/**
 *  \brief
 *      4x4 Matrice structure - f16 (fix16) type.<br>
 *      Internally uses 4 4D vectors.
 */
typedef struct
{
    Vect4D_f16 a;
    Vect4D_f16 b;
    Vect4D_f16 c;
    Vect4D_f16 d;
} Mat4D_f16;


/**
 *  \brief
 *      Binary to Decimal conversion.
 *
 *  \param value
 *      Value to convert.
 */
u32 intToBCD(u32 value);

/**
 *  \deprecated
 *      Use #getApproximatedDistance(..) instead.
 */
u32 distance_approx(s32 dx, s32 dy);
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
 *      Return 16.16 fixed point *approximation* of log2 of the specified 16.16 fixed point value.
 *      Ex:<br>
 *      getLog2(1 << 16) = 0<br>
 *      getLog2(12345 << 16) = ~9.5 (real value = ~13.6)<br>
 *
 *  \param value
 *      16.16 fixed point value to return log2 of
 */
s32 getApproximatedLog2(s32 value);
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
u16 getLog2Int(u32 value);



#endif // _MATHS_H_
