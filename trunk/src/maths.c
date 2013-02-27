#include "config.h"
#include "types.h"

#include "maths.h"

#include "tab_cnv.h"
#include "vdp.h"


u16 randbase;


u16 random()
{
    randbase ^= (randbase >> 1) ^ GET_HVCOUNTER;
    randbase ^= (randbase << 1);

    return randbase;
}


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


#define QSORT(type)                                     \
    u16 Partition_##type(type *data, u16 p, u16 r)      \
    {                                                   \
        type x = data[p];                               \
        u16 i = p - 1;                                  \
        u16 j = r + 1;                                  \
                                                        \
        while (TRUE)                                    \
        {                                               \
            i++;                                        \
            while ((i < r) && (data[i] < x)) i++;       \
            j--;                                        \
            while ((j > p) && (data[j] > x)) j--;       \
                                                        \
            if (i < j)                                  \
            {                                           \
                type tmp;                               \
                                                        \
                tmp = data[i];                          \
                data[i] = data[j];                      \
                data[j] = tmp;                          \
            }                                           \
            else                                        \
                return j;                               \
        }                                               \
    }                                                   \
                                                        \
    void QSort_##type(type *data, u16 p, u16 r)         \
    {                                                   \
        if (p < r)                                      \
        {                                               \
            u16 q = Partition_##type(data, p, r);       \
            QSort_##type(data, p, q);                   \
            QSort_##type(data, q + 1, r);               \
        }                                               \
    }


QSORT(u8)
QSORT(s8)
QSORT(u16)
QSORT(s16)
QSORT(u32)
QSORT(s32)

