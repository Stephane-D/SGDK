#include "config.h"
#include "types.h"

#include "vdp.h"
#include "vdp_spr.h"

#include "dma.h"
#include "tools.h"
#include "string.h"
#include "memory.h"


// important to have a global structure here (consumes 640 bytes of memory)
VDPSprite vdpSpriteCache[MAX_VDP_SPRITE];
// keep trace of last allocated sprite (for special operation as link)
VDPSprite *lastAllocatedVDPSprite;
// keep trace of highest index allocated since the last VDP_resetSprites() or VDP_releaseAllSprites.
// It can be used to define the number of sprite to transfer with VDP_updateSprites(..)
s16 highestVDPSpriteIndex;

// used for VDP sprite allocation
static VDPSprite *allocStack[MAX_VDP_SPRITE];
// point on top of the allocation stack (first available VDP sprite)
static VDPSprite **free;


void VDP_resetSprites()
{
    // reset allocation
    VDP_releaseAllSprites();
    // clear sprite
    VDP_clearSprites();
}

void VDP_releaseAllSprites()
{
    u16 i;

    // reset allocation stack (important that first allocated sprite is sprite 0)
    for(i = 0; i < MAX_VDP_SPRITE; i++)
        allocStack[i] = &vdpSpriteCache[(MAX_VDP_SPRITE - 1) - i];
    // init free position
    free = &allocStack[MAX_VDP_SPRITE];

    highestVDPSpriteIndex = -1;
}


s16 VDP_allocateSprites(u16 num)
{
    VDPSprite* spr;
    u16 remaining;
    s16 res;
    s16 maxInd;

    // enough sprite remaining ?
    if ((free - allocStack) < num)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        KLog_U2("VDP_allocateSprites(", num, ") failed: no enough free VDP Sprite - remaining = ", VDP_getAvailableSprites());
#endif

        return -1;
    }

    spr = *--free;
    // save index
    res = spr - vdpSpriteCache;
    maxInd = res;

    // link sprites
    remaining = num - 1;
    while(remaining--)
    {
        VDPSprite* curSpr = *--free;
        s16 ind = curSpr - vdpSpriteCache;

        // keep trace of highest index
        if (ind > maxInd) maxInd = ind;

        // link and pass to the next
        spr->link = ind;
        spr = curSpr;
    }

    // keep trace of highest index
    if (maxInd > highestVDPSpriteIndex)
        highestVDPSpriteIndex = maxInd;

    // end link with 0
    spr->link = 0;
    // keep trace of last allocated sprite
    lastAllocatedVDPSprite = spr;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    KLog_U3("VDP_allocateSprites(", num, ") success: ", res, " - remaining = ", VDP_getAvailableSprites());
#endif

    // return allocated sprite index
    return res;
}

void VDP_releaseSprites(u16 index, u16 num)
{
    VDPSprite* spr;
    u16 remaining;

    spr = &vdpSpriteCache[index];
    // release sprite
    *free++ = spr;

    remaining = num - 1;
    while(remaining--)
    {
        spr = &vdpSpriteCache[spr->link];
        // release sprite
        *free++ = spr;
    }

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    KLog_U3("VDP_releaseSprites(", index, ", ", num, ") --> remaining = ", VDP_getAvailableSprites());
#endif
}

u16 VDP_getAvailableSprites()
{
    return free - allocStack;
}

s16 VDP_refreshHighestAllocatedSpriteIndex()
{
    s16 res = -1;
    VDPSprite **top = &allocStack[MAX_VDP_SPRITE];

    while(top > free)
    {
        VDPSprite* curSpr = *--top;
        s16 ind = curSpr - vdpSpriteCache;

        // keep trace of highest index
        if (ind > res) res = ind;
    }

    // update maximum index
    highestVDPSpriteIndex = res;

    return res;
}


void VDP_clearSprites()
{
    VDPSprite *sprite = &vdpSpriteCache[0];

    // hide it
    sprite->y = 0;
    sprite->link = 0;
}

void VDP_setSpriteFull(u16 index, s16 x, s16 y, u8 size, u16 attribut, u8 link)
{
    VDPSprite *sprite = &vdpSpriteCache[index];

    sprite->y = y + 0x80;
    sprite->size = size;
    sprite->link = link;
    sprite->attribut = attribut;
    sprite->x = x + 0x80;
}

void VDP_setSprite(u16 index, s16 x, s16 y, u8 size, u16 attribut)
{
    VDPSprite *sprite = &vdpSpriteCache[index];

    sprite->y = y + 0x80;
    sprite->size = size;
    sprite->attribut = attribut;
    sprite->x = x + 0x80;
}

void VDP_setSpritePosition(u16 index, s16 x, s16 y)
{
    VDPSprite *sprite = &vdpSpriteCache[index];

    sprite->y = y + 0x80;
    sprite->x = x + 0x80;
}

void VDP_setSpriteSize(u16 index, u8 size)
{
    vdpSpriteCache[index].size = size;
}

void VDP_setSpriteAttribut(u16 index, u16 attribut)
{
    vdpSpriteCache[index].attribut = attribut;
}

void VDP_setSpriteLink(u16 index, u8 link)
{
    vdpSpriteCache[index].link = link;
}

VDPSprite* VDP_linkSprites(u16 index, u16 num)
{
    VDPSprite* spr;
    u8 ind;
    u16 remaining;

    spr = &vdpSpriteCache[index];
    ind = index;
    remaining = num;

    while(remaining--)
    {
        ind++;
        spr->link = ind;
        spr++;
    }

    return spr;
}

void VDP_updateSprites(u16 num, TransferMethod tm)
{
    // 0 is a special value to hide all sprite
    if (num == 0)
    {
        VDP_clearSprites();
        num = 1;
    }

    DMA_transfer(tm, DMA_VRAM, vdpSpriteCache, VDP_SPRITE_TABLE, (sizeof(VDPSprite) * num) / 2, 2);
}

void VDP_setSpritePriority(u16 index, bool priority)
{
    vdpSpriteCache[index].priority = priority;
}

bool VDP_getSpritePriority(u16 index)
{
    return vdpSpriteCache[index].priority == 1;
}

void VDP_setSpritePalette(u16 index, u16 palette)
{
    vdpSpriteCache[index].palette = palette;
}

u16 VDP_getSpritePalette(u16 index)
{
    return vdpSpriteCache[index].palette;
}

void VDP_setSpriteFlip(u16 index, bool flipH, bool flipV)
{
    VDPSprite *sprite = &vdpSpriteCache[index];
    sprite->flipH = flipH;
    sprite->flipV = flipV;
}

void VDP_setSpriteFlipH(u16 index, bool flipH)
{
    vdpSpriteCache[index].flipH = flipH;
}

void VDP_setSpriteFlipV(u16 index, bool flipV)
{
    vdpSpriteCache[index].flipH = flipV;
}

bool VDP_getSpriteFlipH(u16 index)
{
    return vdpSpriteCache[index].flipH != 0;
}

bool VDP_getSpriteFlipV(u16 index)
{
    return vdpSpriteCache[index].flipV != 0;
}

void VDP_setSpriteTile(u16 index, u16 tile)
{
    vdpSpriteCache[index].tile = tile;
}

u16 VDP_getSpriteTile(u16 index)
{
    return vdpSpriteCache[index].tile;
}

void logVDPSprite(u16 index)
{
    char str[64];
    char strtmp[16];

    VDPSprite* vdpSprite = &vdpSpriteCache[index];

    strcpy(str, "  VDP Sprite #");

    intToStr(index, strtmp, 1);
    strcat(str, strtmp);

    strcat(str, " X=");
    intToStr(vdpSprite->x, strtmp, 1);
    strcat(str, strtmp);
    strcat(str, " Y=");
    intToStr(vdpSprite->y, strtmp, 1);
    strcat(str, strtmp);
    strcat(str, " size=");
    intToHex(vdpSprite->size, strtmp, 2);
    strcat(str, strtmp);
    strcat(str, " attr=");
    intToHex(vdpSprite->attribut, strtmp, 4);
    strcat(str, strtmp);
    strcat(str, " link=");
    intToStr(vdpSprite->link, strtmp, 1);
    strcat(str, strtmp);

    KLog(str);
}
