#ifndef _MATHS_H_
#define _MATHS_H_


extern const fix32 sintab32[1024];
extern const fix16 sintab16[1024];

#ifdef MATHS_BIG_TABLE
extern const fix16 log2tab16[65536];
extern const fix16 pow2tab16[65536];
#endif

#ifndef PI
#define PI 3.14159265358979323846
#endif


#define FIX32_INT_BITS              22
#define FIX32_FRAC_BITS             (32 - FIX32_INT_BITS)

#define FIX32_INT_MASK              (((1 << FIX32_INT_BITS) - 1) << FIX32_FRAC_BITS)
#define FIX32_FRAC_MASK             ((1 << FIX32_FRAC_BITS) - 1)

#define FIX32(value)                ((fix32) ((value) * (1 << FIX32_FRAC_BITS)))

#define intToFix32(value)           ((value) << FIX32_FRAC_BITS)
#define fix32ToInt(value)           ((value) >> FIX32_FRAC_BITS)

#define fix32Frac(value)            ((value) & FIX32_FRAC_MASK)
#define fix32Int(value)             ((value) & FIX32_INT_MASK)

#define fix32Add(val1, val2)        ((val1) + (val2))
#define fix32Sub(val1, val2)        ((val1) - (val2))
#define fix32Neg(value)             (0 - (value))

#define fix32Mul(val1, val2)        (((val1) * (val2)) >> FIX32_FRAC_BITS)
#define fix32Div(val1, val2)        (((val1) << FIX32_FRAC_BITS) / (val2))

#define fix32Sin(value)             sintab32[(value) & 1023]
#define fix32Cos(value)             sintab32[((value) + 256) & 1023]


#define FIX16_INT_BITS              10
#define FIX16_FRAC_BITS             (16 - FIX16_INT_BITS)

#define FIX16_INT_MASK              (((1 << FIX16_INT_BITS) - 1) << FIX16_FRAC_BITS)
#define FIX16_FRAC_MASK             ((1 << FIX16_FRAC_BITS) - 1)

#define FIX16(value)                ((fix16) ((value) * (1 << FIX16_FRAC_BITS)))

#define round16(value)              \
(fix16Frac(value) > FIX16(0.5))?fix16ToInt(value) + 1:fix16ToInt(value)

#define intToFix16(value)           ((value) << FIX16_FRAC_BITS)
#define fix16ToInt(value)           ((value) >> FIX16_FRAC_BITS)

#define fix16Frac(value)            ((value) & FIX16_FRAC_MASK)
#define fix16Int(value)             ((value) & FIX16_INT_MASK)

#define fix16Add(val1, val2)        ((val1) + (val2))
#define fix16Sub(val1, val2)        ((val1) - (val2))
#define fix16Neg(value)             (0 - (value))

#define fix16Mul(val1, val2)        (((val1) * (val2)) >> FIX16_FRAC_BITS)
#define fix16Div(val1, val2)        (((val1) << FIX16_FRAC_BITS) / (val2))

#define fix16Sin(value)             sintab16[(value) & 1023]
#define fix16Cos(value)             sintab16[((value) + 256) & 1023]

#ifdef MATHS_BIG_TABLE

#define fix16Log2(v)    log2tab16[v]
#define fix16Log10(v)   log10tab16[v]
#define fix16Pow2(v)    pow2tab16[v]
#define fix16Sqrt(v)    sqrttab16[v]

#endif

#define fix32ToFix16(value)         (((value) << FIX16_FRAC_BITS) >> FIX32_FRAC_BITS)
#define fix16ToFix32(value)         (((value) << FIX32_FRAC_BITS) >> FIX16_FRAC_BITS)


// 2D STUFF

typedef struct
{
    u16 x;
    u16 y;
} Vect2D_u16;

typedef struct
{
    s16 x;
    s16 y;
} Vect2D_s16;

typedef struct
{
    fix16 x;
    fix16 y;
} Vect2D_f16;


// 3D STUFF

typedef struct
{
    fix16 x;
    fix16 y;
    fix16 z;
} Vect3D_f16;

// 4D STUFF

typedef struct
{
    fix16 x;
    fix16 y;
    fix16 z;
    fix16 w;
} Vect4D_f16;


#endif // _MATHS_H_
