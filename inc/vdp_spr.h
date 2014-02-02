/**
 *  \file vdp_spr.h
 *  \brief VDP Sprite support
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides methods to manipulate the VDP Sprites.<br/>
 * The Sega Genesis VDP can handle up to 80 simultanous sprites of 4x4 tiles (32x32 pixels).
 */

#ifndef _VDP_SPR_H_
#define _VDP_SPR_H_

/**
 *  \def MAX_SPRITE
 *      Maximum number of sprite
 */
#define MAX_SPRITE          80

/**
 *  \def SPRITE_SIZE
 *      Helper to define sprite size in sprite definition structure.
 *
 *  \param w
 *      sprite width (in tile).
 *  \param h
 *      sprite height (in tile).
 */
#define SPRITE_SIZE(w, h)   ((((w) - 1) << 2) | ((h) - 1))


/**
 *  \struct SpriteDef
 *      VDP sprite definition structure.
 *
 *  \param posx
 *      position X
 *  \param posy
 *      position Y
 *  \param tile_attr
 *      tile attributes (palette, priority, HV flip, tile index)
 *  \param size
 *      sprite size (1x1 tile to 4x4 tiles)
 *  \param link
 *      link to next sprite (used for sprite priority)<br/>
 */
typedef struct
{
    s16 posx;
    s16 posy;
    u16 tile_attr;
    u8 size;
    u8 link;
} SpriteDef;


/**
 *  \brief VDP sprite definition cache.
 */
extern SpriteDef vdpSpriteCache[MAX_SPRITE];


/**
 *  \brief
 *      Reset all sprites.
 *
 *  Clear the sprite list.
 */
void VDP_resetSprites();

/**
 *  \brief
 *      Set a sprite (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to set (should be < MAX_SPRITE).
 *  \param x
 *      Sprite position X.
 *  \param y
 *      Sprite position Y.
 *  \param size
 *      Sprite size (see SPRITE_SIZE() macro).
 *  \param tile_attr
 *      Sprite tile attributes (see TILE_ATTR_FULL() macro).
 *  \param link
 *      Sprite link (index of next sprite, 0 for end).
 */
void VDP_setSprite(u16 index, u16 x, u16 y, u8 size, u16 tile_attr, u8 link);
/**
 *  \brief
 *      Set a sprite (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to set (should be < MAX_SPRITE).
 *  \param sprite
 *      Sprite definition.
 *
 * See VDP_setSprite().
 */
void VDP_setSpriteP(u16 index, const SpriteDef *sprite);
/**
 *  \brief
 *      Set a sprite (direct send to VDP).
 *
 *  \param index
 *      Index of the sprite to set (should be < MAX_SPRITE).
 *  \param x
 *      Sprite position X.
 *  \param y
 *      Sprite position Y.
 *  \param size
 *      Sprite size (see SPRITE_SIZE() macro).
 *  \param tile_attr
 *      Sprite tile attributes (see TILE_ATTR_FULL() macro).
 *  \param link
 *      Sprite link (index of next sprite, 0 for end).
 *
 * See VDP_setSprite().
 */
void VDP_setSpriteDirect(u16 index, u16 x, u16 y, u8 size, u16 tile_attr, u8 link);
/**
 *  \brief
 *      Set a sprite (direct send to VDP).
 *
 *  \param index
 *      Index of the sprite to set (should be < MAX_SPRITE).
 *  \param sprite
 *      Sprite definition.
 *
 * See VDP_setSpriteP().
 */
void VDP_setSpriteDirectP(u16 index, const SpriteDef *sprite);

/**
 *  \brief
 *      Set sprite position (use sprite list cache).
 *
 *  \param index
 *      Index of the sprite to modify position (should be < MAX_SPRITE).
 *  \param x
 *      Sprite position X.
 *  \param y
 *      Sprite position Y.
 *
 * See VDP_setSprite().
 */
void VDP_setSpritePosition(u16 index, u16 x, u16 y);


/**
 *  \brief
 *      Set severals sprites (use sprite list cache).
 *
 *  \param index
 *      Index of first sprite to set (should be < MAX_SPRITE).
 *  \param sprites
 *      Sprite definitions.
 *  \param num
 *      Number of sprite to set.
 *
 * See VDP_setSpritesDirect().
 */
void VDP_setSprites(u16 index, const SpriteDef *sprites, u16 num);
/**
 *  \brief
 *      Set severals sprites (direct send to VDP).
 *
 *  \param index
 *      Index of first sprite to set.
 *  \param sprites
 *      Sprite definitions.
 *  \param num
 *      Number of sprite to set.
 *
 * See VDP_setSprites().
 */
void VDP_setSpritesDirect(u16 index, const SpriteDef *sprites, u16 num);

/**
 *  \brief
 *      Send the cached sprite list to the VDP.
 *
 *  You should call this method when you completed your sprite update task.
 */
void VDP_updateSprites();



#endif // _VDP_SPR_H_
