/**
 * \file memory.c
 * \brief Memory handling methods
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides basic memory operations and dynamic memory allocation.
 */

#include "config.h"
#include "types.h"

#include "memory.h"

#include "tab_cnv.h"


#define USED        1
#define MEMORY_HIGH 0xFFFE00


// end of bss segment --> start of heap
extern u32 _bend;

/**
 *
 *  Before allocation                ---->     After allocation of $100 bytes
 *
 *  FREE = HEAP = $FF0100                     FREE = HEAP + ($100+2) = $FF0202
 *  +--------------------+                   +--------------------------------+
 *  | size = $1000 bytes |                   | size = $1000 - ($100+2) = $EFE |
 *  +--------------------+                   +--------------------------------+
 *
 *  HEAP = $FF0100; *HEAP = $1000         HEAP = $FF0100; *HEAP = $1001 (bit 0 = 1 --> used)
 *  FREE = $FF0100; *FREE = $1000         FREE = $FF0202; *FREE = $EFE  (bit 0 = 0 --> free)
 */

typedef struct
{
    u32 size;
} Block;

static Block* free;
static Block* heap;


static Block* pack(u32 nsize)
{
    u32 bsize, psize;
    Block *b;
    Block *best;

    b = heap;
    best = b;
    bsize = 0;

    while ((psize = b->size))
    {
        if (psize & USED)
        {
            if (bsize != 0)
            {
                best->size = bsize;

                if (bsize >= nsize)
                    return best;
            }

            bsize = 0;
            best = b = (Block*) ((u32)b + (psize & ~USED));
        }
        else
        {
            bsize += psize;
            b = (Block*) ((u32)b + psize);
        }
    }

    if (bsize != 0)
    {
        best->size = bsize;

        if (bsize >= nsize)
            return best;
    }

    return NULL;
}

/**
 * \brief
 *      Initialize memory sub system
 */
void MEM_init()
{
    u32 h;
    u32 len;

    // point to end of bss (start of heap)
    h = (u32)&_bend;
    // 4 bytes aligned
    h += 3;
    h >>= 2;
    h <<= 2;
    // define available memory (sizeof(u32) is the memory reserved to indicate heap end)
    len = MEMORY_HIGH - (h + sizeof(u32));

    // define heap
    heap = (Block*) h;
    // and its size
    heap->size = len;

    // free memory : whole heap
    free = heap;

    // mark end of heap memory
    *(u32*)((u8*)h + len) = 0;
}

/**
 * \brief
 *      Return available memory in bytes
 */
u32 MEM_getFree()
{
    Block *b;
    u32 res;
    u32 bsize;

    b = heap;
    res = 0;

    while ((bsize = b->size))
    {
        if (bsize & USED)
            b = (Block*) ((u32)b + (bsize & ~USED));
        else
        {
            // memory block not used, add real available size to result
            res += bsize - sizeof(u32);
            b = (Block*) ((u32)b + bsize);
        }
    }

    return res;
}

/**
 * \brief
 *      Deallocate space in memory
 *
 * \param ptr
 *      Pointer to a memory block previously allocated with Mem_alloc to be deallocated.
 *      If a null pointer is passed as argument, no action occurs.
 *
 * A block of memory previously allocated using a call to Mem_alloc is deallocated, making it available again for further allocations.
 * Notice that this function leaves the value of ptr unchanged, hence it still points to the same (now invalid) location, and not to the null pointer.
 */
void MEM_free(void *ptr)
{
    if (ptr)
    {
        // get block address from pointer
        Block *p = (Block*)((u32)ptr - sizeof(u32));
        // mark block as no more used
        p->size &= ~USED;
    }
}

/**
 * \brief
 *      Allocate memory block
 *
 * \param size
 *      Number of bytes to allocate
 * \return
 *      On success, a pointer to the memory block allocated by the function.
 *      The type of this pointer is always void*, which can be cast to the desired type of data pointer in order to be dereferenceable.
 *      If the function failed to allocate the requested block of memory (or if specified size = 0), a null pointer is returned.
 *
 * Allocates a block of size bytes of memory, returning a pointer to the beginning of the block.
 * The content of the newly allocated block of memory is not initialized, remaining with indeterminate values.
 */
void* MEM_alloc(u32 size)
{
    u32 fsize;
    Block *p;

    if (size == 0) return 0;

    // 4 bytes aligned
    size += 3 + sizeof(u32);
    size >>= 2;
    size <<= 2;

    if ((free == 0) || (size > free->size))
    {
        free = pack(size);

        // no enough memory
        if (free == 0) return NULL;
    }

    p = free;
    fsize = free->size;

    if (fsize >= (size + sizeof(u32)))
    {
        free = (Block*)((u32)p + size);
        free->size = fsize - size;
    }
    else
    {
        free = 0;
        size = fsize;
    }

    p->size = size | USED;

    return (void*)((u32)p + sizeof(u32));
}


/**
 * \brief
 *      Fast fill block of memory
 *
 * \param to
 *      Pointer to the block of memory to fill.
 * \param value
 *      Value to be set.
 * \param len
 *      Number of u8 (byte) to be set to the value.
 *
 * Sets the first num bytes of the block of memory pointed by to with the specified value.
 * This method is optimized for large block fill, use memset for small block fill.
 */
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

/**
 * \brief
 *      Fast fill block of memory (optimized for u16)
 *
 * \param to
 *      Pointer to the block of memory to fill.
 * \param value
 *      Value to be set.
 * \param len
 *      Number of u16 (short) to be set to the value.
 *
 * Sets the first num shorts of the block of memory pointed by to with the specified value.
 * This method is optimized for large block fill, use memset for small block fill.
 */
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

/**
 * \brief
 *      Fast fill block of memory (optimized for u32)
 *
 * \param to
 *      Pointer to the block of memory to fill.
 * \param value
 *      Value to be set.
 * \param len
 *      Number of u32 (long) to be set to the value.
 *
 * Sets the first num longs of the block of memory pointed by to with the specified value.
 * This method is optimized for large block fill, use memset for small block fill.
 */
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


/**
 * \brief
 *      Fast copy block of memory
 *
 * \param to
 *      Pointer to the destination array where the content is to be copied, type-casted to a pointer of type void*.
 * \param from
 *      Pointer to the source of data to be copied, type-casted to a pointer of type void*.
 * \param len
 *      Number of bytes to copy.
 *
 * Copies the values of len long from the location pointed by from directly to the memory block pointed by to.
 * The underlying type of the objects pointed by both the source and destination pointers are irrelevant for this function; The result is a binary copy of the data.
 * This method is optimized for large block copy, use memcpy for small block copy.
 */
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

/**
 * \brief
 *      Fast copy block of memory (optimized for u16 type)
 *
 * \param to
 *      Pointer to the destination u16 array where the content is to be copied
 * \param from
 *      Pointer to the source u16 array to be copied
 * \param len
 *      Number of u16 element (short) to copy.
 *
 * Copies the values of len long from the location pointed by from directly to the memory block pointed by to.
 * This method is optimized for large block copy, use memcpyU16 for small block copy.
 */
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

/**
 * \brief
 *      Fast copy block of memory (optimized for u32 type)
 *
 * \param to
 *      Pointer to the destination u32 array where the content is to be copied
 * \param from
 *      Pointer to the source u32 array to be copied
 * \param len
 *      Number of u32 element (long) to copy.
 *
 * Copies the values of len long from the location pointed by from directly to the memory block pointed by to.
 * This method is optimized for large block copy, use memcpyU32 for small block copy.
 */
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

/**
 * \brief
 *      Fill block of memory
 *
 * \param to
 *      Pointer to the block of memory to fill.
 * \param value
 *      Value to be set.
 * \param len
 *      Number of u8 (byte) to be set to the value.
 *
 * Sets the first num bytes of the block of memory pointed by to with the specified value.
 */
void memset(void *to, u8 value, u32 len)
{
    u8 *dst;
    u32 cnt;

    dst = (u8 *) to;
    cnt = len;

    while(cnt--) *dst++ = value;
}

/**
 * \brief
 *      Fill block of memory (optimized for u16)
 *
 * \param to
 *      Pointer to the block of memory to fill.
 * \param value
 *      Value to be set.
 * \param len
 *      Number of (u16) short to be set to the value.
 *
 * Sets the first num shorts of the block of memory pointed by to with the specified value.
 */
void memsetU16(u16 *to, u16 value, u32 len)
{
    u16 *dst;
    u32 cnt;

    dst = to;
    cnt = len;

    while(cnt--) *dst++ = value;
}

/**
 * \brief
 *      Fill block of memory (optimized for u32)
 *
 * \param to
 *      Pointer to the block of memory to fill.
 * \param value
 *      Value to be set.
 * \param len
 *      Number of u32 (long) to be set to the value.
 *
 * Sets the first num longs of the block of memory pointed by to with the specified value.
 */
void memsetU32(u32 *to, u32 value, u32 len)
{
    u32 *dst;
    u32 cnt;

    dst = to;
    cnt = len;

    while(cnt--) *dst++ = value;
}

/**
 * \brief
 *      Copy block of memory
 *
 * \param to
 *      Pointer to the destination array where the content is to be copied, type-casted to a pointer of type void*.
 * \param from
 *      Pointer to the source of data to be copied, type-casted to a pointer of type void*.
 * \param len
 *      Number of bytes to copy.
 *
 * Copies the values of len long from the location pointed by from directly to the memory block pointed by to.
 * The underlying type of the objects pointed by both the source and destination pointers are irrelevant for this function; The result is a binary copy of the data.
 */
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

/**
 * \brief
 *      Copy block of memory (optimized for u16 type)
 *
 * \param to
 *      Pointer to the destination u16 array where the content is to be copied
 * \param from
 *      Pointer to the source u16 array to be copied
 * \param len
 *      Number of u16 element (short) to copy.
 *
 * Copies the values of len long from the location pointed by from directly to the memory block pointed by to.
 */
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

/**
 * \brief
 *      Copy block of memory (optimized for u32 type)
 *
 * \param to
 *      Pointer to the destination u32 array where the content is to be copied
 * \param from
 *      Pointer to the source u32 array to be copied
 * \param len
 *      Number of u32 element (long) to copy.
 *
 * Copies the values of len long from the location pointed by from directly to the memory block pointed by to.
 */
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


