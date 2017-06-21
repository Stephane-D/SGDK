#include "config.h"
#include "types.h"

#include "string.h"

#include "tab_cnv.h"
#include "maths.h"


static const char const uppercase_hexchars[] = "0123456789ABCDEF";
static const char const lowercase_hexchars[] = "0123456789abcdef";

// FORWARD
static u32 uintToStr_(u32 value, char *str, s16 minsize, s16 maxsize);
static u16 skip_atoi(const char **s);
static u16 vsprintf(char *buf, const char *fmt, va_list args);


u32 strlen(const char *str)
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
    u32 frac;
    char strFrac[8];

    len = 0;
    if (value < 0)
    {
        v = -value;
        str[len++] = '-';
    }
    else v = value;

    len += uintToStr_(fix32ToInt(v), &str[len], 1, 16);
    str[len++] = '.';

    // get fractional part
    frac = fix32Frac(v) * 1000;
    frac /= 1 << FIX32_FRAC_BITS;

    // get fractional string
    uintToStr(frac, strFrac, 3);

    if (numdec >= 3) strcpy(&str[len], strFrac);
    else strncpy(&str[len], strFrac, numdec);
}

void fix16ToStr(fix16 value, char *str, s16 numdec)
{
    u32 len;
    fix16 v;
    u32 frac;
    char strFrac[8];

    len = 0;

    if (value < 0)
    {
        v = -value;
        str[len++] = '-';
    }
    else v = value;

    len += uintToStr_(fix16ToInt(v), &str[len], 1, 16);
    str[len++] = '.';

    // get fractional part
    frac = fix16Frac(v) * 1000;
    frac /= 1 << FIX16_FRAC_BITS;

    // get fractional string
    uintToStr(frac, strFrac, 3);

    if (numdec >= 3) strcpy(&str[len], strFrac);
    else strncpy(&str[len], strFrac, numdec);
}


static u16 skip_atoi(const char **s)
{
    u16 i = 0;

    while(isdigit(**s))
        i = (i * 10) + *((*s)++) - '0';

    return i;
}


static u16 vsprintf(char *buf, const char *fmt, va_list args)
{
    char tmp_buffer[12];
    s16 i;
    s16 len;
    s16 *ip;
    u16 num;
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

        // Process the flags
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
            field_width = va_arg(args, s16);

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
                precision = va_arg(args, s16);
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

                *str++ = (unsigned char) va_arg(args, s16);

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
                num = va_arg(args, u16);

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
                ip = va_arg(args, s16*);
                *ip = (str - buf);
                continue;

            case 'u':
                s = &tmp_buffer[12];
                *--s = 0;
                num = va_arg(args, u16);

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
                i = va_arg(args, s16);

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

u16 sprintf(char *buffer, const char *fmt, ...)
{
    va_list args;
    u16 i;

    va_start(args, fmt);
    i = vsprintf(buffer, fmt, args);
    va_end(args);

    return i;
}
