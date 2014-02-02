#include "config.h"
#include "types.h"

#include "sprite_eng.h"

#include "sys.h"
#include "vdp.h"
#include "vdp_dma.h"
#include "vdp_spr.h"
#include "tile_cache.h"
#include "memory.h"


// we don't want to share it
extern u32 VIntProcess;

// forward
static void computeVisibility(Sprite *sprite);
static void setFrame(Sprite *sprite, AnimationFrame *frame);
static void allocTileSet(AnimationFrame *frame, u16 position);


// no static so it can be read
VDPSprite *VDPSpriteCache = NULL;

static TileCache tcSprite;
static u16 toUpload;


void SPR_init(u16 cacheSize)
{
    u16 index;
    u16 size;

    // already initialized --> end it first
    if (SPR_isInitialized()) SPR_end();

    // alloc cache structure memory
    VDPSpriteCache = MEM_alloc(SPRITE_CACHE_SIZE * sizeof(VDPSprite));
    // reset
    toUpload = 0;

    // enabled sprite engine VInt processing
    VIntProcess |= PROCESS_SPRITEENGINE_TASK;

    size = cacheSize?cacheSize:384;
    // get start tile index for sprite cache (reserve VRAM area just before system font)
    index = TILE_FONTINDEX - size;

    // init tile cache engine if needed
    TC_init();
    // and create a tile cache for the sprite
    TC_createCache(&tcSprite, index, size);
}

void SPR_end()
{
    if (SPR_isInitialized())
    {
        // disabled tile cache Int processing
        VIntProcess &= ~PROCESS_SPRITEENGINE_TASK;

        // send an empty sprite list to VDP to hide them
        VDPSpriteCache->y = 0;
        VDPSpriteCache->size_link = 0;
        VDP_doVRamDMA((u32) VDPSpriteCache, VDP_getSpriteListAddress(), sizeof(VDPSprite) / 2);

        MEM_free(VDPSpriteCache);
        VDPSpriteCache = NULL;

        TC_releaseCache(&tcSprite);
    }
}

u16 SPR_isInitialized()
{
    return VIntProcess & PROCESS_SPRITEENGINE_TASK;
}


void SPR_initSprite(Sprite *sprite, SpriteDefinition *spriteDef, s16 x, s16 y, u16 attribut)
{
    // definition changed ?
    if (sprite->definition != spriteDef)
    {
        sprite->definition = spriteDef;
        sprite->x = x + 0x80;
        sprite->y = y + 0x80;
        sprite->animInd = -1;
        sprite->frameInd = -1;
        sprite->seqInd = -1;
        sprite->frame = NULL;
        sprite->attribut = attribut;
    }
    else
    {
        // update position and attribut
        SPR_setPosition(sprite, x, y);
        SPR_setAttribut(sprite, attribut);
    }

    // alays reset these informations
    sprite->timer = 0;
    sprite->fixedIndex = -1;
    sprite->data = 0;

    // set anim and frame to 0
    SPR_setAnimAndFrame(sprite, 0, 0);
}

void SPR_setPosition(Sprite *sprite, s16 x, s16 y)
{
    const s16 fx = x + 0x80;
    const s16 fy = y + 0x80;

    if ((sprite->x != fx) || (sprite->y != fy))
    {
        sprite->x = fx;
        sprite->y = fy;

        // need to recompute visibility
        sprite->visibility = -1;
    }
}

void SPR_setAttribut(Sprite *sprite, u16 attribut)
{
    if (sprite->attribut != attribut)
    {
        sprite->attribut = attribut;

        // need to recompute visibility
        sprite->visibility = -1;
    }
}

void SPR_setAnimAndFrame(Sprite *sprite, s16 anim, s16 frame)
{
    if ((sprite->animInd != anim) || (sprite->seqInd != frame))
    {
        Animation *animation = sprite->definition->animations[anim];
        const u16 frameInd = animation->sequence[frame];

        sprite->animInd = anim;
        sprite->seqInd = frame;
        sprite->animation = animation;
        sprite->frameInd = frameInd;

        // set current frame
        setFrame(sprite, animation->frames[frameInd]);
    }
}

void SPR_setAnim(Sprite *sprite, s16 anim)
{
    if (sprite->animInd != anim)
    {
        Animation *animation = sprite->definition->animations[anim];
        // first frame by default
        const u16 frameInd = animation->sequence[0];

        sprite->animInd = anim;
        sprite->seqInd = 0;
        sprite->animation = animation;
        sprite->frameInd = frameInd;

        // set current frame
        setFrame(sprite, animation->frames[frameInd]);
    }
}

void SPR_setFrame(Sprite *sprite, s16 frame)
{
    if (sprite->seqInd != frame)
    {
        const Animation *animation = sprite->animation;
        const u16 frameInd = animation->sequence[frame];

        sprite->seqInd = frame;

        if (sprite->frameInd != frameInd)
        {
            sprite->frameInd = frameInd;

            // set current frame
            setFrame(sprite, animation->frames[frameInd]);
        }
    }
}

void SPR_nextFrame(Sprite *sprite)
{
    const Animation *anim = sprite->animation;
    u16 seqInd = sprite->seqInd + 1;

    if (seqInd == anim->length)
        seqInd = anim->loop;

    // set new frame
    SPR_setFrame(sprite, seqInd);
}

void SPR_setVRAMTileIndex(Sprite *sprite, s16 index)
{
    if (sprite->fixedIndex != index)
    {
        sprite->fixedIndex = index;

        // changed to fixed allocation
        if (index != -1)
            allocTileSet(sprite->frame, index);
    };
}

//void SPR_checkAllocation(Sprite *sprite)
//{
//    // ensure sprite tileset is still allocated in VRAM (only for automatic allocation)
//    if (sprite->fixedIndex == -1)
//        autoAllocTileSet(sprite->frame, sprite->tileIndexes);
//}

void SPR_clear()
{
    VDPSprite *cache = VDPSpriteCache;

    // single sprite not visible so nothing is displayed
    cache->y = 0;
    cache->size_link = 0;

    toUpload = 1;
}

void SPR_update(Sprite *sprites, u16 num)
{
    u16 i, j;
    u16 ind;
    Sprite *sprite;
    VDPSprite *cache;

    // flush sprite tile cache
    TC_flushCache(&tcSprite);

    // do a first pass to re allocate tileset still present in cache
    sprite = sprites;
    i = num;
    while(i--)
    {
         // auto allocation
        if (sprite->fixedIndex == -1)
        {
            AnimationFrame *frame;
            FrameSprite **frameSprites;
            s32 visibility;

            frame = sprite->frame;
            j = frame->numSprite;
            frameSprites = frame->frameSprites;
            visibility = sprite->visibility;

            // need update ?
            if (visibility < 0)
            {
                computeVisibility(sprite);
                visibility = sprite->visibility;
            }

            while(j--)
            {
                // sprite visible --> try fast re alloc
                if (visibility & 1)
                    TC_reAlloc(&tcSprite, (*frameSprites)->tileset);

                frameSprites++;
                visibility >>= 1;
            }
        }

        sprite++;
    }


    cache = VDPSpriteCache;

    ind = 0;
    sprite = sprites;
    i = num;
    while(i--)
    {
        AnimationFrame *frame;
        FrameSprite **frameSprites;
        u16 timer;
        u16 attr;
        s16 fw, fh;
        s16 vramInd;
        s32 visibility;

        timer = sprite->timer;
        // handle frame animation
        if (timer)
        {
            // timer elapsed --> next frame
            if (--timer == 0) SPR_nextFrame(sprite);
            // just update remaining timer
            else sprite->timer = timer;
        }

        frame = sprite->frame;
        attr = sprite->attribut;
        vramInd = sprite->fixedIndex;
        visibility = sprite->visibility;

        // need update ?
        if (visibility < 0)
        {
            computeVisibility(sprite);
            visibility = sprite->visibility;
        }

        fw = frame->w;
        fh = frame->h;

        j = frame->numSprite;
        frameSprites = frame->frameSprites;

        if (vramInd == -1)
        {
            // auto allocation
            while(j--)
            {
                FrameSprite* frameSprite = *frameSprites++;

                // sprite visible ?
                if (visibility & 1)
                {
                    if (attr & TILE_ATTR_VFLIP_MASK)
                    {
                        s16 sh = ((frameSprite->vdpSprite.size_link & 0x0300) >> 5) + 8;
                        cache->y = sprite->y + (fh - (frameSprite->vdpSprite.y + sh));
                    }
                    else
                        cache->y = sprite->y + frameSprite->vdpSprite.y;

                    cache->size_link = frameSprite->vdpSprite.size_link | ++ind;
                    cache->attr = (frameSprite->vdpSprite.attr ^ attr) +
                        TC_alloc(&tcSprite, frameSprite->tileset, UPLOAD_VINT);

                    if (attr & TILE_ATTR_HFLIP_MASK)
                    {
                        s16 sw = ((frameSprite->vdpSprite.size_link & 0x0C00) >> 7) + 8;
                        cache->x = sprite->x + (fw - (frameSprite->vdpSprite.x + sw));
                    }
                    else
                        cache->x = sprite->x + frameSprite->vdpSprite.x;

                    cache++;
                }

                visibility >>= 1;
            }
        }
        else
        {
            // fixed allocation
            while(j--)
            {
                FrameSprite* frameSprite = *frameSprites++;

                // sprite visible ?
                if (visibility & 1)
                {
                    if (attr & TILE_ATTR_VFLIP_MASK)
                    {
                        s16 sh = ((frameSprite->vdpSprite.size_link & 0x0300) >> 5) + 8;
                        cache->y = sprite->y + (fh - (frameSprite->vdpSprite.y + sh));
                    }
                    else
                        cache->y = sprite->y + frameSprite->vdpSprite.y;

                    cache->size_link = frameSprite->vdpSprite.size_link | ++ind;
                    cache->attr = (frameSprite->vdpSprite.attr ^ attr) + vramInd;

                    if (attr & TILE_ATTR_HFLIP_MASK)
                    {
                        s16 sw = ((frameSprite->vdpSprite.size_link & 0x0C00) >> 7) + 8;
                        cache->x = sprite->x + (fw - (frameSprite->vdpSprite.x + sw));
                    }
                    else
                        cache->x = sprite->x + frameSprite->vdpSprite.x;

                    cache++;
                }

                visibility >>= 1;
                vramInd += frameSprite->tileset->numTile;
            }
        }


        sprite++;
    }

    // if at least one sprite is visible
    if (ind)
    {
        cache--;
        // end sprite list
        cache->size_link &= 0xFF00;
        // save number of sprite to upload
        toUpload = ind;
    }
    else
    {
        // single sprite not visible so nothing is displayed
        cache->y = 0;
        cache->size_link = 0;

        toUpload = 1;
    }
}

//void SPR_release(Sprite *sprites, u16 num)
//{
//    u16 i;
//    Sprite *sprite;
//
//    sprite = sprites;
//    i = num;
//    while(i--)
//    {
//        releaseTileSet(sprite->frame);
//        sprite++;
//    }
//}

// VInt processing
u16 SPR_doVBlankProcess()
{
    if (toUpload)
    {
        // send to VRAM
        VDP_doVRamDMA((u32) VDPSpriteCache, VDP_getSpriteListAddress(), (toUpload * sizeof(VDPSprite)) / 2);
        toUpload = 0;
    }

    return TRUE;
}


void computeVisibility(Sprite *sprite)
{
    AnimationFrame *frame = sprite->frame;
    FrameSprite **frameSprites;
    u32 visibility;
    s16 xmin, ymin;
    s16 xmax, ymax;
    s16 fw, fh;
    u16 attr;
    u16 i;

    xmin = 0x80 - sprite->x;
    ymin = 0x80 - sprite->y;
    xmax = screenWidth + xmin;
    ymax = screenHeight + ymin;
    fw = frame->w;
    fh = frame->h;
    attr = sprite->attribut;

    i = frame->numSprite;
    // start from the last one
    frameSprites = &(frame->frameSprites[i]);
    visibility = 0;

    while(i--)
    {
        FrameSprite* frameSprite = *--frameSprites;
        s16 x, y;
        s16 w, h;

        w = ((frameSprite->vdpSprite.size_link & 0x0C00) >> 7) + 8;
        h = ((frameSprite->vdpSprite.size_link & 0x0300) >> 5) + 8;

        if (attr & TILE_ATTR_VFLIP_MASK)
            y = fh - (frameSprite->vdpSprite.y + h);
        else
            y = frameSprite->vdpSprite.y;
        if (attr & TILE_ATTR_HFLIP_MASK)
            x = fw - (frameSprite->vdpSprite.x + w);
        else
            x = frameSprite->vdpSprite.x;

        visibility <<= 1;

        // compute visibility
        if (((x + w) > xmin) && (x < xmax) && ((y + h) > ymin) && (y < ymax))
            visibility |= 1;
    }

    // store visibility info
    sprite->visibility = visibility;
}

static void setFrame(Sprite *sprite, AnimationFrame* frame)
{
    s16 index = sprite->fixedIndex;

    // manual allocation (dynamic allocation is done on SPR_update(..))
    if (index != -1)
        allocTileSet(frame, (u16) index);

    sprite->frame = frame;
    // init timer for this frame (+1 as we update animation before sending to VDP)
    if ((sprite->timer = frame->timer))
        sprite->timer++;

    // need to recompute visibility
    sprite->visibility = -1;
}

static void allocTileSet(AnimationFrame *frame, u16 position)
{
    FrameSprite **frameSprites = frame->frameSprites;
    u16 pos = position;
    u16 i = frame->numSprite;

    // fixed allocation
    while(i--)
    {
        FrameSprite* frameSprite = *frameSprites++;
        TileSet* tileset = frameSprite->tileset;

        // alloc tileset
        TC_uploadAtVBlank(tileset, pos);
        pos += tileset->numTile;
    }
}
