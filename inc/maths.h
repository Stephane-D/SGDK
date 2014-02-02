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
 *  \def min
 *      Returns te lowest value between X an Y.
 */
#define min(X, Y) (X < Y)?X:Y

/**
 *  \def max
 *      Returns te highest value between X an Y.
 */
#define max(X, Y) (X > Y)?X:Y


#ifndef PI
/**
 *  \def PI
 *      PI number (3,1415..)
 */
#define PI 3.14159265358979323846
#endif


#define FIX32_INT_BITS              22
#define FIX32_FRAC_BITS             (32 - FIX32_INT_BITS)

#define FIX32_INT_MASK              (((1 << FIX32_INT_BITS) - 1) << FIX32_FRAC_BITS)
#define FIX32_FRAC_MASK             ((1 << FIX32_FRAC_BITS) - 1)

/**
 *  \def FIX32
 *      Convert specified value to fix32
 *
 *  Ex: f32 v = FIX32(34.567);
 */
#define FIX32(value)                ((fix32) ((value) * (1 << FIX32_FRAC_BITS)))

/**
 *  \def intToFix32
 *      Convert integer to fix32.
 */
#define intToFix32(value)           ((value) << FIX32_FRAC_BITS)
/**
 *  \def fix32ToInt
 *      Convert fix32 to integer.
 */
#define fix32ToInt(value)           ((value) >> FIX32_FRAC_BITS)

/**
 *  \def fix32Round
 *      Round the specified value to nearest integer (fix32).
 */
#define fix32Round(value)           \
    (fix32Frac(value) > FIX32(0.5))?fix32Int(value + FIX32(1)) + 1:fix32Int(value)

/**
 *  \def fix32ToRoundedInt
 *      Round and convert the specified fix32 value to integer.
 */
#define fix32ToRoundedInt(value)    \
    (fix32Frac(value) > FIX32(0.5))?fix32ToInt(value) + 1:fix32ToInt(value)

/**
 *  \def fix32Frac
 *      Return fractional part of the specified value (fix32).
 */
#define fix32Frac(value)            ((value) & FIX32_FRAC_MASK)
/**
 *  \def fix32Int
 *      Return integer part of the specified value (fix32).
 */
#define fix32Int(value)             ((value) & FIX32_INT_MASK)

/**
 *  \def fix32Add
 *      Compute and return the result of the addition of val1 and val2 (fix32).
 */
#define fix32Add(val1, val2)        ((val1) + (val2))
/**
 *  \def fix32Sub
 *      Compute and return the result of the substraction of val2 from val1 (fix32).
 */
#define fix32Sub(val1, val2)        ((val1) - (val2))
/**
 *  \def fix32Neg
 *      Return negate of specified value (fix32).
 */
#define fix32Neg(value)             (0 - (value))

/**
 *  \def fix32Mul
 *      Compute and return the result of the multiplication of val1 and val2 (fix32).
 */
#define fix32Mul(val1, val2)        (((val1) * (val2)) >> FIX32_FRAC_BITS)
/**
 *  \def fix32Div
 *      Compute and return the result of the division of val1 by val2 (fix32).
 */
#define fix32Div(val1, val2)        (((val1) << FIX32_FRAC_BITS) / (val2))


#define FIX16_INT_BITS              10
#define FIX16_FRAC_BITS             (16 - FIX16_INT_BITS)

#define FIX16_INT_MASK              (((1 << FIX16_INT_BITS) - 1) << FIX16_FRAC_BITS)
#define FIX16_FRAC_MASK             ((1 << FIX16_FRAC_BITS) - 1)

/**
 *  \def FIX16
 *      Convert specified value to fix16
 *
 *  Ex : f16 v = FIX16(-27.12);
 */
#define FIX16(value)                ((fix16) ((value) * (1 << FIX16_FRAC_BITS)))

/**
 *  \def intToFix16
 *      Convert integer to fix16.
 */
#define intToFix16(value)           ((value) << FIX16_FRAC_BITS)
/**
 *  \def fix16ToInt
 *      Convert fix16 to integer.
 */
#define fix16ToInt(value)           ((value) >> FIX16_FRAC_BITS)

/**
 *  \def fix16Round
 *      Round the specified value to nearest integer (fix16).
 */
#define fix16Round(value)           \
    (fix16Frac(value) > FIX16(0.5))?fix16Int(value + FIX16(1)) + 1:fix16Int(value)

/**
 *  \def fix16ToRoundedInt
 *      Round and convert the specified fix16 value to integer.
 */
#define fix16ToRoundedInt(value)    \
    (fix16Frac(value) > FIX16(0.5))?fix16ToInt(value) + 1:fix16ToInt(value)

/**
 *  \def fix16Frac
 *      Return fractional part of the specified value (fix16).
 */
#define fix16Frac(value)            ((value) & FIX16_FRAC_MASK)
/**
 *  \def fix16Int
 *      Return integer part of the specified value (fix16).
 */
#define fix16Int(value)             ((value) & FIX16_INT_MASK)

/**
 *  \def fix16Add
 *      Compute and return the result of the addition of val1 and val2 (fix16).
 */
#define fix16Add(val1, val2)        ((val1) + (val2))
/**
 *  \def fix16Sub
 *      Compute and return the result of the substraction of val2 from val1 (fix16).
 */
#define fix16Sub(val1, val2)        ((val1) - (val2))
/**
 *  \def fix16Neg
 *      Return negate of specified value (fix16).
 */
#define fix16Neg(value)             (0 - (value))

/**
 *  \def fix16Mul
 *      Compute and return the result of the multiplication of val1 and val2 (fix16).
 */
#define fix16Mul(val1, val2)        (((val1) * (val2)) >> FIX16_FRAC_BITS)
/**
 *  \def fix16Div
 *      Compute and return the result of the division of val1 by val2 (fix16).
 */
#define fix16Div(val1, val2)        (((val1) << FIX16_FRAC_BITS) / (val2))


#if (MATH_BIG_TABLES != 0)

/**
 *  \def fix16Log2
 *      Compute and return the result of the Log2 of specified value (fix16).
 */
#define fix16Log2(v)                log2tab16[v]
/**
 *  \def fix16Log10
 *      Compute and return the result of the Log10 of specified value (fix16).
 */
#define fix16Log10(v)               log10tab16[v]
/**
 *  \def fix16Sqrt
 *      Compute and return the result of the root square of specified value (fix16).
 */
#define fix16Sqrt(v)                sqrttab16[v]

#endif


/**
 *  \def fix32ToFix16
 *      Convert specified fix32 value to fix16.
 */
#define fix32ToFix16(value)         (((value) << FIX16_FRAC_BITS) >> FIX32_FRAC_BITS)
/**
 *  \def fix16ToFix32
 *      Convert specified fix16 value to fix32.
 */
#define fix16ToFix32(value)         (((value) << FIX32_FRAC_BITS) >> FIX16_FRAC_BITS)

/**
 *  \def sinFix32
 *      Compute sinus of specified value and return it as fix32.<br>
 *      The input value is an integer defined as [0..1024] range corresponding to radian [0..2PI] range.
 */
#define sinFix32(value)             sintab32[(value) & 1023]
/**
 *  \def cosFix32
 *      Compute cosinus of specified value and return it as fix32.<br>
 *      The input value is an integer defined as [0..1024] range corresponding to radian [0..2PI] range.
 */
#define cosFix32(value)             sintab32[((value) + 256) & 1023]

/**
 *  \def sinFix16
 *      Compute sinus of specified value and return it as fix16.<br>
 *      The input value is an integer defined as [0..1024] range corresponding to radian [0..2PI] range.
 */
#define sinFix16(value)             sintab16[(value) & 1023]
/**
 *  \def cosFix16
 *      Compute cosinus of specified value and return it as fix16.<br>
 *      The input value is an integer defined as [0..1024] range corresponding to radian [0..2PI] range.
 */
#define cosFix16(value)             sintab16[((value) + 256) & 1023]


// 2D STUFF

/**
 *  \struct Vect2D_u16
 *      2D Vector structure - u16 type.
 */
typedef struct
{
    u16 x;
    u16 y;
} Vect2D_u16;

/**
 *  \struct Vect2D_s16
 *      2D Vector structure - s16 type.
 */
typedef struct
{
    s16 x;
    s16 y;
} Vect2D_s16;

/**
 *  \struct Vect2D_u32
 *      2D Vector structure - u32 type.
 */
typedef struct
{
    u32 x;
    u32 y;
} Vect2D_u32;

/**
 *  \struct Vect2D_s32
 *      2D Vector structure - s32 type.
 */
typedef struct
{
    s32 x;
    s32 y;
} Vect2D_s32;

/**
 *  \struct Vect2D_f16
 *      2D Vector structure - f16 (fix16) type.
 */
typedef struct
{
    fix16 x;
    fix16 y;
} Vect2D_f16;

/**
 *  \struct Mat2D_f16
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
 *  \struct Vect3D_f16
 *      3D Vector structure - f16 (fix16) type.
 */
typedef struct
{
    fix16 x;
    fix16 y;
    fix16 z;
} Vect3D_f16;

/**
 *  \struct Mat3D_f16
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
 *  \struct Vect4D_f16
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
 *  \struct Mat4D_f16
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
 *      Return a random u16 integer.
 */
u16 random();

/**
 *  \brief
 *      Binary to Decimal conversion.
 *
 *  \param value
 *      Value to convert.
 */
u32 intToBCD(u32 value);

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
u32 distance_approx(s32 dx, s32 dy);


/**
 *  \brief
 *      Quick sort algo on u8 data array.
 *
 *  \param data
 *      u8 data pointer.
 *  \param left
 *      left index (should be 0).
 *  \param right
 *      right index (should be table size - 1).
 */
void QSort_u8(u8 *data, u16 left, u16 right);
/**
 *  \brief
 *      Quick sort algo on s8 data array.
 *
 *  \param data
 *      s8 data pointer.
 *  \param left
 *      left index (should be 0).
 *  \param right
 *      right index (should be table size - 1).
 */
void QSort_s8(s8 *data, u16 left, u16 right);
/**
 *  \brief
 *      Quick sort algo on u16 data array.
 *
 *  \param data
 *      u16 data pointer.
 *  \param left
 *      left index (should be 0).
 *  \param right
 *      right index (should be table size - 1).
 */
void QSort_u16(u16 *data, u16 left, u16 right);
/**
 *  \brief
 *      Quick sort algo on s16 data array.
 *
 *  \param data
 *      s16 data pointer.
 *  \param left
 *      left index (should be 0).
 *  \param right
 *      right index (should be table size - 1).
 */
void QSort_s16(s16 *data, u16 left, u16 right);
/**
 *  \brief
 *      Quick sort algo on u32 data array.
 *
 *  \param data
 *      u32 data pointer.
 *  \param left
 *      left index (should be 0).
 *  \param right
 *      right index (should be table size - 1).
 */
void QSort_u32(u32 *data, u16 left, u16 right);
/**
 *  \brief
 *      Quick sort algo on s32 data array.
 *
 *  \param data
 *      s32 data pointer.
 *  \param left
 *      left index (should be 0).
 *  \param right
 *      right index (should be table size - 1).
 */
void QSort_s32(s32 *data, u16 left, u16 right);


#endif // _MATHS_H_
