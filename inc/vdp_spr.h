/**
 *  \file vdp_spr.h
 *  \brief VDP Sprite support
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides methods to allocate and manipulate VDP Sprite at low level.<br>
 * The Sega Genesis VDP can handle up to 80 simultanous sprites of 4x4 tiles (32x32 pixels).
 */

#include "config.h"
#include "types.h"
#include "dma.h"

#ifndef _VDP_SPR_H_
#define _VDP_SPR_H_

/**
 *  \brief
 *      Maximum number of hardware sprite
 */
#define MAX_VDP_SPRITE          80

/**
 *  \brief
 *      Helper to define sprite size in sprite definition structure.
 *
 *  \param w
 *      sprite width (in tile).
 *  \param h
 *      sprite height (in tile).
 */
#define SPRITE_SIZE(w, h)   ((((w) - 1) << 2) | ((h) - 1))


/**
 *  \brief
 *      VDP sprite definition structure replicating VDP hardware sprite.
 *
 *  \param y
 *      Y position - 0x80 (0x80 = 0 on screen)
 *  \param size
 *      sprite size (see SPRITE_SIZE macro)
 *  \param link
 *      sprite link, this information is used to define sprite drawing order (use 0 to force end of list)
 *  \param attr
 *      tile index and sprite attribut (priority, palette, H/V flip), see TILE_ATTR_FULL macro
 *  \param x
 *      X position - 0x80 (0x80 = 0 on screen)
 */
typedef struct
{
    s16 y;
    union
    {
        struct {
            u8 size;
            u8 link;
        };
        u16 size_link;
    };
    u16 attribut;
    s16 x;
}  VDPSprite;


/**
 *  \brief VDP sprite cache
 */
extern VDPSprite vdpSpriteCache[MAX_VDP_SPRITE];

/**
 *  \brief Pointer to last allocated sprite after calling VDP_allocateSprites(..) method.<br>
 *    This can be used to do the link from the last allocated VDP sprite.
 */
extern VDPSprite* lastAllocatedVDPSprite;
/**
 *  \brief Highest index of allocated VDP sprite since the last call to VDP_resetSprites() or VDP_releaseAllSprites().<br>
 *      A value of <i>-1</i> mean no VDP Sprite were allocated..<br>
 *      This can be used to define the number of sprite to transfer with VDP_updateSprites(..) method.<br>
 *      <b>WARNING:</b> this value is not correctly updated on sprite release operation so it may gives an higher index than reality.<br>
 *      You can ue currently VDP_refreshHighestAllocatedSpriteIndex() method to force recomputation of highest index (costs a bit of time).
 *
 *  \see VDP_refreshHighestAllocatedSpriteIndex()
 */
extern s16 highestVDPSpriteIndex;


/**
 *  \brief
 *      Clear all sprites and reset VDP sprite allocation (if any).
 */
void VDP_resetSprites();

/**
 *  \brief
 *      Release all VDP sprite allocation
 */
void VDP_releaseAllSprites();

/**
 *  \brief
 *      Allocate the specified number of hardware VDP sprites and link them together.
 *
 *  \param num
 *      Number of VDP sprite to allocate (need to be > 0)
 *  \return the first VDP sprite index where allocation was made.<br>
 *      -1 if there is not enough available VDP sprite remaining.
 *
 *  This method allocates the specified number of VDP sprite and returns the index of the
 *  first allocated sprite in VDP sprite table (see vdpSpriteCache).<br>
 *  Sprites are linked together using <i>link</i> field (last sprite ends with link 0).<br>
 *  If there is not enough available VDP sprites the allocation operation fails and return -1.
 *  NOTE: The last sprite from the allocated list can be retrieved with <i>lastAllocatedVDPSprite</i>, this is
 *  to avoid parsing all the list to find it, if we want to link it to a specific sprite for instance.<br>
 *
 *  \see VDP_releaseSprites(..)
 */
s16 VDP_allocateSprites(u16 num);
/**
 *  \brief
 *      Release specified number of VDP sprites.
 *
 *  \param index
 *      The index of the first VDP sprite to release (0 <= index < MAX_SPRITE)
 *  \param num
 *      Number of VDP sprite to release (should be > 0)
 *
 *  This method release the specified number of VDP sprite from the specified index using
 *  the <i>link</i> field information to determine which sprites to release when more than
 *  1 sprite is released.
 *
 *  \see VDP_allocateSprites(..)
 */
void VDP_releaseSprites(u16 index, u16 num);
/**
 *  \brief
 *      Return the number of available VDP sprite.
 *
 *  \see VDP_allocateSprites(..)
 *  \see VDP_releaseSprites(..)
 */
u16 VDP_getAvailableSprites();
/**
 *  \brief
 *      Compute and return the highest index of currently allocated VDP sprite.<br>
 *      A value of <i>-1</i> mean no VDP Sprite are allocated.<br>
 *      This value can be used to define the number of sprite to transfer with VDP_updateSprites(..) method.
 *
 *  \see VDP_allocateSprites(..)
 *  \see VDP_releaseSprites(..)
 *  \see highestVDPSpriteIndex
 */
s16 VDP_refreshHighestAllocatedSpriteIndex();

/**
 *  \brief
 *      Clear all sprites.
 */
void VDP_clearSprites();
/**
 *  \brief
 *      Set a sprite (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to set (should be < MAX_VDP_SPRITE).
 *  \param x
 *      Sprite position X on screen.
 *  \param y
 *      Sprite position Y on screen.
 *  \param size
 *      Sprite size (see SPRITE_SIZE() macro).
 *  \param attribut
 *      Sprite tile attributes (see TILE_ATTR_FULL() macro).
 *  \param link
 *      Sprite link (index of next sprite, 0 for end)<br>
 *      Be careful to not modify link made by VDP_allocateSprite(..), use VDP_setSprite(..) instead in that case.
 *
 *  \see VDP_setSprite()
 *  \see VDP_updateSprites()
 */
void VDP_setSpriteFull(u16 index, s16 x, s16 y, u8 size, u16 attribut, u8 link);
/**
 *  \brief
 *      Set a sprite (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to set (should be < MAX_VDP_SPRITE).
 *  \param x
 *      Sprite position X on screen.
 *  \param y
 *      Sprite position Y on screen.
 *  \param size
 *      Sprite size (see SPRITE_SIZE() macro).
 *  \param attribut
 *      Sprite tile attributes (see TILE_ATTR_FULL() macro).
 *
 *  \see VDP_setSpriteFull()
 *  \see VDP_updateSprites()
 */
void VDP_setSprite(u16 index, s16 x, s16 y, u8 size, u16 attribut);
/**
 *  \brief
 *      Set sprite position (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify position (should be < MAX_VDP_SPRITE).
 *  \param x
 *      Sprite position X.
 *  \param y
 *      Sprite position Y.
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites()
 */
void VDP_setSpritePosition(u16 index, s16 x, s16 y);
/**
 *  \brief
 *      Set sprite size (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify size (should be < MAX_VDP_SPRITE).
 *  \param size
 *      Sprite size (see SPRITE_SIZE() macro).
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites()
 */
void VDP_setSpriteSize(u16 index, u8 size);
/**
 *  \brief
 *      Set sprite attributes (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < MAX_VDP_SPRITE).
 *  \param attribut
 *      Sprite tile attributes (see TILE_ATTR_FULL() macro).
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites()
 */
void VDP_setSpriteAttribut(u16 index, u16 attribut);
/**
 *  \brief
 *      Set sprite link (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify link (should be < MAX_VDP_SPRITE).
 *  \param link
 *      Sprite link (index of next sprite, 0 for end).
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites()
 */
void VDP_setSpriteLink(u16 index, u8 link);
/**
 *  \brief
 *      Link sprites starting at the specified index.<br>
 *      Links are created in simple ascending order (1 --> 2 --> 3 --> ...)
 *
 *  \param index
 *      Index of the first sprite we want to link (should be < MAX_VDP_SPRITE).
 *  \param num
 *      Number of link to create (if you want to link 2 sprites you should use 1 here)
 *  \return
 *      The last linked sprite
 */
VDPSprite* VDP_linkSprites(u16 index, u16 num);

/**
 *  \brief
 *      Send the cached sprite list to the VDP.
 *
 *  \param num
 *      Number of sprite to transfer starting at index 0 (max = MAX_SPRITE).<br>
 *      If you use dynamic VDP Sprite allocation you may use 'highestVDPSpriteIndex + 1' here
 *  \param tm
 *      Transfer method.<br>
 *      Accepted values are:<br>
 *      - CPU<br>
 *      - DMA<br>
 *      - DMA_QUEUE<br>
 *      - DMA_QUEUE_COPY
 *
 *  \see highestVDPSpriteIndex
 *  \see VDP_refreshHighestAllocatedSpriteIndex()
 */
void VDP_updateSprites(u16 num, TransferMethod tm);


#endif // _VDP_SPR_H_
