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
 *      Maximum size of Sprite Attribute Table (128 in VRAM but limited to 80 in VDP anyway)
 */
#define SAT_MAX_SIZE            80

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
 *      Y position - 0x80 (0x80 = 0 on screen). Valid values: [0 - 1023]
 *  \param size
 *      sprite size (see SPRITE_SIZE macro)
 *    \param sizeH
 *      horizontal size. Valid values: 0 -> 8, 1 -> 16, 2 -> 24, 3 -> 32
 *    \param sizeV
 *      vertical size. Valid values: 0 -> 8, 1 -> 16, 2 -> 24, 3 -> 32
 *  \param link
 *      sprite link, this information is used to define sprite drawing order (use 0 to force end of list)
 *  \param attribut
 *      tile index and sprite attribut (priority, palette, H/V flip), see TILE_ATTR_FULL macro
 *    \param priority
 *      sprite priority. Valid values: 0 -> low, 1 -> high
 *    \param palette
 *      palette index. Valid values: [0, 3]
 *    \param flipV
 *      vertical flip. Valid values: 0 -> normal, 1 -> flipped
 *    \param flipH
 *      horizontal flip. Valid values: 0 -> normal, 1 -> flipped
 *    \param tile
 *      tile index. Valid values: [0, 2047]
 *  \param x
 *      X position - 0x80 (0x80 = 0 on screen). Valid values: [0 - 1023]
 */
typedef struct {
    s16 y;  // 10 bits
    union {
        struct {
            u16 unused1  : 4;
            u16 sizeH    : 2;
            u16 sizeV    : 2;
            u16 unused2  : 1;
            u16 linkData : 7;
        };
        struct {
            u8 size;
            u8 link;
        };
        u16 size_link;
    };
    union {
        u16 attribut;
        struct {
            u16 priority : 1;
            u16 palette  : 2;
            u16 flipV    : 1;
            u16 flipH    : 1;
            u16 tile     : 11;
        };
    };
    s16 x;  // 10 bits
}  VDPSprite;


/**
 *  \brief VDP sprite cache
 */
extern VDPSprite vdpSpriteCache[SAT_MAX_SIZE + 16];

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
void VDP_resetSprites(void);

/**
 *  \brief
 *      Release all VDP sprite allocation
 */
void VDP_releaseAllSprites(void);

/**
 *  \brief
 *      Allocate the specified number of hardware VDP sprites and link them together.<br>
 *
 *  \param num
 *      Number of VDP sprite to allocate (need to be > 0)
 *  \return the first VDP sprite index where allocation was made.<br>
 *      -1 if there is not enough available VDP sprite remaining.<br>
 *      <b>WARNING:</b> VDP can display up to 80 sprites at once on screen in H40 mode only, in H32 mode
 *      it's limited to 64 sprites even if we allow to allocate up to 80 (SAT size).
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
 *      The index of the first VDP sprite to release (0 <= index < SAT_MAX_SIZE)
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
 *      Returns the number of available VDP sprite from the SAT.
 *      <b>WARNING:</b> the SAT maximum size is 128 entries in VRAM <b>but</b> the VDP can only display up to 80 (H40 mode) or
 *      64 sprites (H32 mode) at once on the screen.
 *
 *  \see VDP_allocateSprites(..)
 *  \see VDP_releaseSprites(..)
 */
u16 VDP_getAvailableSprites(void);
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
s16 VDP_refreshHighestAllocatedSpriteIndex(void);

/**
 *  \brief
 *      Clear all sprites.
 */
void VDP_clearSprites(void);
/**
 *  \brief
 *      Set a sprite (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to set (should be < SAT_MAX_SIZE).
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
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites(..)
 */
void VDP_setSpriteFull(u16 index, s16 x, s16 y, u8 size, u16 attribut, u8 link);
/**
 *  \brief
 *      Set a sprite (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to set (should be < SAT_MAX_SIZE).
 *  \param x
 *      Sprite position X on screen.
 *  \param y
 *      Sprite position Y on screen.
 *  \param size
 *      Sprite size (see SPRITE_SIZE() macro).
 *  \param attribut
 *      Sprite tile attributes (see TILE_ATTR_FULL() macro).
 *
 *  \see VDP_setSpriteFull(..)
 *  \see VDP_updateSprites(..)
 */
void VDP_setSprite(u16 index, s16 x, s16 y, u8 size, u16 attribut);
/**
 *  \brief
 *      Set sprite position (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify position (should be < SAT_MAX_SIZE).
 *  \param x
 *      Sprite position X.
 *  \param y
 *      Sprite position Y.
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites(..)
 */
void VDP_setSpritePosition(u16 index, s16 x, s16 y);
/**
 *  \brief
 *      Set sprite size (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify size (should be < SAT_MAX_SIZE).
 *  \param size
 *      Sprite size (see SPRITE_SIZE() macro).
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites(..)
 */
void VDP_setSpriteSize(u16 index, u8 size);
/**
 *  \brief
 *      Set sprite attributes (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < SAT_MAX_SIZE).
 *  \param attribut
 *      Sprite tile attributes (see TILE_ATTR_FULL() macro).
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites(..)
 */
void VDP_setSpriteAttribut(u16 index, u16 attribut);
/**
 *  \brief
 *      Set sprite link (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify link (should be < SAT_MAX_SIZE).
 *  \param link
 *      Sprite link (index of next sprite, 0 for end).
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites(..)
 */
void VDP_setSpriteLink(u16 index, u8 link);
/**
 *  \brief
 *      Link sprites starting at the specified index.<br>
 *      Links are created in simple ascending order (1 --> 2 --> 3 --> ...)
 *
 *  \param index
 *      Index of the first sprite we want to link (should be < SAT_MAX_SIZE).
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
/**
 *  \brief
 *      Set sprite priority (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < SAT_MAX_SIZE).
 *  \param priority
 *      sprite priority. Valid values: 0 -> low, 1 -> high
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites(..)
 */
void VDP_setSpritePriority(u16 index, bool priority);
/**
 *  \brief
 *      Get sprite priority (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < SAT_MAX_SIZE).
 */
bool VDP_getSpritePriority(u16 index);
/**
 *  \brief
 *      Set sprite palette (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < SAT_MAX_SIZE).
 *  \param palette
 *      palette index. Valid values: [0, 3]
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites(..)
 */
void VDP_setSpritePalette(u16 index, u16 palette);
/**
 *  \brief
 *      Get sprite palette (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < SAT_MAX_SIZE).
 */
u16 VDP_getSpritePalette(u16 index);
/**
 *  \brief
 *      Set sprite horizontal and vertical flip (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < SAT_MAX_SIZE).
 *  \param flipH
 *      Horizontal flip. Valid values: 0 -> normal, 1 -> flipped.
 *  \param flipV
 *      Vertical flip. Valid values: 0 -> normal, 1 -> flipped.
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites(..)
 */
void VDP_setSpriteFlip(u16 index, bool flipH, bool flipV);
/**
 *  \brief
 *      Set sprite horizontal flip (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < SAT_MAX_SIZE).
 *  \param flipH
 *      Horizontal flip. Valid values: 0 -> normal, 1 -> flipped.
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites(..)
 */
void VDP_setSpriteFlipH(u16 index, bool flipH);
/**
 *  \brief
 *      Set sprite vertical flip (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < SAT_MAX_SIZE).
 *  \param flipV
 *      Vertical flip. Valid values: 0 -> normal, 1 -> flipped.
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites(..)
 */
void VDP_setSpriteFlipV(u16 index, bool flipV);
/**
 *  \brief
 *      Get sprite horizontal flip (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < SAT_MAX_SIZE).
 */
bool VDP_getSpriteFlipH(u16 index);
/**
 *  \brief
 *      Get sprite vertical flip (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < SAT_MAX_SIZE).
 */
bool VDP_getSpriteFlipV(u16 index);
/**
 *  \brief
 *      Set sprite tile index (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < SAT_MAX_SIZE).
 *  \param tile
 *      Tile index. Valid values: [0, 2047].
 *
 *  \see VDP_setSprite(..)
 *  \see VDP_updateSprites(..)
 */
void VDP_setSpriteTile(u16 index, u16 tile);
/**
 *  \brief
 *      Get sprite tile index (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify attributes (should be < SAT_MAX_SIZE).
 */
u16 VDP_getSpriteTile(u16 index);

#endif // _VDP_SPR_H_
