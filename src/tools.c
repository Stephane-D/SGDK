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
u16 randbase;


void setRandomSeed(u16 seed)
{
    // xor it with a random value to avoid 0 value
    randbase = seed ^ 0xD94B;
}

u16 random()
{
    randbase ^= (randbase >> 1) ^ GET_HVCOUNTER;
    randbase ^= (randbase << 1);

    return randbase;
}

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

void KLog_U1_(char* t1, u32 v1, char* t2)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    uintToStr(v1, tmp, 1);
    strcat(str, tmp);
    strcat(str, t2);

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

void KLog_U2_(char* t1, u32 v1, char* t2, u32 v2, char* t3)
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

void KLog_U3_(char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3, char *t4)
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

void KLog_U4_(char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3, char* t4, u32 v4, char* t5)
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
    strcat(str, t5);

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

void KLog_U1x_(u16 minSize, char* t1, u32 v1, char* t2)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    uintToStr(v1, tmp, minSize);
    strcat(str, tmp);
    strcat(str, t2);

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

void KLog_U2x_(u16 minSize, char* t1, u32 v1, char* t2, u32 v2, char* t3)
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

void KLog_U3x_(u16 minSize, char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3, char* t4)
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

void KLog_U4x_(u16 minSize, char* t1, u32 v1, char* t2, u32 v2, char* t3, u32 v3, char* t4, u32 v4, char* t5)
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
    strcat(str, t5);

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

void KLog_S1_(char* t1, s32 v1, char* t2)
{
    char str[256];
    char tmp[12];

    strcpy(str, t1);
    intToStr(v1, tmp, 1);
    strcat(str, tmp);
    strcat(str, t2);

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

void KLog_S2_(char* t1, s32 v1, char* t2, s32 v2, char* t3)
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

void KLog_S3_(char* t1, s32 v1, char* t2, s32 v2, char* t3, s32 v3, char* t4)
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

void KLog_S4_(char* t1, s32 v1, char* t2, s32 v2, char* t3, s32 v3, char* t4, s32 v4, char* t5)
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
    strcat(str, t5);

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

Bitmap *allocateBitmapEx(u16 width, u16 heigth)
{
    // allocate
    void *adr = MEM_alloc(((width * heigth) / 2) + sizeof(Bitmap));
    Bitmap *result = (Bitmap*) adr;

    if (result != NULL)
    {
        result->compression = COMPRESSION_NONE;
        // set image pointer
        result->image = (u8*) (adr + sizeof(Bitmap));
    }

    return result;
}

TileSet *allocateTileSet(const TileSet *tileset)
{
    return allocateTileSetInternal(tileset, MEM_alloc(getTileSetAllocSize(tileset) + sizeof(TileSet)));
}

TileSet *allocateTileSetEx(u16 numTile)
{
    // allocate
    void *adr = MEM_alloc((numTile * 32) + sizeof(TileSet));
    TileSet *result = (TileSet*) adr;

    if (result != NULL)
    {
        result->compression = COMPRESSION_NONE;
        // set tiles pointer
        result->tiles = (u32*) (adr + sizeof(TileSet));
    }

    return result;
}

Map *allocateMap(const Map *map)
{
    return allocateMapInternal(map, MEM_alloc(getMapAllocSize(map) + sizeof(Map)));
}

Map *allocateMapEx(u16 width, u16 heigth)
{
    // allocate
    void *adr = MEM_alloc((width * heigth * 2) + sizeof(Map));
    Map *result = (Map*) adr;

    if (result != NULL)
    {
        result->compression = COMPRESSION_NONE;
        // set tilemap pointer
        result->tilemap = (u16*) (adr + sizeof(Map));
    }

    return result;
}

Image *allocateImage(const Image *image)
{
    u16 sizeTileset;
    u16 sizeMap;
    TileSet *tileset = image->tileset;
    Map *map = image->map;

    if (tileset->compression != COMPRESSION_NONE)
        sizeTileset = (tileset->numTile * 32) + sizeof(TileSet);
    else
        sizeTileset = 0;
    if (map->compression != COMPRESSION_NONE)
        sizeMap = (map->w * map->h * 2) + sizeof(Map);
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
        // simple copy if needed
        else if (src->image != result->image)
            memcpy((u8*) result->image, (u8*) src->image, (src->w * src->h) / 2);
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
        // simple copy if needed
        else if (src->tiles != result->tiles)
            memcpy((u8*) result->tiles, (u8*) src->tiles, src->numTile * 32);
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
        // simple copy if needed
        else if (src->tilemap != result->tilemap)
            memcpy((u8*) result->tilemap, (u8*) src->tilemap, (src->w * src->h) * 2);
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

        // unpack tileset if needed
        if (src->tileset != result->tileset)
            unpackTileSet(src->tileset, result->tileset);
        // unpack map if needed
        if (src->map != result->map)
            unpackMap(src->map, result->map);
    }

    return result;
}


u32 unpack(u16 compression, u8 *src, u8 *dest)
{
    switch(compression)
    {
//        case COMPRESSION_NONE:
//            // cannot do anything...
//            if (size == 0) return FALSE;
//
//            // use simple memory copy
//            memcpy(dest, &src[offset], size);
//            break;

        case COMPRESSION_APLIB:
            return aplib_unpack(src, dest);

        case COMPRESSION_LZ4W:
            return lz4w_unpack(src, dest);

        default:
            return 0;
    }
}


#define QSORT(type)                                     \
    u16 partition_##type(type *data, u16 p, u16 r)      \
    {                                                   \
        type x = data[p];                               \
        u16 i = p - 1;                                  \
        u16 j = r + 1;                                  \
                                                        \
        while (TRUE)                                    \
        {                                               \
            i++;                                        \
            while ((i < r) && (data[i] < x)) i++;       \
            j--;                                        \
            while ((j > p) && (data[j] > x)) j--;       \
                                                        \
            if (i < j)                                  \
            {                                           \
                type tmp;                               \
                                                        \
                tmp = data[i];                          \
                data[i] = data[j];                      \
                data[j] = tmp;                          \
            }                                           \
            else                                        \
                return j;                               \
        }                                               \
    }                                                   \
                                                        \
    void qsort_##type(type *data, u16 p, u16 r)         \
    {                                                   \
        if (p < r)                                      \
        {                                               \
            u16 q = partition_##type(data, p, r);       \
            qsort_##type(data, p, q);                   \
            qsort_##type(data, q + 1, r);               \
        }                                               \
    }


QSORT(u8)
QSORT(s8)
QSORT(u16)
QSORT(s16)
QSORT(u32)
QSORT(s32)



//--> try to improve qsort speed (test on spr_engine sort exemple)

//// Sorting generic structure
//struct  QSORT_ENTRY
//{
//    fix16   value;
//    u16    index;
//};
//
//static struct QSORT_ENTRY t;
//
////------------------------------------------------------
//inline void    QSwap (struct QSORT_ENTRY *a, struct QSORT_ENTRY *b)
////------------------------------------------------------
//{
//    // struct QSORT_ENTRY t = *a;
//    t = *a;
//    *a = *b;
//    *b = t;
//}
//
////----------------------------------------
//// http://rosettacode.org/wiki/Sorting_algorithms/Quicksort#C
//inline void    QuickSort (u16 n, struct QSORT_ENTRY *a)
////----------------------------------------
//{
//    short i, j, p;
//    if (n < 2)
//        return;
//    p = a[n >> 1].value;
//    for (i = 0, j = n - 1;; i++, j--) {
//        while (a[i].value < p)
//            i++;
//        while (p < a[j].value)
//            j--;
//        if (i >= j)
//            break;
//
//        QSwap(&a[i], &a[j]);
//    }
//    QuickSort(i, a);
//    QuickSort(n - i, a + i);
//}

//void** qsort_part(void** l, void** r, _comparatorCallback* cb)
//{
//    void** p = l + ((r - l) / 2);
//    void* pivot = *p;
//
//    while (TRUE)
//    {
//        while ((l < r) && (cb(*r, pivot) >= 0)) r--;
//        while ((l < r) && (cb(*l, pivot) <= 0)) l++;
//
//        if (l < r)
//        {
//            void* tmp = *l;
//            *l = *r;
//            *r = tmp;
//        }
//        else return l;
//    }
//}
//
///*
//5 3 8 7  2  1 1 8 7
//l        p        r
//5 3 8 7  2  1 1 8 7
//l             r
//1 3 8 7  2  1 5 8 7
//l             r
//1 3 8 7  2  1 5 8 7
//  l         r
//1 1 8 7  2  3 5 8 7
//  l         r
//1 1 8 7  2  3 5 8 7
//    l    r
//1 1 2 7  8  3 5 8 7
//    l    r
//1 1 2 7  8  3 5 8 7
//      lr
//*/
//
//void qsort_rec(void** l, void** r, _comparatorCallback* cb)
//{
//    if (l < r)
//    {
//        void** p = qsort_part(l, r, cb);
//        qsort_rec(l, p, cb);
//        qsort_rec(p + 1, r, cb);
//    }
//}
//
//void QSort(void** data, u16 size, _comparatorCallback* cb)
//{
//    qsort_rec(&data[0], &data[size - 1], cb);
//}
