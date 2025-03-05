#include "config.h"
#include "types.h"

#if      !LEGACY_SPRITE_ENGINE

#include "sprite_eng.h"

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

// hardware cannot handle more than 80 sprites anyway
#define MAX_SPRITE                          80

// internals
#define VISIBILITY_ON                       0xFFFF
#define VISIBILITY_OFF                      0x0000

#define ALLOCATED                           0x8000
#define CHECK_VDP_SPRITE                    0x8000

#define NEED_VISIBILITY_UPDATE              0x0001
#define NEED_FRAME_UPDATE                   0x0002
#define NEED_TILES_UPLOAD                   0x0004

#define NEED_UPDATE                         0x000F

#define STATE_ANIMATION_DONE                0x0010



// shared from vdp_spr.c unit
extern void logVDPSprite(u16 index);
// shared from vdp.c unit
extern void updateUserTileMaxIndex();


// forward
static Sprite* allocateSprite(u16 head);
static bool releaseSprite(Sprite* sprite);

static u16 updateVisibility(Sprite* sprite, u16 status);
static void setVisibility(Sprite* sprite, u16 newVisibility);
static u16 updateFrame(Sprite* sprite, u16 status);

//static VDPSprite* updateSpriteTable(Sprite* sprite, VDPSprite* vdpSprite);

static void loadTiles(Sprite* sprite);
static Sprite* sortSprite(Sprite* sprite);
static void moveAfter(Sprite* pos, Sprite* sprite);
static u16 getSpriteIndex(Sprite* sprite);
static void logSprite(Sprite* sprite);

// current usage of hardware sprite
static s16 usedVDPSprite;

// pool of Sprite objects (used to know if sprite engine is initialized)
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
#define PROFIL_SET_VRAM_IND             7
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

    // disable VDP sprite check by default
    usedVDPSprite = 0;

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

void NO_INLINE SPR_end()
{
    if (SPR_isInitialized())
    {
        // no active sprites
        firstSprite = NULL;
        lastSprite = NULL;

        // reset and clear VDP sprite
        VDP_resetSprites();
        VDP_updateSprites(1, DMA_QUEUE);

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

    // clear used VDP sprite (only keep check VDP sprite flag)
    usedVDPSprite &= CHECK_VDP_SPRITE;

#ifdef SPR_PROFIL
    memset(profil_time, 0, sizeof(profil_time));
#endif // SPR_PROFIL

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
    KLog("Sprite engine reset");
    KLog_U1("  VRAM region free: ", VRAM_getFree(&vram));
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

#if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
        kprintf("releaseSprite: success - released sprite at pos %d", POOL_find(spritesPool, sprite));
#endif // LIB_DEBUG

        // release sprite (we don't need stack coherency here as we don't use stack iteration)
        POOL_release(spritesPool, sprite, FALSE);

        // remove sprite from chained list
        prev = sprite->prev;
        next = sprite->next;

        // get the last VDP sprite to link from
        if (prev) prev->next = next;
        // update first sprite
        else firstSprite = next;

        // get the next VDP Sprite index to link to
        if (next) next->prev = prev;
        // update last sprite
        else lastSprite = prev;

        // update used VDP sprite
        usedVDPSprite -= sprite->definition->maxNumSprite;

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

static void checkSpriteValid(Sprite* sprite, char* methodName)
{
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    if (!(sprite->status & ALLOCATED))
        kprintf("%s: error - sprite at address %p is invalid (not allocated) !", methodName, sprite);
#endif
}

Sprite* NO_INLINE SPR_addSpriteEx(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut, u16 flag)
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

    // update used VDP sprite
    usedVDPSprite += spriteDef->maxNumSprite;

    // VDP sprite check enable ?
    if (usedVDPSprite & CHECK_VDP_SPRITE)
    {
        // isolate number of used VDP sprite
        const u16 usedSpr = usedVDPSprite & ~CHECK_VDP_SPRITE;

        if (usedSpr >= (VDP_getScreenWidth() >> 2))
        {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
            kprintf("SPR_addSpriteEx failed: not enough hardware sprite (missing %d sprites) !", (usedSpr - (VDP_getScreenWidth() >> 2)) + 1);
#endif

            // revert
            usedVDPSprite -= spriteDef->maxNumSprite;

            releaseSprite(sprite);
            return NULL;
        }
    }
#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
    else
    {
        // isolate number of used VDP sprite
        const u16 usedSpr = usedVDPSprite & ~CHECK_VDP_SPRITE;

        if (usedSpr >= (VDP_getScreenWidth() >> 2))
            kprintf("SPR_addSpriteEx warning: exceeding maximum number of hardware sprite (currently used = %d)", usedSpr);
    }
#endif

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

    // may not be reset in SPR_setAnimAndFrame(..) so we have to reset it here
    sprite->timer = 0;

    sprite->x = x + 0x80;
    sprite->y = y + 0x80;
    // depending sprite position (first or last) we set its default depth
    if (flag & SPR_FLAG_INSERT_HEAD) sprite->depth = SPR_MIN_DEPTH;
    else sprite->depth = SPR_MAX_DEPTH;

    // auto VRAM alloc enabled ?
    if (flag & SPR_FLAG_AUTO_VRAM_ALLOC)
    {
        // allocate VRAM
        ind = VRAM_alloc(&vram, spriteDef->maxNumTile);
        // not enough --> release sprite and return NULL
        if (ind < 0)
        {
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
    return SPR_addSpriteEx(spriteDef, x, y, attribut, SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD);
}

Sprite* SPR_addSpriteExSafe(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut, u16 flag)
{
    Sprite* result = SPR_addSpriteEx(spriteDef, x, y, attribut, flag);

    // allocation failed ?
    if (result == NULL)
    {
        // try to defragment VRAM, it can help
        SPR_defragVRAM();
        // VRAM is now defragmented, so allocation should pass this time
        result = SPR_addSpriteEx(spriteDef, x, y, attribut, flag);
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

u16 SPR_getUsedVDPSprite(void)
{
    return usedVDPSprite & ~CHECK_VDP_SPRITE;
}

u16 SPR_getFreeVRAM(void)
{
    return VRAM_getFree(&vram);
}

u16 SPR_getLargestFreeVRAMBlock(void)
{
    return VRAM_getLargestFreeBlock(&vram);
}

void SPR_enableVDPSpriteChecking()
{
    usedVDPSprite |= CHECK_VDP_SPRITE;

}

void SPR_disableVDPSpriteChecking()
{
    usedVDPSprite &= ~CHECK_VDP_SPRITE;
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

static u16 getTilesetIndex(TileSet** tilesets, u16* indexes, TileSet* tileset, u16 index)
{
    TileSet** t = tilesets;
    u16* i = indexes;

    // try fo find if tileset already exist
    while(*t)
    {
        // find the tileset, return its index
        if (*t == tileset) return *i;
        // next
        t++; i++;
    }

    // set new tileset index
    *t = tileset;
    *i = index;

    return index;
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

    // allocate result table indexes[numAnim][numFrame]
    u16** indexes = MEM_alloc((numAnimation * sizeof(u16*)) + (numFrameTot * sizeof(u16)));
    // used to detect duplicate
    TileSet** tilesets = MEM_alloc(numFrameTot * sizeof(TileSet*));
    u16* tilesetIndexes = MEM_alloc(numFrameTot * sizeof(u16));

    // not enough memory
    if ((tilesets == NULL) || (tilesetIndexes == NULL) || (indexes == NULL))
    {
        if (totalNumTile) *totalNumTile = 0;
        if (tilesets) MEM_free(tilesets);
        if (tilesetIndexes) MEM_free(tilesetIndexes);
        if (indexes) MEM_free(indexes);

        return NULL;
    }

    // clear tilesets table
    memset(tilesets, 0, numFrameTot * sizeof(TileSet*));

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
            const u16 ind = getTilesetIndex(tilesets, tilesetIndexes, (TileSet*) tileset, tileInd);

            // new tileset ?
            if (ind == tileInd)
            {
                // load tileset
                VDP_loadTileSet(tileset, tileInd, DMA);
                // next tileset
                tileInd += tileset->numTile;
            }
            // store frame tile index
            *indFrames++ = ind;
            // next frame
            frame++;
        }

        anim++;
    }

    // store total num tile (discarding duplicated tilesets)
    if (totalNumTile) *totalNumTile = (tileInd - index);

    MEM_free(tilesets);
    MEM_free(tilesetIndexes);
    MEM_pack();

    return result;
}


bool NO_INLINE SPR_setDefinition(Sprite* sprite, const SpriteDefinition* spriteDef)
{
    START_PROFIL

#ifdef SPR_DEBUG
    KLog_U1("SPR_setDefinition: #", getSpriteIndex(sprite));
#endif // SPR_DEBUG

    // for debug
    checkSpriteValid(sprite, "SPR_setDefinition");

    // nothing to do...
    if (sprite->definition == spriteDef) return TRUE;

    // update used VDP sprite
    usedVDPSprite -= sprite->definition->maxNumSprite;
    usedVDPSprite += spriteDef->maxNumSprite;

    // VDP sprite check enable ?
    if (usedVDPSprite & CHECK_VDP_SPRITE)
    {
        // isolate number of used VDP sprite
        const u16 usedSpr = usedVDPSprite & ~CHECK_VDP_SPRITE;

        if (usedSpr >= (VDP_getScreenWidth() >> 2))
        {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
            kprintf("SPR_setDefinition failed: not enough hardware sprite for new definition (missing %d sprites) !", (usedSpr - (VDP_getScreenWidth() >> 2)) + 1);
#endif

            // revert back used VDP sprite
            usedVDPSprite -= spriteDef->maxNumSprite;
            usedVDPSprite += sprite->definition->maxNumSprite;

            // failed
            return FALSE;
        }
    }
#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
    else
    {
        // isolate number of used VDP sprite
        const u16 usedSpr = usedVDPSprite & ~CHECK_VDP_SPRITE;

        if (usedSpr >= (VDP_getScreenWidth() >> 2))
            kprintf("SPR_setDefinition warning: exceeding maximum number of hardware sprite (currently used = %d)", usedSpr);
    }
#endif

    u16 status = sprite->status;
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

    // for debug
    checkSpriteValid(sprite, "SPR_setPosition");

    if ((sprite->x != newx) || (sprite->y != newy))
    {
        sprite->x = newx;
        sprite->y = newy;

        // need to recompute visibility if auto visibility is enabled
        if (sprite->status & SPR_FLAG_AUTO_VISIBILITY)
            sprite->status |= NEED_VISIBILITY_UPDATE;
    }

    END_PROFIL(PROFIL_SET_ATTRIBUTE)
}

void SPR_setHFlip(Sprite* sprite, bool value)
{
    START_PROFIL

    // for debug
    checkSpriteValid(sprite, "SPR_setHFlip");

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
                sprite->status |= NEED_VISIBILITY_UPDATE;
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
                sprite->status |= NEED_VISIBILITY_UPDATE;
        }
    }

    END_PROFIL(PROFIL_SET_ATTRIBUTE)
}

void SPR_setVFlip(Sprite* sprite, bool value)
{
    START_PROFIL

    // for debug
    checkSpriteValid(sprite, "SPR_setVFlip");

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
                sprite->status |= NEED_VISIBILITY_UPDATE;
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
                sprite->status |= NEED_VISIBILITY_UPDATE;
        }
    }

    END_PROFIL(PROFIL_SET_ATTRIBUTE)
}

void SPR_setPriority(Sprite* sprite, bool value)
{
    START_PROFIL

    // for debug
    checkSpriteValid(sprite, "SPR_setPriority");

    const u16 attr = sprite->attribut;

    if (attr & TILE_ATTR_PRIORITY_MASK)
    {
        // priority removed
        if (!value)
        {
            sprite->attribut = attr & (~TILE_ATTR_PRIORITY_MASK);

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

    // for debug
    checkSpriteValid(sprite, "SPR_setPalette");

    const u16 oldAttribut = sprite->attribut;
    const u16 newAttribut = (oldAttribut & (~TILE_ATTR_PALETTE_MASK)) | (value << TILE_ATTR_PALETTE_SFT);

    if (oldAttribut != newAttribut)
    {
        sprite->attribut = newAttribut;

#ifdef SPR_DEBUG
        KLog_U2("SPR_setPalette: #", getSpriteIndex(sprite), " palette=", value);
#endif // SPR_DEBUG
    }

    END_PROFIL(PROFIL_SET_ATTRIBUTE)
}

void SPR_setDepth(Sprite* sprite, s16 value)
{
    START_PROFIL

    // for debug
    checkSpriteValid(sprite, "SPR_setDepth");

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

void SPR_setAlwaysAtBottom(Sprite* sprite)
{
    SPR_setDepth(sprite, SPR_MAX_DEPTH);
}

void SPR_setAnimAndFrame(Sprite* sprite, s16 anim, s16 frame)
{
    START_PROFIL

    // for debug
    checkSpriteValid(sprite, "SPR_setAnimAndFrame");

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
        if (sprite->timer > 0)
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

    // for debug
    checkSpriteValid(sprite, "SPR_setAnim");

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
        if (sprite->timer > 0)
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

    // for debug
    checkSpriteValid(sprite, "SPR_setFrame");

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
        if (sprite->timer > 0)
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

    // for debug
    checkSpriteValid(sprite, "SPR_nextFrame");

    const Animation *anim = sprite->animation;
    u16 frameInd = sprite->frameInd + 1;

    if (frameInd >= anim->numFrame)
    {
        // no loop ?
        if (sprite->status & SPR_FLAG_DISABLE_ANIMATION_LOOP)
        {
            // can quit now
            END_PROFIL(PROFIL_SET_ANIM_FRAME)
            return;
        }

        // loop
        frameInd = anim->loop;
        // set animation done state (allows SPR_isAnimationDone(..) to correctly report state when called from frame change callback)
        sprite->status |= STATE_ANIMATION_DONE;
    }

    // set new frame
    SPR_setFrame(sprite, frameInd);

    END_PROFIL(PROFIL_SET_ANIM_FRAME)
}

void SPR_setAutoAnimation(Sprite* sprite, bool value)
{
    // for debug
    checkSpriteValid(sprite, "SPR_setAutoAnimation");

    if (value)
    {
        // currently disabled ? --> reset timer to current frame timer
        if (sprite->timer == -1)
            sprite->timer = sprite->frame->timer;
    }
    else
    {
        // disable it
        sprite->timer = -1;
    }

#ifdef SPR_DEBUG
    KLog_U2("SPR_setAutoAnimation: #", getSpriteIndex(sprite), " AutoAnimation=", value);
#endif // SPR_DEBUG
}

bool SPR_getAutoAnimation(Sprite* sprite)
{
    // for debug
    checkSpriteValid(sprite, "SPR_getAutoAnimation");

    return (sprite->timer == -1)?FALSE:TRUE;
}

void SPR_setAnimationLoop(Sprite* sprite, bool value)
{
    // for debug
    checkSpriteValid(sprite, "SPR_setAnimationLoop");

    if (value) sprite->status &= ~SPR_FLAG_DISABLE_ANIMATION_LOOP;
    else sprite->status |= SPR_FLAG_DISABLE_ANIMATION_LOOP;

#ifdef SPR_DEBUG
    KLog_U2("SPR_setAnimationLoop: #", getSpriteIndex(sprite), " loop=", value);
#endif // SPR_DEBUG
}

bool SPR_isAnimationDone(Sprite* sprite)
{
    // for debug
    checkSpriteValid(sprite, "SPR_isAnimationDone");

    // when we are in the frame change callback we need to test for the 'animation done state'
    return (sprite->status & STATE_ANIMATION_DONE) ||
            // otherwise we just check if we are on last frame tick (if auto animation is disabled then only test for last frame)
           ((sprite->frameInd == (sprite->animation->numFrame - 1)) && ((sprite->timer == 1) || (sprite->timer == -1)));
}

bool SPR_setVRAMTileIndex(Sprite* sprite, s16 value)
{
    START_PROFIL

    // for debug
    checkSpriteValid(sprite, "SPR_setVRAMTileIndex");

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
            END_PROFIL(PROFIL_SET_VRAM_IND)

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

                END_PROFIL(PROFIL_SET_VRAM_IND)

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
        // auto tile upload enabled ? --> need to re upload tile to new location
        if (status & SPR_FLAG_AUTO_TILE_UPLOAD)
            status |= NEED_TILES_UPLOAD;
    }

    // save status
    sprite->status = status;

    END_PROFIL(PROFIL_SET_VRAM_IND)

    return TRUE;
}

void SPR_setAutoTileUpload(Sprite* sprite, bool value)
{
    // for debug
    checkSpriteValid(sprite, "SPR_setAutoTileUpload");

    if (value) sprite->status |= SPR_FLAG_AUTO_TILE_UPLOAD;
    else sprite->status &= ~SPR_FLAG_AUTO_TILE_UPLOAD;
}

void SPR_setDelayedFrameUpdate(Sprite* sprite, bool value)
{
    // for debug
    checkSpriteValid(sprite, "SPR_setDelayedFrameUpdate");

    if (value) sprite->status &= ~SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE;
    else sprite->status |= SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE;
}

void SPR_setFrameChangeCallback(Sprite* sprite, FrameChangeCallback* callback)
{
    // for debug
    checkSpriteValid(sprite, "SPR_setFrameChangeCallback");

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

bool NO_INLINE SPR_isVisible(Sprite* sprite, bool recompute)
{
    // for debug
    checkSpriteValid(sprite, "SPR_isVisible");

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

    // for debug
    checkSpriteValid(sprite, "SPR_setVisibility");

    u16 status = sprite->status;

    if (status & SPR_FLAG_AUTO_VISIBILITY)
    {
        switch(value)
        {
            case VISIBLE:
                status &= ~(SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_FAST_AUTO_VISIBILITY | NEED_VISIBILITY_UPDATE);
                setVisibility(sprite, VISIBILITY_ON);
                break;

            case HIDDEN:
                status &= ~(SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_FAST_AUTO_VISIBILITY | NEED_VISIBILITY_UPDATE);
                setVisibility(sprite, VISIBILITY_OFF);
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
                setVisibility(sprite, VISIBILITY_ON);
                break;

            case HIDDEN:
                setVisibility(sprite, VISIBILITY_OFF);
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
    u8 linkSave = vdpSpriteCache[0].link;

    VDP_clearSprites();
    // *QUEUE_COPY* as we restore link right after
    VDP_updateSprites(1, DMA_QUEUE_COPY);

    // restore starter link
    vdpSpriteCache[0].link = linkSave;

    END_PROFIL(PROFIL_CLEAR)
}

void NO_INLINE SPR_update()
{
    START_PROFIL

    Sprite* sprite = firstSprite;
    // SAT pointer
    VDPSprite* vdpSprite = vdpSpriteCache;
    // VDP sprite index (for link field)
    u8 vdpSpriteInd = 1;

    // first sprite used by CPU load monitor
    if (SYS_getShowFrameLoad())
    {
        // goes to next VDP sprite then
        vdpSprite->link = vdpSpriteInd++;
        vdpSprite++;
    }

#ifdef SPR_DEBUG
    KLog_U1("----------------- SPR_update:  sprite number = ", SPR_getNumActiveSprite());
#endif // SPR_DEBUG

    // iterate over all sprites
    while(sprite)
    {
        s16 timer = sprite->timer;

#ifdef SPR_DEBUG
        char str1[32];
        char str2[8];

        intToHex(sprite->visibility, str2, 4);
        strcpy(str1, " visibility = ");
        strcat(str1, str2);

        KLog_U2_("  processing sprite pos #", getSpriteIndex(sprite), " - timer = ", timer, str1);
#endif // SPR_DEBUG

        // handle frame animation
        if (timer > 0)
        {
            // timer elapsed --> next frame
            if (--timer == 0) SPR_nextFrame(sprite);
            // just update remaining timer
            else sprite->timer = timer;
        }

        u16 status = sprite->status;

        // order is important: updateFrame first then updateVisibility
        if (status & NEED_FRAME_UPDATE)
            status = updateFrame(sprite, status);
        if (status & NEED_VISIBILITY_UPDATE)
            status = updateVisibility(sprite, status);

        // sprite visible and still allocated (can be released during updateFrame(..) with the frame change callback) with enough entry in SAT ?
        if (sprite->visibility && (status & ALLOCATED) && (vdpSpriteInd <= SAT_MAX_SIZE))
        {
            if (status & NEED_TILES_UPLOAD)
            {
                loadTiles(sprite);
                // tiles upload and sprite table done
                status &= ~NEED_TILES_UPLOAD;
            }

            // update SAT now
            AnimationFrame* frame = sprite->frame;
            FrameVDPSprite* frameSprite = frame->frameVDPSprites;
            u16 attr = sprite->attribut;
            s8 numSprite = frame->numSprite;

            // special case of single VDP sprite with size aligned to sprite size (no offset, no flip calculation required)
            if (numSprite < 0)
            {
                vdpSprite->y = sprite->y;
                vdpSprite->size = frameSprite->size;
                vdpSprite->link = vdpSpriteInd++;
                vdpSprite->attribut = attr;
                vdpSprite->x = sprite->x;
                vdpSprite++;
            }
            else
            {
                static const u16 visibilityMask[17] =
                {
                    0x0000, 0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00,
                    0xFF00, 0xFF80, 0xFFC0, 0xFFE0, 0xFFF0, 0xFFF8, 0xFFFC, 0xFFFE,
                    0xFFFF
                };

                // so visibility also allow to get the number of sprite
                s16 visibility = (s16)(sprite->visibility & visibilityMask[(u8) numSprite]);

                switch(attr & (TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK))
                {
                    case 0:
                        while(visibility)
                        {
                            // current sprite visibility bit is in high bit
                            if (visibility < 0)
                            {
                                vdpSprite->y = sprite->y + frameSprite->offsetY;
                                vdpSprite->size = frameSprite->size;
                                vdpSprite->link = vdpSpriteInd++;
                                vdpSprite->attribut = attr;
                                vdpSprite->x = sprite->x + frameSprite->offsetX;
                                vdpSprite++;
                            }

                            // increment tile index in attribut field
                            attr += frameSprite->numTile;
                            // next
                            frameSprite++;
                            // next VDP sprite
                            visibility <<= 1;

#ifdef SPR_DEBUG
                            logVDPSprite(vdpSpriteInd - 1);
#endif // SPR_DEBUG
                        }
                        break;

                    case TILE_ATTR_HFLIP_MASK:
                        while(visibility)
                        {
                            // current sprite visibility bit is in high bit
                            if (visibility < 0)
                            {
                                vdpSprite->y = sprite->y + frameSprite->offsetY;
                                vdpSprite->size = frameSprite->size;
                                vdpSprite->link = vdpSpriteInd++;
                                vdpSprite->attribut = attr;
                                vdpSprite->x = sprite->x + frameSprite->offsetXFlip;
                                vdpSprite++;
                            }

                            // increment tile index in attribut field
                            attr += frameSprite->numTile;
                            // next
                            frameSprite++;
                            // next VDP sprite
                            visibility <<= 1;

#ifdef SPR_DEBUG
                            logVDPSprite(vdpSpriteInd - 1);
#endif // SPR_DEBUG
                        }
                        break;

                    case TILE_ATTR_VFLIP_MASK:
                        while(visibility)
                        {
                            // current sprite visibility bit is in high bit
                            if (visibility < 0)
                            {
                                vdpSprite->y = sprite->y + frameSprite->offsetYFlip;
                                vdpSprite->size = frameSprite->size;
                                vdpSprite->link = vdpSpriteInd++;
                                vdpSprite->attribut = attr;
                                vdpSprite->x = sprite->x + frameSprite->offsetX;
                                vdpSprite++;
                            }

                            // increment tile index in attribut field
                            attr += frameSprite->numTile;
                            // next
                            frameSprite++;
                            // next VDP sprite
                            visibility <<= 1;

#ifdef SPR_DEBUG
                            logVDPSprite(vdpSpriteInd - 1);
#endif // SPR_DEBUG
                        }
                        break;

                    case (TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK):
                        while(visibility)
                        {
                            // current sprite visibility bit is in high bit
                            if (visibility < 0)
                            {
                                vdpSprite->y = sprite->y + frameSprite->offsetYFlip;
                                vdpSprite->size = frameSprite->size;
                                vdpSprite->link = vdpSpriteInd++;
                                vdpSprite->attribut = attr;
                                vdpSprite->x = sprite->x + frameSprite->offsetXFlip;
                                vdpSprite++;
                            }

                            // increment tile index in attribut field
                            attr += frameSprite->numTile;
                            // next
                            frameSprite++;
                            // next VDP sprite
                            visibility <<= 1;

#ifdef SPR_DEBUG
                            logVDPSprite(vdpSpriteInd - 1);
#endif // SPR_DEBUG
                        }
                        break;
                }
            }
        }

        // processes done
        sprite->status = status;
        // next sprite
        sprite = sprite->next;
    }

    // remove 1 to get number of hard sprite used
    vdpSpriteInd--;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
    // not enough hardware sprite ?
    if (vdpSpriteInd > (VDP_getScreenWidth() >> 2))
        kprintf("SPR_update: not enough hardware sprite to display all active sprites, some sprites may miss !");
#endif // LIB_DEBUG

    // something to display ?
    if (vdpSpriteInd > 0)
    {
        // get back to last sprite
        vdpSprite--;
        // mark as end
        vdpSprite->link = 0;
        // send sprites to VRAM using DMA queue
        DMA_queueDmaFast(DMA_VRAM, vdpSpriteCache, VDP_SPRITE_TABLE, vdpSpriteInd * (sizeof(VDPSprite) / 2), 2);
    }
    // no sprite to display
    else
    {
        // set 1st sprite off screen and mark as end
        vdpSprite->y = 0;
        vdpSprite->link = 0;
        // send sprites to VRAM using DMA queue
        DMA_queueDmaFast(DMA_VRAM, vdpSpriteCache, VDP_SPRITE_TABLE, 1 * (sizeof(VDPSprite) / 2), 2);
    }

    END_PROFIL(PROFIL_UPDATE)
}


void SPR_logProfil()
{
#ifdef SPR_PROFIL
    KLog("SPR Engine profiling ------------------------------------------------------");
    KLog_U2x(4, "Alloc=", profil_time[PROFIL_ALLOCATE_SPRITE], " Release=", profil_time[PROFIL_RELEASE_SPRITE]);
    KLog_U2x(4, "Add=", profil_time[PROFIL_ADD_SPRITE], " Remove=", profil_time[PROFIL_REMOVE_SPRITE]);
    KLog_U2x(4, "Set Def.=", profil_time[PROFIL_SET_DEF], " Set Attr.=", profil_time[PROFIL_SET_ATTRIBUTE]);
    KLog_U2x(4, "Set Anim & Frame=", profil_time[PROFIL_SET_ANIM_FRAME], " Set VRAM Ind=", profil_time[PROFIL_SET_VRAM_IND]);
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


static u16 updateVisibility(Sprite* sprite, u16 status)
{
    START_PROFIL

    u16 visibility;
    const SpriteDefinition* sprDef = sprite->definition;

    const u16 sw = screenWidth;
    const u16 sh = screenHeight;
    const u16 w = sprDef->w - 1;
    const u16 h = sprDef->h - 1;
    const u16 x = sprite->x - 0x80;
    const u16 y = sprite->y - 0x80;

    // fast visibility computation ?
    if (status & SPR_FLAG_FAST_AUTO_VISIBILITY)
    {
        // compute global visibility for sprite (use unsigned for merged <0 test)
        if (((u16)(x + w) < (u16)(sw + w)) && ((u16)(y + h) < (u16)(sh + h)))
            visibility = VISIBILITY_ON;
        else
            visibility = VISIBILITY_OFF;

#ifdef SPR_DEBUG
        KLog_S2("  updateVisibility (fast): global x=", x, " y=", y);
        KLog_S2("    frame w=", sprDef->w, " h=", sprDef->h);
#endif // SPR_DEBUG
    }
    else
    {
#ifdef SPR_DEBUG
        KLog_S2("  updateVisibility (slow): global x=", x, " y=", y);
        KLog_S2("    frame w=", sprDef->w, " h=", sprDef->h);
        KLog_S2("    xmin=", xmin, " xmax=", xmax);
#endif // SPR_DEBUG

        // sprite is fully visible ? --> set all sprite visible (use unsigned for merged <0 test)
        if ((x < (u16)(sw - w)) && (y < (u16)(sh - h)))
        {
            visibility = VISIBILITY_ON;

#ifdef SPR_DEBUG
            KLog_S2("  updateVisibility (slow): global x=", x, " y=", y);
            KLog_S2("    frame w=", sprDef->w, " h=", sprDef->h);
            KLog_S4("    x=", x, " y=", y, " sw-w=", (u16)(sw-w), " sh-h=", (u16)(sh-h));
            KLog("    full ON");
#endif // SPR_DEBUG
        }
        // sprite is fully hidden ? --> set all sprite to hidden (use unsigned for merged <0 test)
        else if (((u16)(x + w) >= (u16)(sw + w)) || ((u16)(y + h) >= (u16)(sh + h)))
        {
            visibility = VISIBILITY_OFF;

#ifdef SPR_DEBUG
            KLog_S2("  updateVisibility (slow): global x=", x, " y=", y);
            KLog_S2("    frame w=", sprDef->w, " h=", sprDef->h);
            KLog_S4("    x+w=", (u16)(x+w), " y+h=", (u16)(y+h), " sw+w=", (u16)(sw+w), " sh+h=", (u16)(sh+h));
            KLog("    full OFF");
#endif // SPR_DEBUG
        }
        else
        {
            const u16 bx = x + 0x1F;    // max hardware sprite size = 32
            const u16 by = y + 0x1F;
            const u16 mx = sw + 0x1F;   // max hardware sprite size = 32
            const u16 my = sh + 0x1F;

            AnimationFrame* frame = sprite->frame;
            s8 num = frame->numSprite;

            // special case of single VDP sprite with size aligned to sprite size (no offset, no flip calculation required)
            if (num < 0) visibility = ((bx < mx) && (by < my))?0x8000:0;
            else
            {
                FrameVDPSprite* frameSprite = frame->frameVDPSprites;
                visibility = 0;

#ifdef SPR_DEBUG
                KLog_S2("  updateVisibility (slow): global x=", x, " y=", y);
                KLog("    partial");
                KLog_S2("    frame w=", sprDef->w, " h=", sprDef->h);
                KLog_S4("    bx=", bx, " by=", by, " mx=", mx, " my=", my);
#endif

                switch(sprite->attribut & (TILE_ATTR_HFLIP_MASK | TILE_ATTR_VFLIP_MASK))
                {
                    case 0:
                        while(num--)
                        {
                            // need to be done first
                            visibility <<= 1;

                            // compute visibility (use unsigned for merged <0 test)
                            if (((u16)(frameSprite->offsetX + bx) < mx) && ((u16)(frameSprite->offsetY + by) < my))
                                visibility |= 1;

#ifdef SPR_DEBUG
                            KLog_S4("    offx+bx=", (u16)(frameSprite->offsetX + bx), " offy+by=", (u16)(frameSprite->offsetY + by), " mx=", mx, " my=", my);
#endif

                            // next
                            frameSprite++;
                        }
                        break;

                    case TILE_ATTR_HFLIP_MASK:
                        while(num--)
                        {
                            // need to be done first
                            visibility <<= 1;

                            // compute visibility (use unsigned for merged <0 test)
                            if (((u16)(frameSprite->offsetXFlip + bx) < mx) && ((u16)(frameSprite->offsetY + by) < my))
                                visibility |= 1;

#ifdef SPR_DEBUG
                            KLog_S4("    offx+bx=", (u16)(frameSprite->offsetXFlip + bx), " offy+by=", (u16)(frameSprite->offsetY + by), " mx=", mx, " my=", my);
#endif

                            // next
                            frameSprite++;
                        }
                        break;

                    case TILE_ATTR_VFLIP_MASK:
                        while(num--)
                        {
                            // need to be done first
                            visibility <<= 1;

                            // compute visibility (use unsigned for merged <0 test)
                            if (((u16)(frameSprite->offsetX + bx) < mx) && ((u16)(frameSprite->offsetYFlip + by) < my))
                                visibility |= 1;

#ifdef SPR_DEBUG
                            KLog_S4("    offx+bx=", (u16)(frameSprite->offsetXFlip + bx), " offy+by=", (u16)(frameSprite->offsetYFlip + by), " mx=", mx, " my=", my);
#endif

                            // next
                            frameSprite++;
                        }
                        break;

                    case TILE_ATTR_HFLIP_MASK | TILE_ATTR_VFLIP_MASK:
                        while(num--)
                        {
                            // need to be done first
                            visibility <<= 1;

                            // compute visibility (use unsigned for merged <0 test)
                            if (((u16)(frameSprite->offsetXFlip + bx) < mx) && ((u16)(frameSprite->offsetYFlip + by) < my))
                                visibility |= 1;

#ifdef SPR_DEBUG
                            KLog_S4("    offx+bx=", (u16)(frameSprite->offsetXFlip + bx), " offy+by=", (u16)(frameSprite->offsetYFlip + by), " mx=", mx, " my=", my);
#endif

                            // next
                            frameSprite++;
                        }
                        break;
                }

                // so visibility is in high bits
                visibility <<= (16 - frame->numSprite);
            }
        }
    }

#ifdef SPR_DEBUG
    KLog_U3("    Sprite at [", sprite->x - 0x80, ",", sprite->y - 0x80, "] visibility = ", visibility);
#endif // SPR_DEBUG

    END_PROFIL(PROFIL_UPDATE_VISIBILITY)

    // set the new computed visibility
    setVisibility(sprite, visibility);

    // visibility update done !
    return status & ~NEED_VISIBILITY_UPDATE;
}

static void setVisibility(Sprite* sprite, u16 newVisibility)
{
    // set new visibility
    sprite->visibility = newVisibility;
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

        if (dmaCapacity && ((DMA_getQueueTransferSize() + (frame->tileset->numTile * 32)) > dmaCapacity))
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

    // set frame
    sprite->frame = frame;
    // init timer for this frame *before* frame change callback so it can modify change it if needed.
    if (SPR_getAutoAnimation(sprite))
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

    // frame update done, also clear ANIMATION_DONE state
    status &= ~(NEED_FRAME_UPDATE | STATE_ANIMATION_DONE);

    END_PROFIL(PROFIL_UPDATE_FRAME)

    return status;
}

static void loadTiles(Sprite* sprite)
{
    START_PROFIL

    TileSet* tileset = sprite->frame->tileset;
    u16 lenInWord = (tileset->numTile * 32) / 2;

    // need to test for empty tileset (blank frame)
    if (lenInWord)
    {
        u16 compression = tileset->compression;

        // TODO: separate tileset per VDP sprite and only unpack/upload visible VDP sprite (using visibility) to VRAM

        // need unpacking ?
        if (compression != COMPRESSION_NONE)
        {
            // get buffer and send to DMA queue
            u8* buf = DMA_allocateAndQueueDma(DMA_VRAM, (sprite->attribut & TILE_INDEX_MASK) * 32, lenInWord, 2);

            // unpack in temp buffer obtained from DMA queue
            if (buf) unpack(compression, (u8*) FAR_SAFE(tileset->tiles, lenInWord * 2), buf);
#if (LIB_LOG_LEVEL >= LOG_LEVEL_ERROR)
            else KLog("  loadTiles: unpack tileset failed (DMA temporary buffer is full)");
#endif

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
            DMA_queueDma(DMA_VRAM, FAR_SAFE(tileset->tiles, lenInWord * 2), (sprite->attribut & TILE_INDEX_MASK) * 32, lenInWord, 2);

#ifdef SPR_DEBUG
            KLog_U3("  loadTiles - queue DMA: from=", (u32) tileset->tiles, " to=", (sprite->attribut & TILE_INDEX_MASK) * 32, " size in word=", lenInWord);
#endif // SPR_DEBUG
        }
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
    KLog_U2("Start depth compare for sprite #", getSpriteIndex(sprite));
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
        if (next) next->prev = prev;
        else lastSprite = prev;
    }
    else
    {
        // 'next' become the first sprite
        firstSprite = next;
        if (next) next->prev = prev;
        // no more sprite (both firstSprite and lastSprite == NULL)
        else lastSprite = prev;
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
    KLog_U2("prev=", (sprite->prev == NULL) ? 128 : getSpriteIndex(sprite->prev), " next=", (sprite->next == NULL) ? 128 : getSpriteIndex(sprite->next));
}

#endif
