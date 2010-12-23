#ifndef _TOOLS_H_
#define _TOOLS_H_


#define GET_DWORDFROMPBYTE(src)     ((src[0] << 24) | (src[1] << 16) | (src[2] << 8) | (src[3] << 0))
#define GET_DWORDFROMPBYTE_LI(src)  ((src[0] << 0) | (src[1] << 8) | (src[2] << 16) | (src[3] << 24))
#define GET_WORDFROMPBYTE(src)      ((src[0] << 8) | (src[1] << 0))
#define GET_WORDFROMPBYTE_LI(src)   ((src[0] << 0) | (src[1] << 8))
#define GET_DWORDFROMPWORD(src)     ((src[0] << 16) | (src[1] << 0))
#define GET_DWORDFROMPWORD_LI(src)  ((src[0] << 0) | (src[1] << 16))


#define SWAP_u8(x, y)       \
{                           \
    u8 swp;                 \
                            \
    swp = x;                \
    x = y;                  \
    y = swp;                \
}

#define SWAP_s8(x, y)       \
{                           \
    s8 swp;                 \
                            \
    swp = x;                \
    x = y;                  \
    y = swp;                \
}

#define SWAP_u16(x, y)      \
{                           \
    u16 swp;                \
                            \
    swp = x;                \
    x = y;                  \
    y = swp;                \
}

#define SWAP_s16(x, y)      \
{                           \
    s16 swp;                \
                            \
    swp = x;                \
    x = y;                  \
    y = swp;                \
}

#define SWAP_u32(x, y)      \
{                           \
    u32 swp;                \
                            \
    swp = x;                \
    x = y;                  \
    y = swp;                \
}

#define SWAP_s32(x, y)      \
{                           \
    s32 swp;                \
                            \
    swp = x;                \
    x = y;                  \
    y = swp;                \
}


void fastmemset(void *to, int value, size_t len);
void fastmemcpy(void *to, const void *from, size_t len);
void memset(void *to, int value, size_t len);
void memcpy(void *to, const void *from, size_t len);

u32 strlen(const char *str);
char* strcpy(char *to, const char *from);

u32 intToStr(s32 value, char *str, s32 minsize);
u32 uintToStr(u32 value, char *str, s32 minsize);
u32 fix32ToStr(fix32 value, char *str);
u32 fix16ToStr(fix16 value, char *str);

void QSort_u8(u8 *data, u16 left, u16 right);
void QSort_s8(s8 *data, u16 left, u16 right);
void QSort_u16(u16 *data, u16 left, u16 right);
void QSort_s16(s16 *data, u16 left, u16 right);
void QSort_u32(u32 *data, u16 left, u16 right);
void QSort_s32(s32 *data, u16 left, u16 right);

u32 getFPS();
void showFPS();

u16 random();


#endif // _TOOLS_H_
