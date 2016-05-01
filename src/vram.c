#include "config.h"
#include "types.h"

#include "tile_cache.h"

#include "vdp.h"
#include "memory.h"
#include "dma.h"
#include "tools.h"
#include "sys.h"
#include "kdebug.h"
#include "vram.h"

#define USED_SFT    15
#define USED_MASK   (1 << USED_SFT)
#define SIZE_MASK   0x7FFF

/*
 * VRAM tile cache allow to handle dynamic VRAM tile allocation.
 *
 * A TilaCache structure define a part of VRAM we want to handle dynamic allocation for.
 * So
 * Bloc description:
 *
 *  tileset             = address of the stored / cached tileset
 *  index b14-b0        = VRAM position of the tileset
 *  index b15           = currently in use
 *                        If this bit is cleared it means the tileset can be released if needed
 *
 * "cache" gives the VRAM tile organization from "cacheStartIndex" for given "cacheSize":
 *
 *  address           value
 *
 *                  +-------------------+
 *  free = 0        | cacheSize  (free) |
 *                  |                   |
 *                  |                   |
 *  cacheSize - 1   |                   |
 *                  +-------------------+
 *  cacheSize       | 0                 |
 *                  +-------------------+
 *
 *
 *  1. Before allocation (with cacheSize = 1000)
 *
 *                  +-------------------+
 *  free = 0        | 1000       (free) |
 *                  |                   |
 *                  |                   |
 *  999             |                   |
 *                  +-------------------+
 *  1000            | 0                 |
 *                  +-------------------+
 *
 *  cache = ???
 *  free = cache            *free = cacheSize
 *  end = cache+cacheSize   *end = 0
 *
 *
 *  2. After allocation of a TileSet of 32 tiles
 *
 *                  +------------------------+
 *  0               | 32              (used) |
 *  free = 32       | 968             (free) |
 *                  |                        |
 *                  |                        |
 *  999             |                        |
 *                  +------------------------+
 *  1000            | 0                      |
 *                  +------------------------+
 *
 *  cache = ???
 *  free = cache + 32       *free = cacheSize - 32
 *
 *
 *  3. After allocation of a TileSet of 128 tiles
 *
 *                  +------------------------+
 *  0               | 32              (used) |
 *  32              | 128             (used) |
 *  free = 32+128   | 840             (free) |
 *                  |                        |
 *                  |                        |
 *  999             |                        |
 *                  +------------------------+
 *  1000            | 0                      |
 *                  +------------------------+
 *
 *
 *  4. After allocation of 64, 500, 100 tiles
 *
 *                  +------------------------+
 *  0               | 32              (used) |
 *  32              | 128             (used) |
 *  160             | 64              (used) |
 *  224             | 500             (used) |
 *  724             | 100             (used) |
 *  free = 824      | 176             (free) |
 *                  |                        |
 *                  |                        |
 *  999             |                        |
 *                  +------------------------+
 *  1000            | 0                      |
 *                  +------------------------+
 *
 *
 *  5. After release of tileset #3 (64 tiles)
 *
 *                  +------------------------+
 *  0               | 32              (used) |
 *  32              | 128             (used) |
 *  160             | 64              (free) |
 *  224             | 500             (used) |
 *  724             | 100             (used) |
 *  free = 824      | 176             (free) |
 *                  |                        |
 *                  |                        |
 *  999             |                        |
 *                  +------------------------+
 *  1000            | 0                      |
 *                  +------------------------+
 *
 *
 *  6. After release of tileset #4 (500 tiles)
 *
 *                  +------------------------+
 *  0               | 32              (used) |
 *  32              | 128             (used) |
 *  160             | 64              (free) |
 *  224             | 500             (free) |
 *  724             | 100             (used) |
 *  free = 824      | 176             (free) |
 *                  |                        |
 *                  |                        |
 *  999             |                        |
 *                  +------------------------+
 *  1000            | 0                      |
 *                  +------------------------+
 *
 *
 *  7. After allocation of 400 tiles
 *
 *                  +------------------------+
 *  0               | 32              (used) |
 *  32              | 128             (used) |
 *  160             | 400             (used) |
 *  560             | 164             (free) |
 *  724             | 100             (used) |
 *  free = 824      | 176             (free) |
 *                  |                        |
 *                  |                        |
 *  999             |                        |
 *                  +------------------------+
 *  1000            | 0                      |
 *                  +------------------------+
 *
 */


// forward
static u16* pack(VRAMRegion *region, u16 nsize);


void VRAM_createRegion(VRAMRegion *region, u16 startIndex, u16 size)
{
    u16 *buf;

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

s16 VRAM_alloc(VRAMRegion *region, u16 size)
{
    u16* p;
    u16* free;
    u16 remaining;

    // cache free pointer
    free = region->free;

    if (size > *free)
    {
        // pack free block
        p = pack(region, size);

        // no enough memory
        if (p == NULL)
        {
#if (LIB_DEBUG != 0)
            KDebug_Alert("VRAM_alloc failed: no enough free VRAM slot !");
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

    // return index position in VRAM
    return ((s16) (p - region->vram)) + region->startIndex;
}

void VRAM_free(VRAMRegion *region, u16 index)
{
    const s16 adjInd = index - region->startIndex;

    // inside region ? --> free block
    if ((adjInd >= 0) && (index <= region->endIndex))
        region->vram[adjInd] &= ~USED_MASK;
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


/*
 * Pack free block and return first matching free block
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
                *best = bsize;

                if (bsize >= nsize)
                    return best;
            }

            bsize = 0;
            b += psize & SIZE_MASK;
            best = b;
        }
        else
        {
            bsize += psize;
            b += psize;
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
