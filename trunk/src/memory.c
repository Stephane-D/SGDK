
#include "config.h"
#include "types.h"

#include "memory.h"

#include "tab_cnv.h"
#include "kdebug.h"


#define USED        1
#define MEMORY_HIGH 0xFFFC00


// end of bss segment --> start of heap
extern u32 _bend;

/*
 * When memory is initialized HEAP point to first bloc as well than FREE.
 *
 *  memory start = HEAP     +-----------+
 *  HEAP+$0                 | size = ?? |
 *  HEAP+$2                 |           |
 *                          |           |
 *                          |           |
 *                          |           |
 *  HEAP+size = FREE        +-----------+
 *  FREE+$0                 | size = 0  |
 *  memory end              +-----------+
 *
 *
 *  1. Before allocation
 *
 *  FREE = HEAP = $FF0100
 *  +-------------------+
 *  | size = $1000      |
 *  +-------------------+
 *
 *  HEAP = $FF0100; *HEAP = $1000
 *  FREE = $FF0100; *FREE = $1000
 *  END  = $FF1100; *END  = $0      --> end memory
 *
 *
 *  2. After allocation of $100 bytes
 *
 *  FREE = $FF0100 + ($100+2) = $FF0202
 *  +--------------------------------+
 *  | size = $1000 - ($100+2) = $EFE |
 *  +--------------------------------+
 *
 *  HEAP = $FF0100; *HEAP = $102+1 (bit 0 = 1 --> used)
 *  FREE = $FF0202; *FREE = $EFE+0 (bit 0 = 0 --> free)
 *  END  = $FF1100; *END  = $0
 *
 *
 *  3. After allocation of $250 bytes
 *
 *  FREE = $FF0202 + ($250+2) = $FF0454
 *  +--------------------------------+
 *  | size = $EFE - ($250+2) = $CAC  |
 *  +--------------------------------+
 *
 *  HEAP = $FF0100; *HEAP = $102+1 (used)
 *  ---- = $FF0202; *---- = $252+1 (used)
 *  FREE = $FF0454; *FREE = $CAC+0 (free)
 *  END  = $FF1100; *END  = $0
 *
 *
 *  4. After allocation of $200, $150 and $730 bytes
 *
 *  FREE = $FF0454 + ($200+2) = $FF0656
 *  FREE = $FF0656 + ($150+2) = $FF07A8
 *  FREE = $FF07A8 + ($730+2) = $FF0EDA
 *  +--------------------------------+
 *  | size = $CAC - ($200+2) = $AAA  |
 *  | size = $AAA - ($150+2) = $958  |
 *  | size = $958 - ($730+2) = $226  |
 *  +--------------------------------+
 *
 *  HEAP = $FF0100; *HEAP = $102+1 (used)
 *  ---- = $FF0202; *---- = $252+1 (used)
 *  ---- = $FF0454; *---- = $202+1 (used)
 *  ---- = $FF0656; *---- = $152+1 (used)
 *  ---- = $FF07A8; *---- = $732+1 (used)
 *  FREE = $FF0EDA; *FREE = $226+0 (free)
 *  END  = $FF1100; *END  = $0
 *
 *
 *  5. After release of $FF0204-2
 *
 *  HEAP = $FF0100; *HEAP = $102+1 (used)
 *  ---- = $FF0202; *---- = $252+0 (free)
 *  ---- = $FF0454; *---- = $202+1 (used)
 *  ---- = $FF0656; *---- = $152+1 (used)
 *  ---- = $FF07A8; *---- = $732+1 (used)
 *  FREE = $FF0EDA; *FREE = $226+0 (free)
 *  END  = $FF1100; *END  = $0
 *
 *
 *  6. After release of $FF0456-2
 *
 *  HEAP = $FF0100; *HEAP = $102+1 (used)
 *  ---- = $FF0202; *---- = $252+0 (free)
 *  ---- = $FF0454; *---- = $202+1 (free)
 *  ---- = $FF0656; *---- = $152+1 (used)
 *  ---- = $FF07A8; *---- = $732+1 (used)
 *  FREE = $FF0EDA; *FREE = $226+0 (free)
 *  END  = $FF1100; *END  = $0
 *
 *
 *  7. After allocation of $400 bytes
 *
 *  *FREE = $226 < $400  --> packing
 *
 *  Starting from HEAP, find contiguous free block and collapse them.
 *  FREE becomes the first collaped block with size >= $400
 *
 *
 *  HEAP = $FF0100; *HEAP = $102+1 (used)
 *  FREE = $FF0202; *FREE = $454+0 (free)
 *  ---- = $FF0656; *---- = $152+1 (used)
 *  ---- = $FF07A8; *---- = $732+1 (used)
 *  ---- = $FF0EDA; *---- = $226+0 (free)
 *  END  = $FF1100; *END  = $0
 *
 */


static u16* free;
static u16* heap;

/*
 * Pack free block and return first matching free block.
 */
static u16* pack(u16 nsize)
{
    u16 bsize, psize;
    u16 *b;
    u16 *best;

    b = heap;
    best = b;
    bsize = 0;

    while ((psize = *b))
    {
        if (psize & USED)
        {
            if (bsize != 0)
            {
                *best = bsize;

                if (bsize >= nsize)
                    return best;
            }

            bsize = 0;
            b += psize >> 1;
            best = b;
        }
        else
        {
            bsize += psize;
            b += psize >> 1;
        }
    }

    if (bsize != 0)
    {
        *best = bsize;

        if (bsize >= nsize)
            return best;
    }

    return NULL;
}

void MEM_init()
{
    u32 h;
    u32 len;

    // point to end of bss (start of heap)
    h = (u32)&_bend;
    // 2 bytes aligned
    h += 1;
    h >>= 1;
    h <<= 1;

    // define available memory (sizeof(u16) is the memory reserved to indicate heap end)
    len = MEMORY_HIGH - (h + sizeof(u16));

    // define heap
    heap = (u16*) h;
    // and its size
    *heap = len;

    // free memory : whole heap
    free = heap;

    // mark end of heap memory
    heap[len >> 1] = 0;
}

u16 MEM_getFree()
{
    u16* b;
    u16 res;
    u16 bsize;

    b = heap;
    res = 0;

    while ((bsize = *b))
    {
        // memory block not used --> add available size to result
        if (!(bsize & USED))
            res += bsize;

        // pass to next block
        b += bsize >> 1;
    }

    return res;
}

void MEM_free(void *ptr)
{
    // valid block --> mark block as no more used
    if (ptr)
        ((u16*)ptr)[-1] &= ~USED;
}

void* MEM_alloc(u16 size)
{
    u16* p;
    u16 adjsize;

    if (size == 0)
        return 0;

    // 2 bytes aligned
    adjsize = (size + sizeof(u16) + 1) & 0xFFFE;

    if (adjsize > *free)
    {
        p = pack(adjsize);

        // no enough memory
        if (p == NULL)
            return NULL;

        free = p;
    }
    else
        // at this point we can allocate memory
        p = free;

    // set free to next free block
    free += adjsize >> 1;
    // set remaining free space (old - allocated)
    *free = *p - adjsize;

    // set block size, mark as used and point to free region
    *p++ = adjsize | USED;

    // return block
    return p;
}


void fastMemset(void *to, u8 value, u32 len)
{
    u8 *dst8;
    u32 *dst32;
    u32 value32;
    u32 cnt;

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

void fastMemsetU16(u16 *to, u16 value, u32 len)
{
    u16 *dst16;
    u32 *dst32;
    u32 value32;
    u32 cnt;

    // small data set --> use standard memset
    if (len < 16)
    {
        memsetU16(to, value, len);
        return;
    }

    dst16 = to;

    // align to dword
    if ((u32) dst16 & 2)
    {
        *dst16++ = value;
        len--;
    }

    value32 = cnv_8to32_tab[value];
    dst32 = (u32 *) dst16;

    cnt = len >> 4;

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

    cnt = (len >> 1) & 7;

    while (cnt--) *dst32++ = value32;

    // last word set
    if (len & 1) *((u16 *) dst32) = value;
}

void fastMemsetU32(u32 *to, u32 value, u32 len)
{
    u32 *dst;
    u32 cnt;

    dst = to;
    cnt = len >> 3;

    while (cnt--)
    {
        *dst++ = value;
        *dst++ = value;
        *dst++ = value;
        *dst++ = value;
        *dst++ = value;
        *dst++ = value;
        *dst++ = value;
        *dst++ = value;
    }

    cnt = len & 7;

    while (cnt--) *dst++ = value;
}


void fastMemcpy(void *to, const void *from, u32 len)
{
    const u8 *src8;
    const u32 *src32;
    u8 *dst8;
    u32 *dst32;
    u32 cnt;

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

void fastMemcpyU16(u16 *to, const u16 *from, u32 len)
{
    const u16 *src16;
    const u32 *src32;
    u16 *dst16;
    u32 *dst32;
    u32 cnt;

    // small transfert or misalignement --> use standard memcpy
    if ((len < 16) || (((u32) to & 2) != ((u32) from & 2)))
    {
        memcpyU16(to, from, len);
        return;
    }

    dst16 = to;
    src16 = from;

    // align to dword
    if ((u32) dst16 & 2)
    {
        *dst16++ = *src16++;
        len--;
    }

    dst32 = (u32 *) dst16;
    src32 = (u32 *) src16;

    cnt = len >> 4;

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

    cnt = (len >> 1) & 7;

    while (cnt--) *dst32++ = *src32++;

    // last word copy
    if (len & 1) *((u16 *) dst32) = *((u16 *) src32);
}

void fastMemcpyU32(u32 *to, const u32 *from, u32 len)
{
    const u32 *src;
    u32 *dst;
    u32 cnt;

    // small transfert --> use standard memcpy
    if (len < 16)
    {
        memcpyU32(to, from, len);
        return;
    }

    dst = to;
    src = from;
    cnt = len >> 3;

    while (cnt--)
    {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
    }

    cnt = len & 7;

    while (cnt--) *dst++ = *src++;
}

void memset(void *to, u8 value, u32 len)
{
    u8 *dst;
    u32 cnt;

    dst = (u8 *) to;
    cnt = len;

    while(cnt--) *dst++ = value;
}

void memsetU16(u16 *to, u16 value, u32 len)
{
    u16 *dst;
    u32 cnt;

    dst = to;
    cnt = len;

    while(cnt--) *dst++ = value;
}

void memsetU32(u32 *to, u32 value, u32 len)
{
    u32 *dst;
    u32 cnt;

    dst = to;
    cnt = len;

    while(cnt--) *dst++ = value;
}

void memcpy(void *to, const void *from, u32 len)
{
    u8 *dst;
    const u8 *src;
    u32 cnt;

    dst = (u8 *) to;
    src = (const u8 *) from;
    cnt = len;

    while(cnt--) *dst++ = *src++;
}

void memcpyU16(u16 *to, const u16 *from, u32 len)
{
    u16 *dst;
    const u16 *src;
    u32 cnt;

    dst = to;
    src = from;
    cnt = len;

    while(cnt--) *dst++ = *src++;
}

void memcpyU32(u32 *to, const u32 *from, u32 len)
{
    u32 *dst;
    const u32 *src;
    u32 cnt;

    dst = to;
    src = from;
    cnt = len;

    while(cnt--) *dst++ = *src++;
}

