#include "config.h"
#include "types.h"

#include "string.h"

#include "tab_cnv.h"
#include "maths.h"
#include "memory.h"


#define P01 10
#define P02 100
#define P03 1000
#define P04 10000
#define P05 100000
#define P06 1000000
#define P07 10000000
#define P08 100000000
#define P09 1000000000
#define P10 10000000000

#if (ENABLE_NEWLIB == 0)
static const char uppercase_hexchars[] = "0123456789ABCDEF";
static const char lowercase_hexchars[] = "0123456789abcdef";
#endif  // ENABLE_NEWLIB
static const char digits[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

// FORWARD
static u16 digits10(const u16 v);
static u16 uint16ToStr(u16 value, char *str, u16 minsize);
#if (ENABLE_NEWLIB == 0)
static u16 skip_atoi(const char **s);
#endif  // ENABLE_NEWLIB


#if (ENABLE_NEWLIB == 0)
u16 strlen(const char *str)
{
    const char *src;

    src = str;
    while (*src++);

    return (src - str) - 1;
}

u16 strnlen(const char *str, u16 maxlen)
{
    const char *src;

    for (src = str; maxlen-- && *src != '\0'; ++src)
        /* nothing */;

    return src - str;
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

char* strcpy(char *to, const char *from)
{
    const char *src;
    char *dst;

    src = from;
    dst = to;
    while ((*dst++ = *src++));

    return to;
}

char* strncpy(char *to, const char *from, u16 len)
{
    const char *src;
    char *dst;
    u16 i;

    src = from;
    dst = to;
    i = 0;
    while ((i++ < len) && (*dst++ = *src++));

    // end string by null character
    if (i > len) *dst = 0;

    return to;
}

//char* strcpy2(char *to, const char *from)
//{
//  char *pto = to;
//  unsigned int n = 0xFFFF;
//
//  asm volatile ("1:\n"
//       "\tmove.b (%0)+,(%1)+\n"
//       "\tbne.b 1b\n" :
//      "=a" (from), "=a" (pto), "=d" (n) :
//       "0" (from),  "1" (pto), "2" (n) :
//       "cc", "memory");
//  return to;
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
#endif  // ENABLE_NEWLIB

char* strclr(char *str)
{
    str[0] = 0;

    return str;
}

char *strreplacechar(char *str, char oldc, char newc)
{
    char *s;

    s =  str;
    while(*s)
    {
        if (*s == oldc)
            *s = newc;
        s++;
    }

    return s;
}

u16 intToStr(s32 value, char *str, u16 minsize)
{
    if (value < -500000000)
    {
        *str = '-';
        return intToHex(-value, str + 1, minsize) + 1;
    }

    if (value < 0)
    {
        *str = '-';
        return uintToStr(-value, str + 1, minsize) + 1;
    }
    else return uintToStr(value, str, minsize);
}

u16 uintToStr(u32 value, char *str, u16 minsize)
{
    // the implentation cannot encode > 500000000 value --> use hexa
    if (value > 500000000)
        return intToHex(value, str, minsize);

    u16 len;

    // need to split in 2 conversions ?
    if (value > 10000)
    {
        const u16 v1 = value / (u16) 10000;
        const u16 v2 = value % (u16) 10000;

        len = uint16ToStr(v1, str, (minsize > 4)?(minsize - 4):1);
        len += uint16ToStr(v2, str + len, 4);
    }
    else len = uint16ToStr(value, str, minsize);

    return len;
}

static u16 uint16ToStr(u16 value, char *str, u16 minsize)
{
    u16 length;
    char *dst;
    u16 v;

    length = digits10(value);
    if (length < minsize) length = minsize;
    dst = &str[length];
    *dst = 0;
    v = value;

    while (v >= 100)
    {
        const u16 quot = v / 100;
        const u16 remain = v % 100;

        const u16 i = remain * 2;
        v = quot;

        *--dst = digits[i + 1];
        *--dst = digits[i + 0];
    }

    // handle last 1-2 digits
    if (v < 10) *--dst = '0' + v;
    else
    {
        const u16 i = v * 2;

        *--dst = digits[i + 1];
        *--dst = digits[i + 0];
    }

    // pad with '0'
    while(dst != str) *--dst = '0';

    return length;
}

u16 intToHex(u32 value, char *str, u16 minsize)
{
    u32 res;
    u16 cnt;
    s16 left;
    char data[16];
    char *src;
    char *dst;

    src = &data[16];
    res = value;
    left = (minsize > 15)?15:minsize;

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

    dst = str;
    while(cnt--) *dst++ = *src++;
    *dst = 0;

    return dst - str;
}

void fix16ToStr(fix16 value, char *str, u16 numdec)
{
    char *dst = str;
    fix16 v = value;

    if (v < 0)
    {
        v = -v;
        *dst++ = '-';
    }

    dst += uint16ToStr((u16) F16_toInt(v), dst, 1);
    *dst++ = '.';

    // get fractional part
    const u16 frac = mulu((u16) F16_frac(v), 1000) >> FIX16_FRAC_BITS;
    u16 len = uint16ToStr(frac, dst, 3);

    if (len < numdec)
    {
        // need to add ending '0'
        dst += len;
        while(len++ < numdec) *dst++ = '0';
        // mark end here
        *dst = 0;
    }
    else dst[numdec] = 0;
}

void fix32ToStr(fix32 value, char *str, u16 numdec)
{
    char *dst = str;
    fix32 v = value;

    if (v < 0)
    {
        v = -v;
        *dst++ = '-';
    }

    dst += uintToStr((u32) F32_toInt(v), dst, 1);
    *dst++ = '.';

    // get fractional part
    const u16 frac = mulu((u16) F32_frac(v), 1000) >> FIX32_FRAC_BITS;
    u16 len = uint16ToStr(frac, dst, 3);

    if (len < numdec)
    {
        // need to add ending '0'
        dst += len;
        while(len++ < numdec) *dst++ = '0';
        // mark end here
        *dst = 0;
    }
    else dst[numdec] = 0;
}

void fastFix16ToStr(fastfix16 value, char *str, u16 numdec)
{
    char *dst = str;
    fastfix16 v = value;
    
    if (v < 0)
    {
        v = -v;
        *dst++ = '-';
    }
    
    dst += uint16ToStr((u16) FF16_toInt(v), dst, 1);
    *dst++ = '.';
    
    // get fractional part
    const u16 frac = mulu((u16) FF16_frac(v), 1000) >> FASTFIX16_FRAC_BITS;
    u16 len = uint16ToStr(frac, dst, 3);
    
    if (len < numdec)
    {
        // need to add ending '0'
        dst += len;
        while(len++ < numdec) *dst++ = '0';
        // mark end here
        *dst = 0;
    }
    else dst[numdec] = 0;
}

void fastFix32ToStr(fastfix32 value, char *str, u16 numdec)
{
    char *dst = str;
    fastfix32 v = value;
    
    if (v < 0)
    {
        v = -v;
        *dst++ = '-';
    }
    
    dst += uint16ToStr((u16) FF32_toInt(v), dst, 1);
    *dst++ = '.';
    
    // get fractional part
    const u16 frac = mulu((u16) FF32_frac(v), 10000) >> FASTFIX32_FRAC_BITS;
    u16 len = uint16ToStr(frac, dst, 4);
    
    if (len < numdec)
    {
        // need to add ending '0'
        dst += len;
        while(len++ < numdec) *dst++ = '0';
        // mark end here
        *dst = 0;
    }
    else dst[numdec] = 0;
}

void F16_toStr(fix16 value, char *str, u16 numdec)
{
    fix16ToStr(value, str, numdec);
}

void F32_toStr(fix32 value, char *str, u16 numdec)
{
    fix32ToStr(value, str, numdec);
}

void FF16_toStr(fastfix16 value, char *str, u16 numdec)
{
    fastFix16ToStr(value, str, numdec);
}

void FF32_toStr(fastfix32 value, char *str, u16 numdec)
{
    fastFix32ToStr(value, str, numdec);
}

static u16 digits10(const u16 v)
{
    if (v < P02)
    {
        if (v < P01) return 1;
        return 2;
    }
    else
    {
        if (v < P03) return 3;
        if (v < P04) return 4;
        return 5;
    }
}

#if (ENABLE_NEWLIB == 0)
static u16 skip_atoi(const char **s)
{
    u16 i = 0;

    while(isdigit(**s))
        i = (i * 10) + *((*s)++) - '0';

    return i;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
    char tmp_buffer[14];
    s32 i;
    s16 len;
    s32 *ip;
    u32 num;
    char *s;
    const char *hexchars;
    char *str;
    s16 left_align;
    s16 plus_sign;
    s16 zero_pad;
    s16 space_sign;
    s16 field_width;
    s16 precision;

    for (str = buf; *fmt; ++fmt)
    {
        if (*fmt != '%')
        {
            *str++ = *fmt;
            continue;
        }

        space_sign = zero_pad = plus_sign = left_align = 0;

        // Process the flag
repeat:
        ++fmt;          // this also skips first '%'

        switch (*fmt)
        {
        case '-':
            left_align = 1;
            goto repeat;

        case '+':
            plus_sign = 1;
            goto repeat;

        case ' ':
            if ( !plus_sign )
                space_sign = 1;

            goto repeat;

        case '0':
            zero_pad = 1;
            goto repeat;
        }

        // Process field width and precision

        field_width = precision = -1;

        if (isdigit(*fmt))
            field_width = skip_atoi(&fmt);
        else if (*fmt == '*')
        {
            ++fmt;
            // it's the next argument
            field_width = va_arg(args, s32);

            if (field_width < 0)
            {
                field_width = -field_width;
                left_align = 1;
            }
        }

        if (*fmt == '.')
        {
            ++fmt;

            if (isdigit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*')
            {
                ++fmt;
                // it's the next argument
                precision = va_arg(args, s32);
            }

            if (precision < 0)
                precision = 0;
        }

        if ((*fmt == 'h') || (*fmt == 'l') || (*fmt == 'L'))
            ++fmt;

        if (left_align)
            zero_pad = 0;

        switch (*fmt)
        {
        case 'c':
            if (!left_align)
                while(--field_width > 0)
                    *str++ = ' ';

            *str++ = (unsigned char) va_arg(args, s32);

            while(--field_width > 0)
                *str++ = ' ';

            continue;

        case 's':
            s = va_arg(args, char *);

            if (!s)
                s = "<NULL>";

            len = strnlen(s, precision);

            if (!left_align)
                while(len < field_width--)
                    *str++ = ' ';

            for (i = 0; i < len; ++i)
                *str++ = *s++;

            while(len < field_width--)
                *str++ = ' ';

            continue;

        case 'p':
            if (field_width == -1)
            {
                field_width = 2 * sizeof(void *);
                zero_pad = 1;
            }

            hexchars = uppercase_hexchars;
            goto hexa_conv;

        case 'x':
            hexchars = lowercase_hexchars;
            goto hexa_conv;

        case 'X':
            hexchars = uppercase_hexchars;

hexa_conv:
            s = &tmp_buffer[12];
            *--s = 0;
            num = va_arg(args, u32);

            if (!num)
                *--s = '0';

            while(num)
            {
                *--s = hexchars[num & 0xF];
                num >>= 4;
            }

            num = plus_sign = 0;

            break;

        case 'n':
            ip = va_arg(args, s32*);
            *ip = (str - buf);
            continue;

        case 'u':
            s = &tmp_buffer[12];
            *--s = 0;
            num = va_arg(args, u32);

            if (!num)
                *--s = '0';

            while(num)
            {
                *--s = (num % 10) + 0x30;
                num /= 10;
            }

            num = plus_sign = 0;

            break;

        case 'd':
        case 'i':
            s = &tmp_buffer[12];
            *--s = 0;
            i = va_arg(args, s32);

            if (!i)
                *--s = '0';

            if (i < 0)
            {
                num = 1;

                while(i)
                {
                    *--s = 0x30 - (i % 10);
                    i /= 10;
                }
            }
            else
            {
                num = 0;

                while(i)
                {
                    *--s = (i % 10) + 0x30;
                    i /= 10;
                }
            }

            break;

        default:
            continue;
        }

        len = strnlen(s, precision);

        if (num)
        {
            *str++ = '-';
            field_width--;
        }
        else if (plus_sign)
        {
            *str++ = '+';
            field_width--;
        }
        else if (space_sign)
        {
            *str++ = ' ';
            field_width--;
        }

        if ( !left_align)
        {
            if (zero_pad)
            {
                while(len < field_width--)
                    *str++ = '0';
            }
            else
            {
                while(len < field_width--)
                    *str++ = ' ';
            }
        }

        for (i = 0; i < len; ++i)
            *str++ = *s++;

        while(len < field_width--)
            *str++ = ' ';
    }

    *str = '\0';

    return str - buf;
}

int sprintf(char *buffer, const char *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    i = vsprintf(buffer, fmt, args);
    va_end(args);

    return i;
}
#endif // ENABLE_NEWLIB
