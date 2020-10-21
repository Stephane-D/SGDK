#include "config.h"
#include "types.h"

#include "vram.h"

#include "vdp.h"
#include "memory.h"
#include "dma.h"
#include "tools.h"
#include "sys.h"
#include "kdebug.h"


#define USED_SFT    15
#define USED_MASK   (1 << USED_SFT)
#define SIZE_MASK   0x7FFF


// forward
static u16* pack(VRAMRegion *region, u16 nsize);


void VRAM_createRegion(VRAMRegion *region, u16 startIndex, u16 size)
{
    region->startIndex = startIndex;
    region->endIndex = startIndex + (size - 1);

    // alloc vram image allocation buffer
    region->vram = MEM_alloc((size + 1) * sizeof(u16));

    VRAM_clearRegion(region);
}

void VRAM_releaseRegion(VRAMRegion *region)
{
    // release vram image buffer
    MEM_free(region->vram);
    region->vram = NULL;
}

void VRAM_clearRegion(VRAMRegion *region)
{
    u16 size = (region->endIndex - region->startIndex) + 1;

    // all region is free :)
    region->vram[0] = size;
    // mark end of VRAM region
    region->vram[size] = 0;

    // init free position
    region->free = region->vram;
}

u16 VRAM_getFree(VRAMRegion *region)
{
    u16* b;
    u16 bsize;
    u16 res;

    b = region->vram;
    res = 0;

    while ((bsize = *b))
    {
        // memory block used ? --> just pass to next block
        if (bsize & USED_MASK) b += bsize & SIZE_MASK;
        else
        {
            // block free --> add available size to result
            res += bsize;
            // pass to next block
            b += bsize;
        }
    }

    return res;
}

u16 VRAM_getLargestFreeBlock(VRAMRegion *region)
{
    u16* b;
    u16 bsize;
    u16 res;

    b = region->vram;
    res = 0;

    while ((bsize = *b))
    {
        // memory block used ? --> just pass to next block
        if (bsize & USED_MASK) b += bsize & SIZE_MASK;
        else
        {
            // block free --> check against largest block
            if (bsize > res)
                res = bsize;

            // pass to next block
            b += bsize;
        }
    }

    return res;
}

u16 VRAM_getAllocated(VRAMRegion *region)
{
    u16* b;
    u16 bsize;
    u16 res;

    b = region->vram;
    res = 0;

    while ((bsize = *b))
    {
        // memory block used ?
        if (bsize & USED_MASK)
        {
            bsize &= SIZE_MASK;

            // add allocated size to result
            res += bsize;
            // pass to next block
            b += bsize;
        }
        else
            // just pass to next block
            b += bsize;
    }

    return res;
}

s16 VRAM_alloc(VRAMRegion *region, u16 size)
{
    u16* p;
    u16* free;
    u16 remaining;
    s16 result;

    // cache free pointer
    free = region->free;

    if (size > *free)
    {
        // pack free block
        p = pack(region, size);

        // no enough memory
        if (p == NULL)
        {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
            if (size > VRAM_getFree(region))
                KLog_U2_("VRAM_alloc(", size, ") failed: no enough free tile in VRAM (free = ", VRAM_getFree(region), ")");
            else
                KLog_U3_("VRAM_alloc(", size, ") failed: cannot find a big enough VRAM tile block (largest free block = ", VRAM_getLargestFreeBlock(region), " - free = ", VRAM_getFree(region), ")");
#endif

            return -1;
        }

        free = p;
    }
    else
        // at this point we can allocate memory
        p = free;

    // set free to next free block
    free += size;

    // get remaining (old - allocated)
    remaining = *p - size;
    // adjust remaining free space
    if (remaining > 0) *free = remaining;
    else
    {
        // no more space in bloc...
        u16 bloc;

        // so we have to find the next free bloc
        while((bloc = *free) & USED_MASK)
            free += bloc & SIZE_MASK;
    }

    // set back new free pointer
    region->free = free;

    // set block size, mark as used and point to free region
    *p = size | USED_MASK;

    // get index position in VRAM region
    result = ((s16) (p - region->vram)) + region->startIndex;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    KLog_U3("VRAM_alloc(", size, ") success: ", result, " - remaining = ", VRAM_getFree(region));
#endif

    // return result
    return result;
}

void VRAM_free(VRAMRegion *region, u16 index)
{
    const s16 adjInd = index - region->startIndex;

    // inside region ? --> free block
    if ((adjInd >= 0) && (index <= region->endIndex))
        region->vram[adjInd] &= ~USED_MASK;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    KLog_U2("VRAM_free(", index, ") --> remaining = ", VRAM_getFree(region));
#endif
}


/*
 * Pack free blocks and return first matching free block
 */
static u16* pack(VRAMRegion *region, u16 nsize)
{
    u16 *b;
    u16 *best;
    u16 bsize, psize;

    b = region->vram;
    best = b;
    bsize = 0;

    while ((psize = *b))
    {
        if (psize & USED_MASK)
        {
            if (bsize != 0)
            {
                 // store packed free memory for this block
                *best = bsize;

                if (bsize >= nsize)
                    return best;

                 // reset packed free size
                 bsize = 0;
            }

            // point to next memory block
            b += psize & SIZE_MASK;
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
            b += psize;
        }
    }

    // last free block update
    if (bsize != 0)
    {
        // store packed free size
        *best = bsize;

        if (bsize >= nsize)
            return best;
    }

    return NULL;
}
