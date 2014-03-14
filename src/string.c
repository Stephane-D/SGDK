#include "config.h"
#include "types.h"

#include "string.h"

#include "tab_cnv.h"
#include "maths.h"


// FORWARD
static u32 uintToStr_(u32 value, char *str, s16 minsize, s16 maxsize);


u32 strlen(const char *str)
{
    const char *src;

    src = str;

    while (*src++);

    return (src - str) - 1;
}

s16 strcmp(const char *str1, const char *str2)
{
    const u8 *p1 = (const u8*) str1;
    const u8 *p2 = (const u8*) str2;
    u8 c1, c2;

    do
    {
        c1 = *p1++;
        c2 = *p2++;
    }
    while (c1 && (c1 == c2));

    return c1 - c2;
}

char* strclr(char *str)
{
    str[0] = 0;

    return str;
}

char* strcpy(char *to, const char *from)
{
    const char *src;
    char *dst;

    src = from;
    dst = to;

    while ((*dst++ = *src++));

    return to;
}

//char* strcpy2(char *to, const char *from)
//{
//	char *pto = to;
//	unsigned int n = 0xFFFF;
//
//	asm volatile ("1:\n"
//	     "\tmove.b (%0)+,(%1)+\n"
//	     "\tbne.b 1b\n" :
//		"=a" (from), "=a" (pto), "=d" (n) :
//		 "0" (from),  "1" (pto), "2" (n) :
//		 "cc", "memory");
//	return to;
//}

char* strcat(char *to, const char *from)
{
    const char *src;
    char *dst;

    src = from;
    dst = to;

    while (*dst++);
    --dst;
    while ((*dst++ = *src++));

    return to;
}


void intToStr(s32 value, char *str, s16 minsize)
{
    u32 v;
    char *dst = str;

    if (value < 0)
    {
        v = -value;
        *dst++ = '-';
    }
    else v = value;

    uintToStr_(v, dst, minsize, 16);
}

void uintToStr(u32 value, char *str, s16 minsize)
{
    uintToStr_(value, str, minsize, 16);
}

static u32 uintToStr_(u32 value, char *str, s16 minsize, s16 maxsize)
{
    u32 res;
    s16 cnt;
    s16 left;
    char data[16];
    char *src;
    char *dst;

    src = &data[16];
    res = value;
    left = minsize;

    cnt = 0;
    while (res)
    {
        *--src = '0' + (res % 10);
        cnt++;
        left--;
        res /= 10;
    }
    while (left > 0)
    {
        *--src = '0';
        cnt++;
        left--;
    }
    if (cnt > maxsize) cnt = maxsize;

    dst = str;
    while(cnt--) *dst++ = *src++;
    *dst = 0;

    return strlen(str);
}

// SLOWER THAN PREVIOUS SOLUTION IN ALMOST CASE
/*
static u32 uintToStr_(u32 value, char *str, s16 minsize, s16 maxsize)
{
    u32 res;
    u16 carry;
    s16 left;
    s16 cnt;
    char data[16];
    char *src;
    char *dst;

    res = value;

    // handle BCD carry
    if (value > 99999999)
    {
        carry = cnv_bcd_tab[value / 99999999];
        res = value % 99999999;
    }
    else
    {
        carry = 0;
        res = intToBCD(value);
    }

    src = &data[16];
    left = minsize;
    cnt = 0;

    while (res)
    {
        *--src = '0' + (res & 0xF);
        res >>= 4;
        cnt++;
        left--;
    }
    while (carry)
    {
        *--src = '0' + (carry & 0xF);
        carry >>= 4;
        cnt++;
        left--;
    }
    while (left > 0)
    {
        *--src = '0';
        cnt++;
        left--;
    }

    if (cnt > maxsize) cnt = maxsize;

    dst = str;
    while(cnt--) *dst++ = *src++;
    *dst = 0;

    return strlen(str);
}
*/

void intToHex(u32 value, char *str, s16 minsize)
{
    u32 res;
    s16 cnt;
    s16 left;
    char data[16];
    char *src;
    char *dst;
    const s16 maxsize = 16;

    src = &data[16];
    res = value;
    left = minsize;

    cnt = 0;
    while (res)
    {
        u8 c;

        c = res & 0xF;
        if (c >= 10) c += ('A' - 10);
        else c += '0';
        *--src = c;
        cnt++;
        left--;
        res >>= 4;
    }
    while (left > 0)
    {
        *--src = '0';
        cnt++;
        left--;
    }
    if (cnt > maxsize) cnt = maxsize;

    dst = str;
    while(cnt--) *dst++ = *src++;
    *dst = 0;
}

void fix32ToStr(fix32 value, char *str, s16 numdec)
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

    len += uintToStr_(fix32ToInt(v), &str[len], 1, 16);
    str[len++] = '.';
    uintToStr_((fix32Frac(v) * 1000) / (1 << FIX32_FRAC_BITS), &str[len], 1, numdec);
}

void fix16ToStr(fix16 value, char *str, s16 numdec)
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

    len += uintToStr_(fix16ToInt(v), &str[len], 1, 16);
    str[len++] = '.';
    uintToStr_((fix16Frac(v) * 1000) / (1 << FIX16_FRAC_BITS), &str[len], 1, numdec);
}

