#ifndef _TILE_TOOLS_H_
#define _TILE_TOOLS_H_


#define TILE_MAX_NUM        (1 << 11)
#define TILE_INDEX_MASK     (TILE_MAX_NUM - 1)

#define TILE_HFLIP_SFT      (11)
#define TILE_VFLIP_SFT      (12)
#define TILE_PALETTE_SFT    (13)
#define TILE_PRIORITY_SFT   (15)

#define TILE_HFLIP_FLAG     (1 << TILE_HFLIP_SFT)
#define TILE_VFLIP_FLAG     (1 << TILE_VFLIP_SFT)
#define TILE_PALETTE_MASK   (3 << TILE_PALETTE_SFT)
#define TILE_PRIORITY_FLAG  (1 << TILE_PRIORITY_SFT)

#define TILE_ATTR_MASK      (TILE_PRIORITY_MASK | TILE_PALETTE_MASK | TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK)

#define TILE_ATTR(pal, prio, flipV, flipH)               (((flipH) << TILE_HFLIP_SFT) + ((flipV) << TILE_VFLIP_SFT) + ((pal) << TILE_PALETTE_SFT) + ((prio) << TILE_PRIORITY_SFT))
#define TILE_ATTR_FULL(pal, prio, flipV, flipH, index)   (((flipH) << TILE_HFLIP_SFT) + ((flipV) << TILE_VFLIP_SFT) + ((pal) << TILE_PALETTE_SFT) + ((prio) << TILE_PRIORITY_SFT) + (index))


typedef struct {
    int packed;
    int packedSize;
    int num;
    unsigned int* tiles;
} tileset_;

typedef struct {
    int packed;
    int packedSize;
    int w;
    int h;
    unsigned short* data;
} tilemap_;

typedef struct {
    tileset_* tileset;
    tilemap_* map;
} tileimg_;

// transform a 4bpp input tiled image to bitmap image
// the input size is given in tiles
unsigned char *tileToBmp(unsigned char *in, int inOffset, int w, int h);
// transform a 4bpp input bitmap image to tiled image
// the input size is given in tiles
unsigned char *bmpToTile(unsigned char *in, int inOffset, int w, int h);

tileset_* createTileSet(unsigned int* tileData, int numTiles);
tilemap_* createMap(unsigned short* mapData, int w, int h);
// return a TiledImage from 8bpp bitmap image
tileimg_ *getTiledImage(unsigned char* image8bpp, int w, int h, int opt, unsigned short baseFlag);

void freeTileset(tileset_* tileset);
void freeMap(tilemap_* map);
void freeTiledImage(tileimg_* image);

int packTileSet(tileset_* tileset, int *method);
int packMap(tilemap_* map, int *method);

int getTile(unsigned char *image8bpp, unsigned int *tileout, int x, int y, int pitch);
void flipTile(unsigned int *tilein, unsigned int *tileout, int hflip, int vflip);
int isSameTile1(unsigned int *t1, unsigned int *t2, int hflip, int vflip);
int isSameTile2(unsigned int *tile, tileset_ *tileset, int index, int hflip, int vflip);
int getTileIndex(unsigned int *tile, tileset_ *tileset, int allowFlip);
int tileExists(unsigned int *tile, tileset_ *tileset, int allowFlip);
int addTile(unsigned int *tile, tileset_ *tileset, int opt);

#endif // _TILE_TOOLS_H_
