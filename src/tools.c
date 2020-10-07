#include "config.h"
#include "types.h"

#include "tools.h"

#include "sys.h"
#include "string.h"
#include "kdebug.h"
#include "timer.h"
#include "maths.h"
#include "memory.h"
#include "mapper.h"
#include "vdp.h"


// forward
static u16 getBitmapAllocSize(const Bitmap *bitmap);
static u16 getTileSetAllocSize(const TileSet *tileset);
static u16 getMapAllocSize(const TileMap *tilemap);
static Bitmap *allocateBitmapInternal(void *adr);
static TileSet *allocateTileSetInternal(void *adr);
static TileMap *allocateMapInternal(void *adr);

// internal
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
    return SYS_getFPS();
}

fix32 getFPS_f()
{
    return SYS_getFPSAsFloat();
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
    return (bitmap->w * bitmap->h) / 2;
}

static u16 getTileSetAllocSize(const TileSet *tileset)
{
    return tileset->numTile * 32;
}

static u16 getMapAllocSize(const TileMap *tilemap)
{
    return tilemap->w * tilemap->h * 2;
}


static Bitmap *allocateBitmapInternal(void *adr)
{
    // cast
    Bitmap *result = (Bitmap*) adr;

    if (result != NULL)
    {
        result->compression = COMPRESSION_NONE;
        // allocate image buffer
        result->image = (u8*) (adr + sizeof(Bitmap));
    }

    return result;
}

static TileSet *allocateTileSetInternal(void *adr)
{
    // cast
    TileSet *result = (TileSet*) adr;

    if (result != NULL)
    {
        result->compression = COMPRESSION_NONE;
        // allocate tiles buffer
        result->tiles = (u32*) (adr + sizeof(TileSet));
    }

    return result;
}

static TileMap *allocateMapInternal(void *adr)
{
    // cast
    TileMap *result = (TileMap*) adr;

    if (result != NULL)
    {
        result->compression = COMPRESSION_NONE;
        // allocate tilemap buffer
        result->tilemap = (u16*) (adr + sizeof(TileMap));
    }

    return result;
}


Bitmap *allocateBitmap(const Bitmap *bitmap)
{
    return allocateBitmapInternal(MEM_alloc(getBitmapAllocSize(bitmap) + sizeof(Bitmap)));
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
    return allocateTileSetInternal(MEM_alloc(getTileSetAllocSize(tileset) + sizeof(TileSet)));
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
        // and tile number
        result->numTile = numTile;
    }

    return result;
}

TileMap *allocateTileMap(const TileMap *tilemap)
{
    return allocateMapInternal(MEM_alloc(getMapAllocSize(tilemap) + sizeof(TileMap)));
}

TileMap *allocateTileMapEx(u16 width, u16 heigth)
{
    // allocate
    void *adr = MEM_alloc((width * heigth * 2) + sizeof(TileMap));
    TileMap *result = (TileMap*) adr;

    if (result != NULL)
    {
        result->compression = COMPRESSION_NONE;
        // set tilemap pointer
        result->tilemap = (u16*) (adr + sizeof(TileMap));
        // and tilemap size
        result->w = width;
        result->h = heigth;
    }

    return result;
}

Image *allocateImage(const Image *image)
{
    TileSet *tileset = image->tileset;
    TileMap *tilemap = image->tilemap;

    // get allocation size
    u16 sizeTileset = getTileSetAllocSize(tileset) + sizeof(TileSet);
    u16 sizeMap = getMapAllocSize(tilemap) + sizeof(TileMap);

    const void *adr = MEM_alloc(sizeTileset + sizeMap + sizeof(Image));

    // cast
    Image *result = (Image*) adr;

    if (result != NULL)
    {
        // allocate tileset buffer
        result->tileset = allocateTileSetInternal((void*) (adr + sizeof(Image)));
        // allocate tilemap buffer
        result->tilemap = allocateMapInternal((void*) (adr + sizeof(Image) + sizeTileset));
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
        // fill infos (always use shallow copy for palette)
        result->w = src->w;
        result->h = src->h;
        result->palette = src->palette;
        result->compression = COMPRESSION_NONE;

        // unpack image
        if (src->compression != COMPRESSION_NONE)
            unpack(src->compression, (u8*) FAR(src->image), (u8*) result->image);
        // simple copy if needed
        else if (src->image != result->image)
            memcpy((u8*) result->image, FAR(src->image), (src->w * src->h) / 2);
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
        result->compression = COMPRESSION_NONE;

        // unpack tiles
        if (src->compression != COMPRESSION_NONE)
            unpack(src->compression, (u8*) FAR(src->tiles), (u8*) result->tiles);
        // simple copy if needed
        else if (src->tiles != result->tiles)
            memcpy((u8*) result->tiles, FAR(src->tiles), src->numTile * 32);
    }

    return result;
}

TileMap *unpackTileMap(const TileMap *src, TileMap *dest)
{
    TileMap *result;

    if (dest) result = dest;
    else result = allocateTileMap(src);

    if (result != NULL)
    {
        // fill infos
        result->w = src->w;
        result->h = src->h;
        result->compression = COMPRESSION_NONE;

        // unpack tilemap
        if (src->compression != COMPRESSION_NONE)
            unpack(src->compression, (u8*) FAR(src->tilemap), (u8*) result->tilemap);
        // simple copy if needed
        else if (src->tilemap != result->tilemap)
            memcpy((u8*) result->tilemap, FAR(src->tilemap), (src->w * src->h) * 2);
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
        // fill infos (always use shallow copy for palette)
        result->palette = src->palette;

        // unpack tileset if needed
        if (src->tileset != result->tileset)
            unpackTileSet(src->tileset, result->tileset);
        // unpack tilemap if needed
        if (src->tilemap != result->tilemap)
            unpackTileMap(src->tilemap, result->tilemap);
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
