#include "config.h"
#include "types.h"

#include "tile_cache.h"

#include "vdp.h"
#include "memory.h"
#include "dma.h"
#include "tools.h"
#include "sys.h"
#include "kdebug.h"


#define DEFAULT_NUM_BLOC    128
#define MAX_UPLOAD          100


/*
 * VRAM tile cache allow to cache up to 128 tileset in VRAM.
 *
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


// we don't want to share it
extern vu32 VIntProcess;

// forward
static TCBloc* getFixedBlock(TileCache *cache, TileSet *tileset);
static TCBloc* getBlock(TileCache *cache, TileSet* tileset);
static u16 findFreeRegion(TileCache *cache, u16 size);
static u16 getConflictRegion(TileCache *cache, u16 start, u16 end);
static void releaseFlushable(TileCache *cache, u16 start, u16 end);
static void addToUploadQueue(TileSet *tileset, u16 index);

// upload cache structure
TileSet** uploads;            // this variable is specifically cleared in SYS reset method
static u16 uploadIndex;
static u16 uploadDone;


void TC_init()
{
    // not yet initialized ?
    if (uploads == NULL)
    {
        // alloc cache structures memory
        uploads = MEM_alloc(MAX_UPLOAD * sizeof(TileSet*));
        // init upload
        uploadIndex = 0;
        uploadDone = FALSE;

        // enabled tile cache Int processing
        VIntProcess |= PROCESS_TILECACHE_TASK;
    }
}

void TC_end()
{
    // initialized ?
    if (uploads != NULL)
    {
        // disabled tile cache Int processing
        VIntProcess &= ~PROCESS_TILECACHE_TASK;

        // release the last uploaded tileset(s)
        if (uploadDone)
        {
            TileSet** tilesets = uploads;
            u16 i = uploadIndex;

            while(i--)
            {
                TileSet* tileset = *tilesets++;

                // released the tileset if we unpacked it here
                if (tileset->compression != COMPRESSION_NONE)
                    MEM_free(tileset);
            }
        }

        // release cache structures memory
        MEM_free(uploads);
        uploads = NULL;
    }
}

void TC_createCache(TileCache *cache, u16 startIndex, u16 size)
{
    TC_createCacheEx(cache, startIndex, size, DEFAULT_NUM_BLOC);
}

void TC_createCacheEx(TileCache *cache, u16 startIndex, u16 size, u16 numBloc)
{
    cache->startIndex = startIndex;
    cache->limit = startIndex + size;

    // alloc cache structures memory
    cache->numBloc = numBloc;
    cache->blocs = MEM_alloc(numBloc * sizeof(TCBloc));

    TC_clearCache(cache);
}

void TC_releaseCache(TileCache *cache)
{
    // release cache memory
    MEM_free(cache->blocs);
}

void TC_clearCache(TileCache *cache)
{
    // init blocs & cache
    cache->nextFixed = 0;
    cache->nextFlush = 0;
    cache->current = cache->startIndex;

}
void TC_flushCache(TileCache *cache)
{
    // just remove fixed blocs
    cache->nextFixed = 0;
}


s16 TC_alloc(TileCache *cache, TileSet *tileset, TCUpload upload)
{
    // try re-allocation first if already present in cache
    u16 index = TC_reAlloc(cache, tileset);

    // block not found --> do allocation
    if ((s16) index == -1)
    {
        TCBloc *block;
        u16 size, lim;
        u16 nextFlush;

        // not more free block
        if (cache->nextFixed >= cache->numBloc)
        {
#if (LIB_DEBUG != 0)
            KDebug_Alert("TC_alloc failed: no more free block !");
#endif

            return -1;
        }

        size = tileset->numTile;
        // search for free region (this method can consume a lot of time :-/)
        index = findFreeRegion(cache, size);

        // not enough space in cache
        if ((s16) index == -1)
            return index;

        // process VDP upload if required
        if (upload != NO_UPLOAD)
        {
            // no compression
            if (tileset->compression == COMPRESSION_NONE)
            {
                // upload the tileset to VRAM now
                if (upload == UPLOAD_NOW) VDP_loadTileData(tileset->tiles, index, size, TRUE);
                // upload at VINT
                else addToUploadQueue(tileset, index);
            }
            else
            {
                // unpack tileset
                TileSet *unpacked = unpackTileSet(tileset, NULL);

                // error while unpacking tileset
                if (unpacked == NULL)
                    return -1;

                // upload the tileset to VRAM now ?
                if (upload == UPLOAD_NOW)
                {
                    // upload
                     VDP_loadTileData(unpacked->tiles, index, size, TRUE);
                     // and release memory
                     MEM_free(unpacked);
                }
                // upload at VINT
                else
                {
                    // we will use that to release automatically the TileSet after upload
                    unpacked->compression = COMPRESSION_APLIB;
                    // put in upload queue
                    addToUploadQueue(unpacked, index);
                }
            }
        }

        lim = index + size;
        // update current position
        cache->current = lim;
        // release any previous flushable block in the allocated area
        releaseFlushable(cache, index, lim);

        // get new allocated block
        block = &cache->blocs[cache->nextFixed++];

        // try to save flush block if we still have available block for that ?
        nextFlush = cache->nextFlush;
        if (nextFlush < cache->numBloc)
        {
            TCBloc *nextFlushBloc = &cache->blocs[nextFlush];

            // test if we have something to save
            if (nextFlushBloc != block)
            {
                nextFlushBloc->index = block->index;
                nextFlushBloc->tileset = block->tileset;
            }

            // increase flush block address
            cache->nextFlush = nextFlush + 1;
        }

        // set block info
        block->tileset = tileset;
        block->index = index;
    }

    return index;
}

s16 TC_reAlloc(TileCache *cache, TileSet *tileset)
{
    TCBloc *block = getBlock(cache, tileset);

    // block found
    if (block != NULL)
    {
        // get address of next fixed block
        u16 nextFixed = cache->nextFixed;
        TCBloc *nextFixedBloc = &cache->blocs[nextFixed];

        // flushed block ? --> re allocate it
        if (block >= nextFixedBloc)
        {
            // need to swap blocs ?
            if (block != nextFixedBloc)
            {
                u16 tmpInd = block->index;
                block->index = nextFixedBloc->index;
                block->tileset = nextFixedBloc->tileset;
                nextFixedBloc->index = tmpInd;
                nextFixedBloc->tileset = tileset;

                block = nextFixedBloc;
            }

            // one more fixed block
            cache->nextFixed = nextFixed + 1;
        }

        return block->index;
    }

    return -1;
}

void TC_free(TileCache *cache, TileSet *tileset)
{
    // find allocated block
    TCBloc *block = getFixedBlock(cache, tileset);

    // block found
    if (block != NULL)
    {
        // get last fixed block and decrease number of fixed block
        TCBloc *lastFixedBloc = &cache->blocs[--cache->nextFixed];

        // exchange block infos if needed
        if (lastFixedBloc != block)
        {
            u16 tmpInd = block->index;
            block->index = lastFixedBloc->index;
            block->tileset = lastFixedBloc->tileset;
            lastFixedBloc->index = tmpInd;
            lastFixedBloc->tileset = tileset;
        }

        // decrease flush block address
        cache->nextFlush--;
    }
}

s16 TC_getTileIndex(TileCache *cache, TileSet *tileset)
{
    TCBloc *block = getBlock(cache, tileset);

    if (block != NULL)
        return block->index;

    return -1;
}

void TC_uploadAtVBlank(TileSet *tileset, u16 index)
{
    TileSet *unpacked;

    if (tileset->compression == COMPRESSION_NONE) unpacked = tileset;
    else
    {
        unpacked = unpackTileSet(tileset, NULL);

        // error while unpacking tileset
        if (unpacked == NULL)
            return;

        // we will use that to release automatically the TileSet after upload
        unpacked->compression = COMPRESSION_APLIB;
    }

    // add to upload queue
    addToUploadQueue(unpacked, index);
}


static TCBloc* getFixedBlock(TileCache *cache, TileSet *tileset)
{
    TCBloc *block;
    u16 i;

    // search in fixed blocs only
    i = cache->nextFixed;
    block = cache->blocs;
    while(i--)
    {
        // found
        if (block->tileset == tileset)
            return block;

        block++;
    }

    return NULL;
}

static TCBloc* getBlock(TileCache *cache, TileSet *tileset)
{
    TCBloc *block;
    u16 i;

    // search in fixed & flushable blocs
    i = cache->nextFlush;
    block = cache->blocs;
    while(i--)
    {
        // found
        if (block->tileset == tileset)
            return block;

        block++;
    }

    return NULL;
}

static u16 findFreeRegion(TileCache *cache, u16 size)
{
    u16 start, end, lim;

    // start from current position
    lim = cache->limit;
    start = cache->current;
    end = start + size;

    // search for a free region
    while(end < lim)
    {
        u16 pos = getConflictRegion(cache, start, end);

        // no conflict --> return region index
        if (!pos) return start;

        start = pos;
        end = start + size;
    }

    // restart from begining
    lim = cache->current;
    start = cache->startIndex;
    end = start + size;

    // search for a free region
    while(end < lim)
    {
        u16 pos = getConflictRegion(cache, start, end);

        // no conflict --> return region index
        if (!pos) return start;

        start = pos;
        end = start + size;
    }

#if (LIB_DEBUG != 0)
    KDebug_Alert("TC_alloc failed: no enough available VRAM in cache !");
#endif

    return (u16) -1;
}

static u16 getConflictRegion(TileCache *cache, u16 start, u16 end)
{
    TCBloc *block;
    u16 i;

    // search only in fixed blocs
    i = cache->nextFixed;
    block = cache->blocs;

    while(i--)
    {
        u16 startBloc = block->index;
        u16 endBloc = startBloc + block->tileset->numTile;

        // conflict ?
        if ((startBloc < end) && (endBloc > start))
            return endBloc;

        block++;
    }

    // no conflict
    return 0;
}

static void releaseFlushable(TileCache *cache, u16 start, u16 end)
{
    TCBloc *block;
    u16 lastFlushInd;
    u16 i;

    lastFlushInd = cache->nextFlush;

    // search only in flushable blocs and from end
    i = lastFlushInd - cache->nextFixed;
    block = &(cache->blocs[lastFlushInd - 1]);
    while(i--)
    {
        u16 index = block->index;

        // need to release block ?
        if ((index < end) && ((index + block->tileset->numTile) > start))
        {
            // get last block location
            TCBloc *lastFlushBloc = &(cache->blocs[--lastFlushInd]);

            // save last flush block data in the new released block to release last block
            if (block != lastFlushBloc)
            {
                block->tileset = lastFlushBloc->tileset;
                block->index = lastFlushBloc->index;
            }
        }

        --block;
    }

    // update first flushable block index
    cache->nextFlush = lastFlushInd;
}

static void addToUploadQueue(TileSet *tileset, u16 index)
{
    // need to clear to queue ?
    if (uploadDone)
    {
        TileSet** tilesets = uploads;
        u16 i = uploadIndex;

        while(i--)
        {
            TileSet* ts = *tilesets++;

            // released the tileset if we unpacked it here
            if (ts->compression != COMPRESSION_NONE)
                MEM_free(ts);
        }

        // prepare for new upload
        uploadDone = FALSE;
        uploadIndex = 0;
    }

    // set upload tileset info
    uploads[uploadIndex++] = tileset;

    // put in DMA queue
    DMA_queueDma(DMA_VRAM, (u32) tileset->tiles, index * 32, tileset->numTile * 16, 2);
}


// VInt processing
void TC_doVBlankProcess()
{
    // just inform the upload has been done (DMA queue) so we can released tilesets
    if (!uploadDone && uploadIndex)
        uploadDone = TRUE;
}
