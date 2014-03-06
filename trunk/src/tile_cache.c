#include "config.h"
#include "types.h"

#include "tile_cache.h"

#include "vdp.h"
#include "memory.h"
#include "tools.h"
#include "sys.h"


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
extern u32 VIntProcess;

// forward
static TCBloc* getFixedBloc(TileCache *cache, TileSet *tileset);
static TCBloc* getBloc(TileCache *cache, TileSet* tileset);
static u16 findFreeRegion(TileCache *cache, u16 size);
static u16 getConflictRegion(TileCache *cache, u16 start, u16 end);
static void releaseFlushable(TileCache *cache, u16 start, u16 end);
static void addToUploadQueue(TileSet *tileset, u16 index);

// upload cache structure
TCBloc *uploads;            // this variable is specifically cleared in SYS reset method
static u16 uploadIndex;
static u16 uploadDone;


void TC_init()
{
    // not yet initialized ?
    if (uploads == NULL)
    {
        // alloc cache structures memory
        uploads = MEM_alloc(MAX_UPLOAD * sizeof(TCBloc));
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

        // release cache structures memory
        MEM_free(uploads);
        uploads = NULL;

        // release the last uploaded tileset(s)
        if (uploadDone)
        {
            TCBloc *bloc = uploads;
            u16 i = uploadIndex;

            while(i--)
            {
                TileSet* tileset = bloc->tileset;

                // released the tileset if we unpacked it here
                if (tileset->compression != COMPRESSION_NONE)
                    MEM_free(tileset);

                // next bloc to check
                bloc++;
            }
        }
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
    TCBloc *bloc;

//    KDebug_Alert("Alloc TC");
//    KDebug_AlertNumber(tileset);

    bloc = getBloc(cache, tileset);

    // bloc found
    if (bloc != NULL)
    {
        // get address of next fixed bloc
        u16 nextFixed = cache->nextFixed;
        TCBloc *nextFixedBloc = &cache->blocs[nextFixed];

        // flushed bloc ? --> re allocate it
        if (bloc >= nextFixedBloc)
        {
            // need to swap blocs ?
            if (bloc != nextFixedBloc)
            {
                u16 tmpInd = bloc->index;
                bloc->index = nextFixedBloc->index;
                bloc->tileset = nextFixedBloc->tileset;
                nextFixedBloc->index = tmpInd;
                nextFixedBloc->tileset = tileset;

                bloc = nextFixedBloc;
            }

            // one more fixed bloc
            cache->nextFixed = nextFixed + 1;
        }

        return bloc->index;
    }
    // bloc not found --> alloc
    else
    {
        u16 index, size, lim;
        u16 nextFlush;

        // not more free bloc
        if (cache->nextFixed >= cache->numBloc)
        {
            if (LIB_DEBUG) KDebug_Alert("TC_alloc failed: no more free bloc !");
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
        // release any previous flushable bloc in the allocated area
        releaseFlushable(cache, index, lim);

        // get new allocated bloc
        bloc = &cache->blocs[cache->nextFixed++];

        // try to save flush bloc if we still have available bloc for that ?
        nextFlush = cache->nextFlush;
        if (nextFlush < cache->numBloc)
        {
            TCBloc *nextFlushBloc = &cache->blocs[nextFlush];

            // test if we have something to save
            if (nextFlushBloc != bloc)
            {
                nextFlushBloc->index = bloc->index;
                nextFlushBloc->tileset = bloc->tileset;
            }

            // increase flush bloc address
            cache->nextFlush = nextFlush + 1;
        }

        // set bloc info
        bloc->tileset = tileset;
        bloc->index = index;
    }

    return bloc->index;
}

s16 TC_reAlloc(TileCache *cache, TileSet *tileset)
{
    TCBloc *bloc;

    bloc = getBloc(cache, tileset);

    // bloc found
    if (bloc != NULL)
    {
        // get address of next fixed bloc
        u16 nextFixed = cache->nextFixed;
        TCBloc *nextFixedBloc = &cache->blocs[nextFixed];

        // flushed bloc ? --> re allocate it
        if (bloc >= nextFixedBloc)
        {
            // need to swap blocs ?
            if (bloc != nextFixedBloc)
            {
                u16 tmpInd = bloc->index;
                bloc->index = nextFixedBloc->index;
                bloc->tileset = nextFixedBloc->tileset;
                nextFixedBloc->index = tmpInd;
                nextFixedBloc->tileset = tileset;

                bloc = nextFixedBloc;
            }

            // one more fixed bloc
            cache->nextFixed = nextFixed + 1;
        }

        return bloc->index;
    }

    return -1;
}

void TC_free(TileCache *cache, TileSet *tileset)
{
    // find allocated bloc
    TCBloc *bloc = getFixedBloc(cache, tileset);

    // bloc found
    if (bloc != NULL)
    {
        // get last fixed bloc and decrease number of fixed bloc
        TCBloc *lastFixedBloc = &cache->blocs[--cache->nextFixed];

        // exchange bloc infos if needed
        if (lastFixedBloc != bloc)
        {
            u16 tmpInd = bloc->index;
            bloc->index = lastFixedBloc->index;
            bloc->tileset = lastFixedBloc->tileset;
            lastFixedBloc->index = tmpInd;
            lastFixedBloc->tileset = tileset;
        }

        // decrease flush bloc address
        cache->nextFlush--;
    }
}

s16 TC_getTileIndex(TileCache *cache, TileSet *tileset)
{
    TCBloc *bloc = getBloc(cache, tileset);

    if (bloc != NULL)
        return bloc->index;

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


static TCBloc* getFixedBloc(TileCache *cache, TileSet *tileset)
{
    TCBloc *bloc;
    u16 i;

    // search in fixed blocs only
    i = cache->nextFixed;
    bloc = cache->blocs;
    while(i--)
    {
        // found
        if (bloc->tileset == tileset)
            return bloc;

        bloc++;
    }

    return NULL;
}

static TCBloc* getBloc(TileCache *cache, TileSet *tileset)
{
    TCBloc *bloc;
    u16 i;

    // search in fixed & flushable blocs
    i = cache->nextFlush;
    bloc = cache->blocs;
    while(i--)
    {
        // found
        if (bloc->tileset == tileset)
            return bloc;

        bloc++;
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

    if (LIB_DEBUG) KDebug_Alert("TC_alloc failed: no enough available VRAM in cache !");

    return (u16) -1;
}

static u16 getConflictRegion(TileCache *cache, u16 start, u16 end)
{
    TCBloc *bloc;
    u16 i;

    // search only in fixed blocs
    i = cache->nextFixed;
    bloc = cache->blocs;

    while(i--)
    {
        u16 startBloc = bloc->index;
        u16 endBloc = startBloc + bloc->tileset->numTile;

        // conflict ?
        if ((startBloc < end) && (endBloc > start))
            return endBloc;

        bloc++;
    }

    // no conflict
    return 0;
}

static void releaseFlushable(TileCache *cache, u16 start, u16 end)
{
    TCBloc *bloc;
    u16 lastFlushInd;
    u16 i;

    lastFlushInd = cache->nextFlush;

    // search only in flushable blocs and from end
    i = lastFlushInd - cache->nextFixed;
    bloc = &(cache->blocs[lastFlushInd - 1]);
    while(i--)
    {
        u16 index = bloc->index;

        // need to release bloc ?
        if ((index < end) && ((index + bloc->tileset->numTile) > start))
        {
            // get last bloc location
            TCBloc *lastFlushBloc = &(cache->blocs[--lastFlushInd]);

            // save last flush bloc data in the new released bloc to release last bloc
            if (bloc != lastFlushBloc)
            {
                bloc->tileset = lastFlushBloc->tileset;
                bloc->index = lastFlushBloc->index;
            }
        }

        --bloc;
    }

    // update first flushable bloc index
    cache->nextFlush = lastFlushInd;
}

static void addToUploadQueue(TileSet *tileset, u16 index)
{
    TCBloc *bloc;
    u16 i;

    // need to clear to queue ?
    if (uploadDone)
    {
        i = uploadIndex;
        bloc = uploads;
        while(i--)
        {
            TileSet* tileset = bloc->tileset;

            // released the tileset if we unpacked it here
            if (tileset->compression != COMPRESSION_NONE)
                MEM_free(tileset);

            // next bloc to check
            bloc++;
        }

        // prepare for new upload
        uploadDone = FALSE;
        uploadIndex = 0;
    }

    // get bloc
    bloc = &uploads[uploadIndex++];
    // set upload bloc info
    bloc->tileset = tileset;
    bloc->index = index;
}


// VInt processing
u16 TC_doVBlankProcess()
{
    TCBloc *src;
    u16 i;

    if (!uploadDone && uploadIndex)
    {
        i = uploadIndex;
        src = uploads;
        while(i--)
        {
            TileSet* tileset = src->tileset;

            // upload the tileset to VRAM (data is already unpacked)
            VDP_loadTileData(tileset->tiles, src->index, tileset->numTile, TRUE);

            // next tileset to upload
            src++;
        }

        // uploads done !
        uploadDone = TRUE;
    }

    return TRUE;
}
