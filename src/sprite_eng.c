#include "config.h"
#include "types.h"

#include "sprite_eng.h"

#include "sys.h"
#include "vdp.h"
#include "vdp_spr.h"
#include "dma.h"
#include "memory.h"
#include "vram.h"
#include "tools.h"

#include "kdebug.h"
#include "string.h"
#include "timer.h"


//#define SPR_DEBUG
//#define SPR_PROFIL


// internals
#define VISIBILITY_ON                       0xFFFF
#define VISIBILITY_OFF                      0x0000

#define NEED_ST_ATTR_UPDATE                 0x0001
#define NEED_ST_POS_UPDATE                  0x0002
#define NEED_ST_ALL_UPDATE                  (NEED_ST_ATTR_UPDATE | NEED_ST_POS_UPDATE)

#define NEED_ST_VISIBILITY_UPDATE           0x0010
#define NEED_VISIBILITY_UPDATE              0x0020
#define NEED_FRAME_UPDATE                   0x0040
#define NEED_TILES_UPLOAD                   0x0080

#define NEED_UPDATE                         0x00FF


// shared from vdp_spr.c unit
extern VDPSprite *lastAllocatedVDPSprite;


// forward
static Sprite* allocateSprite();
//static Sprite** allocateSprites(Sprite** sprites, u16 num);
static void releaseSprite(Sprite* sprite);
//static void releaseSprites(Sprite** sprites, u16 num);

static void setVDPSpriteIndex(Sprite *sprite, u16 ind, u16 num, VDPSprite *last);
static u16 updateVisibility(Sprite *sprite);
static u16 setVisibility(Sprite *sprite, u16 visibility);
static u16 updateFrame(Sprite *sprite);

static void updateSpriteTableVisibility(Sprite *sprite);
static void updateSpriteTableAll(Sprite *sprite);
static void updateSpriteTablePos(Sprite *sprite);
static void updateSpriteTableAttr(Sprite *sprite);
static void loadTiles(Sprite *sprite);

// starter VDP sprite - never visible (used for sprite sorting)
static VDPSprite *starter;

// allocated bank of sprites for the Sprite Engine
static Sprite *spritesBank = NULL;
// maximum number of sprite (number of allocated sprites for the Sprite Engine)
static u16 spritesBankSize;

// used for sprite allocation
static Sprite **allocStack;
// point on top of the allocation stack (first available sprite)
static Sprite **free;

// active sprites list, we use an array of pointer to allow sorting
// [0..(visibleSpriteNum-1)] = visible sprites
// [visibleSpriteNum..(spriteNum-1)] = not visible sprites
Sprite **activeSprites;
// number of active sprite
u16 spriteNum;

static u8 *unpackBuffer;
static u8 *unpackNext;
static VRAMRegion vram;


#ifdef SPR_PROFIL

#define PROFIL_ALLOCATE_SPRITE          0
#define PROFIL_RELEASE_SPRITE           1
#define PROFIL_ADD_SPRITE               2
#define PROFIL_REMOVE_SPRITE            3
#define PROFIL_SET_DEF                  4
#define PROFIL_SET_ATTRIBUTE            5
#define PROFIL_SET_ANIM_FRAME           6
#define PROFIL_SET_VRAM_OR_SPRIND       7
#define PROFIL_SET_VISIBILITY           8
#define PROFIL_CLEAR                    9
#define PROFIL_UPDATE                   10
#define PROFIL_UPDATE_VISIBILITY        11
#define PROFIL_SET_VIS_INT              12
#define PROFIL_UPDATE_FRAME             13
#define PROFIL_UPDATE_VDPSPRIND         14
#define PROFIL_UPDATE_VISTABLE          15
#define PROFIL_UPDATE_SPRITE_TABLE      16
#define PROFIL_LOADTILES                17

static u32 profil_time[20];
#endif


void SPR_init(u16 maxSprite, u16 cacheSize, u16 unpackBufferSize)
{
    u16 adjMax;
    u16 index;
    u16 size;

    // already initialized --> end it first
    if (SPR_isInitialized()) SPR_end();

    // don't allow more than 128 sprites max
    adjMax = maxSprite?min(128, maxSprite):40;

    // alloc sprites bank
    spritesBank = MEM_alloc(adjMax * sizeof(Sprite));
    spritesBankSize = adjMax;
    // allocation stack
    allocStack = MEM_alloc(adjMax * sizeof(Sprite*));
    // alloc active sprite array memory
    activeSprites = MEM_alloc(adjMax * sizeof(Sprite*));
    // alloc sprite tile unpack buffer
    unpackBuffer = MEM_alloc(((unpackBufferSize?unpackBufferSize:256) * 32) + 1024);

    size = cacheSize?cacheSize:384;
    // get start tile index for sprite cache (reserve VRAM area just before system font)
    index = TILE_FONTINDEX - size;

    // and create a VRAM region for sprite tile allocation
    VRAM_createRegion(&vram, index, size);

#ifdef SPR_DEBUG
    KLog("Sprite engine initialized");
    KLog_U1("  maxSprite: ", adjMax);
    KLog_U1("  unpack buffer size:", (unpackBufferSize?unpackBufferSize:256) * 32);
    KLog_U2_("  VRAM region: [", index, " - ", index + (size - 1), "]");
#endif // SPR_DEBUG

    // reset
    SPR_reset();
}

void SPR_end()
{
    if (SPR_isInitialized())
    {
        // reset and clear VDP sprite
        VDP_resetSprites();
        VDP_updateSprites(1, TRUE);

        // release memory
        MEM_free(spritesBank);
        spritesBank = NULL;
        spritesBankSize = 0;
        MEM_free(allocStack);
        allocStack = NULL;
        MEM_free(activeSprites);
        activeSprites = NULL;
        MEM_free(unpackBuffer);
        unpackBuffer = NULL;

        VRAM_releaseRegion(&vram);
    }
}

u16 SPR_isInitialized()
{
    return (spritesBank != NULL);
}

void SPR_reset()
{
    u16 i;

    // release and clear sprites data
    memset(spritesBank, 0, sizeof(Sprite) * spritesBankSize);

    // reset allocation stack
    for(i = 0; i < spritesBankSize; i++)
        allocStack[i] = &spritesBank[i];
    // init free position
    free = &allocStack[spritesBankSize];

    // reset current number of active sprite
    spriteNum = 0;
    // reset unpack pointer
    unpackNext = unpackBuffer;

    // clear VRAM region
    VRAM_clearRegion(&vram);
    // reset VDP sprite (allocation and display)
    VDP_resetSprites();

    // we reserve sprite 0 for sorting (cannot be used for display)
    starter = &vdpSpriteCache[VDP_allocateSprites(1)];
    // hide it
    starter->y = 0;

#ifdef SPR_DEBUG
    KLog("Sprite engine reset");
    KLog_U1("  VRAM region free: ", VRAM_getFree(&vram));
    KLog_U1("  Available VDP sprites: ", VDP_getAvailableSprites());
#endif // SPR_DEBUG
}


static Sprite* allocateSprite()
{
    // enough sprite remaining ?
    if (free == allocStack)
    {
#if (LIB_DEBUG != 0)
        KLog("Couldn't allocate Sprite, no more available sprite(s) !");
#endif

        return NULL;
    }

#ifdef SPR_DEBUG
    KLog_U1("allocateSprite: allocating sprite at pos ", free[-1] - spritesBank);
#endif // SPR_DEBUG

    return *--free;
}

static void releaseSprite(Sprite* sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

#ifdef SPR_DEBUG
    KLog_U1("releaseSprites: releasing sprite at pos ", sprite - spritesBank);
#endif // SPR_DEBUG

    // release sprite
    *free++ = sprite;

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_RELEASE_SPRITE] += prof;
#endif // SPR_PROFIL
}

Sprite* SPR_addSpriteEx(const SpriteDefinition *spriteDef, s16 x, s16 y, u16 attribut, u16 spriteIndex, u16 flags)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    s16 ind;
    u16 numVDPSprite;
    Sprite* sprite;
    VDPSprite* lastVDPSprite;

    // allocate new sprite
    sprite = allocateSprite();

    // can't allocate --> return NULL
    if (!sprite)
    {
#if (LIB_DEBUG != 0)
        KDebug_Alert("SPR_addSpriteEx failed: max sprite number reached !");
#endif

        return NULL;
    }

    sprite->status = flags & SPR_FLAGS_MASK;

    // add the new sprite
    ind = spriteNum;
    sprite->index = ind;
    activeSprites[ind] = sprite;
    spriteNum = ind + 1;

#ifdef SPR_DEBUG
    KLog_U2("SPR_addSpriteEx: added sprite #", ind, " at position ", sprite - spritesBank);
    KLog_U1("  new sprite number = ", spriteNum);
#endif // SPR_DEBUG

    // initialized with specified flags
    sprite->visibility = 0;
    sprite->definition = spriteDef;
    sprite->frame = NULL;
    sprite->animInd = -1;
    sprite->frameInd = -1;
    sprite->seqInd = -1;
    sprite->x = x + 0x80;
    sprite->y = y + 0x80;
    sprite->frameNumSprite = 0;

    numVDPSprite = spriteDef->maxNumSprite;

    // auto VDP sprite alloc enabled ?
    if (flags & SPR_FLAG_AUTO_SPRITE_ALLOC)
    {
        // allocate VDP sprite
        ind = VDP_allocateSprites(numVDPSprite);
        // not enough --> release sprite and return NULL
        if (ind == -1)
        {
            releaseSprite(sprite);
            return NULL;
        }

#ifdef SPR_DEBUG
        KLog_U3("  allocated ", numVDPSprite, " VDP sprite(s) at ", ind, ", remaining VDP sprite = ", VDP_getAvailableSprites());
#endif // SPR_DEBUG

        // get last VDP sprite pointer
        lastVDPSprite = lastAllocatedVDPSprite;
    }
    else
    {
        // use given sprite index
        ind = spriteIndex;
        // need to compute last VDP sprite pointer
        lastVDPSprite = NULL;
    }

    // set the VDP Sprite index for this sprite and do attached operation
    setVDPSpriteIndex(sprite, ind, numVDPSprite, lastVDPSprite);

    // auto VRAM alloc enabled ?
    if (flags & SPR_FLAG_AUTO_VRAM_ALLOC)
    {
        // allocate VRAM
        ind = VRAM_alloc(&vram, spriteDef->maxNumTile);
        // not enough --> release sprite and return NULL
        if (ind == -1)
        {
            // release allocated VDP sprites
            if (flags & SPR_FLAG_AUTO_SPRITE_ALLOC)
                VDP_releaseSprites(sprite->VDPSpriteIndex, numVDPSprite);

            releaseSprite(sprite);
            return NULL;
        }

        // set VRAM index and preserve specific attributs from parameter
        sprite->attribut = ind | (attribut & TILE_ATTR_MASK);

#ifdef SPR_DEBUG
        KLog_U3("  allocated ", spriteDef->maxNumTile, " tiles in VRAM at ", ind, ", remaining VRAM: ", VRAM_getFree(&vram));
#endif // SPR_DEBUG
    }
    // just use the given attribut
    else sprite->attribut = attribut;

    // set anim and frame to 0
    SPR_setAnimAndFrame(sprite, 0, 0);

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_ADD_SPRITE] += prof;
#endif // SPR_PROFIL

    return sprite;
}

Sprite* SPR_addSprite(const SpriteDefinition *spriteDef, s16 x, s16 y, u16 attribut)
{
    return SPR_addSpriteEx(spriteDef, x, y, attribut, 0,
                           SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_SPRITE_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD);
}

void SPR_releaseSprite(Sprite *sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    // release sprite (no error checking here)
    releaseSprite(sprite);

#ifdef SPR_DEBUG
    KLog_U2("SPR_releaseSprite: releasing sprite #", sprite->index, " at position ", sprite - spritesBank);
#endif // SPR_DEBUG

    // get index of sprite in active list
    u16 index = sprite->index;
    // get index of last sprite
    u16 lastSpriteIndex = spriteNum - 1;

    // not the last remaining sprite ?
    if (lastSpriteIndex)
    {
        // get last sprite
        Sprite* lastSprite = activeSprites[lastSpriteIndex];
        // fix link with previous sprite
        activeSprites[lastSpriteIndex - 1]->lastVDPSprite->link = lastSprite->lastVDPSprite->link;

#ifdef SPR_DEBUG
        KLog_U2_("  sprite[", lastSpriteIndex, "] moved to sprite[", index, "]");
        KLog_U2("  sprite[", lastSpriteIndex - 1, "] linked to ", lastSprite->lastVDPSprite->link);
#endif // SPR_DEBUG

        // we need to move last sprite ?
        if (lastSpriteIndex != index)
        {
            // replace removed sprite by the last sprite
            activeSprites[index] = lastSprite;
            // fix its index
            lastSprite->index = index;
            // and set link with sprite following the deleted one
            lastSprite->lastVDPSprite->link = sprite->lastVDPSprite->link;

            // fix link with previous
            if (index) activeSprites[index - 1]->lastVDPSprite->link = lastSprite->VDPSpriteIndex;
            else starter->link = lastSprite->VDPSpriteIndex;

#ifdef SPR_DEBUG
            KLog_U2("  sprite[", index, "] linked to ", sprite->lastVDPSprite->link);
            if (index) KLog_U2("  sprite[", index - 1, "] linked to ", lastSprite->VDPSpriteIndex);
            else KLog_U1("  starter linked to ", lastSprite->VDPSpriteIndex);
#endif // SPR_DEBUG
        }
    }
    // no more visible sprite
    else starter->link = 0;

    // decrement number of sprite
    spriteNum = lastSpriteIndex;

    u16 status = sprite->status;

    // auto VDP sprite alloc enabled --> release VDP sprite(s)
    if (status & SPR_FLAG_AUTO_SPRITE_ALLOC)
    {
        VDP_releaseSprites(sprite->VDPSpriteIndex, sprite->definition->maxNumSprite);

#ifdef SPR_DEBUG
        KLog_U3("  released ", sprite->definition->maxNumSprite, " VDP sprite(s) at ", sprite->VDPSpriteIndex, ", remaining VDP sprite = ", VDP_getAvailableSprites());
#endif // SPR_DEBUG
    }
    // auto VRAM alloc enabled --> release VRAM area allocated for this sprite
    if (status & SPR_FLAG_AUTO_VRAM_ALLOC)
    {
        VRAM_free(&vram, sprite->attribut & TILE_INDEX_MASK);

#ifdef SPR_DEBUG
        KLog_U3("  released ", sprite->definition->maxNumTile, " tiles in VRAM at ", sprite->attribut & TILE_INDEX_MASK, ", remaining VRAM: ", VRAM_getFree(&vram));
#endif // SPR_DEBUG
    }

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_REMOVE_SPRITE] += prof;
#endif // SPR_PROFIL
}

u16 SPR_getNumActiveSprite()
{
    return spriteNum;
}


u16 SPR_setDefinition(Sprite *sprite, const SpriteDefinition *spriteDef)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

#ifdef SPR_DEBUG
    KLog_U1("SPR_setDefinition: #", sprite->index);
#endif // SPR_DEBUG

    // nothing to do...
    if (sprite->definition == spriteDef) return TRUE;

    u16 status = sprite->status;

    // auto VDP sprite alloc enabled --> realloc VDP sprite(s)
    if ((status & SPR_FLAG_AUTO_SPRITE_ALLOC) && (sprite->definition->maxNumSprite != spriteDef->maxNumSprite))
    {
        // we release previous allocated VDP sprite(s)
        VDP_releaseSprites(sprite->VDPSpriteIndex, sprite->definition->maxNumSprite);

#ifdef SPR_DEBUG
        KLog_U3("  released ", sprite->definition->maxNumSprite, " VDP sprite(s) at ", sprite->VDPSpriteIndex, ", remaining VDP sprite = ", VDP_getAvailableSprites());
#endif // SPR_DEBUG

        const s16 num = spriteDef->maxNumSprite;
        // then we allocate the VDP sprite(s) for the new definition
        const s16 ind = VDP_allocateSprites(num);
        // not enough --> return error
        if (ind == -1) return FALSE;

        // set the VDP Sprite index for this sprite and do attached operation
        setVDPSpriteIndex(sprite, ind, num, lastAllocatedVDPSprite);

#ifdef SPR_DEBUG
        KLog_U3("  allocated ", num, " VDP sprite(s) at ", ind, ", remaining VDP sprite = ", VDP_getAvailableSprites());
#endif // SPR_DEBUG

    }
    // auto VRAM alloc enabled --> realloc VRAM tile area
    if ((status & SPR_FLAG_AUTO_VRAM_ALLOC) && (sprite->definition->maxNumTile != spriteDef->maxNumTile))
    {
        // we release previous allocated VRAM
        VRAM_free(&vram, sprite->attribut & TILE_INDEX_MASK);

#ifdef SPR_DEBUG
        KLog_U3("  released ", sprite->definition->maxNumTile, " tiles in VRAM at ", sprite->attribut & TILE_INDEX_MASK, ", remaining VRAM: ", VRAM_getFree(&vram));
#endif // SPR_DEBUG

        // allocate VRAM
        const s16 ind = VRAM_alloc(&vram, spriteDef->maxNumTile);
        // not enough --> return error
        if (ind == -1) return FALSE;

        // preserve the attributes and just overwrite VRAM index (set frame will rebuild everything)
        sprite->attribut = (sprite->attribut & TILE_ATTR_MASK) | ind;

#ifdef SPR_DEBUG
        KLog_U3("  allocated ", spriteDef->maxNumTile, " tiles in VRAM at ", ind, ", remaining VRAM: ", VRAM_getFree(&vram));
#endif // SPR_DEBUG
    }

    sprite->definition = spriteDef;
    sprite->animInd = -1;
    sprite->frameInd = -1;
    sprite->seqInd = -1;
    sprite->frame = NULL;

    // set anim and frame to 0
    SPR_setAnimAndFrame(sprite, 0, 0);

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_DEF] += prof;
#endif // SPR_PROFIL

    return TRUE;
}

void SPR_setPosition(Sprite *sprite, s16 x, s16 y)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    const s16 newx = x + 0x80;
    const s16 newy = y + 0x80;

#ifdef SPR_DEBUG
    KLog_U3("SPR_setPosition: #", sprite->index, "  X=", newx, " Y=", newy);
#endif // SPR_DEBUG

    if ((sprite->x != newx) || (sprite->y != newy))
    {
        u16 status = sprite->status;

        sprite->x = newx;
        sprite->y = newy;

        // need to recompute visibility if auto visibility is enabled
        if (status & SPR_FLAG_AUTO_VISIBILITY)
            status |= NEED_VISIBILITY_UPDATE;

        sprite->status = status | NEED_ST_POS_UPDATE;
    }

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_ATTRIBUTE] += prof;
#endif // SPR_PROFIL
}

void SPR_setHFlip(Sprite *sprite, u16 value)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    const u16 oldAttribut = sprite->attribut;

    if (oldAttribut & TILE_ATTR_HFLIP_MASK)
    {
        // H flip removed
        if (!value)
        {
            u16 status = sprite->status;
            sprite->attribut = oldAttribut & (~TILE_ATTR_HFLIP_MASK);

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setHFlip: #", sprite->index, " removed HFlip");
#endif // SPR_DEBUG

            // need to recompute visibility if auto visibility is enabled
            if (status & SPR_FLAG_AUTO_VISIBILITY)
                status |= NEED_VISIBILITY_UPDATE;

            // need to also recompute complete VDP sprite table
            sprite->status = status | NEED_ST_ALL_UPDATE;
        }
    }
    else
    {
        // H flip set
        if (value)
        {
            u16 status = sprite->status;
            sprite->attribut = oldAttribut | TILE_ATTR_HFLIP_MASK;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setHFlip: #", sprite->index, " added HFlip");
#endif // SPR_DEBUG

            // need to recompute visibility if auto visibility is enabled
            if (status & SPR_FLAG_AUTO_VISIBILITY)
                status |= NEED_VISIBILITY_UPDATE;

            // need to also recompute complete VDP sprite table
            sprite->status = status | NEED_ST_ALL_UPDATE;
        }
    }

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_ATTRIBUTE] += prof;
#endif // SPR_PROFIL
}

void SPR_setVFlip(Sprite *sprite, u16 value)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    const u16 oldAttribut = sprite->attribut;

    if (oldAttribut & TILE_ATTR_VFLIP_MASK)
    {
        // V flip removed
        if (!value)
        {
            u16 status = sprite->status;
            sprite->attribut = oldAttribut & (~TILE_ATTR_VFLIP_MASK);

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setVFlip: #", sprite->index, " removed VFlip");
#endif // SPR_DEBUG

            // need to recompute visibility if auto visibility is enabled
            if (status & SPR_FLAG_AUTO_VISIBILITY)
                status |= NEED_VISIBILITY_UPDATE;

            // need to also recompute complete VDP sprite table
            sprite->status = status | NEED_ST_ALL_UPDATE;
        }
    }
    else
    {
        // V flip set
        if (value)
        {
            u16 status = sprite->status;
            sprite->attribut = oldAttribut | TILE_ATTR_VFLIP_MASK;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setVFlip: #", sprite->index, " added VFlip");
#endif // SPR_DEBUG

            // need to recompute visibility if auto visibility is enabled
            if (status & SPR_FLAG_AUTO_VISIBILITY)
                status |= NEED_VISIBILITY_UPDATE;

            // need to also recompute complete VDP sprite table
            sprite->status = status | NEED_ST_ALL_UPDATE;
        }
    }

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_ATTRIBUTE] += prof;
#endif // SPR_PROFIL
}

void SPR_setPriorityAttribut(Sprite *sprite, u16 value)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    const u16 oldAttribut = sprite->attribut;

    if (oldAttribut & TILE_ATTR_PRIORITY_MASK)
    {
        // priority removed
        if (!value)
        {
            sprite->attribut = oldAttribut & (~TILE_ATTR_PRIORITY_MASK);
            sprite->status |= NEED_ST_ATTR_UPDATE;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setPriorityAttribut: #", sprite->index, " removed priority");
#endif // SPR_DEBUG
        }
    }
    else
    {
        // priority set
        if (value)
        {
            sprite->attribut = oldAttribut | TILE_ATTR_PRIORITY_MASK;
            sprite->status |= NEED_ST_ATTR_UPDATE;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setPriorityAttribut: #", sprite->index, " added priority");
#endif // SPR_DEBUG
        }
    }

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_ATTRIBUTE] += prof;
#endif // SPR_PROFIL
}

void SPR_setPalette(Sprite *sprite, u16 value)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    const u16 oldAttribut = sprite->attribut;
    const u16 newAttribut = (oldAttribut & (~TILE_ATTR_PALETTE_MASK)) | (value << TILE_ATTR_PALETTE_SFT);

    if (oldAttribut != newAttribut)
    {
        sprite->attribut = newAttribut;
        // need to update VDP sprite attribut field only
        sprite->status |= NEED_ST_ATTR_UPDATE;

#ifdef SPR_DEBUG
        KLog_U2("SPR_setPalette: #", sprite->index, " palette=", value);
#endif // SPR_DEBUG
    }

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_ATTRIBUTE] += prof;
#endif // SPR_PROFIL
}

void SPR_setAnimAndFrame(Sprite *sprite, s16 anim, s16 frame)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    if ((sprite->animInd != anim) || (sprite->seqInd != frame))
    {
        Animation *animation = sprite->definition->animations[anim];
        const u16 frameInd = animation->sequence[frame];

        sprite->animInd = anim;
        sprite->seqInd = frame;
        sprite->animation = animation;
        sprite->frameInd = frameInd;
        sprite->frame = animation->frames[frameInd];

#ifdef SPR_DEBUG
        KLog_U4("SPR_setAnimAndFrame: #", sprite->index, " anim=", anim, " frame=", frame, " adj frame=", frameInd);
#endif // SPR_DEBUG

        sprite->status |= NEED_FRAME_UPDATE;
    }

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_ANIM_FRAME] += prof;
#endif // SPR_PROFIL
}

void SPR_setAnim(Sprite *sprite, s16 anim)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    if (sprite->animInd != anim)
    {
        Animation *animation = sprite->definition->animations[anim];
        // first frame by default
        const u16 frameInd = animation->sequence[0];

        sprite->animInd = anim;
        sprite->seqInd = 0;
        sprite->animation = animation;
        sprite->frameInd = frameInd;
        sprite->frame = animation->frames[frameInd];

#ifdef SPR_DEBUG
        KLog_U3("SPR_setAnim: #", sprite->index, " anim=", anim, " frame=0 adj frame=", frameInd);
#endif // SPR_DEBUG

        sprite->status |= NEED_FRAME_UPDATE;
    }

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_ANIM_FRAME] += prof;
#endif // SPR_PROFIL
}

void SPR_setFrame(Sprite *sprite, s16 frame)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    if (sprite->seqInd != frame)
    {
        const Animation *animation = sprite->animation;
        const u16 frameInd = animation->sequence[frame];

        sprite->seqInd = frame;

        if (sprite->frameInd != frameInd)
        {
            sprite->frameInd = frameInd;
            sprite->frame = animation->frames[frameInd];

#ifdef SPR_DEBUG
            KLog_U3("SPR_setFrame: #", sprite->index, "  frame=", frame, " adj frame=", frameInd);
#endif // SPR_DEBUG

            sprite->status |= NEED_FRAME_UPDATE;
        }
    }

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_ANIM_FRAME] += prof;
#endif // SPR_PROFIL
}

void SPR_nextFrame(Sprite *sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    const Animation *anim = sprite->animation;
    u16 seqInd = sprite->seqInd + 1;

    if (seqInd >= anim->length)
        seqInd = anim->loop;

    // set new frame
    SPR_setFrame(sprite, seqInd);

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_ANIM_FRAME] += prof;
#endif // SPR_PROFIL
}

u16 SPR_setVRAMTileIndex(Sprite *sprite, s16 value)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    s16 newInd;
    u16 status = sprite->status;
    u16 oldAttribut = sprite->attribut;

    if (status & SPR_FLAG_AUTO_VRAM_ALLOC)
    {
        // pass to manual allocation
        if (value != -1)
        {
            // remove auto vram alloc flag
            status &= ~SPR_FLAG_AUTO_VRAM_ALLOC;
            // release allocated VRAM first
            VRAM_free(&vram, oldAttribut & TILE_INDEX_MASK);
            // set fixed VRAM index
            newInd = value;

#ifdef SPR_DEBUG
            KLog_U2("SPR_setVRAMTileIndex: #", sprite->index, " passed to manual allocation, VRAM index =", value);
            KLog_U3("  released ", sprite->definition->maxNumTile, " tiles in VRAM at ", sprite->attribut & TILE_INDEX_MASK, ", remaining VRAM: ", VRAM_getFree(&vram));
#endif // SPR_DEBUG
        }
        // nothing to do --> just return TRUE
        else
        {
#ifdef SPR_PROFIL
            prof = getSubTick() - prof;
            // rollback correction
            if (prof < 0) prof = 100;
            profil_time[PROFIL_SET_VRAM_OR_SPRIND] += prof;
#endif // SPR_PROFIL

            return TRUE;
        }
    }
    else
    {
        // pass to auto allocation
        if (value == -1)
        {
            // set auto vram alloc flag
            status |= SPR_FLAG_AUTO_VRAM_ALLOC;
            // allocate VRAM
            newInd = VRAM_alloc(&vram, sprite->definition->maxNumTile);

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setVRAMTileIndex: #", sprite->index, " passed to auto allocation");
            KLog_U3("  allocated ", sprite->definition->maxNumTile, " tiles in VRAM at ", newInd, ", remaining VRAM: ", VRAM_getFree(&vram));
#endif // SPR_DEBUG

            // can't allocate ?
            if (newInd == -1)
            {
                // save status and return FALSE
                sprite->status = status;

#ifdef SPR_PROFIL
                prof = getSubTick() - prof;
                // rollback correction
                if (prof < 0) prof = 100;
                profil_time[PROFIL_SET_VRAM_OR_SPRIND] += prof;
#endif // SPR_PROFIL

                return FALSE;
            }
        }
        // just use the new value for index
        else newInd = value;
    }

    // VRAM tile index changed ?
    if ((oldAttribut & TILE_INDEX_MASK) != newInd)
    {
        sprite->attribut = (oldAttribut & TILE_ATTR_MASK) | newInd;
        // need to update 'attribut' field of sprite table
        status |= NEED_ST_ATTR_UPDATE;
    }

    // save status
    sprite->status = status;

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_VRAM_OR_SPRIND] += prof;
#endif // SPR_PROFIL

    return TRUE;
}

u16 SPR_setSpriteTableIndex(Sprite *sprite, s16 value)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    s16 newInd;
    u16 status = sprite->status;
    u16 num = sprite->definition->maxNumSprite;
    VDPSprite* lastVDPSprite = NULL;

    if (status & SPR_FLAG_AUTO_SPRITE_ALLOC)
    {
        // pass to manual allocation
        if (value != -1)
        {
            // remove auto VDP sprite alloc flag
            status &= ~SPR_FLAG_AUTO_SPRITE_ALLOC;
            // release allocated VDP sprite
            VDP_releaseSprites(sprite->VDPSpriteIndex, num);
            // set manually the VDP sprite index
            newInd = value;

#ifdef SPR_DEBUG
            KLog_U2("SPR_setSpriteTableIndex: #", sprite->index, " passed to manual allocation, VDP Sprite index =", value);
            KLog_U3("  released ", num, " VDP sprite(s) at ", sprite->VDPSpriteIndex, ", remaining VDP sprite = ", VDP_getAvailableSprites());
#endif // SPR_DEBUG
        }
        // nothing to do --> return TRUE
        else
        {
#ifdef SPR_PROFIL
            prof = getSubTick() - prof;
            // rollback correction
            if (prof < 0) prof = 100;
            profil_time[PROFIL_SET_VRAM_OR_SPRIND] += prof;
#endif // SPR_PROFIL

            return TRUE;
        }
    }
    else
    {
        // pass to auto allocation
        if (value == -1)
        {
            // set auto VDP sprite alloc flag
            status |= SPR_FLAG_AUTO_SPRITE_ALLOC;
            // allocate VDP sprite
            newInd = VDP_allocateSprites(num);

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setSpriteTableIndex: #", sprite->index, " passed to auto allocation");
            KLog_U3("  allocated ", num, " VDP sprite(s) at ", newInd, ", remaining VDP sprite = ", VDP_getAvailableSprites());
#endif // SPR_DEBUG

            // can't allocate ?
            if (newInd == -1)
            {
                // save status and return FALSE
                sprite->status = status;

#ifdef SPR_PROFIL
                prof = getSubTick() - prof;
                // rollback correction
                if (prof < 0) prof = 100;
                profil_time[PROFIL_SET_VRAM_OR_SPRIND] += prof;
#endif // SPR_PROFIL

                return FALSE;
            }

            // get last allocated VDP sprite pointer
            lastVDPSprite = lastAllocatedVDPSprite;
        }
        // just use the new value for index
        else newInd = value;
    }

    // VDP sprite index changed ?
    if (sprite->VDPSpriteIndex != newInd)
    {
        // set the VDP Sprite index for this sprite and do attached operation
        setVDPSpriteIndex(sprite, newInd, num, lastVDPSprite);
        // need to update complete sprite table infos
        status |= NEED_ST_VISIBILITY_UPDATE | NEED_ST_ALL_UPDATE;
    }

    // save status
    sprite->status = status;

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_VRAM_OR_SPRIND] += prof;
#endif // SPR_PROFIL

    return TRUE;
}

void SPR_setAutoTileUpload(Sprite *sprite, u16 value)
{
    if (value) sprite->status |= SPR_FLAG_AUTO_TILE_UPLOAD;
    else sprite->status &= ~SPR_FLAG_AUTO_TILE_UPLOAD;
}

void SPR_setVisibility(Sprite *sprite, SpriteVisibility value)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    u16 status = sprite->status;

    if (status & SPR_FLAG_AUTO_VISIBILITY)
    {
        switch(value)
        {
            case VISIBLE:
                status &= ~(SPR_FLAG_AUTO_VISIBILITY | NEED_VISIBILITY_UPDATE);
                status |= setVisibility(sprite, VISIBILITY_ON);
                break;

            case HIDDEN:
                status &= ~(SPR_FLAG_AUTO_VISIBILITY | NEED_VISIBILITY_UPDATE);
                status |= setVisibility(sprite, VISIBILITY_OFF);
                break;

            case AUTO_FAST:
                status |= SPR_FLAG_FAST_AUTO_VISIBILITY;
                break;

            case AUTO_SLOW:
                // passed from fast to slow visibility compute method
                if (status & SPR_FLAG_FAST_AUTO_VISIBILITY)
                {
                    status &= ~SPR_FLAG_FAST_AUTO_VISIBILITY;
                    status |= NEED_VISIBILITY_UPDATE;
                }
                break;
        }
    }
    else
    {
        switch(value)
        {
            case VISIBLE:
                status |= setVisibility(sprite, VISIBILITY_ON);
                break;

            case HIDDEN:
                status |= setVisibility(sprite, VISIBILITY_OFF);
                break;

            case AUTO_FAST:
                status |= SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_FAST_AUTO_VISIBILITY | NEED_VISIBILITY_UPDATE;
                break;

            case AUTO_SLOW:
                status |= SPR_FLAG_AUTO_VISIBILITY | NEED_VISIBILITY_UPDATE;
                break;
        }
    }

    // update status
    sprite->status = status;

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SET_VISIBILITY] += prof;
#endif // SPR_PROFIL
}

void SPR_setAlwaysVisible(Sprite *sprite, u16 value)
{
    if (value) SPR_setVisibility(sprite, VISIBLE);
}

void SPR_setNeverVisible(Sprite *sprite, u16 value)
{
    if (value) SPR_setVisibility(sprite, HIDDEN);
}

u16 SPR_computeVisibility(Sprite *sprite)
{
    u16 status = sprite->status;

    // update visibility if needed
    if (status & NEED_VISIBILITY_UPDATE)
        sprite->status = (status & ~NEED_VISIBILITY_UPDATE) | updateVisibility(sprite);

    return (sprite->visibility)?TRUE:FALSE;
}


void SPR_clear()
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    // save starter link
    u8 linkSave = starter->link;

    VDP_clearSprites();
    VDP_updateSprites(1, TRUE);

    // restore starter link
    starter->link = linkSave;

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_CLEAR] += prof;
#endif // SPR_PROFIL
}

void SPR_update()
{
#ifdef SPR_PROFIL
    memset(profil_time, 0, sizeof(profil_time));

    s32 prof = getSubTick();
#endif // SPR_PROFIL

    Sprite **sprites;
    u16 i;

#ifdef SPR_DEBUG
    KLog_U1("----------------- SPR_update:  sprite number = ", spriteNum);
#endif // SPR_DEBUG

    // disable interrupts (we want to avoid DMA queue process when executing this method)
    SYS_disableInts();

#ifdef SPR_DEBUG
    KLog_U1_("  Send sprites to DMA queue: ", highestVDPSpriteIndex + 1, " sprite(s) sent");
#endif // SPR_DEBUG

    // send sprites to VRAM using DMA queue (better to do it before sprite tiles upload to avoid being ignored by DMA queue)
    VDP_updateSprites(highestVDPSpriteIndex + 1, TRUE);

    // iterate over all sprites as some can become visible and others can become hidden
    sprites = activeSprites;
    i = spriteNum;
    while(i--)
    {
        Sprite *sprite = *sprites++;
        u16 timer = sprite->timer;

#ifdef SPR_DEBUG
        char str1[32];
        char str2[8];

        intToHex(sprite->visibility, str2, 4);
        strcpy(str1, " visibility = ");
        strcat(str1, str2);

        KLog_U2_("  processing sprite pos #", sprite->index, " - timer = ", timer, str1);
#endif // SPR_DEBUG

        // handle frame animation
        if (timer)
        {
            // timer elapsed --> next frame
            if (--timer == 0) SPR_nextFrame(sprite);
            // just update remaining timer
            else sprite->timer = timer;
        }

        u16 status = sprite->status;

        // trivial optimization
        if (status & NEED_UPDATE)
        {
            // ! order is important !
            if (status & NEED_FRAME_UPDATE)
                status |= updateFrame(sprite);
            if (status & NEED_VISIBILITY_UPDATE)
                status |= updateVisibility(sprite);
            if (status & NEED_ST_VISIBILITY_UPDATE)
                updateSpriteTableVisibility(sprite);

            // general processes done
            status &= ~(NEED_FRAME_UPDATE | NEED_VISIBILITY_UPDATE | NEED_ST_VISIBILITY_UPDATE);

            // only if sprite is visible
            if (sprite->visibility)
            {
                if (status & NEED_TILES_UPLOAD)
                    loadTiles(sprite);

                if (status & NEED_ST_POS_UPDATE)
                {
                    // not only position to update --> update whole table
                    if (status & NEED_ST_ATTR_UPDATE)
                        updateSpriteTableAll(sprite);
                    else
                        updateSpriteTablePos(sprite);
                }
                else if (status & NEED_ST_ATTR_UPDATE)
                    updateSpriteTableAttr(sprite);

                // tiles upload and sprite table done
                status &= ~(NEED_TILES_UPLOAD | NEED_ST_ALL_UPDATE);
            }

            // processes done !
            sprite->status = status;
        }
    }

    // reset unpack buffer address
    unpackNext = unpackBuffer;

    // re-enable interrupts
    SYS_enableInts();

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_UPDATE] += prof;

    KLog("SPR Engine profiling ------------------------------------------------------");
    KLog_U2x(4, "Alloc=", profil_time[PROFIL_ALLOCATE_SPRITE], " Release=", profil_time[PROFIL_RELEASE_SPRITE]);
    KLog_U2x(4, "Add=", profil_time[PROFIL_ADD_SPRITE], " Remove=", profil_time[PROFIL_REMOVE_SPRITE]);
    KLog_U2x(4, "Set Def.=", profil_time[PROFIL_SET_DEF], " Set Attr.=", profil_time[PROFIL_SET_ATTRIBUTE]);
    KLog_U2x(4, "Set Anim & Frame=", profil_time[PROFIL_SET_ANIM_FRAME], " Set VRAM & Sprite Ind=", profil_time[PROFIL_SET_VRAM_OR_SPRIND]);
    KLog_U1x(4, "Set Visibility=", profil_time[PROFIL_SET_VISIBILITY], "  Clear=", profil_time[PROFIL_CLEAR]);
    KLog_U1x_(4, " Update all=", profil_time[PROFIL_UPDATE], " -------------");
    KLog_U2x(4, "Update visibility=", profil_time[PROFIL_UPDATE_VISIBILITY], "  Update frame=", profil_time[PROFIL_UPDATE_FRAME],
    KLog_U2x(4, "Update vdp_spr_ind=", profil_time[PROFIL_UPDATE_VDPSPRIND], "  Update vis spr table=", profil_time[PROFIL_UPDATE_VISTABLZ]);
    KLog_U2x(4, "Update Sprite Table=", profil_time[PROFIL_UPDATE_SPRITE_TABLE], " Load Tiles=", profil_time[PROFIL_LOADTILES]);
#endif // SPR_PROFIL
}


static void setVDPSpriteIndex(Sprite *sprite, u16 ind, u16 num, VDPSprite *last)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    u16 activeIndex;

#ifdef SPR_DEBUG
    KLog_U2("setVDPSpriteIndex: sprite #", sprite->index, "  new VDP Sprite index = ", ind);
#endif // SPR_DEBUG

    sprite->VDPSpriteIndex = ind;

    // link with previous sprite
    if ((activeIndex = sprite->index)) activeSprites[activeIndex - 1]->lastVDPSprite->link = ind;
    else starter->link = ind;

    // set last sprite pointer
    if (last) sprite->lastVDPSprite = last;
    else
    {
        // compute it using the slow sprite list parsing
        VDPSprite* vspr = &vdpSpriteCache[ind];
        u16 remaining = num - 1;

        while(remaining--) vspr = &vdpSpriteCache[vspr->link];

        // set last VDP sprite pointer for this sprite
        sprite->lastVDPSprite = vspr;
    }

#ifdef SPR_DEBUG
    KLog_U1("  last VDP sprite = ", sprite->lastVDPSprite - vdpSpriteCache);
#endif // SPR_DEBUG

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_UPDATE_VDPSPRIND] += prof;
#endif // SPR_PROFIL
}

static u16 updateVisibility(Sprite *sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    u16 visibility;
    AnimationFrame *frame = sprite->frame;

    // fast visibility computation ?
    if (sprite->status & SPR_FLAG_FAST_AUTO_VISIBILITY)
    {
        s16 x, y;

        x = sprite->x;
        y = sprite->y;

        // compute global visibility for sprite
        if (((x + frame->w) > 0x80) && (x < (screenWidth + 0x80)) && ((y + frame->h) > 0x80) && (y < (screenHeight + 0x80)))
            visibility = VISIBILITY_ON;
        else
            visibility = VISIBILITY_OFF;

#ifdef SPR_DEBUG
        KLog_S2("  updateVisibility (fast): global x=", x, " global y=", y);
        KLog_S2("    frame w=", frame->w, " h=", frame->h);
#endif // SPR_DEBUG
    }
    else
    {
        s16 xmin, ymin;
        s16 xmax, ymax;
        s16 fw, fh;

        xmin = 0x80 - sprite->x;
        ymin = 0x80 - sprite->y;
        xmax = screenWidth + xmin;
        ymax = screenHeight + ymin;
        frame = sprite->frame;
        fw = frame->w;
        fh = frame->h;

#ifdef SPR_DEBUG
        KLog_S2("  updateVisibility (slow): global x=", sprite->x, " global y=", sprite->y);
        KLog_S2("    frame w=", fw, " h=", fh);
        KLog_S4("    xmin=", xmin, " xmax=", xmax, " ymin=", ymin, " ymax=", ymax);
#endif // SPR_DEBUG

        // full visibility ? --> just use number of total sprite and set full visible
        if ((xmin <= 0) && (xmax >= fw) && (ymin <= 0) && (ymax >= fh)) visibility = VISIBILITY_ON;
        else
        {
            VDPSpriteInf **spritesInf;
            u16 attr;
            u16 i;

            attr = sprite->attribut;
            i = frame->numSprite;
            // start from the last one
            spritesInf = &(frame->vdpSpritesInf[i - 1]);
            visibility = 0;

            while(i--)
            {
                VDPSpriteInf* spriteInf;
                u16 size;
                s16 x, y;
                s16 w, h;

                spriteInf = *spritesInf--;

                size = spriteInf->size;
                w = ((size & 0x0C) << 1) + 8;
                h = ((size & 0x03) << 3) + 8;

                // Y first to respect spriteInf field order
                if (attr & TILE_ATTR_VFLIP_MASK)
                    y = fh - (spriteInf->y + h);
                else
                    y = spriteInf->y;
                if (attr & TILE_ATTR_HFLIP_MASK)
                    x = fw - (spriteInf->x + w);
                else
                    x = spriteInf->x;

    #ifdef SPR_DEBUG
                KLog_S4("    spriteInf x=", spriteInf->x, " y=", spriteInf->y, " w=", w, " h=", h);
                KLog_S3("    attr=", attr, " adjX=", x, " adjY=", y);
    #endif // SPR_DEBUG

                visibility <<= 1;

                // compute visibility
                if (((x + w) > xmin) && (x < xmax) && ((y + h) > ymin) && (y < ymax))
                {
                    visibility |= 1;

    #ifdef SPR_DEBUG
                    KLog("      visible");
    #endif // SPR_DEBUG
                }
                else
                {
    #ifdef SPR_DEBUG
                    KLog("      not visible");
    #endif // SPR_DEBUG
                }
            }
        }
    }

#ifdef SPR_DEBUG
    KLog_U3("    Sprite at [", sprite->x - 0x80, ",", sprite->y - 0x80, "] visibility = ", visibility);
#endif // SPR_DEBUG

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_UPDATE_VISIBILITY] += prof;
#endif // SPR_PROFIL

    // set the new computed visibility
    return setVisibility(sprite, visibility);
}

static u16 setVisibility(Sprite *sprite, u16 newVisibility)
{
    // visibility changed ?
    if (sprite->visibility != newVisibility)
    {
        // set new visibility info
        sprite->visibility = newVisibility;

        // need to recompute the visibility info in sprite table (and so fix others positions)
        return NEED_ST_VISIBILITY_UPDATE | NEED_ST_POS_UPDATE;
    }

    return 0;
}

static u16 updateFrame(Sprite *sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

#ifdef SPR_DEBUG
    KLog_U1("  updateFrame: sprite #", sprite->index);
#endif // SPR_DEBUG

    // init timer for this frame (+1 as we update animation before sending to VDP)
    if ((sprite->timer = sprite->frame->timer))
        sprite->timer++;

    // get status
    u16 status = sprite->status;

    // require tile data upload
    if (status & SPR_FLAG_AUTO_TILE_UPLOAD)
        status |= NEED_TILES_UPLOAD;
    // require visibility update
    if (status & SPR_FLAG_AUTO_VISIBILITY)
        status |= NEED_VISIBILITY_UPDATE;

    u16 numSpriteFrame = sprite->frame->numSprite;

    // number of VDP sprite to use for the current frame changed ?
    if (sprite->frameNumSprite != numSpriteFrame)
    {
        sprite->frameNumSprite = numSpriteFrame;
        // need to udpate sprite table visibility info (and so fix others positions)
        status |= NEED_ST_VISIBILITY_UPDATE | NEED_ST_POS_UPDATE;
    }

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_UPDATE_FRAME] += prof;
#endif // SPR_PROFIL

    // need to update all sprite table
    return status | NEED_ST_ALL_UPDATE;
}

static void updateSpriteTableVisibility(Sprite *sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

#ifdef SPR_DEBUG
    KLog_U2("  updateSpriteTableVisibility: sprite #", sprite->index, "  visibility = ", sprite->visibility);
#endif // SPR_DEBUG

    u16 visibility = sprite->visibility;

    if (visibility)
    {
        // all visible ?
        if (visibility == VISIBILITY_ON)
        {
            VDPSprite *vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];
            u16 num = sprite->frameNumSprite;
            u16 hiddenNum = sprite->definition->maxNumSprite - num;
            u16 i;

            i = num;
            // pass visible sprite
            while(i--) vdpSprite = &vdpSpriteCache[vdpSprite->link];

            if (hiddenNum)
            {
                // hide sprite
                vdpSprite->y = 0;

                // take remaining minus 1
                i = hiddenNum - 1;
                while(i--)
                {
                    // get next sprite
                    vdpSprite = &vdpSpriteCache[vdpSprite->link];
                    // hide it
                    vdpSprite->y = 0;
                }
            }
        }
        // partially visible
        else
        {
            VDPSprite *vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

            // sprite not visible ? --> hide it
            if (!(visibility & 1)) vdpSprite->y = 0;
            // pass to next VDP sprite
            visibility >>= 1;

            s16 i = sprite->definition->maxNumSprite - 1;
            while(visibility && i--)
            {
                // get next sprite
                vdpSprite = &vdpSpriteCache[vdpSprite->link];
                // sprite not visible ? --> hide it
                if (!(visibility & 1)) vdpSprite->y = 0;
                // pass to next VDP sprite
                visibility >>= 1;
            }

            // FIXME: this test may be useless
            if (i > 0)
            {
                // hide remaining sprites
                while(i--)
                {
                    // get next sprite
                    vdpSprite = &vdpSpriteCache[vdpSprite->link];
                    // hide it
                    vdpSprite->y = 0;
                }
            }
        }
    }
    // all hidden
    else
    {
        VDPSprite *vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

        // hide sprite
        vdpSprite->y = 0;

        u16 i = sprite->definition->maxNumSprite - 1;
        while(i--)
        {
            // get next sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
            // hide it
            vdpSprite->y = 0;
        }
    }

#ifdef SPR_DEBUG
    {
        u16 ind = sprite->VDPSpriteIndex;
        u16 i = sprite->definition->maxNumSprite;
        while(i--)
        {
            logVDPSprite(ind);
            // get next sprite
            ind = vdpSpriteCache[ind].link;
        }
    }
#endif // SPR_DEBUG


#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_UPDATE_VISTABLE] += prof;
#endif // SPR_PROFIL
}

static void updateSpriteTableAll(Sprite *sprite)
{
    inline void updateSpriteTableAll_allVisible(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        u16 attr;
        u16 j;

        frame = sprite->frame;
        attr = sprite->attribut;
        j = frame->numSprite;
        spritesInf = frame->vdpSpritesInf;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTableAll_allVisible: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(j--)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;

            // Y first to respect VDP field order
            vdpSprite->y = sprite->y + spriteInf->y;
            vdpSprite->size = spriteInf->size;
            vdpSprite->attribut = attr;
            vdpSprite->x = sprite->x + spriteInf->x;

            // increment tile index in attribut field
            attr += spriteInf->numTile;
            // next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTableAll_allVisibleHFlip(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        u16 attr;
        s16 fw, fh;
        u16 j;

        frame = sprite->frame;
        attr = sprite->attribut;
        j = frame->numSprite;
        spritesInf = frame->vdpSpritesInf;
        fw = frame->w;
        fh = frame->h;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTableAll_allVisibleHFlip: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(j--)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;
            u16 size = spriteInf->size;

            // Y first to respect VDP field order
            vdpSprite->y = sprite->y + spriteInf->y;
            vdpSprite->size = size;
            vdpSprite->attribut = attr;
            vdpSprite->x = sprite->x + (fw - (spriteInf->x + (((size & 0x0C) << 1) + 8)));

            // increment tile index in attribut field
            attr += spriteInf->numTile;
            // next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTableAll_allVisibleVFlip(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        u16 attr;
        s16 fw, fh;
        u16 j;

        frame = sprite->frame;
        attr = sprite->attribut;
        j = frame->numSprite;
        spritesInf = frame->vdpSpritesInf;
        fw = frame->w;
        fh = frame->h;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTableAll_allVisibleVFlip: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(j--)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;
            u16 size = spriteInf->size;

            // Y first to respect VDP field order
            vdpSprite->y = sprite->y + (fh - (spriteInf->y + (((size & 0x03) << 3) + 8)));
            vdpSprite->size = size;
            vdpSprite->attribut = attr;
            vdpSprite->x = sprite->x + spriteInf->x;

            // increment tile index in attribut field
            attr += spriteInf->numTile;
            // next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTableAll_allVisibleHFlipVFlip(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        u16 attr;
        s16 fw, fh;
        u16 j;

        frame = sprite->frame;
        attr = sprite->attribut;
        j = frame->numSprite;
        spritesInf = frame->vdpSpritesInf;
        fw = frame->w;
        fh = frame->h;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTableAll_allVisibleVFlipHFlip: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(j--)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;
            u16 size = spriteInf->size;

            // Y first to respect VDP field order
            vdpSprite->y = sprite->y + (fh - (spriteInf->y + (((size & 0x03) << 3) + 8)));
            vdpSprite->size = size;
            vdpSprite->attribut = attr;
            vdpSprite->x = sprite->x + (fw - (spriteInf->x + (((size & 0x0C) << 1) + 8)));

            // increment tile index in attribut field
            attr += spriteInf->numTile;
            // next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTableAll_partialVisible(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        u16 attr;
        u16 visibility;

        frame = sprite->frame;
        attr = sprite->attribut;
        visibility = sprite->visibility;
        spritesInf = frame->vdpSpritesInf;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTableAll_partialVisible: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(visibility)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;

            // sprite visible ?
            if (visibility & 1)
            {
                // Y first to respect VDP field order
                vdpSprite->y = sprite->y + spriteInf->y;
                vdpSprite->x = sprite->x + spriteInf->x;
            }

            vdpSprite->size = spriteInf->size;
            vdpSprite->attribut = attr;
            // increment tile index in attribut field
            attr += spriteInf->numTile;
            // pass to next VDP sprite
            visibility >>= 1;
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTableAll_partialVisibleHFlip(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        u16 attr;
        s16 fw, fh;
        u16 visibility;

        frame = sprite->frame;
        attr = sprite->attribut;
        visibility = sprite->visibility;
        spritesInf = frame->vdpSpritesInf;
        fw = frame->w;
        fh = frame->h;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTableAll_partialVisibleHFlip: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(visibility)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;
            u16 size = spriteInf->size;

            // sprite visible ?
            if (visibility & 1)
            {
                // Y first to respect VDP field order
                vdpSprite->y = sprite->y + spriteInf->y;
                vdpSprite->x = sprite->x + (fw - (spriteInf->x + (((size & 0x0C) << 1) + 8)));
            }

            vdpSprite->size = size;
            vdpSprite->attribut = attr;
            // increment tile index in attribut field
            attr += spriteInf->numTile;
            // pass to next VDP sprite
            visibility >>= 1;
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTableAll_partialVisibleVFlip(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        u16 attr;
        s16 fw, fh;
        u16 visibility;

        frame = sprite->frame;
        attr = sprite->attribut;
        visibility = sprite->visibility;
        spritesInf = frame->vdpSpritesInf;
        fw = frame->w;
        fh = frame->h;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTableAll_partialVisibleVFlip: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(visibility)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;
            u16 size = spriteInf->size;

            // sprite visible ?
            if (visibility & 1)
            {
                // Y first to respect VDP field order
                vdpSprite->y = sprite->y + (fh - (spriteInf->y + (((size & 0x03) << 3) + 8)));
                vdpSprite->x = sprite->x + spriteInf->x;
            }

            vdpSprite->size = size;
            vdpSprite->attribut = attr;
            // increment tile index in attribut field
            attr += spriteInf->numTile;
            // pass to next VDP sprite
            visibility >>= 1;
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTableAll_partialVisibleHFlipVFlip(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        u16 attr;
        s16 fw, fh;
        u16 visibility;

        frame = sprite->frame;
        attr = sprite->attribut;
        visibility = sprite->visibility;
        spritesInf = frame->vdpSpritesInf;
        fw = frame->w;
        fh = frame->h;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTableAll_partialVisibleHFlipVFlip: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(visibility)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;
            u16 size = spriteInf->size;

            // sprite visible ?
            if (visibility & 1)
            {
                // Y first to respect VDP field order
                vdpSprite->y = sprite->y + (fh - (spriteInf->y + (((size & 0x03) << 3) + 8)));
                vdpSprite->x = sprite->x + (fw - (spriteInf->x + (((size & 0x0C) << 1) + 8)));
            }

            vdpSprite->size = size;
            vdpSprite->attribut = attr;
            // increment tile index in attribut field
            attr += spriteInf->numTile;
            // pass to next VDP sprite
            visibility >>= 1;
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    u16 attr= sprite->attribut;

    // all visible ?
    if (sprite->visibility == VISIBILITY_ON)
    {
        if (attr & TILE_ATTR_VFLIP_MASK)
        {
            if (attr & TILE_ATTR_HFLIP_MASK)
                updateSpriteTableAll_allVisibleHFlipVFlip(sprite);
            else
                updateSpriteTableAll_allVisibleVFlip(sprite);
        }
        else
        {
            if (attr & TILE_ATTR_HFLIP_MASK)
                updateSpriteTableAll_allVisibleHFlip(sprite);
            else
                updateSpriteTableAll_allVisible(sprite);
        }
    }
    else
    {
        if (attr & TILE_ATTR_VFLIP_MASK)
        {
            if (attr & TILE_ATTR_HFLIP_MASK)
                updateSpriteTableAll_partialVisibleHFlipVFlip(sprite);
            else
                updateSpriteTableAll_partialVisibleVFlip(sprite);
        }
        else
        {
            if (attr & TILE_ATTR_HFLIP_MASK)
                updateSpriteTableAll_partialVisibleHFlip(sprite);
            else
                updateSpriteTableAll_partialVisible(sprite);
        }
    }

#ifdef SPR_DEBUG
    {
        u16 ind = sprite->VDPSpriteIndex;
        u16 i = sprite->definition->maxNumSprite;
        while(i--)
        {
            logVDPSprite(ind);
            // get next sprite
            ind = vdpSpriteCache[ind].link;
        }
    }
#endif // SPR_DEBUG


#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_UPDATE_SPRITE_TABLE] += prof;
#endif // SPR_PROFIL
}

static void updateSpriteTablePos(Sprite *sprite)
{
    inline void updateSpriteTablePos_allVisible(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        u16 j;

        frame = sprite->frame;
        j = frame->numSprite;
        spritesInf = frame->vdpSpritesInf;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTablePos_allVisible: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(j--)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;

            // Y first to respect VDP field order
            vdpSprite->y = sprite->y + spriteInf->y;
            vdpSprite->x = sprite->x + spriteInf->x;

            // pass to next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTablePos_allVisibleHFlip(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        s16 fw, fh;
        u16 j;

        frame = sprite->frame;
        j = frame->numSprite;
        spritesInf = frame->vdpSpritesInf;
        fw = frame->w;
        fh = frame->h;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTablePos_allVisibleHFlip: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(j--)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;

            // Y first to respect VDP field order
            vdpSprite->y = sprite->y + spriteInf->y;
            vdpSprite->x = sprite->x + (fw - (spriteInf->x + (((spriteInf->size & 0x0C) << 1) + 8)));

            // pass to next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTablePos_allVisibleVFlip(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        s16 fw, fh;
        u16 j;

        frame = sprite->frame;
        j = frame->numSprite;
        spritesInf = frame->vdpSpritesInf;
        fw = frame->w;
        fh = frame->h;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTablePos_allVisibleVFlip: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(j--)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;

            // Y first to respect VDP field order
            vdpSprite->y = sprite->y + (fh - (spriteInf->y + (((spriteInf->size & 0x03) << 3) + 8)));
            vdpSprite->x = sprite->x + spriteInf->x;

            // pass to next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTablePos_allVisibleHFlipVFlip(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        s16 fw, fh;
        u16 j;

        frame = sprite->frame;
        j = frame->numSprite;
        spritesInf = frame->vdpSpritesInf;
        fw = frame->w;
        fh = frame->h;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTablePos_allVisibleVFlipHFlip: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(j--)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;
            u16 size = spriteInf->size;

            // Y first to respect VDP field order
            vdpSprite->y = sprite->y + (fh - (spriteInf->y + (((size & 0x03) << 3) + 8)));
            vdpSprite->x = sprite->x + (fw - (spriteInf->x + (((size & 0x0C) << 1) + 8)));

            // pass to next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTablePos_partialVisible(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        u16 visibility;

        frame = sprite->frame;
        visibility = sprite->visibility;
        spritesInf = frame->vdpSpritesInf;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTablePos_partialVisible: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(visibility)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;

            // sprite visible ?
            if (visibility & 1)
            {
                // Y first to respect VDP field order
                vdpSprite->y = sprite->y + spriteInf->y;
                vdpSprite->x = sprite->x + spriteInf->x;
            }

            // pass to next VDP sprite
            visibility >>= 1;
            // pass to next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTablePos_partialVisibleHFlip(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        s16 fw, fh;
        u16 visibility;

        frame = sprite->frame;
        visibility = sprite->visibility;
        spritesInf = frame->vdpSpritesInf;
        fw = frame->w;
        fh = frame->h;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTablePos_partialVisibleHFlip: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(visibility)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;

            // sprite visible ?
            if (visibility & 1)
            {
                // Y first to respect VDP field order
                vdpSprite->y = sprite->y + spriteInf->y;
                vdpSprite->x = sprite->x + (fw - (spriteInf->x + (((spriteInf->size & 0x0C) << 1) + 8)));
            }

            // pass to next VDP sprite
            visibility >>= 1;
            // pass to next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTablePos_partialVisibleVFlip(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        s16 fw, fh;
        u16 visibility;

        frame = sprite->frame;
        visibility = sprite->visibility;
        spritesInf = frame->vdpSpritesInf;
        fw = frame->w;
        fh = frame->h;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTablePos_partialVisibleVFlip: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(visibility)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;

            // sprite visible ?
            if (visibility & 1)
            {
                // Y first to respect VDP field order
                vdpSprite->y = sprite->y + (fh - (spriteInf->y + (((spriteInf->size & 0x03) << 3) + 8)));
                vdpSprite->x = sprite->x + spriteInf->x;
            }

            // pass to next VDP sprite
            visibility >>= 1;
            // pass to next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }

    inline void updateSpriteTablePos_partialVisibleHFlipVFlip(Sprite *sprite)
    {
        AnimationFrame *frame;
        VDPSpriteInf **spritesInf;
        VDPSprite *vdpSprite;
        s16 fw, fh;
        u16 visibility;

        frame = sprite->frame;
        visibility = sprite->visibility;
        spritesInf = frame->vdpSpritesInf;
        fw = frame->w;
        fh = frame->h;
        vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    #ifdef SPR_DEBUG
        KLog_U3("  updateSpriteTablePos_partialVisibleHFlipVFlip: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
    #endif // SPR_DEBUG

        while(visibility)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;

            // sprite visible ?
            if (visibility & 1)
            {
                u16 size = spriteInf->size;

                // Y first to respect VDP field order
                vdpSprite->y = sprite->y + (fh - (spriteInf->y + (((size & 0x03) << 3) + 8)));
                vdpSprite->x = sprite->x + (fw - (spriteInf->x + (((size & 0x0C) << 1) + 8)));
            }

            // pass to next VDP sprite
            visibility >>= 1;
            // pass to next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }


#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    u16 attr= sprite->attribut;

    // all visible ?
    if (sprite->visibility == VISIBILITY_ON)
    {
        if (attr & TILE_ATTR_VFLIP_MASK)
        {
            if (attr & TILE_ATTR_HFLIP_MASK)
                updateSpriteTablePos_allVisibleHFlipVFlip(sprite);
            else
                updateSpriteTablePos_allVisibleVFlip(sprite);
        }
        else
        {
            if (attr & TILE_ATTR_HFLIP_MASK)
                updateSpriteTablePos_allVisibleHFlip(sprite);
            else
                updateSpriteTablePos_allVisible(sprite);
        }
    }
    else
    {
        if (attr & TILE_ATTR_VFLIP_MASK)
        {
            if (attr & TILE_ATTR_HFLIP_MASK)
                updateSpriteTablePos_partialVisibleHFlipVFlip(sprite);
            else
                updateSpriteTablePos_partialVisibleVFlip(sprite);
        }
        else
        {
            if (attr & TILE_ATTR_HFLIP_MASK)
                updateSpriteTablePos_partialVisibleHFlip(sprite);
            else
                updateSpriteTablePos_partialVisible(sprite);
        }
    }

#ifdef SPR_DEBUG
    {
        u16 ind = sprite->VDPSpriteIndex;
        u16 i = sprite->definition->maxNumSprite;
        while(i--)
        {
            logVDPSprite(ind);
            // get next sprite
            ind = vdpSpriteCache[ind].link;
        }
    }
#endif // SPR_DEBUG

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_UPDATE_SPRITE_TABLE] += prof;
#endif // SPR_PROFIL
}

static void updateSpriteTableAttr(Sprite *sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    AnimationFrame *frame;
    VDPSpriteInf **spritesInf;
    VDPSprite *vdpSprite;
    u16 attr;
    u16 j;

    frame = sprite->frame;
    attr = sprite->attribut;
    j = frame->numSprite;
    spritesInf = frame->vdpSpritesInf;
    vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

#ifdef SPR_DEBUG
    KLog_U3("  updateSpriteTableAttr_allVisible: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
#endif // SPR_DEBUG

    while(j--)
    {
        VDPSpriteInf* spriteInf = *spritesInf++;

        vdpSprite->attribut = attr;

        // increment tile index in attribut field
        attr += spriteInf->numTile;
        // pass to next VDP sprite
        vdpSprite = &vdpSpriteCache[vdpSprite->link];
    }

#ifdef SPR_DEBUG
    {
        u16 ind = sprite->VDPSpriteIndex;
        u16 i = sprite->definition->maxNumSprite;
        while(i--)
        {
            logVDPSprite(ind);
            // get next sprite
            ind = vdpSpriteCache[ind].link;
        }
    }
#endif // SPR_DEBUG

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_UPDATE_SPRITE_TABLE] += prof;
#endif // SPR_PROFIL
}


static void loadTiles(Sprite *sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    TileSet *tileset = sprite->frame->tileset;
    u16 compression = tileset->compression;
    u16 lenInWord = (tileset->numTile * 32) / 2;

    // need unpacking ?
    if (compression != COMPRESSION_NONE)
    {
        // unpack
        unpack(compression, (u8*) tileset->tiles, unpackNext);
        // queue DMA operation to transfert unpacked data to VRAM
        DMA_queueDma(DMA_VRAM, (u32) unpackNext, (sprite->attribut & TILE_INDEX_MASK) * 32, lenInWord, 2);

#ifdef SPR_DEBUG
        char str1[32];
        char str2[8];

        intToHex((u32) unpackNext, str2, 4);
        strcpy(str1, " at ");
        strcat(str1, str2);

        KLog_U1_("  loadTiles: unpack tileset, numTile= ", tileset->numTile, str1);
        KLog_U2("    Queue DMA: to=", (sprite->attribut & TILE_INDEX_MASK) * 32, " size in word=", lenInWord);
#endif // SPR_DEBUG

        // update unpacking address
        unpackNext += lenInWord * 2;
    }
    // just queue DMA operation to transfert tileset data to VRAM
    else
    {
        DMA_queueDma(DMA_VRAM, (u32) tileset->tiles, (sprite->attribut & TILE_INDEX_MASK) * 32, lenInWord, 2);

#ifdef SPR_DEBUG
        KLog_U3("  loadTiles - queue DMA: from=", (u32) tileset->tiles, " to=", (sprite->attribut & TILE_INDEX_MASK) * 32, " size in word=", lenInWord);
#endif // SPR_DEBUG
    }

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_LOADTILES] += prof;
#endif // SPR_PROFIL
}
