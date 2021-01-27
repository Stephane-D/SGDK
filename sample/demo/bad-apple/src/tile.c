#include "genesis.h"
#include "kdebug.h"

#include "tile.h"
#include "tilemap.h"
#include "tools.h"

#include "movie.h"


#define MAXTILE_TRANSFER_PER_FRAME  180


// tile buffer 0
static u8 buf1[512 * 32];
// tile buffer 1
static u8 buf2[512 * 32];

// current write buffer
static u8* backBuffer;
// current read buffer
static u8* frontBuffer;

static u16 baseTileInd;


void ts_init()
{
    backBuffer = buf1;
    frontBuffer = buf2;

    baseTileInd = 16;
}

u16 ts_unpack(u16 frame)
{
    TileSet* tileSet = movie[frame]->tileset;
    u16 size = tileSet->numTile * 32;

    if (size)
    {
        switch(tileSet->compression)
        {
            case COMPRESSION_NONE:
                memcpy(backBuffer, FAR_SAFE(tileSet->tiles, size), size);
                break;

            case COMPRESSION_LZ4W:
                lz4w_unpack((u8*) FAR_SAFE(tileSet->tiles, size), (u8*) backBuffer);
                break;

            case COMPRESSION_APLIB:
                aplib_unpack((u8*) FAR_SAFE(tileSet->tiles, size), (u8*) backBuffer);
                break;
        }
    }

    // flip buffers
    u8* tmp = backBuffer;
    backBuffer = frontBuffer;
    frontBuffer = tmp;

    // num of unpacked tile (+1 to avoid case of numTile = 0)
    return tileSet->numTile + 1;
}

bool ts_transfer(u16 numTile)
{
    static u32* tileSrc;
    static u16 tileInd;
    static u16 tileLeft;
    u16 num;

    // start transfer
    if (numTile)
    {
        tileSrc = (u32*) frontBuffer;
        tileInd = baseTileInd;
        tileLeft = numTile - 1;
    }

    // limit transfer
    if (tileLeft > MAXTILE_TRANSFER_PER_FRAME) num = MAXTILE_TRANSFER_PER_FRAME;
    else num = tileLeft;

    if (num)
    {
        VDP_loadTileData(tileSrc, tileInd, num, DMA);

        // next
        tileSrc += num * 8;
        tileInd += num;
        tileLeft -= num;
    }

    // transfer not yet done
    if (tileLeft > 0) return TRUE;

    // switch base tile index
    if (baseTileInd == 16) baseTileInd = 16 + TILESPACE_FRAME;
    else baseTileInd = 16;

    // transfer done
    return FALSE;
}