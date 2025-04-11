/**
 *  \file memory.h
 *  \brief Memory handling methods
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides memory copy/set operation and dynamic memory allocation.<br>
 *<br>
 * <b>Memory organization :</b><br>
 *<br>
 * Memory is composed of bloc, the first 2 bytes of a bloc define its size and its state:<br>
 * b15-b1 = size in number of word (2 bytes)<br>
 * b0 = used state (1=used, 0=free)
 *<br>
 * To reach the next bloc you just need to do:<br>
 * <code>next_bloc_address = bloc_addres + bloc_size</code>
 * The end of memory is defined with a 0 sized bloc.<br>
 */

#ifndef _MEMORY_H_
#define _MEMORY_H_

// asics memory address definitions
#include "memory_base.h"

/**
 *  \brief
 *      Get u32 from u8 array (BigEndian order).
 */
#define GET_DWORDFROMPBYTE(src)     ((src[0] << 24) | (src[1] << 16) | (src[2] << 8) | (src[3] << 0))
/**
 *  \brief
 *      Get u32 from u8 array (LittleEndian order).
 */
#define GET_DWORDFROMPBYTE_LI(src)  ((src[0] << 0) | (src[1] << 8) | (src[2] << 16) | (src[3] << 24))
/**
 *  \brief
 *      Get u16 from u8 array (BigEndian order).
 */
#define GET_WORDFROMPBYTE(src)      ((src[0] << 8) | (src[1] << 0))
/**
 *  \brief
 *      Get u16 from u8 array (LittleEndian order).
 */
#define GET_WORDFROMPBYTE_LI(src)   ((src[0] << 0) | (src[1] << 8))
/**
 *  \brief
 *      Get u32 from u16 array (BigEndian order).
 */
#define GET_DWORDFROMPWORD(src)     ((src[0] << 16) | (src[1] << 0))
/**
 *  \brief
 *      Get u32 from u16 array (LittleEndian order).
 */
#define GET_DWORDFROMPWORD_LI(src)  ((src[0] << 0) | (src[1] << 16))


/**
 *  \brief
 *      Exchange value of specified u8 variables.
 */
#define SWAP_u8(x, y)       \
{                           \
    u8 swp;                 \
                            \
    swp = x;                \
    x = y;                  \
    y = swp;                \
}

/**
 *  \brief
 *      Exchange value of specified s8 variables.
 */
#define SWAP_s8(x, y)       \
{                           \
    s8 swp;                 \
                            \
    swp = x;                \
    x = y;                  \
    y = swp;                \
}

/**
 *  \brief
 *      Exchange value of specified u16 variables.
 */
#define SWAP_u16(x, y)      \
{                           \
    u16 swp;                \
                            \
    swp = x;                \
    x = y;                  \
    y = swp;                \
}

/**
 *  \brief
 *      Exchange value of specified s16 variables.
 */
#define SWAP_s16(x, y)      \
{                           \
    s16 swp;                \
                            \
    swp = x;                \
    x = y;                  \
    y = swp;                \
}

/**
 *  \brief
 *      Exchange value of specified u32 variables.
 */
#define SWAP_u32(x, y)      \
{                           \
    u32 swp;                \
                            \
    swp = x;                \
    x = y;                  \
    y = swp;                \
}

/**
 *  \brief
 *      Exchange value of specified s32 variables.
 */
#define SWAP_s32(x, y)      \
{                           \
    s32 swp;                \
                            \
    swp = x;                \
    x = y;                  \
    y = swp;                \
}


#if (ENABLE_NEWLIB == 0)
// enable standard libc compatibility
#define malloc(x)       MEM_alloc(x)
#define free(x)         MEM_free(x)
#endif  // ENABLE_NEWLIB


/**
 *  \brief
 *      Return available memory in bytes
 */
u16  MEM_getFree(void);
/**
 *  \brief
 *      Return allocated memory in bytes
 */
u16  MEM_getAllocated(void);
/**
 *  \brief
 *      Return largest free memory block in bytes
 */
u16  MEM_getLargestFreeBlock(void);

/**
 *  \brief
 *      Deallocate space in memory
 *
 *  \param ptr
 *      Pointer to a memory block previously allocated with Mem_alloc to be deallocated.<br>
 *      If a null pointer is passed as argument, no action occurs.
 *
 * A block of memory previously allocated using a call to Mem_alloc is deallocated, making it available again for further allocations.
 * Notice that this function leaves the value of ptr unchanged, hence it still points to the same (now invalid) location, and not to the null pointer.
 */
void MEM_free(void *ptr);
/**
 *  \brief
 *      Allocate memory block
 *
 *  \param size
 *      Number of bytes to allocate
 *  \return
 *      On success, a pointer to the memory block allocated by the function.
 *      The type of this pointer is always void*, which can be cast to the desired type of data pointer in order to be dereferenceable.
 *      If the function failed to allocate the requested block of memory (or if specified size = 0), a <i>NULL</i> pointer is returned.
 *
 * Allocates a block of size bytes of memory, returning a pointer to the beginning of the block.
 * The content of the newly allocated block of memory is not initialized, remaining with indeterminate values.
 */
void* MEM_alloc(u16 size);
/**
 *  \brief
 *      Allocate memory block at a specific address (useful for short addressing or fixed low level working address)
 *
 *  \param addr
 *      Address where we want to allocate memory
 *  \param size
 *      Number of bytes to allocate
 *  \return
 *      On success, a pointer to the memory block allocated by the function (should match the <i>addr</i> parameter).
 *      The type of this pointer is always void*, which can be cast to the desired type of data pointer in order to be dereferenceable.
 *      If the function failed to allocate the requested block of memory (or if specified size = 0), a <i>NULL</i> pointer is returned.
 *
 * Allocates a block of size bytes of memory at the given address, returning a pointer to the beginning of the block (equal to the address here).
 * The content of the newly allocated block of memory is not initialized, remaining with indeterminate values.
 */
 void* MEM_allocAt(u32 addr, u16 size);

/**
 *  \brief
 *      Pack all free blocks and reset allocation search from start of heap.<br>
 *      You can call this method before trying to allocate small block of memory to reduce memory fragmentation.<br>
 *      Note that since SGDK 2.12 the memory manager is doing "auto packing" operation so you don't need to call this method anymore.
 */
void MEM_pack(void);
/**
 *  \brief
 *      Show memory dump
 *
 * Do a memory dump in GensKMod console (useful for debugging).
 */
void MEM_dump(void);

#if (ENABLE_NEWLIB == 0)
/**
 *  \brief
 *      Fill block of memory
 *
 *  \param to
 *      Pointer to the block of memory to fill.
 *  \param value
 *      Value to be set.
 *  \param len
 *      Number of u8 (byte) to be set to the value.
 *
 * Sets the first num bytes of the block of memory pointed by to with the specified value.
 */
void memset(void *to, u8 value, u16 len);
#endif  // ENABLE_NEWLIB

/**
 *  \brief
 *      Fill block of memory (optimized for u16)
 *
 *  \param to
 *      Pointer to the block of memory to fill.
 *  \param value
 *      Value to be set.
 *  \param len
 *      Number of (u16) short to be set to the value.
 *
 * Sets the first num shorts of the block of memory pointed by to with the specified value.
 */
void memsetU16(u16 *to, u16 value, u16 len);
/**
 *  \brief
 *      Fill block of memory (optimized for u32)
 *
 *  \param to
 *      Pointer to the block of memory to fill.
 *  \param value
 *      Value to be set.
 *  \param len
 *      Number of u32 (long) to be set to the value.
 *
 * Sets the first num longs of the block of memory pointed by to with the specified value.
 */
void memsetU32(u32 *to, u32 value, u16 len);

#if (ENABLE_NEWLIB == 0)
/**
 *  \brief
 *      Copy block of memory
 *
 *  \param to
 *      Pointer to the destination array where the content is to be copied, type-casted to a pointer of type void*.
 *  \param from
 *      Pointer to the source of data to be copied, type-casted to a pointer of type void*.
 *  \param len
 *      Number of bytes to copy.
 *
 * Copies the values of len long from the location pointed by from directly to the memory block pointed by to.
 * The underlying type of the objects pointed by both the source and destination pointers are irrelevant for this function; The result is a binary copy of the data.
 */
void memcpy(void *to, const void *from, u16 len);
#endif  // ENABLE_NEWLIB

/**
 *  \deprecated Uses memcpy(void *to, const void *from, u16 len) instead.
 */
#define memcpyU16(to, from, len)        _Pragma("GCC error \"This method is deprecated, use memcpy(..) instead.\"")
/**
 *  \deprecated Uses memcpy(void *to, const void *from, u16 len) instead.
 */
#define memcpyU32(to, from, len)        _Pragma("GCC error \"This method is deprecated, use memcpy(..) instead.\"")
/**
 *  \deprecated Uses memset(void *to, u8 value, u16 len) instead.
 */
#define fastMemset(to, value, len)      _Pragma("GCC error \"This method is deprecated, use memset(..) instead.\"")
/**
 *  \deprecated Uses memsetU16(void *to, u16 value, u16 len) instead.
 */
#define fastMemsetU16(to, value, len)   _Pragma("GCC error \"This method is deprecated, use memsetU16(..) instead.\"")
/**
 *  \deprecated Uses memsetU32(void *to, u32 value, u16 len) instead.
 */
#define fastMemsetU32(to, value, len)   _Pragma("GCC error \"This method is deprecated, use memsetU32(..) instead.\"")
/**
 *  \deprecated Uses memcpy(void *to, const void *from, u16 len) instead.
 */
#define astMemcpy(to, from, len)        _Pragma("GCC error \"This method is deprecated, use memcpy(..) instead.\"")
/**
 *  \deprecated Uses memcpy(void *to, const void *from, u16 len) instead.
 */
#define fastMemcpyU16(to, from, len)    _Pragma("GCC error \"This method is deprecated, use memcpy(..) instead.\"")
/**
 *  \deprecated Uses memcpy(void *to, const void *from, u16 len) instead.
 */
#define fastMemcpyU32(to, from, len)    _Pragma("GCC error \"This method is deprecated, use memcpy(..) instead.\"")


#endif // _MEMORY_H_
