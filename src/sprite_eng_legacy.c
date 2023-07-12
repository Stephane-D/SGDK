#include "config.h"
#include "types.h"

#if      LEGACY_SPRITE_ENGINE

#include "sprite_eng_legacy.h"

#include "sys.h"
#include "vdp.h"
#include "vdp_spr.h"
#include "dma.h"
#include "memory.h"
#include "pool.h"
#include "vram.h"
#include "tools.h"
#include "mapper.h"

#include "kdebug.h"
#include "string.h"
#include "timer.h"


//#define SPR_DEBUG
//#define SPR_PROFIL

#ifdef SPR_PROFIL
#define START_PROFIL      s32 prof = getSubTick();
#define END_PROFIL(x)     profil_time[x] += getSubTick() - prof;
#else
#define START_PROFIL
#define END_PROFIL(x)
#endif // SPR_PROFIL

// first hardware sprite is reserved (used internally for sorting)
#define MAX_SPRITE                          (80 - 1)

// internals
#define VISIBILITY_ON                       0xFFFF
#define VISIBILITY_OFF                      0x0000

#define ALLOCATED                           0x8000

#define NEED_ST_POS_UPDATE                  0x0001
#define NEED_ST_ALL_UPDATE                  0x0002
#define NEED_ST_UPDATE                      0x0003
#define NEED_VISIBILITY_UPDATE              0x0004
#define NEED_FRAME_UPDATE                   0x0008
#define NEED_TILES_UPLOAD                   0x0010

#define NEED_UPDATE                         0x001F


// shared from vdp_spr.c unit
extern void logVDPSprite(u16 index);
// shared from vdp.c unit
extern void updateUserTileMaxIndex();


// forward
static Sprite* allocateSprite(u16 head);
static bool releaseSprite(Sprite* sprite);

static void setVDPSpriteIndex(Sprite* sprite, u16 ind, u16 num);
static u16 updateVisibility(Sprite* sprite, u16 status);
static u16 setVisibility(Sprite* sprite, u16 visibility);
static u16 updateFrame(Sprite* sprite, u16 status);

static void updateSpriteTableAll(Sprite* sprite);
static void updateSpriteTablePos(Sprite* sprite);
static void updateSpriteTableHide(Sprite* sprite);

static void loadTiles(Sprite* sprite);
static Sprite* sortSprite(Sprite* sprite);
static void moveAfter(Sprite* pos, Sprite* sprite);
static u16 getSpriteIndex(Sprite* sprite);
static void logSprite(Sprite* sprite);

// starter VDP sprite - never visible (used for sprite sorting)
static VDPSprite* starter;

// pool of Sprite objects
Pool* spritesPool;

// pointer on first and last active sprite in the linked list
Sprite* firstSprite;
Sprite* lastSprite;

// VRAM region allocated for the Sprite Engine
static VRAMRegion vram;

// size of VRAM allocated for Sprite Engine
u16 spriteVramSize;

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


void NO_INLINE SPR_initEx(u16 vramSize)
{
    u16 index;
    u16 size;

    // end it first (if initialized)
    SPR_end();

    // create sprites object pool
    spritesPool = POOL_create(MAX_SPRITE, sizeof(Sprite));

    size = vramSize ? vramSize : 420;
    // get start tile index for sprite data (reserve VRAM area just before system font)
    index = TILE_FONT_INDEX - size;

    // and create a VRAM region for sprite tile allocation
    VRAM_createRegion(&vram, index, size);
    // store allocated VRAM size to let SGDK know about it
    spriteVramSize = size;

    // need to update user tile max index
    updateUserTileMaxIndex();

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    KLog("Sprite engine initialized !");
    KLog_U2_("  VRAM region: [", index, " - ", index + (size - 1), "]");
#endif // LIB_DEBUG

    // reset
    SPR_reset();
}

void SPR_init()
{
    SPR_initEx(420);
}

void SPR_end()
{
    if (SPR_isInitialized())
    {
        // reset and clear VDP sprite
        VDP_resetSprites();
        VDP_updateSprites(1, DMA_QUEUE_COPY);

        // release memory
        POOL_destroy(spritesPool);
        spritesPool = NULL;
        VRAM_releaseRegion(&vram);
        spriteVramSize = 0;

        // need to update user tile max index
        updateUserTileMaxIndex();

        // try to pack memory free blocks (before to avoid memory fragmentation)
        MEM_pack();

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
        KLog("Sprite engine ended !");
#endif
    }
}

bool SPR_isInitialized()
{
    return (spritesPool != NULL);
}

void SPR_reset()
{
    // release all and clear sprites data
    POOL_reset(spritesPool, TRUE);

    // no active sprites
    firstSprite = NULL;
    lastSprite = NULL;

    // clear VRAM region
    VRAM_clearRegion(&vram);
    // reset VDP sprite (allocation and display)
    VDP_resetSprites();

    // we reserve sprite 0 for sorting (cannot be used for display)
    starter = &vdpSpriteCache[VDP_allocateSprites(1)];
    // hide it (should be already done by VDP_resetSprites)
    starter->y = 0;

#ifdef SPR_PROFIL
    memset(profil_time, 0, sizeof(profil_time));
#endif // SPR_PROFIL

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    KLog("Sprite engine reset");
    KLog_U1("  VRAM region free: ", VRAM_getFree(&vram));
    KLog_U1("  Available VDP sprites: ", VDP_getAvailableSprites());
#endif // LIB_DEBUG
}


static Sprite* allocateSprite(u16 head)
{
    Sprite* result;

    // allocate
    result = POOL_allocate(spritesPool);
    // enough sprite remaining ?
    if (result == NULL)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        KLog("allocateSprite(): failed - no more available sprite !");
#endif

        return NULL;
    }

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    kprintf("allocateSprite(): success - allocating sprite at pos %d", POOL_find(spritesPool, result));
#endif // LIB_DEBUG

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
    START_PROFIL

    // really allocated ?
    if (sprite->status & ALLOCATED)
    {
        Sprite* prev;
        Sprite* next;
        VDPSprite* lastVDPSprite;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
        kprintf("releaseSprite: success - released sprite at pos %d", POOL_find(spritesPool, sprite));
#endif // LIB_DEBUG

        // release sprite (we don't need stack coherency here as we don't use stack iteration)
        POOL_release(spritesPool, sprite, FALSE);

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

        END_PROFIL(PROFIL_RELEASE_SPRITE)

        return TRUE;
    }

#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    kprintf("SPR_internalReleaseSprite: failed - sprite at address %p is not allocated !", sprite);
#endif // LIB_DEBUG

    END_PROFIL(PROFIL_RELEASE_SPRITE)

    return FALSE;
}

static bool isSpriteValid(Sprite* sprite, char* methodName)
{
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    if (!(sprite->status & ALLOCATED))
    {
        kprintf("%s: error - sprite at address %p is invalid (not allocated) !", methodName, sprite);
        return FALSE;
    }
#endif

    return TRUE;
}

Sprite* NO_INLINE SPR_addSpriteEx(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut, u16 spriteIndex, u16 flag)
{
    START_PROFIL

    s16 ind;
    Sprite* sprite;

    // allocate new sprite
    sprite = allocateSprite(flag & SPR_FLAG_INSERT_HEAD);

    // can't allocate --> return NULL
    if (!sprite)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
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
    sprite->frame = NULL;

    sprite->animInd = -1;
    sprite->frameInd = -1;
//    sprite->seqInd = -1;

    sprite->x = x + 0x80;
    sprite->y = y + 0x80;
    // depending sprite position (first or last) we set its default depth
    if (flag & SPR_FLAG_INSERT_HEAD) sprite->depth = SPR_MIN_DEPTH;
    else sprite->depth = SPR_MAX_DEPTH;

    const u16 numVDPSprite = spriteDef->maxNumSprite;

    // default on sprite init
    sprite->lastNumSprite = numVDPSprite;
    sprite->spriteToHide = 0;

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

    END_PROFIL(PROFIL_ADD_SPRITE)

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
    START_PROFIL

#ifdef SPR_DEBUG
    KLog_U2("SPR_releaseSprite: releasing sprite #", getSpriteIndex(sprite), " - internal position = ", sprite - spritesBank);
#endif // SPR_DEBUG

    // release sprite
    if (!releaseSprite(sprite))
    {
        END_PROFIL(PROFIL_REMOVE_SPRITE)

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

    END_PROFIL(PROFIL_REMOVE_SPRITE)
}

u16 SPR_getNumActiveSprite()
{
    return POOL_getNumAllocated(spritesPool);
}

void NO_INLINE SPR_defragVRAM()
{
    START_PROFIL

    Sprite* sprite;

    // release VRAM region
    VRAM_releaseRegion(&vram);
    // pack
    MEM_pack();
    // and re-create it (useful if TILE_FONT_INDEX changed, when we modify plane size for instance)
    VRAM_createRegion(&vram, TILE_FONT_INDEX - spriteVramSize, spriteVramSize);

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

                // need to update VDP sprite table
                status |= NEED_ST_ALL_UPDATE;
                // auto tile upload enabled ? --> need to re upload tile to new location
                if (status & SPR_FLAG_AUTO_TILE_UPLOAD)
                    status |= NEED_TILES_UPLOAD;

                sprite->status = status;
            }
        }

        // next sprite
        sprite = sprite->next;
    }

    END_PROFIL(PROFIL_VRAM_DEFRAG)
}

u16** NO_INLINE SPR_loadAllFrames(const SpriteDefinition* sprDef, u16 index, u16* totalNumTile)
{
    u16 numFrameTot = 0;
    u16 numTileTot = 0;
    Animation** anim = sprDef->animations;
    const u16 numAnimation = sprDef->numAnimation;

    for(u16 indAnim = 0; indAnim < numAnimation; indAnim++)
    {
        AnimationFrame** frame = (*anim)->frames;
        const u16 numFrame = (*anim)->numFrame;

        for(u16 indFrame = 0; indFrame < numFrame; indFrame++)
        {
            numTileTot += (*frame)->tileset->numTile;
            frame++;
        }

        numFrameTot += numFrame;
        anim++;
    }

    // store total num tile if needed
    if (totalNumTile) *totalNumTile = numTileTot;

    // allocate result table indexes[numAnim][numFrame]
    u16** indexes = MEM_alloc((numAnimation * sizeof(u16*)) + (numFrameTot * sizeof(u16)));
    // store pointer
    u16** result = indexes;
    // init frames indexes pointer
    u16* indFrames = (u16*) (indexes + numAnimation);

    // start index
    u16 tileInd = index;

    anim = sprDef->animations;

    for(u16 indAnim = 0; indAnim < numAnimation; indAnim++)
    {
        // store frames indexes pointer for this animation
        *indexes++ = indFrames;

        AnimationFrame** frame = (*anim)->frames;
        const u16 numFrame = (*anim)->numFrame;

        for(u16 indFrame = 0; indFrame < numFrame; indFrame++)
        {
            const TileSet* tileset = (*frame)->tileset;

            // load tileset
            VDP_loadTileSet(tileset, tileInd, DMA);
            // store frame tile index
            *indFrames++ = tileInd;
            // next tileset
            tileInd += tileset->numTile;
            // next frame
            frame++;
        }

        anim++;
    }

    return result;
}


bool NO_INLINE SPR_setDefinition(Sprite* sprite, const SpriteDefinition* spriteDef)
{
    START_PROFIL

#ifdef SPR_DEBUG
    KLog_U1("SPR_setDefinition: #", getSpriteIndex(sprite));
#endif // SPR_DEBUG

    if (!isSpriteValid(sprite, "SPR_setDefinition"))
        return FALSE;

    // nothing to do...
    if (sprite->definition == spriteDef) return TRUE;

    u16 status = sprite->status;
    const u16 oldNumSprite = sprite->definition->maxNumSprite;
    const u16 newNumSprite = spriteDef->maxNumSprite;

    // definition changed --> re-init that
    sprite->lastNumSprite = newNumSprite;
    sprite->spriteToHide = 0;

    // auto VDP sprite alloc enabled and sprite number changed ? --> realloc VDP sprite(s)
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
//    FIXME: not needed
//    sprite->animation = NULL;
    sprite->frame = NULL;
    sprite->animInd = -1;
    sprite->frameInd = -1;
//    sprite->seqInd = -1;

    // set anim and frame to 0
    SPR_setAnimAndFrame(sprite, 0, 0);

    END_PROFIL(PROFIL_SET_DEF)

    return TRUE;
}

s16 SPR_getPositionX(Sprite* sprite)
{
    return sprite->x - 0x80;
}

s16 SPR_getPositionY(Sprite* sprite)
{
    return sprite->y - 0x80;
}

void SPR_setPosition(Sprite* sprite, s16 x, s16 y)
{
    START_PROFIL

    const s16 newx = x + 0x80;
    const s16 newy = y + 0x80;

#ifdef SPR_DEBUG
    KLog_U3("SPR_setPosition: #", getSpriteIndex(sprite), "  X=", newx, " Y=", newy);
#endif // SPR_DEBUG

    if (!isSpriteValid(sprite, "SPR_setPosition"))
        return;

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

    END_PROFIL(PROFIL_SET_ATTRIBUTE)
}

void SPR_setHFlip(Sprite* sprite, bool value)
{
    START_PROFIL

    if (!isSpriteValid(sprite, "SPR_setHFlip"))
        return;

    u16 attr = sprite->attribut;

    if (attr & TILE_ATTR_HFLIP_MASK)
    {
        // H flip removed
        if (!value)
        {
            // update attribut
            sprite->attribut = attr & ~TILE_ATTR_HFLIP_MASK;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setHFlip: #", getSpriteIndex(sprite), " removed HFlip");
#endif // SPR_DEBUG

            // need to recompute visibility if auto visibility is enabled
            if (sprite->status & SPR_FLAG_AUTO_VISIBILITY)
                sprite->status |= NEED_VISIBILITY_UPDATE | NEED_ST_ALL_UPDATE;
            else
                sprite->status |= NEED_ST_ALL_UPDATE;
        }
    }
    else
    {
        // H flip set
        if (value)
        {
            // update attribut
            sprite->attribut = attr | TILE_ATTR_HFLIP_MASK;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setHFlip: #", getSpriteIndex(sprite), " added HFlip");
#endif // SPR_DEBUG

            // need to recompute visibility if auto visibility is enabled
            if (sprite->status & SPR_FLAG_AUTO_VISIBILITY)
                sprite->status |= NEED_VISIBILITY_UPDATE | NEED_ST_ALL_UPDATE;
            else
                sprite->status |= NEED_ST_ALL_UPDATE;
        }
    }

    END_PROFIL(PROFIL_SET_ATTRIBUTE)
}

void SPR_setVFlip(Sprite* sprite, bool value)
{
    START_PROFIL

    if (!isSpriteValid(sprite, "SPR_setVFlip"))
        return;

    u16 attr = sprite->attribut;

    if (attr & TILE_ATTR_VFLIP_MASK)
    {
        // V flip removed
        if (!value)
        {
            // update attribut
            sprite->attribut = attr & ~TILE_ATTR_VFLIP_MASK;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setVFlip: #", getSpriteIndex(sprite), " removed VFlip");
#endif // SPR_DEBUG

            // need to recompute visibility if auto visibility is enabled
            if (sprite->status & SPR_FLAG_AUTO_VISIBILITY)
                sprite->status |= NEED_VISIBILITY_UPDATE | NEED_ST_ALL_UPDATE;
            else
                sprite->status |= NEED_ST_ALL_UPDATE;
        }
    }
    else
    {
        // V flip set
        if (value)
        {
            // update attribut
            sprite->attribut = attr | TILE_ATTR_VFLIP_MASK;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setVFlip: #", getSpriteIndex(sprite), " added VFlip");
#endif // SPR_DEBUG

            // need to recompute visibility if auto visibility is enabled
            if (sprite->status & SPR_FLAG_AUTO_VISIBILITY)
                sprite->status |= NEED_VISIBILITY_UPDATE | NEED_ST_ALL_UPDATE;
            else
                sprite->status |= NEED_ST_ALL_UPDATE;
        }
    }

    END_PROFIL(PROFIL_SET_ATTRIBUTE)
}

void SPR_setPriority(Sprite* sprite, bool value)
{
    START_PROFIL

    if (!isSpriteValid(sprite, "SPR_setPriority"))
        return;

    const u16 attr = sprite->attribut;

    if (attr & TILE_ATTR_PRIORITY_MASK)
    {
        // priority removed
        if (!value)
        {
            sprite->attribut = attr & (~TILE_ATTR_PRIORITY_MASK);
            sprite->status |= NEED_ST_ALL_UPDATE;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setPriority: #", getSpriteIndex(sprite), " removed priority");
#endif // SPR_DEBUG
        }
    }
    else
    {
        // priority set
        if (value)
        {
            sprite->attribut = attr | TILE_ATTR_PRIORITY_MASK;
            sprite->status |= NEED_ST_ALL_UPDATE;

#ifdef SPR_DEBUG
            KLog_U1_("SPR_setPriority: #", getSpriteIndex(sprite), " added priority");
#endif // SPR_DEBUG
        }
    }

    END_PROFIL(PROFIL_SET_ATTRIBUTE)
}

void SPR_setPalette(Sprite* sprite, u16 value)
{
    START_PROFIL

    if (!isSpriteValid(sprite, "SPR_setPalette"))
        return;

    const u16 oldAttribut = sprite->attribut;
    const u16 newAttribut = (oldAttribut & (~TILE_ATTR_PALETTE_MASK)) | (value << TILE_ATTR_PALETTE_SFT);

    if (oldAttribut != newAttribut)
    {
        sprite->attribut = newAttribut;
        // need to update VDP sprite attribut field only
        sprite->status |= NEED_ST_ALL_UPDATE;

#ifdef SPR_DEBUG
        KLog_U2("SPR_setPalette: #", getSpriteIndex(sprite), " palette=", value);
#endif // SPR_DEBUG
    }

    END_PROFIL(PROFIL_SET_ATTRIBUTE)
}

void SPR_setDepth(Sprite* sprite, s16 value)
{
    START_PROFIL

    if (!isSpriteValid(sprite, "SPR_setDepth"))
        return;

    // depth changed ?
    if (sprite->depth != value)
    {
#ifdef SPR_DEBUG
        KLog_U2("SPR_setDepth: #", getSpriteIndex(sprite), "  Depth=", value);
#endif // SPR_DEBUG

        // set depth and sort sprite (need to be done immediately to get consistent sort)
        sprite->depth = value;
        sortSprite(sprite);
    }

    END_PROFIL(PROFIL_SET_ATTRIBUTE)
}

void SPR_setZ(Sprite* sprite, s16 value)
{
    SPR_setDepth(sprite, value);
}

void SPR_setAlwaysOnTop(Sprite* sprite)
{
    SPR_setDepth(sprite, SPR_MIN_DEPTH);
}

void SPR_setAnimAndFrame(Sprite* sprite, s16 anim, s16 frame)
{
    START_PROFIL

    if (!isSpriteValid(sprite, "SPR_setAnimAndFrame"))
        return;

    if ((sprite->animInd != anim) || (sprite->frameInd != frame))
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        if (anim >= (s16) sprite->definition->numAnimation)
        {
            kprintf("SPR_setAnimAndFrame: error - trying to use non existing animation #%d on definition=%p - num animation = %d", anim, sprite->definition, sprite->definition->numAnimation);
            return;
        }
#endif // LIB_DEBUG

        Animation* animation = sprite->definition->animations[anim];

#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        if (frame >= (s16) animation->numFrame)
        {
            kprintf("SPR_setAnimAndFrame: error - trying to use non existing frame #%d for animation %d on definition=%p - num frame = %d", frame, anim, sprite->definition, animation->numFrame);
            return;
        }
#endif // LIB_DEBUG

        sprite->animInd = anim;
        sprite->frameInd = frame;
        sprite->animation = animation;

        // set timer to 0 to prevent auto animation to change frame in between
        sprite->timer = 0;

#ifdef SPR_DEBUG
        KLog_U3("SPR_setAnimAndFrame: #", getSpriteIndex(sprite), " anim=", anim, " frame=", frame);
#endif // SPR_DEBUG

        sprite->status |= NEED_FRAME_UPDATE;
    }

    END_PROFIL(PROFIL_SET_ANIM_FRAME)
}

void SPR_setAnim(Sprite* sprite, s16 anim)
{
    START_PROFIL

    if (!isSpriteValid(sprite, "SPR_setAnim"))
        return;

    if (sprite->animInd != anim)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        if (anim >= (s16) sprite->definition->numAnimation)
        {
            kprintf("SPR_setAnim: error - trying to use non existing animation #%d on definition=%p - num animation = %d", anim, sprite->definition, sprite->definition->numAnimation);
            return;
        }
#endif // LIB_DEBUG

        sprite->animInd = anim;
        // first frame by default
        sprite->frameInd = 0;
        sprite->animation = sprite->definition->animations[anim];

        // set timer to 0 to prevent auto animation to change frame in between
        sprite->timer = 0;

#ifdef SPR_DEBUG
        KLog_U2_("SPR_setAnim: #", getSpriteIndex(sprite), " anim=", anim, " frame=0");
#endif // SPR_DEBUG

        sprite->status |= NEED_FRAME_UPDATE;
    }

    END_PROFIL(PROFIL_SET_ANIM_FRAME)
}

void SPR_setFrame(Sprite* sprite, s16 frame)
{
    START_PROFIL

    if (!isSpriteValid(sprite, "SPR_setFrame"))
        return;

    if (sprite->frameInd != frame)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        if (frame >= (s16) sprite->animation->numFrame)
        {
            kprintf("SPR_setFrame: error - trying to use non existing frame #%d for animation %d on definition=%p - num frame = %d", frame, sprite->animInd, sprite->definition, sprite->animation->numFrame);
            return;
        }
#endif // LIB_DEBUG

        sprite->frameInd = frame;

        // set timer to 0 to prevent auto animation to change frame in between
        sprite->timer = 0;

#ifdef SPR_DEBUG
        KLog_U2("SPR_setFrame: #", getSpriteIndex(sprite), "  frame=", frame);
#endif // SPR_DEBUG

        sprite->status |= NEED_FRAME_UPDATE;
    }

    END_PROFIL(PROFIL_SET_ANIM_FRAME)
}

void SPR_nextFrame(Sprite* sprite)
{
    START_PROFIL

    if (!isSpriteValid(sprite, "SPR_nextFrame"))
        return;

    const Animation *anim = sprite->animation;
    u16 frameInd = sprite->frameInd + 1;

    if (frameInd >= anim->numFrame)
        frameInd = anim->loop;

    // set new frame
    SPR_setFrame(sprite, frameInd);

    END_PROFIL(PROFIL_SET_ANIM_FRAME)
}

bool SPR_setVRAMTileIndex(Sprite* sprite, s16 value)
{
    START_PROFIL

    if (!isSpriteValid(sprite, "SPR_setVRAMTileIndex"))
        return FALSE;

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
            END_PROFIL(PROFIL_SET_VRAM_OR_SPRIND)

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

                END_PROFIL(PROFIL_SET_VRAM_OR_SPRIND)

                return FALSE;
            }
        }
        // just use the new value for index
        else newInd = value;
    }

    // VRAM tile index changed ?
    if ((oldAttribut & TILE_INDEX_MASK) != (u16) newInd)
    {
        sprite->attribut = (oldAttribut & TILE_ATTR_MASK) | newInd;
        // need to update sprite table
        status |= NEED_ST_ALL_UPDATE;
        // auto tile upload enabled ? --> need to re upload tile to new location
        if (status & SPR_FLAG_AUTO_TILE_UPLOAD)
            status |= NEED_TILES_UPLOAD;
    }

    // save status
    sprite->status = status;

    END_PROFIL(PROFIL_SET_VRAM_OR_SPRIND)

    return TRUE;
}

bool SPR_setSpriteTableIndex(Sprite* sprite, s16 value)
{
    START_PROFIL

    if (!isSpriteValid(sprite, "SPR_setSpriteTableIndex"))
        return FALSE;

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
            END_PROFIL(PROFIL_SET_VRAM_OR_SPRIND)

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
            if (newInd < 0)
            {
                // save status and return FALSE
                sprite->status = status;

                END_PROFIL(PROFIL_SET_VRAM_OR_SPRIND)

                return FALSE;
            }
        }
        // just use the new value for index
        else newInd = value;
    }

    // VDP sprite index changed ?
    if (sprite->VDPSpriteIndex != (u16) newInd)
    {
        // set the VDP Sprite index for this sprite and do attached operation
        setVDPSpriteIndex(sprite, newInd, num);
        // need to update complete sprite table infos
        status |= NEED_ST_ALL_UPDATE;
    }

    // save status
    sprite->status = status;

    END_PROFIL(PROFIL_SET_VRAM_OR_SPRIND)

    return TRUE;
}

void SPR_setAutoTileUpload(Sprite* sprite, bool value)
{
    if (!isSpriteValid(sprite, "SPR_setAutoTileUpload"))
        return;

    if (value) sprite->status |= SPR_FLAG_AUTO_TILE_UPLOAD;
    else sprite->status &= ~SPR_FLAG_AUTO_TILE_UPLOAD;
}

void SPR_setDelayedFrameUpdate(Sprite* sprite, bool value)
{
    if (!isSpriteValid(sprite, "SPR_setDelayedFrameUpdate"))
        return;

    if (value) sprite->status &= ~SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE;
    else sprite->status |= SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE;
}

void SPR_setFrameChangeCallback(Sprite* sprite, FrameChangeCallback* callback)
{
    if (!isSpriteValid(sprite, "SPR_setFrameChangeCallback"))
        return;

    sprite->onFrameChange = callback;
}

SpriteVisibility SPR_getVisibility(Sprite* sprite)
{
    u16 status = sprite->status;

    if (status & SPR_FLAG_AUTO_VISIBILITY)
    {
        if (status & SPR_FLAG_FAST_AUTO_VISIBILITY) return AUTO_FAST;
        else return AUTO_SLOW;
    }

    if (sprite->visibility) return VISIBLE;
    else return HIDDEN;
}

bool SPR_isVisible(Sprite* sprite, bool recompute)
{
    if (!isSpriteValid(sprite, "SPR_isVisible"))
        return FALSE;

    if (recompute)
    {
        u16 status = sprite->status;

        // update visibility if needed
        if (status & NEED_VISIBILITY_UPDATE)
        {
            // frame update need to be done first
            if (status & NEED_FRAME_UPDATE)
                status = updateFrame(sprite, status);
            // then do visibility update
            sprite->status = updateVisibility(sprite, status);
        }
    }

    return (sprite->visibility) ? TRUE : FALSE;
}

void SPR_setVisibility(Sprite* sprite, SpriteVisibility value)
{
    START_PROFIL

    if (!isSpriteValid(sprite, "SPR_setVisibility"))
        return;

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

    END_PROFIL(PROFIL_SET_VISIBILITY)
}


void SPR_clear()
{
    START_PROFIL

    // save starter link
    u8 linkSave = starter->link;

    VDP_clearSprites();
    VDP_updateSprites(1, DMA_QUEUE_COPY);

    // restore starter link
    starter->link = linkSave;

    END_PROFIL(PROFIL_CLEAR)
}

void NO_INLINE SPR_update()
{
    START_PROFIL

    Sprite* sprite;

#ifdef SPR_DEBUG
    KLog_U1("----------------- SPR_update:  sprite number = ", SPR_getNumActiveSprite());
#endif // SPR_DEBUG

#ifdef SPR_DEBUG
    KLog_U1_("  Send sprites to DMA queue: ", highestVDPSpriteIndex + 1, " sprite(s) sent");
#endif // SPR_DEBUG

    const u16 sprNum = highestVDPSpriteIndex + 1;
    // send sprites to VRAM using DMA queue (better to do it before sprite tiles upload to avoid being ignored by DMA queue)
    void* vdpSpriteTableCopy = DMA_allocateAndQueueDma(DMA_VRAM, VDP_SPRITE_TABLE, (sizeof(VDPSprite) * sprNum) / 2, 2);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    // DMA temporary buffer is full ? --> can't do sprite update
    if (!vdpSpriteTableCopy)
    {
        KLog("SPR_update(): failed... DMA data buffer is full.");
        return;
    }
#endif // LIB_DEBUG

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
                    // hide sprite
                    updateSpriteTableHide(sprite);
                    status &= ~NEED_ST_POS_UPDATE;
                }
            }
            // only if sprite is visible
            else
            {
                if (status & NEED_TILES_UPLOAD)
                    loadTiles(sprite);

                if (status & NEED_ST_ALL_UPDATE)
                    updateSpriteTableAll(sprite);
                else if (status & NEED_ST_POS_UPDATE)
                    updateSpriteTablePos(sprite);

                // tiles upload and sprite table done
                status &= ~(NEED_TILES_UPLOAD | NEED_ST_UPDATE);
            }

            // processes done !
            sprite->status = status;
        }

        // next sprite
        sprite = sprite->next;
    }

    // TODO: maybe prevent using SPR_xxx methods between SPR_update() and SYS_doVBlankProcess() call
    // so we don't need to do a copy of vdpSpriteCache here

    // VDP sprite cache is now updated, copy it to the temporary cache copy we got from DMA queue buffer
    memcpy(vdpSpriteTableCopy, vdpSpriteCache, sizeof(VDPSprite) * sprNum);

    END_PROFIL(PROFIL_UPDATE)
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
    START_PROFIL

    Sprite* spr;
    VDPSprite* vdpSprite;
    u16 i;

#ifdef SPR_DEBUG
    KLog_U2("setVDPSpriteIndex: sprite #", getSpriteIndex(sprite), "  new VDP Sprite index = ", ind);
#endif // SPR_DEBUG

    sprite->VDPSpriteIndex = ind;

    // we don't need to hide sprite by default anymore as we take care of it with 'lastNumSprite' field
//    // hide all sprites by default and get last sprite
//    vdpSprite = &vdpSpriteCache[ind];
//    vdpSprite->y = 0;
//
//    i = num - 1;
//    while(i--)
//    {
//        vdpSprite = &vdpSpriteCache[vdpSprite->link];
//        vdpSprite->y = 0;
//    }

    // get the last vdpSprite
    vdpSprite = &vdpSpriteCache[ind];
    i = num - 1;
    while(i--) vdpSprite = &vdpSpriteCache[vdpSprite->link];

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

    END_PROFIL(PROFIL_UPDATE_VDPSPRIND)
}

static u16 updateVisibility(Sprite* sprite, u16 status)
{
    START_PROFIL

    u16 visibility;
    const SpriteDefinition* sprDef = sprite->definition;

    // fast visibility computation ?
    if (status & SPR_FLAG_FAST_AUTO_VISIBILITY)
    {
        const s16 x = sprite->x - 0x80;

        // compute global visibility for sprite
        if (((x + sprDef->w) > (s16) 0) && (x < (s16) screenWidth))
            visibility = VISIBILITY_ON;
        else
            visibility = VISIBILITY_OFF;

#ifdef SPR_DEBUG
        KLog_S1("  updateVisibility (fast): global x=", x);
        KLog_S2("    frame w=", sprDef->w, " h=", sprDef->h);
#endif // SPR_DEBUG
    }
    else
    {
        AnimationFrame* frame = sprite->frame;

        // attributes
        u16 attr = sprite->attribut;
        // xmin relative to sprite pos
        const s16 xmin = 0x80 - sprite->x;
        // xmax relative to sprite pos
        const s16 xmax = screenWidth + xmin;
        const s16 fw = sprDef->w;

#ifdef SPR_DEBUG
        KLog_S1("  updateVisibility (slow): global x=", sprite->x);
        KLog_S1("    frame w=", fw);
        KLog_S2("    xmin=", xmin, " xmax=", xmax);
#endif // SPR_DEBUG

        // sprite is fully visible ? --> set all sprite visible
        if ((xmin <= 0) && (xmax >= fw)) visibility = VISIBILITY_ON;
        // sprite is fully hidden ? --> set all sprite to hidden
        else if ((xmax < 0) || ((xmin - fw) > 0)) visibility = VISIBILITY_OFF;
        else
        {
            u16 num = frame->numSprite;
            // start from the last one
            FrameVDPSprite* frameSprite = &(frame->frameVDPSprites[num]);
            visibility = 0;

            if (attr & TILE_ATTR_HFLIP_MASK)
            {
                // H flip
                while(num--)
                {
                    // next
                    frameSprite--;
                    visibility <<= 1;

                    s16 w = ((frameSprite->size & 0x0C) << 1) + 8;
                    s16 x = frameSprite->offsetXFlip;

                    // compute visibility
                    if (((x + w) > xmin) && (x < xmax))
                        visibility |= 1;
                }
            }
            else
            {
                while(num--)
                {
                    // next
                    frameSprite--;
                    visibility <<= 1;

                    s16 w = ((frameSprite->size & 0x0C) << 1) + 8;
                    s16 x = frameSprite->offsetX;

                    // compute visibility
                    if (((x + w) > xmin) && (x < xmax))
                        visibility |= 1;
                }
            }
        }
    }

#ifdef SPR_DEBUG
    KLog_U3("    Sprite at [", sprite->x - 0x80, ",", sprite->y - 0x80, "] visibility = ", visibility);
#endif // SPR_DEBUG

    END_PROFIL(PROFIL_UPDATE_VISIBILITY)

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
    START_PROFIL

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
#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
            KLog_U4_("Warning: sprite #", getSpriteIndex(sprite), " update delayed on frame #", vtimer, " - exceeding DMA capacity: ", DMA_getQueueTransferSize(), " bytes already queued and require ", frame->tileset->numTile * 32, " more bytes");
#endif // LIB_DEBUG

            // initial frame update ? --> better to set frame at least
            if (sprite->frame == NULL)
                sprite->frame = frame;

            // delay frame update (when we will have enough DMA capacity to do it)
            return status;
        }
    }

    // detect if we need to hide some VDP sprite
    s16 currentNumSprite = frame->numSprite;

    // adjust number of sprite to hide
    sprite->spriteToHide += sprite->lastNumSprite - currentNumSprite;

//    if (sprite->status & SPR_FLAG_AUTO_VISIBILITY)
//        KLog_U3("currNumSprite= ", currentNumSprite, " lastNumSprite= ", sprite->lastNumSprite, " spriteToHide= ", sprite->spriteToHide);

    // store last used number of sprite
    sprite->lastNumSprite = currentNumSprite;
    // set frame
    sprite->frame = frame;

    // init timer for this frame
    sprite->timer = frame->timer;

    // frame change event handler defined ? --> call it
    if (sprite->onFrameChange)
    {
        // important to preserve status value which may be modified externally here
        sprite->status = status;
        sprite->onFrameChange(sprite);
        status = sprite->status;
    }

    // require tile data upload
    if (status & SPR_FLAG_AUTO_TILE_UPLOAD)
        status |= NEED_TILES_UPLOAD;
    // require visibility update
    if (status & SPR_FLAG_AUTO_VISIBILITY)
        status |= NEED_VISIBILITY_UPDATE;

    // frame update done
    status &= ~NEED_FRAME_UPDATE;

    END_PROFIL(PROFIL_UPDATE_FRAME)

    // need to update all sprite table
    return status | NEED_ST_ALL_UPDATE;
}

static void updateSpriteTableAll(Sprite* sprite)
{
    START_PROFIL

    AnimationFrame* frame;
    FrameVDPSprite* frameSprite;
    VDPSprite* vdpSprite;
    u16 attr;
    s16 num;
    u16 visibility;

    visibility = sprite->visibility;
    attr = sprite->attribut;
    frame = sprite->frame;
    num = frame->numSprite;
    frameSprite = frame->frameVDPSprites;
    vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    if (visibility == VISIBILITY_ON)
    {
        while(num--)
        {
            if (attr & TILE_ATTR_VFLIP_MASK) vdpSprite->y = sprite->y + frameSprite->offsetYFlip;
            else vdpSprite->y = sprite->y + frameSprite->offsetY;
            vdpSprite->size = frameSprite->size;
            vdpSprite->attribut = attr;
            if (attr & TILE_ATTR_HFLIP_MASK) vdpSprite->x = sprite->x + frameSprite->offsetXFlip;
            else vdpSprite->x = sprite->x + frameSprite->offsetX;

            // increment tile index in attribut field
            attr += frameSprite->numTile;
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
            // next
            frameSprite++;
        }
    }
    else
    {
        while(num--)
        {
            if (visibility & 1)
            {
                if (attr & TILE_ATTR_VFLIP_MASK) vdpSprite->y = sprite->y + frameSprite->offsetYFlip;
                else vdpSprite->y = sprite->y + frameSprite->offsetY;
                if (attr & TILE_ATTR_HFLIP_MASK) vdpSprite->x = sprite->x + frameSprite->offsetXFlip;
                else vdpSprite->x = sprite->x + frameSprite->offsetX;
            }
            else vdpSprite->y = 0;
            vdpSprite->size = frameSprite->size;
            vdpSprite->attribut = attr;

            // increment tile index in attribut field
            attr += frameSprite->numTile;
            // next VDP sprite
            visibility >>= 1;
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
            // next
            frameSprite++;
        }
    }

    // hide sprites that were used by previous frame
    if ((num = sprite->spriteToHide) > 0)
    {
        while(num--)
        {
            vdpSprite->y = 0;
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }
    sprite->spriteToHide = 0;

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

    END_PROFIL(PROFIL_UPDATE_SPRITE_TABLE)
}

static void updateSpriteTablePos(Sprite* sprite)
{
    START_PROFIL

    AnimationFrame* frame;
    FrameVDPSprite* frameSprite;
    VDPSprite* vdpSprite;
    u16 attr;
    s16 num;
    u16 visibility;

    visibility = sprite->visibility;
    attr = sprite->attribut;
    frame = sprite->frame;
    num = frame->numSprite;
    frameSprite = frame->frameVDPSprites;
    vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];

    if (visibility == VISIBILITY_ON)
    {
        while(num--)
        {
            if (attr & TILE_ATTR_VFLIP_MASK) vdpSprite->y = sprite->y + frameSprite->offsetYFlip;
            else vdpSprite->y = sprite->y + frameSprite->offsetY;
            if (attr & TILE_ATTR_HFLIP_MASK) vdpSprite->x = sprite->x + frameSprite->offsetXFlip;
            else vdpSprite->x = sprite->x + frameSprite->offsetX;

            // pass to next VDP sprite
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
            // next
            frameSprite++;
        }
    }
    else
    {
        while(num--)
        {
            if (visibility & 1)
            {
                if (attr & TILE_ATTR_VFLIP_MASK) vdpSprite->y = sprite->y + frameSprite->offsetYFlip;
                else vdpSprite->y = sprite->y + frameSprite->offsetY;
                if (attr & TILE_ATTR_HFLIP_MASK) vdpSprite->x = sprite->x + frameSprite->offsetXFlip;
                else vdpSprite->x = sprite->x + frameSprite->offsetX;
            }
            else vdpSprite->y = 0;

            // pass to next VDP sprite
            visibility >>= 1;
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
            // next
            frameSprite++;
        }
    }

    // hide sprites that were used by previous frame
    if ((num = sprite->spriteToHide) > 0)
    {
        while(num--)
        {
            vdpSprite->y = 0;
            vdpSprite = &vdpSpriteCache[vdpSprite->link];
        }
    }
    sprite->spriteToHide = 0;

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

    END_PROFIL(PROFIL_UPDATE_SPRITE_TABLE)
}

static void updateSpriteTableHide(Sprite* sprite)
{
    START_PROFIL

    VDPSprite* vdpSprite = &vdpSpriteCache[sprite->VDPSpriteIndex];
    // don't forget to hide sprites that were used by previous frame
    s16 num = sprite->frame->numSprite;

    if (sprite->spriteToHide > 0) num += sprite->spriteToHide;

//    KLog_U3("updateSpriteTableHide():  sprite->spriteToHide= ", sprite->spriteToHide, " num= ", num, " maxNumSprite= ", sprite->definition->maxNumSprite);

    while(num--)
    {
        vdpSprite->y = 0;
        vdpSprite = &vdpSpriteCache[vdpSprite->link];
    }

    sprite->spriteToHide = 0;

    END_PROFIL(PROFIL_UPDATE_SPRITE_TABLE)
}

static void loadTiles(Sprite* sprite)
{
    START_PROFIL

    TileSet* tileset = sprite->frame->tileset;
    u16 compression = tileset->compression;
    u16 lenInWord = (tileset->numTile * 32) / 2;

    // TODO: separate tileset per VDP sprite and only unpack/upload visible VDP sprite (using visibility) to VRAM

    // need unpacking ?
    if (compression != COMPRESSION_NONE)
    {
        // get buffer and send to DMA queue
        u8* buf = DMA_allocateAndQueueDma(DMA_VRAM, (sprite->attribut & TILE_INDEX_MASK) * 32, lenInWord, 2);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
        if (!buf) KLog("  loadTiles: unpack tileset failed (DMA temporary buffer is full)");
        else
#endif
            // unpack in temp buffer obtained from DMA queue
            unpack(compression, (u8*) FAR_SAFE(tileset->tiles, tileset->numTile * 32), buf);

#ifdef SPR_DEBUG
        char str1[32];
        char str2[8];

        intToHex((u32) buf, str2, 4);
        strcpy(str1, " at ");
        strcat(str1, str2);

        KLog_U1_("  loadTiles: unpack tileset, numTile= ", tileset->numTile, str1);
        KLog_U2("    Queue DMA: to=", (sprite->attribut & TILE_INDEX_MASK) * 32, " size in word=", lenInWord);
#endif // SPR_DEBUG
    }
    else
    {
        // just queue DMA operation to transfer tileset data to VRAM
        DMA_queueDma(DMA_VRAM, FAR_SAFE(tileset->tiles, tileset->numTile * 32), (sprite->attribut & TILE_INDEX_MASK) * 32, lenInWord, 2);

#ifdef SPR_DEBUG
        KLog_U3("  loadTiles - queue DMA: from=", (u32) tileset->tiles, " to=", (sprite->attribut & TILE_INDEX_MASK) * 32, " size in word=", lenInWord);
#endif // SPR_DEBUG
    }

    END_PROFIL(PROFIL_LOADTILES)
}

static Sprite* sortSprite(Sprite* sprite)
{
    START_PROFIL

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
    if (s != next) moveAfter(s ? s->prev : lastSprite, sprite);
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

    END_PROFIL(PROFIL_SORT)

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
    KLog_U2("animInd=", sprite->animInd, " frameInd=", sprite->frameInd);
    KLog_S4("attribut=", sprite->attribut, " x=", sprite->x, " y=", sprite->y, " depth=", sprite->depth);
    KLog_U2("visibility=", sprite->visibility, " timer=", sprite->timer);
    KLog_U2("VDPSpriteInd=", sprite->VDPSpriteIndex, " link=", sprite->lastVDPSprite->link);
    KLog_U2("prev=", (sprite->prev == NULL) ? 128 : getSpriteIndex(sprite->prev), " next=", (sprite->next == NULL) ? 128 : getSpriteIndex(sprite->next));
}

#endif