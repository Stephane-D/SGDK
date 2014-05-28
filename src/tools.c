#include "config.h"
#include "types.h"

#include "tools.h"

#include "string.h"
#include "kdebug.h"
#include "timer.h"
#include "maths.h"
#include "memory.h"
#include "vdp.h"


//forward
static u16 getBitmapAllocSize(const Bitmap *bitmap);
static u16 getTileSetAllocSize(const TileSet *tileset);
static u16 getMapAllocSize(const Map *map);
static Bitmap *allocateBitmapInternal(const Bitmap *bitmap, void *adr);
static TileSet *allocateTileSetInternal(const TileSet *tileset, void *adr);
static Map *allocateMapInternal(const Map *map, void *adr);

// internal
static u32 framecnt;
static u32 last;

u32 getFPS()
{
    static u32 result;

    const u32 current = getSubTick();
    const u32 delta = current - last;

	if (delta > 19200)
    {
        result = framecnt / delta;
        if (result > 999) result = 999;
        last = current;
        framecnt = 76800;
    }
	else framecnt += 76800;

	return result;
}

fix32 getFPS_f()
{
    static fix32 result;

    const u32 current = getSubTick();
    const u32 delta = current - last;

	if (delta > 19200)
    {
        if (framecnt > (250 * 76800)) result = FIX32(999);
        else
        {
            result = (framecnt << FIX16_FRAC_BITS) / delta;
            if (result > (999 << FIX16_FRAC_BITS)) result = FIX32(999);
            else result <<= (FIX32_FRAC_BITS - FIX16_FRAC_BITS);
        }
        last = current;
        framecnt = 76800;
    }
	else framecnt += 76800;

	return result;
}

void KLog(char* text)
{
    if (*text == 0)
        KDebug_Alert(" ");
    else
        KDebug_Alert(text);
}

void KLog_U1(char* t1, u32 v1)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    uintToStr(v1, tmp, 1);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_U2(char* t1, u32 v1, char* t2, u32 v2)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    uintToStr(v1, tmp, 1);
    strcat(str, tmp);
    strcat(str, t2);
    uintToStr(v2, tmp, 1);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_U3(char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    uintToStr(v1, tmp, 1);
    strcat(str, tmp);
    strcat(str, t2);
    uintToStr(v2, tmp, 1);
    strcat(str, tmp);
    strcat(str, t3);
    uintToStr(v3, tmp, 1);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_U4(char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3, char* t4, u32 v4)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    uintToStr(v1, tmp, 1);
    strcat(str, tmp);
    strcat(str, t2);
    uintToStr(v2, tmp, 1);
    strcat(str, tmp);
    strcat(str, t3);
    uintToStr(v3, tmp, 1);
    strcat(str, tmp);
    strcat(str, t4);
    uintToStr(v4, tmp, 1);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_U1x(u16 minSize, char* t1, u32 v1)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    uintToStr(v1, tmp, minSize);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_U2x(u16 minSize, char* t1, u32 v1, char* t2, u32 v2)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    uintToStr(v1, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t2);
    uintToStr(v2, tmp, minSize);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_U3x(u16 minSize, char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    uintToStr(v1, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t2);
    uintToStr(v2, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t3);
    uintToStr(v3, tmp, minSize);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_U4x(u16 minSize, char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3, char* t4, u32 v4)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    uintToStr(v1, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t2);
    uintToStr(v2, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t3);
    uintToStr(v3, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t4);
    uintToStr(v4, tmp, minSize);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_S1(char* t1, s32 v1)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    intToStr(v1, tmp, 1);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_S2(char* t1, s32 v1, char* t2, s32 v2)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    intToStr(v1, tmp, 1);
    strcat(str, tmp);
    strcat(str, t2);
    intToStr(v2, tmp, 1);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_S3(char* t1, s32 v1, char* t2, s32 v2, char* t3, s32 v3)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    intToStr(v1, tmp, 1);
    strcat(str, tmp);
    strcat(str, t2);
    intToStr(v2, tmp, 1);
    strcat(str, tmp);
    strcat(str, t3);
    intToStr(v3, tmp, 1);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_S4(char* t1, s32 v1, char* t2, s32 v2, char* t3, s32 v3, char* t4, s32 v4)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    intToStr(v1, tmp, 1);
    strcat(str, tmp);
    strcat(str, t2);
    intToStr(v2, tmp, 1);
    strcat(str, tmp);
    strcat(str, t3);
    intToStr(v3, tmp, 1);
    strcat(str, tmp);
    strcat(str, t4);
    intToStr(v4, tmp, 1);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_S1x(u16 minSize, char* t1, s32 v1)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    intToStr(v1, tmp, minSize);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_S2x(u16 minSize, char* t1, s32 v1, char* t2, s32 v2)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    intToStr(v1, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t2);
    intToStr(v2, tmp, minSize);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_S3x(u16 minSize, char* t1, s32 v1, char* t2, s32 v2, char* t3, s32 v3)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    intToStr(v1, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t2);
    intToStr(v2, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t3);
    intToStr(v3, tmp, minSize);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_S4x(u16 minSize, char* t1, s32 v1, char* t2, s32 v2, char* t3, s32 v3, char* t4, s32 v4)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    intToStr(v1, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t2);
    intToStr(v2, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t3);
    intToStr(v3, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t4);
    intToStr(v4, tmp, minSize);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_f1(char* t1, fix16 v1)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix16ToStr(v1, tmp, 2);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_f2(char* t1, fix16 v1, char* t2, fix16 v2)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix16ToStr(v1, tmp, 2);
    strcat(str, tmp);
    strcat(str, t2);
    fix16ToStr(v2, tmp, 2);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_f3(char* t1, fix16 v1, char* t2, fix16 v2, char* t3, fix16 v3)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix16ToStr(v1, tmp, 2);
    strcat(str, tmp);
    strcat(str, t2);
    fix16ToStr(v2, tmp, 2);
    strcat(str, tmp);
    strcat(str, t3);
    fix16ToStr(v3, tmp, 2);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_f4(char* t1, fix16 v1, char* t2, fix16 v2, char* t3, fix16 v3, char* t4, fix16 v4)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix16ToStr(v1, tmp, 2);
    strcat(str, tmp);
    strcat(str, t2);
    fix16ToStr(v2, tmp, 2);
    strcat(str, tmp);
    strcat(str, t3);
    fix16ToStr(v3, tmp, 2);
    strcat(str, tmp);
    strcat(str, t4);
    fix16ToStr(v4, tmp, 2);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_f1x(s16 numDec, char* t1, fix16 v1)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix16ToStr(v1, tmp, numDec);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_f2x(s16 numDec, char* t1, fix16 v1, char* t2, fix16 v2)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix16ToStr(v1, tmp, numDec);
    strcat(str, tmp);
    strcat(str, t2);
    fix16ToStr(v2, tmp, numDec);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_f3x(s16 numDec, char* t1, fix16 v1, char* t2, fix16 v2, char* t3, fix16 v3)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix16ToStr(v1, tmp, numDec);
    strcat(str, tmp);
    strcat(str, t2);
    fix16ToStr(v2, tmp, numDec);
    strcat(str, tmp);
    strcat(str, t3);
    fix16ToStr(v3, tmp, numDec);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_f4x(s16 numDec, char* t1, fix16 v1, char* t2, fix16 v2, char* t3, fix16 v3, char* t4, fix16 v4)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix16ToStr(v1, tmp, numDec);
    strcat(str, tmp);
    strcat(str, t2);
    fix16ToStr(v2, tmp, numDec);
    strcat(str, tmp);
    strcat(str, t3);
    fix16ToStr(v3, tmp, numDec);
    strcat(str, tmp);
    strcat(str, t4);
    fix16ToStr(v4, tmp, numDec);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_F1(char* t1, fix32 v1)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix32ToStr(v1, tmp, 2);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_F2(char* t1, fix32 v1, char* t2, fix32 v2)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix32ToStr(v1, tmp, 2);
    strcat(str, tmp);
    strcat(str, t2);
    fix32ToStr(v2, tmp, 2);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_F3(char* t1, fix32 v1, char* t2, fix32 v2, char* t3, fix32 v3)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix32ToStr(v1, tmp, 2);
    strcat(str, tmp);
    strcat(str, t2);
    fix32ToStr(v2, tmp, 2);
    strcat(str, tmp);
    strcat(str, t3);
    fix32ToStr(v3, tmp, 2);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_F4(char* t1, fix32 v1, char* t2, fix32 v2, char* t3, fix32 v3, char* t4, fix32 v4)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix32ToStr(v1, tmp, 2);
    strcat(str, tmp);
    strcat(str, t2);
    fix32ToStr(v2, tmp, 2);
    strcat(str, tmp);
    strcat(str, t3);
    fix32ToStr(v3, tmp, 2);
    strcat(str, tmp);
    strcat(str, t4);
    fix32ToStr(v4, tmp, 2);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_F1x(s16 numDec, char* t1, fix32 v1)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix32ToStr(v1, tmp, numDec);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_F2x(s16 numDec, char* t1, fix32 v1, char* t2, fix32 v2)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix32ToStr(v1, tmp, numDec);
    strcat(str, tmp);
    strcat(str, t2);
    fix32ToStr(v2, tmp, numDec);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_F3x(s16 numDec, char* t1, fix32 v1, char* t2, fix32 v2, char* t3, fix32 v3)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix32ToStr(v1, tmp, numDec);
    strcat(str, tmp);
    strcat(str, t2);
    fix32ToStr(v2, tmp, numDec);
    strcat(str, tmp);
    strcat(str, t3);
    fix32ToStr(v3, tmp, numDec);
    strcat(str, tmp);

    KDebug_Alert(str);
}

void KLog_F4x(s16 numDec, char* t1, fix32 v1, char* t2, fix32 v2, char* t3, fix32 v3, char* t4, fix32 v4)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    fix32ToStr(v1, tmp, numDec);
    strcat(str, tmp);
    strcat(str, t2);
    fix32ToStr(v2, tmp, numDec);
    strcat(str, tmp);
    strcat(str, t3);
    fix32ToStr(v3, tmp, numDec);
    strcat(str, tmp);
    strcat(str, t4);
    fix32ToStr(v4, tmp, numDec);
    strcat(str, tmp);

    KDebug_Alert(str);
}


static u16 getBitmapAllocSize(const Bitmap *bitmap)
{
    // need space to decompress
    if (bitmap->compression != COMPRESSION_NONE)
        return (bitmap->w * bitmap->h) / 2;

    return 0;
}

static u16 getTileSetAllocSize(const TileSet *tileset)
{
    // need space to decompress
    if (tileset->compression != COMPRESSION_NONE)
        return tileset->numTile * 32;

    return 0;
}

static u16 getMapAllocSize(const Map *map)
{
    // need space to decompress
    if (map->compression != COMPRESSION_NONE)
        return map->w * map->h * 2;

    return 0;
}


static Bitmap *allocateBitmapInternal(const Bitmap *bitmap, void *adr)
{
    // cast
    Bitmap *result = (Bitmap*) adr;

    if (result != NULL)
    {
        result->compression = COMPRESSION_NONE;

        if (bitmap->compression != COMPRESSION_NONE)
            // allocate sub buffers (no need to allocate palette as we directly use the source pointer)
            result->image = (u8*) (adr + sizeof(Bitmap));
        else
            // image is not compressed --> directly use source pointer
            result->image = bitmap->image;
    }

    return result;
}

static TileSet *allocateTileSetInternal(const TileSet *tileset, void *adr)
{
    // cast
    TileSet *result = (TileSet*) adr;

    if (result != NULL)
    {
        result->compression = COMPRESSION_NONE;

        if (tileset->compression != COMPRESSION_NONE)
            // allocate sub buffers (no need to allocate palette as we directly use the source pointer)
            result->tiles = (u32*) (adr + sizeof(TileSet));
        else
            // tileset is not compressed --> directly use source pointer
            result->tiles = tileset->tiles;
    }

    return result;
}

static Map *allocateMapInternal(const Map *map, void *adr)
{
    // cast
    Map *result = (Map*) adr;

    if (result != NULL)
    {
        result->compression = COMPRESSION_NONE;

        if (map->compression != COMPRESSION_NONE)
            // allocate sub buffers (no need to allocate palette as we directly use the source pointer)
            result->tilemap = (u16*) (adr + sizeof(Map));
        else
            // tilemap is not compressed --> directly use source pointer
            result->tilemap = map->tilemap;
    }

    return result;
}

Bitmap *allocateBitmap(const Bitmap *bitmap)
{
    return allocateBitmapInternal(bitmap, MEM_alloc(getBitmapAllocSize(bitmap) + sizeof(Bitmap)));
}

TileSet *allocateTileSet(const TileSet *tileset)
{
    return allocateTileSetInternal(tileset, MEM_alloc(getTileSetAllocSize(tileset) + sizeof(TileSet)));
}

Map *allocateMap(const Map *map)
{
    return allocateMapInternal(map, MEM_alloc(getMapAllocSize(map) + sizeof(Map)));
}

Image *allocateImage(const Image *image)
{
    u16 sizeTileset;
    u16 sizeMap;
    TileSet *tileset = image->tileset;
    Map *map = image->map;

    if (tileset->compression != COMPRESSION_NONE)
        sizeTileset = getTileSetAllocSize(tileset) + sizeof(TileSet);
    else
        sizeTileset = 0;
    if (map->compression != COMPRESSION_NONE)
        sizeMap = getMapAllocSize(map) + sizeof(Map);
    else
        sizeMap = 0;

    const void *adr = MEM_alloc(sizeTileset + sizeMap + sizeof(Image));

    // cast
    Image *result = (Image*) adr;

    if (result != NULL)
    {
        if (tileset->compression != COMPRESSION_NONE)
            // allocate tileset buffer
            result->tileset = allocateTileSetInternal(tileset, (void*) (adr + sizeof(Image)));
        else
            // tileset is not compressed --> directly use source pointer
            result->tileset = tileset;

        if (map->compression != COMPRESSION_NONE)
            // allocate map buffer
            result->map = allocateMapInternal(map, (void*) (adr + sizeof(Image) + sizeTileset));
        else
            // map is not compressed --> directly use source pointer
            result->map = map;
    }

    return result;
}


Bitmap *unpackBitmap(const Bitmap *src, Bitmap *dest)
{
    Bitmap *result;

    if (dest) result = dest;
    else result = allocateBitmap(src);

    if (result != NULL)
    {
        // fill infos (palette is never packed)
        result->w = src->w;
        result->h = src->h;
        result->palette = src->palette;

        // unpack image
        if (src->compression != COMPRESSION_NONE)
            unpack(src->compression, (u8*) src->image, (u8*) result->image);
    }

    return result;
}

TileSet *unpackTileSet(const TileSet *src, TileSet *dest)
{
    TileSet *result;

    if (dest) result = dest;
    else result = allocateTileSet(src);

    if (result != NULL)
    {
        // fill infos
        result->numTile = src->numTile;

        // unpack tiles
        if (src->compression != COMPRESSION_NONE)
            unpack(src->compression, (u8*) src->tiles, (u8*) result->tiles);
    }

    return result;
}

Map *unpackMap(const Map *src, Map *dest)
{
    Map *result;

    if (dest) result = dest;
    else result = allocateMap(src);

    if (result != NULL)
    {
        // fill infos
        result->w = src->w;
        result->h = src->h;

        // unpack tilemap
        if (src->compression != COMPRESSION_NONE)
            unpack(src->compression, (u8*) src->tilemap, (u8*) result->tilemap);
    }

    return result;
}

Image *unpackImage(const Image *src, Image *dest)
{
    Image *result;

    if (dest) result = dest;
    else result = allocateImage(src);

    if (result != NULL)
    {
        // fill infos (palette is never packed)
        result->palette = src->palette;

        // unpack tileset
        if (src->tileset->compression != COMPRESSION_NONE)
            unpackTileSet(src->tileset, result->tileset);
        // unpack map
        if (src->map->compression != COMPRESSION_NONE)
            unpackMap(src->map, result->map);
    }

    return result;
}


void unpack(u16 compression, u8 *src, u8 *dest)
{
    switch(compression)
    {
        // nothing to do
        default:
            break;

        case COMPRESSION_APLIB:
            aplib_unpack(src, dest);
            break;

//        case COMPRESSION_LZKN:
//            lzkn_unpack(src, dest);
//            break;

        case COMPRESSION_RLE:
            rle4b_unpack(src, dest);
            break;

        case COMPRESSION_MAP_RLE:
            rlemap_unpack(src, dest);
            break;
    }
}


void rle4b_unpack(u8 *src, u8 *dest)
{
    u8 *s;
    u32 *d;
    u32 data;
    u16 blocnum;
    u16 data_cnt;

    blocnum = *((unsigned short*)src);
    s = src + 2;
    d = (u32*) dest;
    data = 0;
    data_cnt = 7;
    while (blocnum--)
    {
        const u8 v8 = *s++;
        const u32 d4 = v8 & 0xF;
        u16 len = (v8 >> 4) + 1;

        while(len--)
        {
            data <<= 4;
            data |= d4;

            if (!data_cnt--)
            {
                *d++ = data;
                data_cnt = 7;
            }
        }
    }
}

void rlemap_unpack(u8 *src, u8 *dest)
{
    u8 *s;
    u16 *d;
    u16 blocnum;

    blocnum = *((unsigned short*)src);
    s = src + 2;
    d = (u16*) dest;
    while (blocnum--)
    {
        u16 data;
        const u8 v8 = *s++;
        u16 len = (v8 & 0x7F) + 1;

        data = (s[0] << 8) | (s[1] << 0);
        s += 2;

        // increment mode
        if (v8 & 0x80)
        {
            while(len--)
                *d++ = data++;
        }
        else
        {
            while(len--)
                *d++ = data;
        }
    }
}

void rle4b_unpackVRam(u8 *src, u16 dest)
{
    vu32 *plctrl;
    vu32 *pldata;
    u8 *s;
    u32 data;
    u16 blocnum;
    u16 data_cnt;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pldata = (u32 *) GFX_DATA_PORT;

    *plctrl = GFX_WRITE_VRAM_ADDR(dest);

    blocnum = *((unsigned short*)src);
    s = src + 2;
    data = 0;
    data_cnt = 7;
    while (blocnum--)
    {
        const u8 v8 = *s++;
        const u32 d4 = v8 & 0xF;
        u16 len = (v8 >> 4) + 1;

        while(len--)
        {
            data <<= 4;
            data |= d4;

            if (!data_cnt--)
            {
                *pldata = data;
                data_cnt = 7;
            }
        }
    }
}

void rlemap_unpackVRam(u8 *src, u16 dest, u16 basetile)
{
    vu32 *plctrl;
    vu16 *pwdata;
    u8 *s;
    u16 blocnum;

    VDP_setAutoInc(2);

    /* point to vdp port */
    plctrl = (u32 *) GFX_CTRL_PORT;
    pwdata = (u16 *) GFX_DATA_PORT;

    *plctrl = GFX_WRITE_VRAM_ADDR(dest);

    blocnum = *((unsigned short*)src);
    s = src + 2;
    while (blocnum--)
    {
        u16 data;
        const u8 v8 = *s++;
        u16 len = (v8 & 0x7F) + 1;

        data = (s[0] << 8) | (s[1] << 0);
        data += basetile;
        s += 2;

        // increment mode
        if (v8 & 0x80)
        {
            while(len--)
                *pwdata = data++;
        }
        else
        {
            while(len--)
                *pwdata = data;
        }
    }
}
