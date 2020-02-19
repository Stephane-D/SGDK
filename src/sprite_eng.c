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
#include "mapper.h"

#include "kdebug.h"
#include "string.h"
#include "timer.h"


//#define SPR_DEBUG
//#define SPR_PROFIL

#ifdef SPR_DEBUG
    // force LIB_DEBUG
    #define LIB_DEBUG   1
#endif // SPR_DEBUG


// first hardware sprite is reserved (used internally for sorting)
#define MAX_SPRITE                          (80 - 1)

// internals
#define VISIBILITY_ON                       0xFFFF
#define VISIBILITY_OFF                      0x0000

#define ALLOCATED                           0x8000

#define NEED_ST_ATTR_UPDATE                 0x0001
#define NEED_ST_POS_UPDATE                  0x0002
#define NEED_ST_ALL_UPDATE                  (NEED_ST_ATTR_UPDATE | NEED_ST_POS_UPDATE)

#define NEED_VISIBILITY_UPDATE              0x0010
#define NEED_FRAME_UPDATE                   0x0020
#define NEED_TILES_UPLOAD                   0x0040

#define NEED_UPDATE                         0x00FF


// shared from vdp_spr.c unit
extern void logVDPSprite(u16 index);


// forward
static Sprite* allocateSprite(u16 head);
//static Sprite** allocateSprites(Sprite** sprites, u16 num);
static bool releaseSprite(Sprite* sprite);
//static void releaseSprites(Sprite** sprites, u16 num);

static void setVDPSpriteIndex(Sprite* sprite, u16 ind, u16 num);
static bool updateVisibility(Sprite* sprite, u16 status);
static u16 setVisibility(Sprite* sprite, u16 visibility);
static u16 updateFrame(Sprite* sprite, u16 status);

static void updateSpriteTableAll(Sprite* sprite);
static void updateSpriteTablePos(Sprite* sprite);
static void updateSpriteTableAttr(Sprite* sprite);
static void loadTiles(Sprite* sprite);
static Sprite* sortSprite(Sprite* sprite);
static void moveAfter(Sprite* pos, Sprite* sprite);
static u16 getSpriteIndex(Sprite* sprite);
static void logSprite(Sprite* sprite);

// starter VDP sprite - never visible (used for sprite sorting)
static VDPSprite* starter;

// allocated bank of sprites for the Sprite Engine
static Sprite* spritesBank = NULL;

// used for sprite allocation
static Sprite** allocStack;
// point on top of the allocation stack (first available sprite)
static Sprite** free;

// pointer on first and last active sprite in the linked list
Sprite* firstSprite;
Sprite* lastSprite;

static u8* unpackBuffer;
static u8* unpackNext;
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
#define PROFIL_VRAM_DEFRAG              19

static u32 profil_time[20];
#endif


void SPR_initEx(u16 vramSize, u16 unpackBufferSize)
{
    u16 index;
    u16 size;

    // already initialized --> end it first
    if (SPR_isInitialized()) SPR_end();

    // alloc sprites bank
    spritesBank = MEM_alloc(MAX_SPRITE * sizeof(Sprite));
    // allocation stack
    allocStack = MEM_alloc(MAX_SPRITE * sizeof(Sprite*));
    // alloc sprite tile unpack buffer
    unpackBuffer = MEM_alloc(((unpackBufferSize?unpackBufferSize:256) * 32) + 1024);

    size = vramSize?vramSize:384;
    // get start tile index for sprite data (reserve VRAM area just before system font)
    index = TILE_FONTINDEX - size;

    // and create a VRAM region for sprite tile allocation
    VRAM_createRegion(&vram, index, size);

#if (LIB_DEBUG != 0)
    KLog("Sprite engine initialized !");
    KLog_U1("  unpack buffer size:", (unpackBufferSize?unpackBufferSize:256) * 32);
    KLog_U2_("  VRAM region: [", index, " - ", index + (size - 1), "]");
#endif // LIB_DEBUG

    // reset
    SPR_reset();
}

void SPR_init()
{
    SPR_initEx(512, 320);
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
        MEM_free(allocStack);
        allocStack = NULL;
        MEM_free(unpackBuffer);
        unpackBuffer = NULL;

        VRAM_releaseRegion(&vram);
    }

#if (LIB_DEBUG != 0)
    KLog("Sprite engine ended !");
#endif
}

bool SPR_isInitialized()
{
    return (spritesBank != NULL);
}

void SPR_reset()
{
    u16 i;

    // release and clear sprites data
    memset(spritesBank, 0, sizeof(Sprite) * MAX_SPRITE);

    // reset allocation stack
    for(i = 0; i < MAX_SPRITE; i++)
        allocStack[i] = &spritesBank[i];
    // init free position
    free = &allocStack[MAX_SPRITE];

    // no active sprites
    firstSprite = NULL;
    lastSprite = NULL;
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

#if (LIB_DEBUG != 0)
    KLog("Sprite engine reset");
    KLog_U1("  VRAM region free: ", VRAM_getFree(&vram));
    KLog_U1("  Available VDP sprites: ", VDP_getAvailableSprites());
#endif // LIB_DEBUG
}


static Sprite* allocateSprite(u16 head)
{
    Sprite* result;

    // enough sprite remaining ?
    if (free == allocStack)
    {
#if (LIB_DEBUG != 0)
        KLog("SPR_internalAllocateSprite(): failed - no more available sprite !");
#endif

        return NULL;
    }

#if (LIB_DEBUG != 0)
    KLog_U1("SPR_internalAllocateSprite(): success - allocating sprite at pos ", free[-1] - spritesBank);
#endif // LIB_DEBUG

    // allocate
    result = *--free;

    if (head)
    {
        // add the new sprite at the beginning of the chained list
        if (firstSprite) firstSprite->prev = result;
        result->prev = NULL;
        result->next = firstSprite;
        // update first and last sprite
        if (lastSprite == NULL) lastSprite = result;
        firstSprite = result;
    }
    else
    {
        // add the new sprite at the end of the chained list
        if (lastSprite) lastSprite->next = result;
        result->prev = lastSprite;
        result->next = NULL;
        // update first and last sprite
        if (firstSprite == NULL) firstSprite = result;
        lastSprite = result;
    }

    // mark as allocated --> this is done after allocate call, not needed here
    // result->status = ALLOCATED;

    return result;
}

static bool releaseSprite(Sprite* sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    // really allocated ?
    if (sprite->status & ALLOCATED)
    {
        Sprite* prev;
        Sprite* next;
        VDPSprite* lastVDPSprite;

#if (LIB_DEBUG != 0)
        KLog_U1("SPR_internalReleaseSprite: success - released sprite at pos ", sprite - spritesBank);
#endif // LIB_DEBUG

        // release sprite
        *free++ = sprite;

        // remove sprite from chained list
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

        // not anymore allocated
        sprite->status &= ~ALLOCATED;

#ifdef SPR_PROFIL
        profil_time[PROFIL_RELEASE_SPRITE] += getSubTick() - prof;
#endif // SPR_PROFIL

        return TRUE;
    }

#if (LIB_DEBUG != 0)
    KLog_U1_("SPR_internalReleaseSprite: failed - sprite at pos ", sprite - spritesBank, " is not allocated !");
#endif // LIB_DEBUG

#ifdef SPR_PROFIL
    profil_time[PROFIL_RELEASE_SPRITE] += getSubTick() - prof;
#endif // SPR_PROFIL

    return FALSE;
}

Sprite* SPR_addSpriteEx(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut, u16 spriteIndex, u16 flag)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    s16 ind;
    Sprite* sprite;

    // allocate new sprite
    sprite = allocateSprite(flag & SPR_FLAG_INSERT_HEAD);

    // can't allocate --> return NULL
    if (!sprite)
    {
#if (LIB_DEBUG != 0)
        KDebug_Alert("SPR_addSpriteEx failed: max sprite number reached !");
#endif

        return NULL;
    }

    sprite->status = ALLOCATED | (flag & SPR_FLAG_MASK);

#ifdef SPR_DEBUG
    KLog_U2("SPR_addSpriteEx: added sprite #", getSpriteIndex(sprite), " - internal position = ", sprite - spritesBank);
#endif // SPR_DEBUG

    // auto visibility ?
    if (flag & SPR_FLAG_AUTO_VISIBILITY) sprite->visibility = 0;
    // otherwise we set it to visible by default
    else sprite->visibility = VISIBILITY_ON;
    // initialized with specified flag
    sprite->definition = spriteDef;
    sprite->onFrameChange = NULL;
//    FIXME: not needed
//    sprite->animation = NULL;
//    sprite->frame = NULL;
//    sprite->frameInfo = NULL;

    sprite->animInd = -1;
    sprite->frameInd = -1;
    sprite->seqInd = -1;
    // to avoid NULL pointer access as frame can be used without being yet initialized
    sprite->frame = spriteDef->animations[0]->frames[0];
    sprite->frameInfo = &(sprite->frame->frameInfos[0]);

    sprite->x = x + 0x80;
    sprite->y = y + 0x80;
    // depending sprite position (first or last) we set its default depth
    if (flag & SPR_FLAG_INSERT_HEAD) sprite->depth = SPR_MIN_DEPTH;
    else sprite->depth = SPR_MAX_DEPTH;
    sprite->spriteToHide = 0;

    const u16 numVDPSprite = spriteDef->maxNumSprite;

    // auto VDP sprite alloc enabled ?
    if (flag & SPR_FLAG_AUTO_SPRITE_ALLOC)
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
    }
    // use given sprite index
    else ind = spriteIndex;

    // set the VDP Sprite index for this sprite and do attached operation
    setVDPSpriteIndex(sprite, ind, numVDPSprite);

    // auto VRAM alloc enabled ?
    if (flag & SPR_FLAG_AUTO_VRAM_ALLOC)
    {
        // allocate VRAM
        ind = VRAM_alloc(&vram, spriteDef->maxNumTile);
        // not enough --> release sprite and return NULL
        if (ind < 0)
        {
            // release allocated VDP sprites
            if (flag & SPR_FLAG_AUTO_SPRITE_ALLOC)
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

    // set anim and frame to 0 (important to do it after sprite->attribut has been set)
    SPR_setAnimAndFrame(sprite, 0, 0);

#ifdef SPR_PROFIL
    profil_time[PROFIL_ADD_SPRITE] += getSubTick() - prof;
#endif // SPR_PROFIL

    return sprite;
}

Sprite* SPR_addSprite(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut)
{
    return SPR_addSpriteEx(spriteDef, x, y, attribut, 0,
                            SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_SPRITE_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD);
}

Sprite* SPR_addSpriteExSafe(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut, u16 spriteIndex, u16 flag)
{
    Sprite* result = SPR_addSpriteEx(spriteDef, x, y, attribut, spriteIndex, flag);

    // allocation failed ?
    if (result == NULL)
    {
        // try to defragment VRAM, it can help
        SPR_defragVRAM();
        // VRAM is now defragmented, so allocation should pass this time
        result = SPR_addSpriteEx(spriteDef, x, y, attribut, spriteIndex, flag);
    }

    return result;
}

Sprite* SPR_addSpriteSafe(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut)
{
    Sprite* result = SPR_addSprite(spriteDef, x, y, attribut);

    // allocation failed ?
    if (result == NULL)
    {
        // try to defragment VRAM, it can help
        SPR_defragVRAM();
        // VRAM is now defragmented, so allocation should pass this time
        result = SPR_addSprite(spriteDef, x, y, attribut);
    }

    return result;
}

void SPR_releaseSprite(Sprite* sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

#ifdef SPR_DEBUG
    KLog_U2("SPR_releaseSprite: releasing sprite #", getSpriteIndex(sprite), " - internal position = ", sprite - spritesBank);
#endif // SPR_DEBUG

    // release sprite
    if (!releaseSprite(sprite))
    {
#ifdef SPR_PROFIL
        profil_time[PROFIL_REMOVE_SPRITE] += getSubTick() - prof;
#endif // SPR_PROFIL

        return;
    }

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
    profil_time[PROFIL_REMOVE_SPRITE] += getSubTick() - prof;
#endif // SPR_PROFIL
}

u16 SPR_getNumActiveSprite()
{
    return &allocStack[MAX_SPRITE] - free;
}

void SPR_defragVRAM()
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    Sprite* sprite;

    // release all VRAM region
    VRAM_clearRegion(&vram);

    // iterate over all sprites to re-allocate auto allocated VRAM
    sprite = firstSprite;
    while(sprite)
    {
        u16 status = sprite->status;

        // sprite is using auto VRAM allocation ?
        if (status & SPR_FLAG_AUTO_VRAM_ALLOC)
        {
            // re-allocate VRAM for this sprite (can't fail here)
            const u16 ind = VRAM_alloc(&vram, sprite->definition->maxNumTile);
            const u16 attr = sprite->attribut;

            // VRAM allocation changed ?
            if ((attr & ~TILE_ATTR_MASK) != ind)
            {
                // set VRAM index and preserve previous attributs
                sprite->attribut = ind | (attr & TILE_ATTR_MASK);

                // need to update attribute VDP sprite table
                status |= NEED_ST_ATTR_UPDATE;
                // auto tile upload enabled ? --> need to re upload tile to new location
                if (status & SPR_FLAG_AUTO_TILE_UPLOAD)
                    status |= NEED_TILES_UPLOAD;

                sprite->status = status;
            }
        }

        // next sprite
        sprite = sprite->next;
    }

#ifdef SPR_PROFIL
    profil_time[PROFIL_VRAM_DEFRAG] += getSubTick() - prof;
#endif // SPR_PROFIL
}

bool SPR_setDefinition(Sprite* sprite, const SpriteDefinition* spriteDef)
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
    const u16 oldNumSprite = sprite->definition->maxNumSprite;
    const u16 newNumSprite = spriteDef->maxNumSprite;

    // auto VDP sprite alloc enabled --> realloc VDP sprite(s)
    if ((status & SPR_FLAG_AUTO_SPRITE_ALLOC) && (oldNumSprite != newNumSprite))
    {
        // we release previous allocated VDP sprite(s)
        VDP_releaseSprites(sprite->VDPSpriteIndex, oldNumSprite);

#ifdef SPR_DEBUG
        KLog_U3("  released ", oldNumSprite, " VDP sprite(s) at ", sprite->VDPSpriteIndex, ", remaining VDP sprite = ", VDP_getAvailableSprites());
#endif // SPR_DEBUG

        // then we allocate the VDP sprite(s) for the new definition
        const s16 ind = VDP_allocateSprites(newNumSprite);
        // not enough --> return error
        if (ind == -1) return FALSE;

        // set the VDP Sprite index for this sprite and do attached operation
        setVDPSpriteIndex(sprite, ind, newNumSprite);

#ifdef SPR_DEBUG
        KLog_U3("  allocated ", newNumSprite, " VDP sprite(s) at ", ind, ", remaining VDP sprite = ", VDP_getAvailableSprites());
#endif // SPR_DEBUG
    }

    const u16 newNumTile = spriteDef->maxNumTile;

    // auto VRAM alloc enabled --> realloc VRAM tile area
    if ((status & SPR_FLAG_AUTO_VRAM_ALLOC) && (sprite->definition->maxNumTile != newNumTile))
    {
        // we release previous allocated VRAM
        VRAM_free(&vram, sprite->attribut & TILE_INDEX_MASK);

#ifdef SPR_DEBUG
        KLog_U3("  released ", sprite->definition->maxNumTile, " tiles in VRAM at ", sprite->attribut & TILE_INDEX_MASK, ", remaining VRAM: ", VRAM_getFree(&vram));
#endif // SPR_DEBUG

        // allocate VRAM
        const s16 ind = VRAM_alloc(&vram, newNumTile);
        // not enough --> return error
        if (ind < 0) return FALSE;

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

    // to avoid NULL pointer access as frame can be used without being yet initialized
    sprite->frame = spriteDef->animations[0]->frames[0];
    sprite->frameInfo = &(sprite->frame->frameInfos[0]);

    // set anim and frame to 0
    SPR_setAnimAndFrame(sprite, 0, 0);

#ifdef SPR_PROFIL
    profil_time[PROFIL_SET_DEF] += getSubTick() - prof;
#endif // SPR_PROFIL

    return TRUE;
}

void SPR_setPosition(Sprite* sprite, s16 x, s16 y)
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
        sprite->y = newy;

        // need to recompute visibility if auto visibility is enabled
        if (status & SPR_FLAG_AUTO_VISIBILITY)
            status |= NEED_VISIBILITY_UPDATE;

        sprite->status = status | NEED_ST_POS_UPDATE;
    }

#ifdef SPR_PROFIL
    profil_time[PROFIL_SET_ATTRIBUTE] += getSubTick() - prof;
#endif // SPR_PROFIL
}

void SPR_setHFlip(Sprite* sprite, u16 value)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    u16 attr = sprite->attribut;

    if (attr & TILE_ATTR_HFLIP_MASK)
    {
        // H flip removed
        if (!value)
        {
            attr &= ~TILE_ATTR_HFLIP_MASK;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setHFlip: #", getSpriteIndex(sprite), " removed HFlip");
#endif // SPR_DEBUG

            u16 status = sprite->status;

            // need to recompute visibility if auto visibility is enabled
            if (status & SPR_FLAG_AUTO_VISIBILITY)
                status |= NEED_VISIBILITY_UPDATE;

            // need to also recompute complete VDP sprite table
            sprite->status = status | NEED_ST_ALL_UPDATE;

            // update attribut and frameInfo (depend from HV flip state)
            sprite->attribut = attr;
            sprite->frameInfo = &(sprite->frame->frameInfos[(attr & (TILE_ATTR_HFLIP_MASK | TILE_ATTR_VFLIP_MASK)) >> TILE_ATTR_HFLIP_SFT]);
        }
    }
    else
    {
        // H flip set
        if (value)
        {
            attr |= TILE_ATTR_HFLIP_MASK;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setHFlip: #", getSpriteIndex(sprite), " added HFlip");
#endif // SPR_DEBUG

            u16 status = sprite->status;

            // need to recompute visibility if auto visibility is enabled
            if (status & SPR_FLAG_AUTO_VISIBILITY)
                status |= NEED_VISIBILITY_UPDATE;

            // need to also recompute complete VDP sprite table
            sprite->status = status | NEED_ST_ALL_UPDATE;

            // update attribut and frameInfo (depend from HV flip state)
            sprite->attribut = attr;
            sprite->frameInfo = &(sprite->frame->frameInfos[(attr & (TILE_ATTR_HFLIP_MASK | TILE_ATTR_VFLIP_MASK)) >> TILE_ATTR_HFLIP_SFT]);
        }
    }

#ifdef SPR_PROFIL
    profil_time[PROFIL_SET_ATTRIBUTE] += getSubTick() - prof;
#endif // SPR_PROFIL
}

void SPR_setVFlip(Sprite* sprite, u16 value)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    u16 attr = sprite->attribut;

    if (attr & TILE_ATTR_VFLIP_MASK)
    {
        // V flip removed
        if (!value)
        {
            attr &= ~TILE_ATTR_VFLIP_MASK;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setVFlip: #", getSpriteIndex(sprite), " removed VFlip");
#endif // SPR_DEBUG

            u16 status = sprite->status;

            // need to recompute visibility if auto visibility is enabled
            if (status & SPR_FLAG_AUTO_VISIBILITY)
                status |= NEED_VISIBILITY_UPDATE;

            // need to also recompute complete VDP sprite table
            sprite->status = status | NEED_ST_ALL_UPDATE;

            // update attribut and frameInfo (depend from HV flip state)
            sprite->attribut = attr;
            sprite->frameInfo = &(sprite->frame->frameInfos[(attr & (TILE_ATTR_HFLIP_MASK | TILE_ATTR_VFLIP_MASK)) >> TILE_ATTR_HFLIP_SFT]);
        }
    }
    else
    {
        // V flip set
        if (value)
        {
            attr |= TILE_ATTR_VFLIP_MASK;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setVFlip: #", getSpriteIndex(sprite), " added VFlip");
#endif // SPR_DEBUG

            u16 status = sprite->status;

            // need to recompute visibility if auto visibility is enabled
            if (status & SPR_FLAG_AUTO_VISIBILITY)
                status |= NEED_VISIBILITY_UPDATE;

            // need to also recompute complete VDP sprite table
            sprite->status = status | NEED_ST_ALL_UPDATE;

            // update attribut and frameInfo (depend from HV flip state)
            sprite->attribut = attr;
            sprite->frameInfo = &(sprite->frame->frameInfos[(attr & (TILE_ATTR_HFLIP_MASK | TILE_ATTR_VFLIP_MASK)) >> TILE_ATTR_HFLIP_SFT]);
        }
    }


#ifdef SPR_PROFIL
    profil_time[PROFIL_SET_ATTRIBUTE] += getSubTick() - prof;
#endif // SPR_PROFIL
}

void SPR_setPriorityAttribut(Sprite* sprite, u16 value)
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
    profil_time[PROFIL_SET_ATTRIBUTE] += getSubTick() - prof;
#endif // SPR_PROFIL
}

void SPR_setPalette(Sprite* sprite, u16 value)
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
    profil_time[PROFIL_SET_ATTRIBUTE] += getSubTick() - prof;
#endif // SPR_PROFIL
}

void SPR_setDepth(Sprite* sprite, s16 value)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

#ifdef SPR_DEBUG
    KLog_U2("SPR_setDepth: #", getSpriteIndex(sprite), "  Depth=", value);
#endif // SPR_DEBUG

    // depth changed ?
    if (sprite->depth != value)
    {
        // set depth and sort sprite (need to be done immediately to get consistent sort)
        sprite->depth = value;
        sortSprite(sprite);
    }

#ifdef SPR_PROFIL
    profil_time[PROFIL_SET_ATTRIBUTE] += getSubTick() - prof;
#endif // SPR_PROFIL
}

void SPR_setZ(Sprite* sprite, s16 value)
{
    SPR_setDepth(sprite, value);
}

void SPR_setAlwaysOnTop(Sprite* sprite, u16 value)
{
    if (value) SPR_setDepth(sprite, SPR_MIN_DEPTH);
}

void SPR_setAnimAndFrame(Sprite* sprite, s16 anim, s16 frame)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    if ((sprite->animInd != anim) || (sprite->seqInd != frame))
    {
#if (LIB_DEBUG != 0)
        if (anim >= sprite->definition->numAnimation)
        {
            KLog_U2("SPR_setAnimAndFrame: error - trying to use non existing animation #", anim, " - num animation = ", sprite->definition->numAnimation);
            return;
        }
#endif // LIB_DEBUG

        Animation* animation = sprite->definition->animations[anim];

#if (LIB_DEBUG != 0)
        if (frame >= animation->length)
        {
            KLog_U3("SPR_setAnimAndFrame: error - trying to use non existing frame #", frame, " for animation #", anim, " - num frame = ", animation->length);
            return;
        }
#endif // LIB_DEBUG

        const u16 frameInd = animation->sequence[frame];

        sprite->animInd = anim;
        sprite->seqInd = frame;
        sprite->animation = animation;
        sprite->frameInd = frameInd;

        // set timer to 0 to prevent auto animation to change frame in between
        sprite->timer = 0;

#ifdef SPR_DEBUG
        KLog_U4("SPR_setAnimAndFrame: #", getSpriteIndex(sprite), " anim=", anim, " frame=", frame, " adj frame=", frameInd);
#endif // SPR_DEBUG

        sprite->status |= NEED_FRAME_UPDATE;
    }

#ifdef SPR_PROFIL
    profil_time[PROFIL_SET_ANIM_FRAME] += getSubTick() - prof;
#endif // SPR_PROFIL
}

void SPR_setAnim(Sprite* sprite, s16 anim)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    if (sprite->animInd != anim)
    {
#if (LIB_DEBUG != 0)
        if (anim >= sprite->definition->numAnimation)
        {
            KLog_U2("SPR_setAnim: error - trying to use non existing animation #", anim, " - num animation = ", sprite->definition->numAnimation);
            return;
        }
#endif // LIB_DEBUG

        Animation *animation = sprite->definition->animations[anim];
        // first frame by default
        const u16 frameInd = animation->sequence[0];

        sprite->animInd = anim;
        sprite->seqInd = 0;
        sprite->animation = animation;
        sprite->frameInd = frameInd;

        // set timer to 0 to prevent auto animation to change frame in between
        sprite->timer = 0;

#ifdef SPR_DEBUG
        KLog_U3("SPR_setAnim: #", getSpriteIndex(sprite), " anim=", anim, " frame=0 adj frame=", frameInd);
#endif // SPR_DEBUG

        sprite->status |= NEED_FRAME_UPDATE;
    }

#ifdef SPR_PROFIL
    profil_time[PROFIL_SET_ANIM_FRAME] += getSubTick() - prof;
#endif // SPR_PROFIL
}

void SPR_setFrame(Sprite* sprite, s16 frame)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    if (sprite->seqInd != frame)
    {
#if (LIB_DEBUG != 0)
        if (frame >= sprite->animation->length)
        {
            KLog_U3("SPR_setFrame: error - trying to use non existing frame #", frame, " for animation #", sprite->animInd, " - num frame = ", sprite->animation->length);
            return;
        }
#endif // LIB_DEBUG

        const u16 frameInd = sprite->animation->sequence[frame];

        sprite->seqInd = frame;

        if (sprite->frameInd != frameInd)
        {
            sprite->frameInd = frameInd;

            // set timer to 0 to prevent auto animation to change frame in between
            sprite->timer = 0;

#ifdef SPR_DEBUG
            KLog_U3("SPR_setFrame: #", getSpriteIndex(sprite), "  frame=", frame, " adj frame=", frameInd);
#endif // SPR_DEBUG

            sprite->status |= NEED_FRAME_UPDATE;
        }
    }

#ifdef SPR_PROFIL
    profil_time[PROFIL_SET_ANIM_FRAME] += getSubTick() - prof;
#endif // SPR_PROFIL
}

void SPR_nextFrame(Sprite* sprite)
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
    profil_time[PROFIL_SET_ANIM_FRAME] += getSubTick() - prof;
#endif // SPR_PROFIL
}

bool SPR_setVRAMTileIndex(Sprite* sprite, s16 value)
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
            profil_time[PROFIL_SET_VRAM_OR_SPRIND] += getSubTick() - prof;
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
            if (newInd < 0)
            {
                // save status and return FALSE
                sprite->status = status;

#ifdef SPR_PROFIL
                profil_time[PROFIL_SET_VRAM_OR_SPRIND] += getSubTick() - prof;
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
        // auto tile upload enabled ? --> need to re upload tile to new location
        if (status & SPR_FLAG_AUTO_TILE_UPLOAD)
            status |= NEED_TILES_UPLOAD;
    }

    // save status
    sprite->status = status;

#ifdef SPR_PROFIL
    profil_time[PROFIL_SET_VRAM_OR_SPRIND] += getSubTick() - prof;
#endif // SPR_PROFIL

    return TRUE;
}

bool SPR_setSpriteTableIndex(Sprite* sprite, s16 value)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    s16 newInd;
    u16 status = sprite->status;
    u16 num = sprite->definition->maxNumSprite;

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
            profil_time[PROFIL_SET_VRAM_OR_SPRIND] += getSubTick() - prof;
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
                profil_time[PROFIL_SET_VRAM_OR_SPRIND] += getSubTick() - prof;
#endif // SPR_PROFIL

                return FALSE;
            }
        }
        // just use the new value for index
        else newInd = value;
    }

    // VDP sprite index changed ?
    if (sprite->VDPSpriteIndex != newInd)
    {
        // set the VDP Sprite index for this sprite and do attached operation
        setVDPSpriteIndex(sprite, newInd, num);
        // need to update complete sprite table infos
        status |= NEED_ST_ALL_UPDATE;
    }

    // save status
    sprite->status = status;

#ifdef SPR_PROFIL
    profil_time[PROFIL_SET_VRAM_OR_SPRIND] += getSubTick() - prof;
#endif // SPR_PROFIL

    return TRUE;
}

void SPR_setAutoTileUpload(Sprite* sprite, bool value)
{
    if (value) sprite->status |= SPR_FLAG_AUTO_TILE_UPLOAD;
    else sprite->status &= ~SPR_FLAG_AUTO_TILE_UPLOAD;
}

void SPR_setDelayedFrameUpdate(Sprite* sprite, bool value)
{
    if (value) sprite->status &= ~SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE;
    else sprite->status |= SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE;
}

void SPR_setFrameChangeCallback(Sprite* sprite, FrameChangeCallback* callback)
{
    sprite->onFrameChange = callback;
}

void SPR_setVisibility(Sprite* sprite, SpriteVisibility value)
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
                status &= ~(SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_FAST_AUTO_VISIBILITY | NEED_VISIBILITY_UPDATE);
                status |= setVisibility(sprite, VISIBILITY_ON);
                break;

            case HIDDEN:
                status &= ~(SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_FAST_AUTO_VISIBILITY | NEED_VISIBILITY_UPDATE);
                status |= setVisibility(sprite, VISIBILITY_OFF);
                break;

            case AUTO_FAST:
                // passed from slow to fast visibility compute method
                if (!(status & SPR_FLAG_FAST_AUTO_VISIBILITY))
                    status |= SPR_FLAG_FAST_AUTO_VISIBILITY | NEED_VISIBILITY_UPDATE;
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
    profil_time[PROFIL_SET_VISIBILITY] += getSubTick() - prof;
#endif // SPR_PROFIL
}

void SPR_setAlwaysVisible(Sprite* sprite, u16 value)
{
    if (value) SPR_setVisibility(sprite, VISIBLE);
}

void SPR_setNeverVisible(Sprite* sprite, u16 value)
{
    if (value) SPR_setVisibility(sprite, HIDDEN);
}

bool SPR_computeVisibility(Sprite* sprite)
{
    u16 status = sprite->status;

    // update visibility if needed
    if (status & NEED_VISIBILITY_UPDATE)
        sprite->status = updateVisibility(sprite, status);

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
    profil_time[PROFIL_CLEAR] += getSubTick() - prof;
#endif // SPR_PROFIL
}

void SPR_update()
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    Sprite* sprite;

#ifdef SPR_DEBUG
    KLog_U1("----------------- SPR_update:  sprite number = ", SPR_getNumActiveSprite());
#endif // SPR_DEBUG

    // disable interrupts (we want to avoid DMA queue process when executing this method)
    SYS_disableInts();

#ifdef SPR_DEBUG
    KLog_U1_("  Send sprites to DMA queue: ", highestVDPSpriteIndex + 1, " sprite(s) sent");
#endif // SPR_DEBUG

    const u16 sprNum = highestVDPSpriteIndex + 1;

    // send sprites to VRAM using DMA queue (better to do it before sprite tiles upload to avoid being ignored by DMA queue)
    DMA_queueDma(DMA_VRAM, (u32) vdpSpriteCacheQueue, VDP_SPRITE_TABLE, (sizeof(VDPSprite) / 2) * sprNum, 2);

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

        u16 status = sprite->status;

        // trivial optimization
        if (status & NEED_UPDATE)
        {
            // ! order is important !
            if (status & NEED_FRAME_UPDATE)
                status = updateFrame(sprite, status);
            if (status & NEED_VISIBILITY_UPDATE)
                status = updateVisibility(sprite, status);

            // sprite not visible ?
            if (!sprite->visibility)
            {
                // need to update its visibility (done via pos Y) ?
                if (status & NEED_ST_POS_UPDATE)
                {
                    // update position (and so visibility)
                    updateSpriteTablePos(sprite);
                    status &= ~NEED_ST_POS_UPDATE;
                }
            }
            // only if sprite is visible
            else
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

        // next sprite
        sprite = sprite->next;
    }

    // VDP sprite cache is now updated, copy it to the queue cache copy
    memcpy(vdpSpriteCacheQueue, vdpSpriteCache, sizeof(VDPSprite) * sprNum);

    // reset unpack buffer address
    unpackNext = unpackBuffer;

    // re-enable interrupts
    SYS_enableInts();

#ifdef SPR_PROFIL
    profil_time[PROFIL_UPDATE] += getSubTick() - prof;
#endif // SPR_PROFIL
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

    KLog_U1_("Num sprite = ", SPR_getNumActiveSprite(), " -----------------------------");
    while(sprite)
    {
        logSprite(sprite);
        sprite = sprite->next;
    }
}


static void setVDPSpriteIndex(Sprite* sprite, u16 ind, u16 num)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    Sprite* spr;
    VDPSprite* vdpSprite;
    u16 i;

#ifdef SPR_DEBUG
    KLog_U2("setVDPSpriteIndex: sprite #", getSpriteIndex(sprite), "  new VDP Sprite index = ", ind);
#endif // SPR_DEBUG

    sprite->VDPSpriteIndex = ind;

    // hide all sprites by default and get last sprite
    vdpSprite = &vdpSpriteCache[ind];
    vdpSprite->y = 0;

    // get the last vdpSprite while hiding them
    i = num - 1;
    while(i--)
    {
        vdpSprite = &vdpSpriteCache[vdpSprite->link];
        vdpSprite->y = 0;
    }

    // adjust VDP sprites links
    spr = sprite->prev;
    // do we have a previous sprite ? --> set its next link to current sprite index
    if (spr) spr->lastVDPSprite->link = ind;
    // othrwise we set started link
    else starter->link = ind;

    spr = sprite->next;
    // do we have a next sprite ? --> set link on next sprite
    if (spr) vdpSprite->link = spr->VDPSpriteIndex;
    // last sprite
    else vdpSprite->link = 0;

    // set last VDP sprite pointer for this sprite
    sprite->lastVDPSprite = vdpSprite;

#ifdef SPR_DEBUG
    KLog_U1("  last VDP sprite = ", sprite->lastVDPSprite - vdpSpriteCache);
#endif // SPR_DEBUG

#ifdef SPR_PROFIL
    profil_time[PROFIL_UPDATE_VDPSPRIND] += getSubTick() - prof;
#endif // SPR_PROFIL
}

static u16 updateVisibility(Sprite* sprite, u16 status)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    u16 visibility;
    AnimationFrame* frame = sprite->frame;

    // fast visibility computation ?
    if (status & SPR_FLAG_FAST_AUTO_VISIBILITY)
    {
        const s16 x = sprite->x;
        const s16 y = sprite->y;

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
        // xmin relative to sprite pos
        const s16 xmin = 0x80 - sprite->x;
        // ymin relative to sprite pos
        const s16 ymin = 0x80 - sprite->y;
        // xmax relative to sprite pos
        const s16 xmax = screenWidth + xmin;
        // ymax relative to sprite pos
        const s16 ymax = screenHeight + ymin;
        const s16 fw = frame->w;
        const s16 fh = frame->h;

#ifdef SPR_DEBUG
        KLog_S2("  updateVisibility (slow): global x=", sprite->x, " global y=", sprite->y);
        KLog_S2("    frame w=", fw, " h=", fh);
        KLog_S4("    xmin=", xmin, " xmax=", xmax, " ymin=", ymin, " ymax=", ymax);
#endif // SPR_DEBUG

        // sprite is fully visible ? --> set all sprite visible
        if ((xmin <= 0) && (xmax >= fw) && (ymin <= 0) && (ymax >= fh)) visibility = VISIBILITY_ON;
        // sprite is fully hidden ? --> set all sprite to hidden
        else if ((xmax < 0) || ((xmin - fw) > 0) || (ymax < 0) || ((ymin - fh) > 0)) visibility = VISIBILITY_OFF;
        else
        {
            FrameVDPSprite** frameSprites;
            u16 num;

            num = frame->numSprite;
            // start from the last one
            frameSprites = &(sprite->frameInfo->frameVDPSprites[num - 1]);
            visibility = 0;

            while(num--)
            {
                FrameVDPSprite* frameSprite;
                u16 size;
                s16 x, y;
                s16 w, h;

                frameSprite = *frameSprites--;

                // Y first to respect frameSprite field order
                y = frameSprite->offsetY;
                size = frameSprite->size;
                w = ((size & 0x0C) << 1) + 8;
                h = ((size & 0x03) << 3) + 8;
                x = frameSprite->offsetX;

    #ifdef SPR_DEBUG
                KLog_S4("    frameSprite offX=", frameSprite->offsetX, " offY=", frameSprite->offsetY, " w=", w, " h=", h);
                KLog_S3("    size=", size, " adjX=", x, " adjY=", y);
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
    profil_time[PROFIL_UPDATE_VISIBILITY] += getSubTick() - prof;
#endif // SPR_PROFIL

    // set the new computed visibility
    status |= setVisibility(sprite, visibility);

    // visibility update done !
    return status & ~NEED_VISIBILITY_UPDATE;
}

static u16 setVisibility(Sprite* sprite, u16 newVisibility)
{
    // visibility changed ?
    if (sprite->visibility != newVisibility)
    {
        // set new visibility info
        sprite->visibility = newVisibility;

        // need to recompute the position info (hidding a sprite is done by setting posY to 0)
        return NEED_ST_POS_UPDATE;
    }

    return 0;
}

static u16 updateFrame(Sprite* sprite, u16 status)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

#ifdef SPR_DEBUG
    KLog_U1("  updateFrame: sprite #", getSpriteIndex(sprite));
#endif // SPR_DEBUG

    AnimationFrame* frame = sprite->animation->frames[sprite->frameInd];

    // we need to transfert tiles data for this sprite and frame delay is not disabled ?
    if ((status & (SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE)) == SPR_FLAG_AUTO_TILE_UPLOAD)
    {
        // not enough DMA capacity to transfer sprite tile data ?
        const u16 dmaCapacity = DMA_getMaxTransferSize();

        if (dmaCapacity && (DMA_getQueueTransferSize() + (frame->tileset->numTile * 32)) > dmaCapacity)
        {
#if (LIB_DEBUG != 0)
            KLog_U3_("Warning: sprite #", getSpriteIndex(sprite), " update delayed (exceeding DMA capacity: ", DMA_getQueueTransferSize(), " bytes already queued and require ", frame->tileset->numTile * 32, " more bytes)");
#endif // LIB_DEBUG

            // delay frame update (when we will have enough DMA capacity to do it)
            return status;
        }
    }

    // detect if we need to hide some VDP sprite
    s16 spriteToHide = sprite->frame->numSprite - frame->numSprite;

    if (spriteToHide > 0)
        sprite->spriteToHide = spriteToHide;

    // set frame
    sprite->frame = frame;
    // get frame info depending HV flip state
    sprite->frameInfo = &(frame->frameInfos[(sprite->attribut & (TILE_ATTR_HFLIP_MASK | TILE_ATTR_VFLIP_MASK)) >> TILE_ATTR_HFLIP_SFT]);

    // init timer for this frame
    sprite->timer = frame->timer;

    // frame change event handler defined ? --> call it
    if (sprite->onFrameChange) sprite->onFrameChange(sprite);

    // require tile data upload
    if (status & SPR_FLAG_AUTO_TILE_UPLOAD)
        status |= NEED_TILES_UPLOAD;
    // require visibility update
    if (status & SPR_FLAG_AUTO_VISIBILITY)
        status |= NEED_VISIBILITY_UPDATE;

    // frame update done
    status &= ~NEED_FRAME_UPDATE;

#ifdef SPR_PROFIL
    profil_time[PROFIL_UPDATE_FRAME] += getSubTick() - prof;
#endif // SPR_PROFIL

    // need to update all sprite table
    return status | NEED_ST_ALL_UPDATE;
}

static void updateSpriteTableAll(Sprite* sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    FrameVDPSprite** frameSprites;
    VDPSprite* vdpSprite;
    u16 attr;
    u16 num;
    u16 visibility;

    visibility = sprite->visibility;
    attr = sprite->attribut;
    num = sprite->frame->numSprite;
    frameSprites = sprite->frameInfo->frameVDPSprites;
    vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    while(num--)
    {
        FrameVDPSprite* frameSprite = *frameSprites++;

        // Y first to respect VDP field order
        if (visibility & 1) vdpSprite->y = sprite->y + frameSprite->offsetY;
        else vdpSprite->y = 0;
        vdpSprite->size = frameSprite->size;
        vdpSprite->attribut = attr;
        vdpSprite->x = sprite->x + frameSprite->offsetX;

        // increment tile index in attribut field
        attr += frameSprite->numTile;
        // next VDP sprite
        visibility >>= 1;
        vdpSprite = &vdpSpriteCache[vdpSprite->link];
    }

    // hide sprites that were used by previous frame
    if ((num = sprite->spriteToHide))
    {
        while(num--)
        {
            vdpSprite->y = 0;
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }

        sprite->spriteToHide = 0;
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
    profil_time[PROFIL_UPDATE_SPRITE_TABLE] += getSubTick() - prof;
#endif // SPR_PROFIL
}

static void updateSpriteTablePos(Sprite* sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    FrameVDPSprite** frameSprites;
    VDPSprite* vdpSprite;
    u16 num;
    u16 visibility;

    visibility = sprite->visibility;
    num = sprite->frame->numSprite;
    frameSprites = sprite->frameInfo->frameVDPSprites;
    vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    while(num--)
    {
        FrameVDPSprite* frameSprite = *frameSprites++;

        // Y first to respect VDP field order
        if (visibility & 1) vdpSprite->y = sprite->y + frameSprite->offsetY;
        else vdpSprite->y = 0;
        vdpSprite->x = sprite->x + frameSprite->offsetX;

        // pass to next VDP sprite
        visibility >>= 1;
        vdpSprite = &vdpSpriteCache[vdpSprite->link];
    }

    // hide sprites that were used by previous frame
    if ((num = sprite->spriteToHide))
    {
        while(num--)
        {
            vdpSprite->y = 0;
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }

        sprite->spriteToHide = 0;
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
    profil_time[PROFIL_UPDATE_SPRITE_TABLE] += getSubTick() - prof;
#endif // SPR_PROFIL
}

static void updateSpriteTableAttr(Sprite* sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    FrameVDPSprite** frameSprites;
    VDPSprite* vdpSprite;
    u16 attr;
    u16 num;

    attr = sprite->attribut;
    num = sprite->frame->numSprite;
    frameSprites = sprite->frameInfo->frameVDPSprites;
    vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

#ifdef SPR_DEBUG
    KLog_U3("  updateSpriteTableAttr_allVisible: numSprite=", sprite->frame->numSprite, " visibility=", sprite->visibility, " VDPSprIndex=", sprite->VDPSpriteIndex);
#endif // SPR_DEBUG

    while(num--)
    {
        FrameVDPSprite* frameSprite = *frameSprites++;

        vdpSprite->attribut = attr;

        // increment tile index in attribut field
        attr += frameSprite->numTile;
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
    profil_time[PROFIL_UPDATE_SPRITE_TABLE] += getSubTick() - prof;
#endif // SPR_PROFIL
}

static void loadTiles(Sprite* sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    TileSet* tileset = sprite->frame->tileset;
    u16 compression = tileset->compression;
    u16 lenInWord = (tileset->numTile * 32) / 2;

    // TODO: separate tileset per VDP sprite and only unpack/upload visible VDP sprite (using visibility) to VRAM

    // need unpacking ?
    if (compression != COMPRESSION_NONE)
    {
        // unpack
        unpack(compression, (u8*) FAR(tileset->tiles), unpackNext);
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
        DMA_queueDma(DMA_VRAM, (u32) FAR(tileset->tiles), (sprite->attribut & TILE_INDEX_MASK) * 32, lenInWord, 2);

#ifdef SPR_DEBUG
        KLog_U3("  loadTiles - queue DMA: from=", (u32) tileset->tiles, " to=", (sprite->attribut & TILE_INDEX_MASK) * 32, " size in word=", lenInWord);
#endif // SPR_DEBUG
    }

#ifdef SPR_PROFIL
    profil_time[PROFIL_LOADTILES] += getSubTick() - prof;
#endif // SPR_PROFIL
}

static Sprite* sortSprite(Sprite* sprite)
{
#ifdef SPR_PROFIL
    s32 prof = getSubTick();
#endif // SPR_PROFIL

    Sprite* const prev = sprite->prev;
    Sprite* const next = sprite->next;
    Sprite* s;

    // cache sprite depth coordinate
    const s16 sdepth = sprite->depth;

#ifdef SPR_DEBUG
    KLog_U2("Start depth compare for sprite #", getSpriteIndex(sprite), " VDP Sprite Ind=", sprite->VDPSpriteIndex);
#endif // SPR_DEBUG

    // find position forward first
    s = next;
    while(s && (s->depth < sdepth)) s = s->next;
    // position changed ? --> insert sprite after s->prev (as s is pointing on 'next')
    if (s != next) moveAfter(s?s->prev:lastSprite, sprite);
    else
    {
        // try to find position backward then
        s = prev;
        while(s && (s->depth > sdepth)) s = s->prev;
        // position changed ? --> insert sprite after s
        if (s != prev) moveAfter(s, sprite);
    }

#ifdef SPR_DEBUG
    if (s) KLog_U1("Position for sprite = ", getSpriteIndex(s) + 1);
    else KLog_U1("Position for sprite = ", 0);
#endif // SPR_DEBUG


#ifdef SPR_PROFIL
    profil_time[PROFIL_SORT] += getSubTick() - prof;
#endif // SPR_PROFIL

    // return prev just for convenience on full sorting
    return prev;
}

static void moveAfter(Sprite* pos, Sprite* sprite)
{
    Sprite* prev = sprite->prev;
    Sprite* next = sprite->next;

#ifdef SPR_DEBUG
    if (pos) KLog_U2("Insert #", getSpriteIndex(sprite), "  after #", getSpriteIndex(pos));
    else KLog_U1_("Insert #", getSpriteIndex(sprite), "  at #0");
#endif // SPR_DEBUG

    // we first remove the sprite from its current position
    if (prev)
    {
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
        // 'next' become the first sprite
        firstSprite = next;
        if (next)
        {
            next->prev = prev;
            // fix sprite link from previous sprite
            starter->link = next->VDPSpriteIndex;
        }
        else
        {
            // no more sprite (both firstSprite and lastSprite == NULL)
            lastSprite = prev;
            // fix sprite link from previous sprite
            starter->link = 0;
        }
    }

    // then we re-insert after 'pos'
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
    // or we insert before 'firstSprite' (become the new first sprite)
    else
    {
        sprite->next = firstSprite;
        sprite->prev = NULL;
        // sprite become the preceding sprite of previous first sprite
        if (firstSprite) firstSprite->prev = sprite;
        // no previous first sprite ? --> sprite becomes last sprite then
        else lastSprite = sprite;
        // sprite become first sprite
        firstSprite = sprite;
        // fix sprite link
        sprite->lastVDPSprite->link = starter->link;
        starter->link = sprite->VDPSpriteIndex;
    }
}

static u16 getSpriteIndex(Sprite* sprite)
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

static void logSprite(Sprite* sprite)
{
    KLog_U2("Sprite #", getSpriteIndex(sprite), " ------------- status=", sprite->status);
    KLog_U3("animInd=", sprite->animInd, " seqInd=", sprite->seqInd, " frameInd=", sprite->frameInd);
    KLog_S4("attribut=", sprite->attribut, " x=", sprite->x, " y=", sprite->y, " depth=", sprite->depth);
    KLog_U2("visibility=", sprite->visibility, " timer=", sprite->timer);
    KLog_U2("VDPSpriteInd=", sprite->VDPSpriteIndex, " link=", sprite->lastVDPSprite->link);
    KLog_U2("prev=", (sprite->prev==NULL)?128:getSpriteIndex(sprite->prev), " next=", (sprite->next==NULL)?128:getSpriteIndex(sprite->next));
}
