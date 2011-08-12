#ifndef _MEMORY_H_
#define _MEMORY_H_


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


void fastMemset(void *to, u8 value, u32 len);
void fastMemsetU16(u16 *to, u16 value, u32 len);
void fastMemsetU32(u32 *to, u32 value, u32 len);
void fastMemcpy(void *to, const void *from, u32 len);
void fastMemcpyU16(u16 *to, const u16 *from, u32 len);
void fastMemcpyU32(u32 *to, const u32 *from, u32 len);

void memset(void *to, u8 value, u32 len);
void memsetU16(u16 *to, u16 value, u32 len);
void memsetU32(u32 *to, u32 value, u32 len);
void memcpy(void *to, const void *from, u32 len);
void memcpyU16(u16 *to, const u16 *from, u32 len);
void memcpyU32(u32 *to, const u32 *from, u32 len);

void MEM_init();

u32  MEM_getFree();

void MEM_free(void *ptr);
void* MEM_alloc(u32 size);


#endif // _MEMORY_H_


