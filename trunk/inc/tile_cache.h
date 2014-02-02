/**
 *  \file tile_cache.h
 *  \brief SGDK Tile cache engine (tile VRAM management)
 *  \author Stephane Dallongeville
 *  \date 11/2013
 *
 * Tile (in TileSet form) cache engine.<br/>
 * It offerts methods to manage VRAM usage for tile data.
 */

#ifndef _TILE_CACHE_H_
#define _TILE_CACHE_H_


#include "vdp_tile.h"


/**
 *  \brief
 *      Tile cache upload action.
 */
typedef enum
{
    NO_UPLOAD,
    UPLOAD_VINT,
    UPLOAD_NOW
} TCUpload;

/**
 *  \brief
 *      Tile cache bloc structure.
 *
 * Define information for a single tileset VRAM allocation bloc.
 */
typedef struct
{
    TileSet *tileset;
    u16 index;
} TCBloc;

/**
 *  \brief
 *      Tile cache information structure.
 *
 * Define cache information for a VRAM region dedicated to tile storage.
 */
typedef struct
{
    u16 startIndex;
    u16 limit;
    u16 current;
    u16 nextFixed;
    u16 nextFlush;
    u16 numBloc;
    TCBloc *blocs;
} TileCache;


/**
 *  \brief
 *      Initialize the TileSet cache engine.
 *
 * Allocate some memory and enable VInt process.
 */
void TC_init();
/**
 *  \brief
 *      End the TileSet cache engine.
 *
 * Release some memory and disable attached VInt processing.
 */
void TC_end();

/**
 *  \brief
 *      Create the given TileSet cache structure.
 *
 *  \param startIndex
 *      Tile start index in VRAM for the cache.
 *  \param size
 *      Size in tile of the cache.
 *
 * Set parameters and allocate some memory for the cache (~1KB).
 */
void TC_createCache(TileCache *cache, u16 startIndex, u16 size);
/**
 *  \brief
 *      Create the given TileSet cache structure.
 *
 *  \param startIndex
 *      Tile start index in VRAM for the cache.
 *  \param size
 *      Size in tile of the cache.
 *  \param numBloc
 *      Number of bloc of the cache.
 *
 * Set parameters and allocate some memory for the cache (~1KB).
 */
void TC_createCacheEx(TileCache *cache, u16 startIndex, u16 size, u16 numBloc);
/**
 *  \brief
 *      Release the TileSet cache structure.
 *
 * Release memory used by TileSet cache structure (~1 KB).
 */
void TC_releaseCache(TileCache *cache);
/**
 *  \brief
 *      Clear the specified TileSet cache.
 */
void TC_clearCache(TileCache *cache);
/**
 *  \brief
 *      Flush the specified TileSet cache.
 *
 * This is not exactly the same as the clear operation:<br/>
 * Allocated tileset remains in cache but they can be erased with newly allocated tileset.
 */
void TC_flushCache(TileCache *cache);

/**
 *  \brief
 *      Allocate the specified TileSet in VRAM with given Tile cache and return its index.<br>
 *      If TileSet is already present in VRAM, no special operation is done else
 *      the TileSet will be automatically uploaded at the next VInt.<br>
 *      If the specified TileSet is compressed the method unpack it and store it
 *      in a temporary TileSet until it is send to VRAM.
 *
 *  \param cache
 *      Cache used for allocation.
 *  \param tileset
 *      The TileSet to put in VRAM.
 *  \param upload
 *      Upload action, possible values are:<br/>
 *      <b>NO_UPLOAD</b> don't upload tileset to VRAM (only return index)<br/>
 *      <b>UPLOAD_VINT</b> upload to VRAM will be done automatically at VInt time (VBlank area)<br/>
 *      <b>UPLOAD_NOW</b> upload to VRAM now<br/>
 *  \return
 *      the index of the TileSet in VRAM.<br/>
 *      -1 if there is no enough available VRAM.
 */
s16 TC_alloc(TileCache *cache, TileSet *tileset, TCUpload upload);
/**
 *  \brief
 *      Re-allocate a TileSet which is still present in cache after a flush operation.<br/>
 *      If the TileSet is not anymore in the tile cache the method return -1.
 *
 *  \param cache
 *      Cache where we want to reallocate the TileSet.
 *  \param tileset
 *      The TileSet to re allocate if possible.
 *  \return
 *      the index of the TileSet in VRAM.<br/>
 *      -1 if the TileSet is not anymore in the cache.
 *  \see TC_flushCache(TileCache *cache)
 */
s16 TC_reAlloc(TileCache *cache, TileSet *tileset);

/**
 *  \brief
 *      Release VRAM allocation of the specified TileSet.<br>
 *      The tileSet remains in VRAM but will be the bloc will be marked as flushed so
 *      it can be overwritten by another bloc at any time.
 *
 *  \param cache
 *      Cache used for allocation.
 *  \param tileset
 *      The TileSet to release from VRAM.
 */
void TC_free(TileCache *cache, TileSet *tileset);

/**
 *  \brief
 *      Return the VRAM tile index of specified TileSet in the given Tile cache.<br>
 *      If the TileSet is not found then -1 is returned.
 *
 *  \param cache
 *      Cache where to search the TileSet
 *  \param tileset
 *      The TileSet we want to retrieve VRAM tile index.
 *  \return
 *      the index of the TileSet in VRAM.<br/>
 *      -1 if not found.
 */
s16 TC_getTileIndex(TileCache *cache, TileSet *tileset);

/**
 *  \brief
 *      Will upload the specified Tileset at given VRAM tile position during VBlank.
 *
 *  \param tileset
 *      The TileSet we want to upload to VRAM.
 *  \param index
 *      the tile position where to upload in VRAM.
 */
 void TC_uploadAtVBlank(TileSet *tileset, u16 index);


#endif // _TILE_CACHE_H_
