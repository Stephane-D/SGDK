#include "genesis.h"
#include "kdebug.h"

#include "tilemap.h"
#include "tile.h"

#include "movie.h"


// unpack buffer
static u16 unpack_buf[TILEMAP_SIZE];
// tilemap buffers
static u16 buf1[64 * 28];
static u16 buf2[64 * 28];

// current read buffer
static u16* backBuffer;
// current write buffer
static u16* frontBuffer;

static u16 baseTileInd;


void tm_init()
{
    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));

    backBuffer = buf1;
    frontBuffer = buf2;

    baseTileInd = 0;
}

void tm_unpack(u16 frame)
{
    TileMap* tileMap = movie[frame]->tilemap;
    u16 size = tileMap->w * tileMap->h * 2;

    switch(tileMap->compression)
    {
        case COMPRESSION_NONE:
            memcpy(unpack_buf, (u8*) FAR_SAFE(tileMap->tilemap, size), size);
            break;

        case COMPRESSION_LZ4W:
            lz4w_unpack((u8*) FAR_SAFE(tileMap->tilemap, size), (u8*) unpack_buf);
            break;

        case COMPRESSION_APLIB:
            aplib_unpack((u8*) FAR_SAFE(tileMap->tilemap, size), (u8*) unpack_buf);
            break;
    }

    // then copy to real tilemap
    u16* s = unpack_buf;
    u16* d = backBuffer;
    u16 j = 28;

    while(j--)
    {
        u16 i = 10;
        u16 v;

        while(i--)
        {
            v = *s++; *d++ = (v >= 16)?(v + baseTileInd):v;
            v = *s++; *d++ = (v >= 16)?(v + baseTileInd):v;
            v = *s++; *d++ = (v >= 16)?(v + baseTileInd):v;
            v = *s++; *d++ = (v >= 16)?(v + baseTileInd):v;
        }

        d += 24;
    }

    // flip buffers
    u16* tmp = backBuffer;
    backBuffer = frontBuffer;
    frontBuffer = tmp;
}

bool tm_transfer()
{
    // send tilemap to VRAM
    if (baseTileInd == 0)
        VDP_setTileMapData(VDP_BG_B, frontBuffer, 0, 64 * TILEMAP_HEIGHT, 2, DMA);
    else
        VDP_setTileMapData(VDP_BG_B, frontBuffer, 32 * 64, 64 * TILEMAP_HEIGHT, 2, DMA);

    // switch base tile index
    if (baseTileInd == 0) baseTileInd = TILESPACE_FRAME;
    else baseTileInd = 0;

    // means that transfer is completed
    return FALSE;
}

