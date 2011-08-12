#ifndef _MATHS_H_
#define _MATHS_H_


extern const fix32 sintab32[1024];
extern const fix16 sintab16[1024];

extern const fix16 log2tab16[0x10000];
extern const fix16 log10tab16[0x10000];
extern const fix16 sqrttab16[0x10000];


#ifndef PI
#define PI 3.14159265358979323846
#endif


#define FIX32_INT_BITS              22
#define FIX32_FRAC_BITS             (32 - FIX32_INT_BITS)

#define FIX32_INT_MASK              (((1 << FIX32_INT_BITS) - 1) << FIX32_FRAC_BITS)
#define FIX32_FRAC_MASK             ((1 << FIX32_FRAC_BITS) - 1)

#define FIX32(value)                ((fix32) ((value) * (1 << FIX32_FRAC_BITS)))

#define roundFix32(value)           \

#define intToFix32(value)           ((value) << FIX32_FRAC_BITS)
#define fix32ToInt(value)           ((value) >> FIX32_FRAC_BITS)

#define fix32Round(value)           \
(fix32Frac(value) > FIX32(0.5))?fix32Int(value + FIX32(1)) + 1:fix32Int(value)

#define fix32ToRoundedInt(value)    \
(fix32Frac(value) > FIX32(0.5))?fix32ToInt(value) + 1:fix32ToInt(value)

#define fix32Frac(value)            ((value) & FIX32_FRAC_MASK)
#define fix32Int(value)             ((value) & FIX32_INT_MASK)

#define fix32Add(val1, val2)        ((val1) + (val2))
#define fix32Sub(val1, val2)        ((val1) - (val2))
#define fix32Neg(value)             (0 - (value))

#define fix32Mul(val1, val2)        (((val1) * (val2)) >> FIX32_FRAC_BITS)
#define fix32Div(val1, val2)        (((val1) << FIX32_FRAC_BITS) / (val2))



#define FIX16_INT_BITS              10
#define FIX16_FRAC_BITS             (16 - FIX16_INT_BITS)

#define FIX16_INT_MASK              (((1 << FIX16_INT_BITS) - 1) << FIX16_FRAC_BITS)
#define FIX16_FRAC_MASK             ((1 << FIX16_FRAC_BITS) - 1)

#define FIX16(value)                ((fix16) ((value) * (1 << FIX16_FRAC_BITS)))

#define intToFix16(value)           ((value) << FIX16_FRAC_BITS)
#define fix16ToInt(value)           ((value) >> FIX16_FRAC_BITS)

#define fix16Round(value)           \
(fix16Frac(value) > FIX16(0.5))?fix16Int(value + FIX16(1)) + 1:fix16Int(value)

#define fix16ToRoundedInt(value)    \
(fix16Frac(value) > FIX16(0.5))?fix16ToInt(value) + 1:fix16ToInt(value)

#define fix16Frac(value)            ((value) & FIX16_FRAC_MASK)
#define fix16Int(value)             ((value) & FIX16_INT_MASK)

#define fix16Add(val1, val2)        ((val1) + (val2))
#define fix16Sub(val1, val2)        ((val1) - (val2))
#define fix16Neg(value)             (0 - (value))

#define fix16Mul(val1, val2)        (((val1) * (val2)) >> FIX16_FRAC_BITS)
#define fix16Div(val1, val2)        (((val1) << FIX16_FRAC_BITS) / (val2))

#define fix16Log2(v)                log2tab16[v]
#define fix16Log10(v)               log10tab16[v]
#define fix16Sqrt(v)                sqrttab16[v]


#define fix32ToFix16(value)         (((value) << FIX16_FRAC_BITS) >> FIX32_FRAC_BITS)
#define fix16ToFix32(value)         (((value) << FIX32_FRAC_BITS) >> FIX16_FRAC_BITS)

// sin & cos input : 0 --> 2PI  =  0 --> 1024

#define sinFix32(value)             sintab32[(value) & 1023]
#define cosFix32(value)             sintab32[((value) + 256) & 1023]

#define sinFix16(value)             sintab16[(value) & 1023]
#define cosFix16(value)             sintab16[((value) + 256) & 1023]


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


u16 random();

u32 intToBCD(u32 value);

void QSort_u8(u8 *data, u16 left, u16 right);
void QSort_s8(s8 *data, u16 left, u16 right);
void QSort_u16(u16 *data, u16 left, u16 right);
void QSort_s16(s16 *data, u16 left, u16 right);
void QSort_u32(u32 *data, u16 left, u16 right);
void QSort_s32(s32 *data, u16 left, u16 right);


#endif // _MATHS_H_
