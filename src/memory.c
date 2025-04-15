#include "config.h"
#include "types.h"

#include "memory.h"

#include "tab_cnv.h"
#include "sys.h"
#include "string.h"
#include "kdebug.h"
#include "tools.h"

#define USED        1


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
 *  HEAP = $FF0100; *HEAP = $102+1 (used)
 *  FREE = $FF0202; *FREE = $454+0 (free)
 *  ---- = $FF0656; *---- = $152+1 (used)
 *  ---- = $FF07A8; *---- = $732+1 (used)
 *  ---- = $FF0EDA; *---- = $226+0 (free)
 *  END  = $FF1100; *END  = $0
 *
 *  then
 *
 *  HEAP = $FF0100; *HEAP = $102+1 (used)
 *  ---- = $FF0202; *---- = $402+1 (used)
 *  FREE = $FF0604; *FREE = $052+0 (free)
 *  ---- = $FF0656; *---- = $152+1 (used)
 *  ---- = $FF07A8; *---- = $732+1 (used)
 *  ---- = $FF0EDA; *---- = $226+0 (free)
 *  END  = $FF1100; *END  = $0
 *
 *
 *  8. After allocation of $52 bytes
 *
 *  HEAP = $FF0100; *HEAP = $102+1 (used)
 *  ---- = $FF0202; *---- = $402+1 (used)
 *  ---- = $FF0604; *---- = $052+1 (used)
 *  ---- = $FF0656; *---- = $152+1 (used)
 *  ---- = $FF07A8; *---- = $732+1 (used)
 *  FREE = $FF0EDA; *FREE = $226+0 (free)
 *  END  = $FF1100; *END  = $0
 *
 */

 // forward
static u16* pack(u16 nsize);

static u16* free;
static u16* heap;
static u16* heapEnd;
static bool needPack;

void MEM_init()
{
    // point to end of bss (start of heap)
    u32 h = (u32)&_bend;
    // 2 bytes aligned
    h += 1;
    h >>= 1;
    h <<= 1;

    // define available memory (sizeof(u16) is the memory reserved to indicate heap end)
    u16 len = (u16)(MEMORY_HIGH - (h + sizeof(u16)));

    // define heap
    heap = (u16*) h;
    // and its size
    *heap = len;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    kprintf("MEM_init: heap = %lx", (u32) heap);
#endif

    // free memory: whole heap
    free = heap;
    // mark end of heap memory (free block with size 0)
    heapEnd = heap + (len >> 1);
    *heapEnd = 0;
    needPack = FALSE;
}

u16 MEM_getFree()
{
    u16* b = heap;
    u16 res = 0;
    u16 bsize;

    while ((bsize = *b) != 0)
    {
        // memory block not used --> add available size to result
        if (!(bsize & USED)) res += bsize;

        // pass to next block
        b += bsize >> 1;
    }

    return res;
}

u16 MEM_getLargestFreeBlock()
{
    u16* b = heap;
    u16 res = 0;
    u16 csize = 0;
    u16 bsize;

    while ((bsize = *b) != 0)
    {
        // memory block is used --> reset cumulated size
        if (bsize & USED) csize = 0;
        else
        {
            csize += bsize;
            // largest available bloc ?
            if (csize > res) res = csize;
        }

        // pass to next block
        b += bsize >> 1;
    }

    return res;
}

u16 MEM_getAllocated()
{
    u16* b = heap;
    u16 res = 0;
    u16 bsize;

    while ((bsize = *b) != 0)
    {
        // memory block used --> add allocated size to result
        if (bsize & USED) res += bsize & ~USED;

        // pass to next block
        b += bsize >> 1;
    }

    return res;
}

NO_INLINE void* MEM_alloc(u16 size)
{
    if (size == 0)
        return 0;

    u16* p;
    // 2 bytes aligned
    u16 adjsize = (size + sizeof(u16) + 1) & 0xFFFE;

    if (adjsize > *free)
    {
        p = pack(adjsize);
        // no enough memory
        if (p == NULL)
        {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
            if (size > MEM_getFree())
                KLog_U2_("MEM_alloc(", size, ") failed: no enough free memory (free = ", MEM_getFree(), ")");
            else
                KLog_U3_("MEM_alloc(", size, ") failed: cannot find a big enough memory block (largest free block = ", MEM_getLargestFreeBlock(), " - free = ", MEM_getFree(), ")");
#endif

            return NULL;
        }

        free = p;
    }
    // at this point we can allocate memory
    else
    {
        // check if we need to pack memory
        if (needPack)
        {
            MEM_pack();

            // find the first big enough free block if needed
            while (adjsize > *free)
            {
                // next block
                free += *free >> 1;
                // bypass used blocks
                while(*free & USED) free += *free >> 1;
            }
        }

        p = free;
    }

    // set free to next free block
    free += adjsize >> 1;

    // get remaining (old - allocated)
    u16 remaining = *p - adjsize;
    // adjust remaining free space
    if (remaining > 0) *free = remaining;
    else
    {
        // no more space in block so we have to find the next free bloc
        u16 *newfree = free;
        u16 bloc;

        while((bloc = *newfree) & USED)
            newfree += bloc >> 1;

        free = newfree;
    }

    // set block size, mark as used and point to free region
    *p++ = adjsize | USED;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    kprintf("MEM_alloc(%d) success: %lx - remaining = %d", size, (u32) p, MEM_getFree());
#endif

    // return block
    return p;
}

NO_INLINE void* MEM_allocAt(u32 addr, u16 size)
{
    if (size == 0)
        return 0;

    // 2 bytes aligned
    u16 adjsize = (size + sizeof(u16) + 1) & 0xFFFE;

    u16* found = NULL;
    u16* b = heap;
    u16* best = b;
    u16 bsize = 0;
    u16 psize;

    while ((psize = *b) != 0)
    {
        if (psize & USED)
        {
            // we cumulated free blocks ?
            if (bsize != 0)
            {
                // store packed free size
                *best = bsize;

                // get block address
                u32 baddr = (u32) best;
                // block address still valid ?
                if (baddr <= addr)
                {
                    // test if the block contains the block we want
                    if ((baddr + bsize) >= (addr + adjsize))
                    {
                        u16 delta = addr - baddr;
                        // free space before wanted block ? --> split block
                        if (delta)
                        {
                            // change available size for previous block
                            *best = delta;
                            // move on desired block
                            best += delta >> 1;
                            // set free space here
                            *best = bsize - delta;
                        }

                        // found a matching block --> stop here
                        found = best;
                        break;
                    }
                }
                // block address not valid anymore --> stop here 
                else break;

                // reset packed free size
                bsize = 0;
            }

            // point to next memory block
            b += psize >> 1;
            // remember it in case it becomes free
            best = b;
        }
        else
        {
            // increment free size
            bsize += psize;
            // clear this memory block as it will be packed
            *b = 0;
            // point to next memory block
            b += psize >> 1;
        }
    }

    // last free block update
    if (bsize != 0)
    {
        // store packed free size
        *best = bsize;

        // not yet found a matching block ?
        if (found == NULL)
        {
            // get block address
            u32 baddr = (u32) best;
            // block address is valid ?
            if (baddr <= addr)
            {
                // test if the block contains the block we want
                if ((baddr + bsize) >= (addr + adjsize))
                {
                    u16 delta = addr - baddr;
                    // free space before wanted block ? --> split block
                    if (delta)
                    {
                        // change available size for previous block
                        *best = delta;
                        // move on desired block
                        best += delta >> 1;
                        // set free space here
                        *best = bsize - delta;
                    }

                    // found a matching block
                    found = best;
                }
            }
        }
    }

    if (found == NULL)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        kprintf("MEM_allocEx(%8lx, %d) failed: wanted block is not available", addr, size);
#endif
        return NULL;
    }

    free = found;
    // set free to next free block
    free += adjsize >> 1;

    // get remaining (old - allocated)
    u16 remaining = *found - adjsize;
    // adjust remaining free space
    if (remaining > 0) *free = remaining;
    else
    {
        // no more space in block so we have to find the next free bloc
        u16 *newfree = free;
        u16 bloc;

        while((bloc = *newfree) & USED)
            newfree += bloc >> 1;

        free = newfree;
    }

    // set block size, mark as used and point to free region
    *found++ = adjsize | USED;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    kprintf("MEM_allocEx(%8lx, %d) success: %lx - remaining = %d", addr, size, (u32) found, MEM_getFree());
#endif

    // return block
    return found;
}

void MEM_free(void *ptr)
{
    // valid block ?
    if (ptr)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        // not in use ?
        if (!(((u16*)ptr)[-1] & USED))
        {
            kprintf("MEM_free(%lx) failed: block is not allocated !", (u32) ptr);
            return;
        }
#endif

        // mark block as no more used
        ((u16*)ptr)[-1] &= ~USED;
        // request packing on next allocation
        needPack = TRUE;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
        kprintf("MEM_free(%lx) --> remaining = %d", (u32) ptr, MEM_getFree());
#endif
    }
    else
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        kprintf("MEM_free(NULL) failed: tried to release a NULL pointer !");
#endif
    }
}

NO_INLINE void MEM_pack()
{
    u16* b = heap;
    u16* best = b;
    u16 bsize = 0;
    u16* nfree = NULL;
    u16 psize;

    while ((psize = *b) != 0)
    {
        if (psize & USED)
        {
            if (bsize != 0)
            {
                // store packed free memory for this block
                *best = bsize;
                // reset packed free size
                bsize = 0;
                // first free block found ? --> reset new 'free' block
                if (nfree == NULL) nfree = best;
            }

            // point to next memory block
            b += psize >> 1;
            // remember it in case it becomes free
            best = b;
        }
        else
        {
            // increment free size
            bsize += psize;
            // clear this memory block as it will be packed
            *b = 0;
            // point to next memory block
            b += psize >> 1;
        }
    }

    // last free block update
    if (bsize != 0)
    {
        *best = bsize;
        // first free block found ? --> reset new 'free' block
        if (nfree == NULL) nfree = best;
    }

    // store new 'free' 
    if (nfree != NULL) free = nfree;
    // no more free memory
    else free = heapEnd;

    needPack = FALSE;
}


NO_INLINE bool MEM_checkIntegrity()
{
    bool result = TRUE;

    // point to end of bss (start of heap)
    u32 h = (u32)&_bend;
    // 2 bytes aligned
    h += 1;
    h >>= 1;
    h <<= 1;
    // size of heap available for dynamic memory allocation
    u16 len = (u16)(MEMORY_HIGH - (h + sizeof(u16)));

    if ((MEM_getFree() + MEM_getAllocated()) != len)
    {
        KDebug_Alert("MEM_checkIntegrity: memory size mismatch !");
        KDebug_Alert("Total memory:");
        kprintf("%05d", len);
        KDebug_Alert("  Free memory:");
        kprintf("  %05d", MEM_getFree());
        KDebug_Alert("  Allocated memory:");
        kprintf("  %05d", MEM_getAllocated());
        result = FALSE;
    }

    u32 stackAdr = SYS_getStackPointer();
    // check if stack pointer is outside its range
    if (stackAdr < MEMORY_HIGH)
    {
        KDebug_Alert("MEM_checkIntegrity: stack overflow !");
        KDebug_Alert("Stack pointer:");
        kprintf("%08lx", stackAdr);
        result = FALSE;
    }
    else if (stackAdr >= (MEMORY_HIGH + STACK_SIZE))
    {
        KDebug_Alert("MEM_checkIntegrity: stack underflow !");
        KDebug_Alert("Stack pointer:");
        kprintf("%08lx", stackAdr);
        result = FALSE;
    }

    return result;
}

NO_INLINE void MEM_dump()
{
    KDebug_Alert("Memory dump:");
    KDebug_Alert(" Used blocks:");

    char str[40];
    char strNum[16];

    u16 *b = heap;
    u16 memused = 0;
    u16 psize;

    while ((psize = *b))
    {
        if (psize & USED)
        {
            strcpy(str, "    ");
            intToHex((u32)b, strNum, 8);
            strcat(str, strNum);
            strcat(str, ": ");

            intToStr(psize & 0xFFFE, strNum, 0);
            strcat(str, strNum);
            KDebug_Alert(str);

            memused += psize & 0xFFFE;
        }

        b += psize >> 1;
        KDebug_Alert("");
    }

    KDebug_Alert(" Free blocks:");

    b = heap;
    u16 memfree = 0;
    while ((psize = *b))
    {
        if (!(psize & USED))
        {
            strcpy(str, "    ");
            intToHex((u32)b, strNum, 8);
            strcat(str, strNum);
            strcat(str, ": ");

            intToStr(psize & 0xFFFE, strNum, 0);
            strcat(str, strNum);
            KDebug_Alert(str);

            memfree += psize & 0xFFFE;
        }

        b += psize >> 1;
        KDebug_Alert("");
    }

    KDebug_Alert("Total used:");
    KDebug_AlertNumber(memused);
    KDebug_Alert("Total free:");
    KDebug_AlertNumber(memfree);
}

/*
 * Pack free blocks and return first free block matching size
 */
static u16* pack(u16 nsize)
{
    u16* b = heap;
    u16* best = b;
    u16 bsize = 0;
    u16 psize;

    while ((psize = *b))
    {
        if (psize & USED)
        {
            if (bsize != 0)
            {
                // store packed free size
                *best = bsize;

                // return it if greater than what we're looking for
                if (bsize >= nsize)
                    return best;

                // reset packed free size
                bsize = 0;
            }

            // point to next memory block
            b += psize >> 1;
            // remember it in case it becomes free
            best = b;
        }
        else
        {
            // increment free size
            bsize += psize;
            // clear this memory block as it will be packed
            *b = 0;
            // point to next memory block
            b += psize >> 1;
        }
    }

    // last free block update
    if (bsize != 0)
    {
        // store packed free size
        *best = bsize;

        // return it if greater than what we're looking for
        if (bsize >= nsize)
            return best;
    }

    return NULL;
}
