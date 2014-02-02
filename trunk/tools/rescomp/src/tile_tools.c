#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

#include "../inc/tile_tools.h"
#include "../inc/tools.h"


unsigned char *tileToBmp(unsigned char *in, int inOffset, int w, int h)
{
    unsigned char *result;
    int i, j, k;
    const int pitch = w * 4;

    result = malloc(w * h * 32);

    for(j = 0; j < h; j++)
    {
        for(i = 0; i < w; i++)
        {
            unsigned char *src = &in[inOffset + ((i + (j * w)) * 32)];
            unsigned char *dst = &result[((j * 8) * pitch) + (i * 4)];

            // write tile Y pixels
            for(k = 0; k < 8; k++)
            {
                memcpy(dst, src, 4);
                src += 4;
                dst += pitch;
            }
        }
    }

    return result;
}

unsigned char *bmpToTile(unsigned char *in, int inOffset, int w, int h)
{
    unsigned char *result;
    int i, j, k;
    const int pitch = w * 4;

    result = malloc(w * h * 32);

    for(j = 0; j < h; j++)
    {
        for(i = 0; i < w; i++)
        {
            unsigned char *src = &in[inOffset + ((j * 8) * pitch) + (i * 4)];
            unsigned char *dst = &result[((i + (j * w)) * 32)];

            // write tile Y pixels
            for(k = 0; k < 8; k++)
            {
                memcpy(dst, src, 4);
                src += pitch;
                dst += 4;
            }
        }
    }

    return result;
}

tileset_* createTileSet(unsigned int* tileData, int numTiles)
{
    tileset_* result;

    // alloc tileset structure
    result = malloc(sizeof(tileset_));
    // set infos
    result->packed = PACK_NONE;
    result->packedSize = 0;
    result->num = numTiles;
    result->tiles = tileData;

    return result;
}

tilemap_* createMap(unsigned short* mapData, int w, int h)
{
    tilemap_* result;

    // alloc tileset structure
    result = malloc(sizeof(tilemap_));
    // set infos
    result->packed = PACK_NONE;
    result->packedSize = 0;
    result->w = w;
    result->h = h;
    result->data = mapData;

    return result;
}

tileimg_* getTiledImage(unsigned char* image8bpp, int w, int h, int opt, unsigned short baseFlag)
{
    int i, j;
    int pal, index;
    tileimg_ *result;
    unsigned int tile[8];
    unsigned short *m;

    result = malloc(sizeof(tileimg_));
    result->map = createMap(malloc(w * h * 2), w, h);
    result->tileset = createTileSet(malloc(TILE_MAX_NUM * 32), 0);

    m = result->map->data;

    for(j = 0; j < h; j++)
    {
        for(i = 0; i < w; i++)
        {
            pal = getTile(image8bpp, tile, i, j, w * 8);
            // error retrieving tile --> return NULL
            if (pal == -1)
            {
                freeTiledImage(result);
                return NULL;
            }

            index = addTile(tile, result->tileset, opt);
            // error adding new tile --> return NULL
            if (index == -1)
            {
                freeTiledImage(result);
                return NULL;
            }

            // set tilemap
            *m++ = (pal << TILE_PALETTE_SFT) | baseFlag | index;
        }
    }

    return result;
}

void freeTileset(tileset_ *tileset)
{
    free(tileset->tiles);
    free(tileset);
}

void freeMap(tilemap_ *map)
{
    free(map->data);
    free(map);
}

void freeTiledImage(tileimg_ *image)
{
    freeTileset(image->tileset);
    freeMap(image->map);
    free(image);
}


int packTileSet(tileset_* tileset, int *method)
{
    int size;

    tileset->tiles = (unsigned int*) pack((unsigned char*) tileset->tiles, 0, tileset->num * 32, &size, method);
    if (!tileset->tiles) return FALSE;

    tileset->packed = *method;
    tileset->packedSize = size;

    return TRUE;
}

int packMap(tilemap_* map, int *method)
{
    int size;

    map->data = (unsigned short*) packEx((unsigned char*) map->data, 0, map->w * map->h * 2, 2, TRUE, &size, method);
    if (!map->data) return FALSE;

    map->packed = *method;
    map->packedSize = size;

    return TRUE;
}


int getTile(unsigned char *image8bpp, unsigned int *tileout, int x, int y, int pitch)
{
    int i, j;
    unsigned char *src = &image8bpp[(x * 8) + ((y * 8) * pitch)];
    unsigned char *dst = (unsigned char*) tileout;
    int pal = -1;

    for(j = 0; j < 8; j++)
    {
        for(i = 0; i < 4; i++)
        {
            if (pal == -1)
                pal = src[0] >> 4;
            // palette value changed !
            else if ((pal != (src[0] >> 4)) || (pal != (src[1] >> 4)))
            {
                printf("Error: tile [%d,%d] reference different palette.", x, y);
                return -1;
            }

            *dst++ = ((src[0] & 0x0F) << 4) | ((src[1] & 0x0F) << 0);
            src += 2;
        }

        src += pitch - 8;
    }

    // invalid palette value
    if ((pal < 0) || (pal > 3))
    {
        printf("Error: more than 4 palettes in use in the image.");
        return -1;
    }

    return pal;
}

void flipTile(unsigned int *tilein, unsigned int *tileout, int hflip, int vflip)
{
    int i;

    for(i = 0; i < 8; i++)
    {
        int line;

        if (vflip) line = 7 - i;
        else line = i;

        if (hflip) tileout[i] = swapNibble32(tilein[line]);
        else tileout[i] = tilein[line];
    }
}

int isSameTile1(unsigned int *t1, unsigned int *t2, int hflip, int vflip)
{
    unsigned int tile[8];

    flipTile(t1, tile, hflip, vflip);
    if (!memcmp(t1, t2, 8 * 4)) return true;

    return false;
}

int isSameTile2(unsigned int *tile, tileset_ *tileset, int index, int hflip, int vflip)
{
    if (index >= tileset->num) return false;

    return isSameTile1(tile, &tileset->tiles[index * (32/4)], hflip, vflip);
}

int getTileIndex(unsigned int *tile, tileset_ *tileset, int allowFlip)
{
    int i;

    for(i = 0; i < tileset->num; i++)
    {
        if (isSameTile2(tile, tileset, i, false, false)) return i;
        if (allowFlip)
        {
            if (isSameTile2(tile, tileset, i, true, false)) return (i | TILE_HFLIP_FLAG);
            if (isSameTile2(tile, tileset, i, false, true)) return (i | TILE_VFLIP_FLAG);
            if (isSameTile2(tile, tileset, i, true, true)) return (i | TILE_HFLIP_FLAG | TILE_VFLIP_FLAG);
        }
    }

    return -1;
}

int tileExists(unsigned int *tile, tileset_ *tileset, int allowFlip)
{
    return getTileIndex(tile, tileset, allowFlip) != -1;
}

int addTile(unsigned int *tile, tileset_ *tileset, int opt)
{
    int result;

    // search if tile already exist
    if (opt)
    {
         result = getTileIndex(tile, tileset, true);
         // exist --> return it
         if (result != -1) return result;
    }

    // maximum number of tile reached
    if (tileset->num >= TILE_MAX_NUM)
    {
        printf("Error: maximum number of tile (2048) reached.\n");
        return -1;
    }

    // get tile index
    result = tileset->num;
    // increase number of tile
    tileset->num++;

    // copy new tile
    memcpy(&tileset->tiles[result * (32/4)], tile, 32);

    // return tile index
    return result;
}
