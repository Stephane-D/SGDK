/**
 * \file memory.h
 * \brief Memory handling methods
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides basic memory operations and dynamic memory allocation.
 */

#ifndef _MEMORY_H_
#define _MEMORY_H_


/**
 *  \def GET_DWORDFROMPBYTE
 *      Get u32 from u8 array (BigEndian order).
 */
#define GET_DWORDFROMPBYTE(src)     ((src[0] << 24) | (src[1] << 16) | (src[2] << 8) | (src[3] << 0))
/**
 *  \def GET_DWORDFROMPBYTE_LI
 *      Get u32 from u8 array (LittleEndian order).
 */
#define GET_DWORDFROMPBYTE_LI(src)  ((src[0] << 0) | (src[1] << 8) | (src[2] << 16) | (src[3] << 24))
/**
 *  \def GET_WORDFROMPBYTE
 *      Get u16 from u8 array (BigEndian order).
 */
#define GET_WORDFROMPBYTE(src)      ((src[0] << 8) | (src[1] << 0))
/**
 *  \def GET_WORDFROMPBYTE_LI
 *      Get u16 from u8 array (LittleEndian order).
 */
#define GET_WORDFROMPBYTE_LI(src)   ((src[0] << 0) | (src[1] << 8))
/**
 *  \def GET_DWORDFROMPWORD
 *      Get u32 from u16 array (BigEndian order).
 */
#define GET_DWORDFROMPWORD(src)     ((src[0] << 16) | (src[1] << 0))
/**
 *  \def GET_DWORDFROMPWORD_LI
 *      Get u32 from u16 array (LittleEndian order).
 */
#define GET_DWORDFROMPWORD_LI(src)  ((src[0] << 0) | (src[1] << 16))


/**
 *  \def SWAP_u8
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
 *  \def SWAP_s8
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
 *  \def SWAP_u16
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
 *  \def SWAP_s16
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
 *  \def SWAP_u32
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
 *  \def SWAP_s32
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


/**
 * \brief
 *      Initialize memory sub system
 */
void MEM_init();
/**
 * \brief
 *      Return available memory in bytes
 */
u32  MEM_getFree();
/**
 * \brief
 *      Deallocate space in memory
 *
 * \param ptr
 *      Pointer to a memory block previously allocated with Mem_alloc to be deallocated.<br>
 *      If a null pointer is passed as argument, no action occurs.
 *
 * A block of memory previously allocated using a call to Mem_alloc is deallocated, making it available again for further allocations.
 * Notice that this function leaves the value of ptr unchanged, hence it still points to the same (now invalid) location, and not to the null pointer.
 */
void MEM_free(void *ptr);
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
void* MEM_alloc(u32 size);


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
void fastMemset(void *to, u8 value, u32 len);
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
void fastMemsetU16(u16 *to, u16 value, u32 len);
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
void fastMemsetU32(u32 *to, u32 value, u32 len);
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
void fastMemcpy(void *to, const void *from, u32 len);
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
void fastMemcpyU16(u16 *to, const u16 *from, u32 len);
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
void fastMemcpyU32(u32 *to, const u32 *from, u32 len);

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
void memset(void *to, u8 value, u32 len);
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
void memsetU16(u16 *to, u16 value, u32 len);
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
void memsetU32(u32 *to, u32 value, u32 len);
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
void memcpy(void *to, const void *from, u32 len);
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
void memcpyU16(u16 *to, const u16 *from, u32 len);
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
void memcpyU32(u32 *to, const u32 *from, u32 len);


#endif // _MEMORY_H_


