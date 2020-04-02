/**
 *  \file vram.h
 *  \brief SGDK VRAM (Video Memory) management unit
 *  \author Stephane Dallongeville
 *  \date 11/2015
 *
 * Video Memory management unit.<br>
 * It offerts methods to manage dynamic VRAM allocation for tile data.<br>
 * Tile data should always be located before tilemap in VRAM (0000-XXXX = tile data, XXXX-FFFF = tilemaps).<br>
 *<br>
 *<pre>
 * VRAMRegion structure define a VRAM region where we want to use dynamic allocation.
 * 'vram' field is a buffer representing the VRAM region usage. For each entry:
 *  b14-b0 = size of the bloc (in tile)
 *  b15    = 1:used, 0:free
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
 *  2. After allocation of 32 tiles
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
 *  3. After allocation of 128 tiles
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
 *  5. After release of allocation #3 (64 tiles)
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
 *  6. After release of allocation #4 (500 tiles)
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
 *</pre>
 */

#ifndef _VRAM_H_
#define _VRAM_H_


/**
 *  \brief
 *      VRAM region structure.
 *
 *  \param startIndex
 *      start position in tile for the VRAM region
 *  \param endIndex
 *      end position in tile for the VRAM region
 *  \param free
 *      position of next free area
 *  \param vram
 *      allocation buffer
 *
 * Define cache information for a VRAM region dedicated to tile storage.
 */
typedef struct
{
    u16 startIndex;
    u16 endIndex;
    u16 *free;
    u16 *vram;
} VRAMRegion;


/**
 *  \brief
 *      Initialize a new VRAM region structure.
 *
 *  \param region
 *      Region to initialize.
 *  \param startIndex
 *      Tile start index in VRAM.
 *  \param size
 *      Size in tile of the region.
 *
 * Set parameters and allocate memory for the VRAM region structure.
 *
 * \see VRAM_releaseRegion(..)
 *
 */
void VRAM_createRegion(VRAMRegion *region, u16 startIndex, u16 size);
/**
 *  \brief
 *      Release the VRAM region structure.
 *
 *  \param region
 *      VRAMRegion we want to release.
 *
 * Release memory used by the VRAM region structure.
 *
 * \see VRAM_createRegion(..)
 */
void VRAM_releaseRegion(VRAMRegion *region);
/**
 *  \brief
 *      Release all allocations from specified VRAM region.
 *
 *  \param region
 *      VRAM region we want to clear.
 */
void VRAM_clearRegion(VRAMRegion *region);

/**
 *  \brief
 *      Return the number of free tile remaining in the specified VRAM region.
 *
 *  \param region
 *      VRAM region
 *  \return
 *      the number of free tile in the specified VRAM region
 */
u16 VRAM_getFree(VRAMRegion *region);
/**
 *  \brief
 *      Return the number of allocated tile in the specified VRAM region.
 *
 *  \param region
 *      VRAM region
 *  \return
 *      the number of allocated tile in the specified VRAM region.
 */
u16 VRAM_getAllocated(VRAMRegion *region);
/**
 *  \brief
 *      Return the largest free block index in the specified VRAM region.
 *
 *  \param region
 *      VRAM region
 *  \return
 *      the largest free block index in the specified VRAM region.
 */
u16 VRAM_getLargestFreeBlock(VRAMRegion *region);

/**
 *  \brief
 *      Try to allocate the specified number of tile in the given VRAM region and return its index.
 *
 *  \param region
 *      VRAM region
 *  \param size
 *      Number of tile we want to allocate in VRAM (need to be > 0).
 *  \return
 *      the index in VRAM where we allocated the bloc of tile.<br>
 *      -1 if there is no enough available VRAM in the region.
 *
 *  \see VRAM_free(..)
 */
s16 VRAM_alloc(VRAMRegion *region, u16 size);
/**
 *  \brief
 *      Release the previously allocated VRAM block at specified index in the given VRAM region.<br>
 *
 *  \param region
 *      VRAM region
 *  \param index
 *      The index of the VRAM block we want to release
 *
 *  \see VRAM_alloc(..)
 */
void VRAM_free(VRAMRegion *region, u16 index);


#endif // _VRAM_H_
