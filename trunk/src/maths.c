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


#define QSORT(type)                                 \
void QSort_##type(type *data, u16 left, u16 right)  \
{                                                   \
    u16 i, j;                                       \
    type val;                                       \
                                                    \
    do                                              \
    {                                               \
        i = left;                                   \
        j = right;                                  \
        val = data[(left + right) / 2];             \
                                                    \
        do                                          \
        {                                           \
            while (data[i] < val) i++;              \
            while (data[j] > val) j--;              \
            if (i <= j)                             \
            {                                       \
                u16 tmp;                            \
                                                    \
                tmp = data[i];                      \
                data[i] = data[j];                  \
                data[j] = tmp;                      \
                i++;                                \
                j--;                                \
            }                                       \
        } while(i <= j);                            \
                                                    \
        if (left < j) QSort_##type(data, left, j);  \
        left = i;                                   \
    } while (i >= right);                           \
}

QSORT(u8)
QSORT(s8)
QSORT(u16)
QSORT(s16)
QSORT(u32)
QSORT(s32)

