#include "config.h"
#include "types.h"

#include "sega.h"
#include "tools.h"

#include "tab_cnv.h"
#include "maths.h"
#include "timer.h"
#include "vdp.h"
#include "vdp_tile.h"
#include "vdp_bg.h"


u16 randbase;


void fastmemset(void *to, int value, size_t len)
{
    u8 *dst8;
    u32 *dst32;
    u32 value32;
    int cnt;

    // small data set --> use standard memset
    if (len < 16)
    {
        memset(to, value, len);
        return;
    }

    dst8 = (u8 *) to;

    // align to dword
    while ((u32) dst8 & 3)
    {
        *dst8++ = value;
        len--;
    }

    value32 = cnv_8to32_tab[value];
    dst32 = (u32 *) dst8;

    cnt = len >> 5;
    while (cnt--)
    {
        *dst32++ = value32;
        *dst32++ = value32;
        *dst32++ = value32;
        *dst32++ = value32;
        *dst32++ = value32;
        *dst32++ = value32;
        *dst32++ = value32;
        *dst32++ = value32;
    }

    cnt = (len >> 2) & 7;
    while (cnt--) *dst32++ = value32;

    dst8 = (u8 *) dst32;

    cnt = len & 3;
    while (cnt--) *dst8++ = value;
}

void fastmemcpy(void *to, const void *from, size_t len)
{
    const u8 *src8;
    const u32 *src32;
    u8 *dst8;
    u32 *dst32;
    int cnt;

    // small transfert or misalignement --> use standard memcpy
    if ((len < 16) || (((u32) to & 3) != ((u32) from & 3)))
    {
        memcpy(to, from, len);
        return;
    }

    dst8 = (u8 *) to;
    src8 = (u8 *) from;

    // align to dword
    while ((u32) dst8 & 3)
    {
        *dst8++ = *src8++;
        len--;
    }

    dst32 = (u32 *) dst8;
    src32 = (u32 *) src8;

    cnt = len >> 5;
    while (cnt--)
    {
        *dst32++ = *src32++;
        *dst32++ = *src32++;
        *dst32++ = *src32++;
        *dst32++ = *src32++;
        *dst32++ = *src32++;
        *dst32++ = *src32++;
        *dst32++ = *src32++;
        *dst32++ = *src32++;
    }

    cnt = (len >> 2) & 7;
    while (cnt--) *dst32++ = *src32++;

    dst8 = (u8 *) dst32;
    src8 = (u8 *) src32;

    cnt = len & 3;
    while (cnt--) *dst8++ = *src8++;
}

// C ISO compatible
void memset(void *to, int value, size_t len)
{
    u8 *dst;
    int cnt;

    dst = (u8 *) to;
    cnt = len;
    while(cnt--) *dst++ = value;
}

// C ISO compatible
void memcpy(void *to, const void *from, size_t len)
{
    u8 *dst;
    u8 *src;
    int cnt;

    dst = (u8 *) to;
    src = (u8 *) from;
    cnt = len;
    while(cnt--) *dst++ = *src++;
}


u32 strlen(const char *str)
{
    const char *src;

    src = str;
    while (*src++);

    return (src - str) - 1;
}

char* strcpy(register char *to, const char *from)
{
    const char *src;
    char *dst;

    src = from;
    dst = to;

    while ((*dst++ = *src++));

    return to;
}


u32 intToStr(s32 value, char *str, s32 minsize)
{
    u32 res;
    char data[16];
    char *dst;

    data[15] = 0;
    dst = &data[15];
    if (value < 0) res = -value;
    else res = value;

    while (res)
    {
        *--dst = '0' + (res % 10);
        res /= 10;
        minsize--;
    }
    while (minsize > 0)
    {
        *--dst = '0';
        minsize--;
    }
    if (value < 0) *--dst = '-';

    strcpy(str, dst);

    return strlen(str);
}

u32 uintToStr(u32 value, char *str, s32 minsize)
{
    u32 res;
    char data[16];
    char *dst;

    data[15] = 0;
    dst = &data[15];
    res = value;

    while (res)
    {
        *--dst = '0' + (res % 10);
        res /= 10;
        minsize--;
    }
    while (minsize > 0)
    {
        *--dst = '0';
        minsize--;
    }

    strcpy(str, dst);

    return strlen(str);
}

u32 fix32ToStr(fix32 value, char *str)
{
    u32 len;
    fix32 v;

    len = 0;
    if (value < 0)
    {
        v = -value;
        str[len++] = '-';
    }
    else v = value;
    len += uintToStr(fix32ToInt(v), &str[len], 1);
    str[len++] = '.';
    len += uintToStr((fix32Frac(v) * 1000) / (1 << FIX32_FRAC_BITS), &str[len], 3);

    return len;
}

u32 fix16ToStr(fix16 value, char *str)
{
    u32 len;
    fix16 v;

    len = 0;
    if (value < 0)
    {
        v = -value;
        str[len++] = '-';
    }
    else v = value;
    len += uintToStr(fix16ToInt(v), &str[len], 1);
    str[len++] = '.';
    len += uintToStr((fix16Frac(v) * 1000) / (1 << FIX16_FRAC_BITS), &str[len], 3);

    return len;
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


u32 getFPS()
{
    static u32 framecnt;
    static u32 last;
    static u32 result;

    const u32 current = getSubTick();
    const u32 delta = current - last;

	if (delta > 19200)
    {
        result = (76800 * framecnt) / delta;
        if (result > 999) result = 999;
        last = current;
        framecnt = 1;
    }
	else framecnt++;

	return result;
}

void showFPS()
{
    char str[16];

    // display FPS
    intToStr(getFPS(), str, 1);
    VDP_clearTextBG(APLAN, 1, 1, 3);
    VDP_drawTextBG(APLAN, str, 0x8000, 1, 1);
}


u16 random()
{
    randbase ^= (randbase >> 1) ^ GET_HVCOUNTER;
    randbase ^= (randbase << 1);

    return randbase;
}
