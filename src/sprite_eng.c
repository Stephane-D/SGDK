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

#define NEED_YSORTING                       0x0004

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
static Sprite* sortSpriteOnY(Sprite* sprite);
static Sprite* sortSprite(Sprite* sprite, _spriteComparatorCallback* sorter);
static void moveAfter(Sprite* pos, Sprite* sprite);
static u16 getSpriteIndex(Sprite *sprite);
static void logSprite(Sprite *sprite);

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

// pointer on first and last active sprite in the linked list
Sprite *firstSprite;
Sprite *lastSprite;
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
#define PROFIL_SORT                     18

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

    // no active sprites
    firstSprite = NULL;
    lastSprite = NULL;
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

#ifdef SPR_PROFIL
    memset(profil_time, 0, sizeof(profil_time));
#endif // SPR_PROFIL

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
    if (lastSprite) lastSprite->next = sprite;
    sprite->prev = lastSprite;
    sprite->next = NULL;
    // update first and last sprite
    if (firstSprite == NULL) firstSprite = sprite;
    lastSprite = sprite;
    // update sprite number
    spriteNum++;

#ifdef SPR_DEBUG
    KLog_U2("SPR_addSpriteEx: added sprite #", getSpriteIndex(sprite), " - internal position = ", sprite - spritesBank);
#endif // SPR_DEBUG

    // auto visibility ?
    if (flags & SPR_FLAG_AUTO_VISIBILITY) sprite->visibility = 0;
    // otherwise we set it to visible by default
    else sprite->visibility = VISIBILITY_ON;
    // initialized with specified flags
    sprite->definition = spriteDef;
    sprite->frame = NULL;
    sprite->animInd = -1;
    sprite->frameInd = -1;
    sprite->seqInd = -1;
    sprite->x = x + 0x80;
    sprite->aot = 0;
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
                            SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_SPRITE_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD);
}

void SPR_releaseSprite(Sprite *sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    Sprite* prev;
    Sprite* next;
    VDPSprite* lastVDPSprite;

#ifdef SPR_DEBUG
    KLog_U2("SPR_releaseSprite: releasing sprite #", getSpriteIndex(sprite), " - internal position = ", sprite - spritesBank);
#endif // SPR_DEBUG

    // release sprite (no error checking here)
    releaseSprite(sprite);

    prev = sprite->prev;
    next = sprite->next;

    // get the last VDP sprite to link from
    if (prev)
    {
        lastVDPSprite = prev->lastVDPSprite;
        prev->next = next;
    }
    else
    {
        lastVDPSprite = starter;
        // update first sprite
        firstSprite = next;
    }
    // get the next VDP Sprite index to link to
    if (next)
    {
        lastVDPSprite->link = next->VDPSpriteIndex;
        next->prev = prev;
    }
    else
    {
        lastVDPSprite->link = 0;
        // update last sprite
        lastSprite = prev;
    }

    // decrement number of sprite
    spriteNum--;

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
    KLog_U1("SPR_setDefinition: #", getSpriteIndex(sprite));
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
    KLog_U3("SPR_setPosition: #", getSpriteIndex(sprite), "  X=", newx, " Y=", newy);
#endif // SPR_DEBUG

    if ((sprite->x != newx) || (sprite->y != newy))
    {
        u16 status = sprite->status;

        sprite->x = newx;
        // Y sorting enable for this sprite and Y position changed ? --> need sorting
        if ((status & SPR_FLAG_AUTO_YSORTING) && (sprite->y != newy))
            status |= NEED_YSORTING;
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
            KLog_U1_("SPR_setHFlip: #", getSpriteIndex(sprite), " removed HFlip");
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
            KLog_U1_("SPR_setHFlip: #", getSpriteIndex(sprite), " added HFlip");
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
            KLog_U1_("SPR_setVFlip: #", getSpriteIndex(sprite), " removed VFlip");
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
            KLog_U1_("SPR_setVFlip: #", getSpriteIndex(sprite), " added VFlip");
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
            KLog_U1_("SPR_setPriorityAttribut: #", getSpriteIndex(sprite), " removed priority");
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
            KLog_U1_("SPR_setPriorityAttribut: #", getSpriteIndex(sprite), " added priority");
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
        KLog_U2("SPR_setPalette: #", getSpriteIndex(sprite), " palette=", value);
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
        KLog_U4("SPR_setAnimAndFrame: #", getSpriteIndex(sprite), " anim=", anim, " frame=", frame, " adj frame=", frameInd);
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
        KLog_U3("SPR_setAnim: #", getSpriteIndex(sprite), " anim=", anim, " frame=0 adj frame=", frameInd);
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
            KLog_U3("SPR_setFrame: #", getSpriteIndex(sprite), "  frame=", frame, " adj frame=", frameInd);
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
            KLog_U2("SPR_setVRAMTileIndex: #", getSpriteIndex(sprite), " passed to manual allocation, VRAM index =", value);
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
            KLog_U1_("SPR_setVRAMTileIndex: #", getSpriteIndex(sprite), " passed to auto allocation");
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
            KLog_U2("SPR_setSpriteTableIndex: #", getSpriteIndex(sprite), " passed to manual allocation, VDP Sprite index =", value);
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
            KLog_U1_("SPR_setSpriteTableIndex: #", getSpriteIndex(sprite), " passed to auto allocation");
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

void SPR_setYSorting(Sprite *sprite, u16 value)
{
    if (value) sprite->status |= SPR_FLAG_AUTO_YSORTING;
    else sprite->status &= ~SPR_FLAG_AUTO_YSORTING;
}

void SPR_setAlwaysOnTop(Sprite *sprite, u16 value)
{
    if (value)
    {
        // use this for easy Y sorting
        sprite->aot = TRUE;
        sprite->status |= SPR_FLAG_ALWAYS_ON_TOP;
    }
    else
    {
        // use this for easy Y sorting
        sprite->aot = FALSE;
        sprite->status &= ~SPR_FLAG_ALWAYS_ON_TOP;
    }
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
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    Sprite* sprite;

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

    // iterate over all sprites
    sprite = firstSprite;
    while(sprite)
    {
        u16 timer = sprite->timer;

#ifdef SPR_DEBUG
        char str1[32];
        char str2[8];

        intToHex(sprite->visibility, str2, 4);
        strcpy(str1, " visibility = ");
        strcat(str1, str2);

        KLog_U2_("  processing sprite pos #", getSpriteIndex(sprite), " - timer = ", timer, str1);
#endif // SPR_DEBUG

        // handle frame animation
        if (timer)
        {
            // timer elapsed --> next frame
            if (--timer == 0) SPR_nextFrame(sprite);
            // just update remaining timer
            else sprite->timer = timer;
        }

        // can be changed by Y sorting so store it now
        Sprite* next = sprite->next;
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
                if (status & NEED_YSORTING)
                    sortSpriteOnY(sprite);
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
                status &= ~(NEED_YSORTING | NEED_TILES_UPLOAD | NEED_ST_ALL_UPDATE);
            }

            // processes done !
            sprite->status = status;
        }

        // next sprite
        sprite = next;
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
#endif // SPR_PROFIL
}

void SPR_sortOnYPos()
{
    SPR_sort(NULL);
}

void SPR_sort(_spriteComparatorCallback* sorter)
{
    if (spriteNum)
    {
        Sprite *sprite;
//        VDPSprite *prevVDPSprite;

        // disable interrupts (we want to avoid DMA queue process when executing this method)
        SYS_disableInts();

        // sort sprites
        sprite = lastSprite;
        if (sorter)
        {
            while (sprite)
                sprite = sortSprite(sprite, sorter);
        }
        else
        {
            while (sprite)
                sprite = sortSpriteOnY(sprite);
        }

//        // rebuils all links
//        sprite = firstSprite;
//        prevVDPSprite = starter;
//
//        // then rebuild others links
//        while(sprite)
//        {
//#ifdef SPR_DEBUG
//            KLog_U3("VDPSprite #", prevVDPSprite - vdpSpriteCache, " linked to Sprite #", getSpriteIndex(sprite), " - VDP Sprite=", sprite->VDPSpriteIndex);
//#endif // SPR_DEBUG
//
//            // update link
//            prevVDPSprite->link = sprite->VDPSpriteIndex;
//            prevVDPSprite = sprite->lastVDPSprite;
//            // next sprite
//            sprite = sprite->next;
//        }
//
//        // end link
//        prevVDPSprite->link = 0;

        // re-enable interrupts
        SYS_enableInts();

    }
}

void SPR_logProfil()
{
#ifdef SPR_PROFIL
    KLog("SPR Engine profiling ------------------------------------------------------");
    KLog_U2x(4, "Alloc=", profil_time[PROFIL_ALLOCATE_SPRITE], " Release=", profil_time[PROFIL_RELEASE_SPRITE]);
    KLog_U2x(4, "Add=", profil_time[PROFIL_ADD_SPRITE], " Remove=", profil_time[PROFIL_REMOVE_SPRITE]);
    KLog_U2x(4, "Set Def.=", profil_time[PROFIL_SET_DEF], " Set Attr.=", profil_time[PROFIL_SET_ATTRIBUTE]);
    KLog_U2x(4, "Set Anim & Frame=", profil_time[PROFIL_SET_ANIM_FRAME], " Set VRAM & Sprite Ind=", profil_time[PROFIL_SET_VRAM_OR_SPRIND]);
    KLog_U2x(4, "Set Visibility=", profil_time[PROFIL_SET_VISIBILITY], "  Clear=", profil_time[PROFIL_CLEAR]);
    KLog_U1x(4, "Sort Sprite list=", profil_time[PROFIL_SORT]);
    KLog_U1x_(4, " Update all=", profil_time[PROFIL_UPDATE], " -------------");
    KLog_U2x(4, "Update visibility=", profil_time[PROFIL_UPDATE_VISIBILITY], "  Update frame=", profil_time[PROFIL_UPDATE_FRAME]);
    KLog_U2x(4, "Update vdp_spr_ind=", profil_time[PROFIL_UPDATE_VDPSPRIND], "  Update vis spr table=", profil_time[PROFIL_UPDATE_VISTABLE]);
    KLog_U2x(4, "Update Sprite Table=", profil_time[PROFIL_UPDATE_SPRITE_TABLE], " Load Tiles=", profil_time[PROFIL_LOADTILES]);

    // reset profil counters
    memset(profil_time, 0, sizeof(profil_time));
#endif // SPR_PROFIL
}

void SPR_logSprites()
{
    Sprite* sprite = firstSprite;

    KLog_U1_("Num sprite = ", spriteNum, " -----------------------------");
    while(sprite)
    {
        logSprite(sprite);
        sprite = sprite->next;
    }
}


static void setVDPSpriteIndex(Sprite *sprite, u16 ind, u16 num, VDPSprite *last)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    Sprite* prev;

#ifdef SPR_DEBUG
    KLog_U2("setVDPSpriteIndex: sprite #", getSpriteIndex(sprite), "  new VDP Sprite index = ", ind);
#endif // SPR_DEBUG

    sprite->VDPSpriteIndex = ind;

    // link with previous sprite
    if ((prev = sprite->prev)) prev->lastVDPSprite->link = ind;
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
            u16 num;

            attr = sprite->attribut;
            num = frame->numSprite;
            // start from the last one
            spritesInf = &(frame->vdpSpritesInf[num - 1]);
            // adjust for HV flip
            if (attr & TILE_ATTR_HFLIP_MASK) spritesInf += num;
            if (attr & TILE_ATTR_VFLIP_MASK) spritesInf += num << 1;
            visibility = 0;

            while(num--)
            {
                VDPSpriteInf* spriteInf;
                u16 size;
                s16 x, y;
                s16 w, h;

                spriteInf = *spritesInf--;

                // Y first to respect spriteInf field order
                y = spriteInf->y;
                size = spriteInf->size;
                w = ((size & 0x0C) << 1) + 8;
                h = ((size & 0x03) << 3) + 8;
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
    KLog_U1("  updateFrame: sprite #", getSpriteIndex(sprite));
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
    KLog_U2("  updateSpriteTableVisibility: sprite #", getSpriteIndex(sprite), "  visibility = ", sprite->visibility);
#endif // SPR_DEBUG

    VDPSprite *vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];
    u16 visibility = sprite->visibility;

    if (visibility)
    {
        // all visible ?
        if (visibility == VISIBILITY_ON)
        {
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
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    AnimationFrame *frame;
    VDPSpriteInf **spritesInf;
    VDPSprite *vdpSprite;
    u16 attr;
    u16 num;
    u16 visibility;

    visibility = sprite->visibility;
    frame = sprite->frame;
    attr = sprite->attribut;
    num = frame->numSprite;
    spritesInf = frame->vdpSpritesInf;
    if (attr & TILE_ATTR_HFLIP_MASK) spritesInf += num;
    if (attr & TILE_ATTR_VFLIP_MASK) spritesInf += num << 1;
    vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    // all visible ?
    if (visibility == VISIBILITY_ON)
    {
        while(num--)
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
    else
    {
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
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    AnimationFrame *frame;
    VDPSpriteInf **spritesInf;
    VDPSprite *vdpSprite;
    u16 attr;
    u16 num;
    u16 visibility;

    visibility = sprite->visibility;
    frame = sprite->frame;
    attr = sprite->attribut;
    num = frame->numSprite;
    spritesInf = frame->vdpSpritesInf;
    if (attr & TILE_ATTR_HFLIP_MASK) spritesInf += num;
    if (attr & TILE_ATTR_VFLIP_MASK) spritesInf += num << 1;
    vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    // all visible ?
    if (visibility == VISIBILITY_ON)
    {
        while(num--)
        {
            VDPSpriteInf* spriteInf = *spritesInf++;

            // Y first to respect VDP field order
            vdpSprite->y = sprite->y + spriteInf->y;
            vdpSprite->x = sprite->x + spriteInf->x;

            // pass to next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }
    else
    {
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
    u16 num;

    frame = sprite->frame;
    attr = sprite->attribut;
    num = frame->numSprite;
    spritesInf = frame->vdpSpritesInf;
    vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

#ifdef SPR_DEBUG
    KLog_U3("  updateSpriteTableAttr_allVisible: numSprite=", frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
#endif // SPR_DEBUG

    while(num--)
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

static Sprite* sortSpriteOnY(Sprite* sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    Sprite* prev = sprite->prev;
    Sprite* next = sprite->next;
    Sprite* s;

    // cache sprite y coordinate
    const s32 sy = sprite->ylong;

#ifdef SPR_DEBUG
    KLog_U2("Start compare for sprite #", getSpriteIndex(sprite), " VDP Sprite Ind=", sprite->VDPSpriteIndex);
#endif // SPR_DEBUG

    // find position forward first
    s = next;
    while(s && (s->ylong > sy)) s = s->next;
    // position changed ?
    if (s != next)
    {
        // adjust on previous as we insert *after*
        if (s) s = s->prev;
        else s = lastSprite;
    }
    else
    {
        // try to find position babckward then
        s = prev;
        while(s && (s->ylong < sy)) s = s->prev;
    }

#ifdef SPR_DEBUG
    if (s) KLog_U1("Position for sprite = ", getSpriteIndex(s) + 1);
    else KLog_U1("Position for sprite = ", 0);
#endif // SPR_DEBUG

    // position changed ? --> insert sprite after s
    if (s != prev) moveAfter(s, sprite);

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SORT] += prof;
#endif // SPR_PROFIL

    // return prev just for convenience on full sorting
    return prev;
}

static Sprite* sortSprite(Sprite* sprite, _spriteComparatorCallback* sorter)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    Sprite* prev = sprite->prev;
    Sprite* next = sprite->next;
    Sprite* s;

#ifdef SPR_DEBUG
    KLog_U2("Start compare for sprite #", getSpriteIndex(sprite), " VDP Sprite Ind=", sprite->VDPSpriteIndex);
#endif // SPR_DEBUG

    // find position forward first
    s = next;
    while(s && (sorter(s, sprite) > 0)) s = s->next;

    // position changed ?
    if (s != next)
    {
        // adjust on previous as we insert *after*
        if (s) s = s->prev;
        else s = lastSprite;
    }
    else
    {
        // try to find position babckward then
        s = prev;
        while(s && (sorter(s, sprite) < 0)) s = s->prev;
    }

#ifdef SPR_DEBUG
    if (s) KLog_U1("Position for sprite = ", getSpriteIndex(s) + 1);
    else KLog_U1("Position for sprite = ", 0);
#endif // SPR_DEBUG

    // position changed ? --> insert sprite after s
    if (s != prev) moveAfter(s, sprite);

    // return prev just for convenience on full sorting
    return prev;

#ifdef SPR_PROFIL
    prof = getSubTick() - prof;
    // rollback correction
    if (prof < 0) prof = 100;
    profil_time[PROFIL_SORT] += prof;
#endif // SPR_PROFIL
}

static void moveAfter(Sprite* pos, Sprite* sprite)
{
    Sprite* prev = sprite->prev;
    Sprite* next = sprite->next;

#ifdef SPR_DEBUG
    if (pos) KLog_U2("Insert #", getSpriteIndex(sprite), "  after #", getSpriteIndex(pos));
    else KLog_U1_("Insert #", getSpriteIndex(sprite), "  at #0");
#endif // SPR_DEBUG

    if (prev)
    {
        // remove sprite from its position
        prev->next = next;
        if (next)
        {
            next->prev = prev;
            // fix sprite link from previous sprite
            prev->lastVDPSprite->link = next->VDPSpriteIndex;
        }
        else
        {
            lastSprite = prev;
            // fix sprite link from previous sprite
            prev->lastVDPSprite->link = 0;
        }
    }
    else
    {
        // remove sprite from its position
        firstSprite = next;
        if (next)
        {
            next->prev = prev;
            // fix sprite link from previous sprite
            starter->link = next->VDPSpriteIndex;
        }
        else
        {
            lastSprite = prev;
            // fix sprite link from previous sprite
            starter->link = 0;
        }
    }

    // and insert after 'pos'
    if (pos)
    {
        next = pos->next;
        sprite->next = next;
        sprite->prev = pos;
        if (next) next->prev = sprite;
        else lastSprite = sprite;
        pos->next = sprite;
        // fix sprite link
        sprite->lastVDPSprite->link = pos->lastVDPSprite->link;
        pos->lastVDPSprite->link = sprite->VDPSpriteIndex;
    }
    // or set it as first sprite
    else
    {
        sprite->next = firstSprite;
        sprite->prev = NULL;
        firstSprite->prev = sprite;
        firstSprite = sprite;
        // fix sprite link
        sprite->lastVDPSprite->link = starter->link;
        starter->link = sprite->VDPSpriteIndex;
    }
}

static u16 getSpriteIndex(Sprite *sprite)
{
    u16 res = 0;
    Sprite* s = firstSprite;

    while(s != sprite)
    {
        s = s->next;
        res++;
    }

    return res;
}

static void logSprite(Sprite *sprite)
{
    KLog_U2("Sprite #", getSpriteIndex(sprite), " ------------- status=", sprite->status);
    KLog_U3("animInd=", sprite->animInd, " seqInd=", sprite->seqInd, " frameInd=", sprite->frameInd);
    KLog_U4("attribut=", sprite->attribut, " x=", sprite->x, " y=", sprite->y, " visibility=", sprite->visibility);
    KLog_U3("timer=", sprite->timer, " frameNumSpr=", sprite->frameNumSprite, " VDPSpriteInd=", sprite->VDPSpriteIndex);
    KLog_U2("prev=", (sprite->prev==NULL)?128:getSpriteIndex(sprite->prev), " next=", (sprite->next==NULL)?128:getSpriteIndex(sprite->next));
}
