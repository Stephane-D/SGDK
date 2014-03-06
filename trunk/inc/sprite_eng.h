/**
 *  \file sprite_eng.h
 *  \brief Sprite engine
 *  \author Stephane Dallongeville
 *  \date 10/2013
 *
 * Sprite engine providing advanced sprites manipulation and operations.<br/>
 * This unit use both the tile cache engine (see tilecache.h file for more info)<br/>
 * and the Sega Genesis VDP sprite capabilities (see vdp_spr.h file for more info).
 */

#ifndef _SPRITE_ENG_H_
#define _SPRITE_ENG_H_

#include "vdp_pal.h"
#include "vdp_tile.h"
#include "vdp_spr.h"


/**
 *  \def SPRITE_CACHE_SIZE
 *      Maximum number of sprite in the cache
 */
#define SPRITE_CACHE_SIZE           128

/**
 *  \def COLLISION_TYPE_NONE
 *      No collision tpye
 */
#define COLLISION_TYPE_NONE     0
/**
 *  \def COLLISION_TYPE_BOX
 *      Bouding box collision tpye
 */
#define COLLISION_TYPE_BOX      1
/**
 *  \def COLLISION_TYPE_CIRCLE
 *      Round circle collision tpye
 */
#define COLLISION_TYPE_CIRCLE   2

/**
 *  \struct VDPSprite
 *      VDP sprite definition structure replicating VDP hardware sprite.
 *
 *  \param y
 *      Y position - 0x80 (0x80 = 0)
 *  \param size_link
 *      sprite size (see SPRITE_SIZE macro) in high byte and link in low byte
 *  \param attr
 *      tile index and sprite attribut (priority, palette, H/V flip), see TILE_ATTR_FULL macro
 *  \param x
 *      X position - 0x80 (0x80 = 0)
 */
typedef struct
{
	s16 y;
	u16 size_link;
    u16 attr;
    s16 x;
}  VDPSprite;

/**
 *  \struct FrameSprite
 *      Single frame sprite definition structure close to the VDP hardware sprite.
 *
 *  \param Sprite.y
 *      Y offset in Frame
 *  \param Sprite.attr
 *      first tile index (relative to the AnimationFrame.tileset)
 *      and sprite attribut (priority, palette, H/V flip)
 *  \param Sprite.size_link
 *      sprite size (see SPRITE_SIZE macro) in high byte
 *  \param Sprite.x
 *      X offset in Frame
 *  \param tileset
 *      tileset containing tiles for this animation frame (ordered for sprite)
 */
typedef struct
{
    VDPSprite vdpSprite;
	TileSet *tileset;
}  FrameSprite;

/**
 *  \struct AnimationFrame
 *      Sprite animation frame structure.
 *
 *  \param numSprite
 *      number of sprite which compose this frame
 *  \param frameSprites
 *      sprites composing the frame
 *  \param numCollision
 *      number of collision structure for this frame
 *  \param collisions
 *      collisions structures (can be either Box or Circle)
 *  \param w
 *      frame width in pixel
 *  \param h
 *      frame height in pixel
 *  \param tc
 *      collision type, accepted values are:<br/>
 *      <b>COLLISION_TYPE_NONE</b><br/>
 *      <b>COLLISION_TYPE_BOX</b><br/>
 *      <b>COLLISION_TYPE_CIRCLE</b><br/>
 *  \param timer
 *      active time for this frame (in 1/60 of second)
 */
typedef struct
{
	u16 numSprite;
	FrameSprite **frameSprites;
	u16 numCollision;
	void **collisions;
	s16 w;
	s16 h;
	u8 tc;
	u8 timer;
} AnimationFrame;

/**
 *  \struct Animation
 *      Sprite animation structure.
 *
 *  \param numFrame
 *      number of different frame for this animation
 *  \param frames
 *      frames composing the animation
 *  \param length
 *      animation sequence length
 *  \param sequence
 *      frame sequence animation (for instance: 0-1-2-2-1-2-3-4..)
 *  \param loop
 *      frame sequence index for loop (-1 if no loop).
 *  \param timer
 *      active time for this frame (in 1/60 of second)
 */
typedef struct
{
	u16 numFrame;
	AnimationFrame **frames;
	u16 length;
	u8 *sequence;
	s16 loop;
} Animation;

/**
 *  \struct SpriteDefinition
 *      Sprite definition structure.<br/>
 *      Contains all animations for a Sprite.
 *
 *  \param palette
 *      Default palette data
 *  \param numAnimation
 *      number of animation for this sprite
 *  \param animations
 *      animation definitions
 */
typedef struct
{
	Palette *palette;
	u16 numAnimation;
	Animation **animations;
} SpriteDefinition;

/**
 *  \struct Sprite
 *      Sprite structure.<br/>
 *      Used to manage an active sprite in game condition.
 *
 *  \param spriteDef
 *      Sprite definition pointer
 *  \param animation
 *      Animation pointer cache
 *  \param frame
 *      AnimationFrame pointer cache
 *  \param x
 *      current sprite X position
 *  \param y
 *      current sprite Y position
 *  \param animInd
 *      current animation index
 *  \param frameInd
 *      current frame animation index
 *  \param seqInd
 *      current frame animation sequence index
 *  \param timer
 *      timer for current frame
 *  \param attribut
 *      sprite extra attribut (see TILE_ATTR() macro)
 *  \param fixedIndex
 *      fixed VRAM tile index for this sprite, by default it is set to -1 for dynamic allocation
 *  \param data
 *      misc data to handle sprite (free use for user)
 *  \param visibility
 *      visibility flag for each VDP sprite of current frame
 */
typedef struct
{
	SpriteDefinition *definition;
	Animation *animation;
	AnimationFrame *frame;
	s16 x;
	s16 y;
	s16 animInd;
	s16 frameInd;
	s16 seqInd;
	u16 timer;
	u16 attribut;
	s16 fixedIndex;
	u32 data;
	s32 visibility;
} Sprite;


/**
 *  \brief
 *      Init the Sprite engine.
 *
 *  \param cacheSize
 *      size of the tile cache (in tile) for the automatic tile allocation.<br/>
 *      If set to 0 the default size is used (384 tiles)
 *
 * Initialize the sprite engine.<br/>
 * This actually allocate memory for sprite cache and initialize the tile cache engine
 * if this is not alreay done.
 */
void SPR_init(u16 cacheSize);
/**
 *  \brief
 *      End the Sprite engine.
 *
 * End the sprite engine.<br/>
 * This actually release memory allocated for sprite cache and release the tile cache engine
 * if it was started by Sprite engine.
 */
void SPR_end();

/**
 *  \brief
 *      FALSE if sprite cache engine is not initialized.
 */
u16 SPR_isInitialized();

/**
 *  \brief
 *      Initialize the specified Sprite.<br/>
 *      By default a sprite use automatic VRAM tile allocation but you can fix it
 *      by using the SPR_setVRAMTileIndex(..) method.
 *
 *  \param sprite
 *      Sprite to initialize.
 *  \param spriteDef
 *      the SpriteDefinition data to assign to this sprite.
 *  \param x
 *      default X position.
 *  \param y
 *      default Y position.
 *  \param attribut
 *      sprite attribut (see TILE_ATTR() macro).
 */
void SPR_initSprite(Sprite *sprite, SpriteDefinition *spriteDef, s16 x, s16 y, u16 attribut);
/**
 *  \brief
 *      Set sprite position.
 *
 *  \param sprite
 *      Sprite to set position for
 *  \param x
 *      X position
 *  \param y
 *      Y position
 */
void SPR_setPosition(Sprite *sprite, s16 x, s16 y);
/**
 *  \brief
 *      Set sprite attribut.
 *
 *  \param sprite
 *      Sprite to set attribut for
 *  \param attribut
 *      attribut (see TILE_ATTRIBUT() macro)
 */
void SPR_setAttribut(Sprite *sprite, u16 attribut);
/**
 *  \brief
 *      Set current sprite animation and frame.
 *
 *  \param sprite
 *      Sprite to set animation and frame for
 *  \param anim
 *      animation index to set
 *  \param frame
 *      frame index to set
 */
void SPR_setAnimAndFrame(Sprite *sprite, s16 anim, s16 frame);

/**
 *  \brief
 *      Set current sprite animation.
 *
 *  \param sprite
 *      Sprite to set animation for
 *  \param anim
 *      animation index to set.
 */
void SPR_setAnim(Sprite *sprite, s16 anim);
/**
 *  \brief
 *      Set current sprite frame.
 *
 *  \param sprite
 *      Sprite to set frame for
 *  \param frame
 *      frame index to set.
 */
void SPR_setFrame(Sprite *sprite, s16 frame);
/**
 *  \brief
 *      Pass to the next sprite frame.
 *
 *  \param sprite
 *      Sprite to pass to next frame for
 */
void SPR_nextFrame(Sprite *sprite);

/**
 *  \brief
 *      Set the VRAM tile position for this sprite.
 *
 *  \param sprite
 *      Sprite to set the VRAM tile position for
 *  \param index
 *      the tile position in VRAM where we will upload the sprite tiles.<br/>
 *      Set to <b>-1</b> for automatic allocation (default).
 */
void SPR_setVRAMTileIndex(Sprite *sprite, s16 index);
/**
 *  \brief
 *      Set the <i>always visible</i> flag this sprite.<br>
 *      By setting it to TRUE the sprite is considered as always visible<br>
 *      and won't require any visibility computation.
 *
 *  \param sprite
 *      Sprite to set the <i>always visible</i> flag
 *  \param value
 *      The always visible flag value (<i>TRUE</i> or <i>FALSE</i>)
 *
 *  \see SPR_setNeverVisible(..)
 */
void SPR_setAlwaysVisible(Sprite *sprite, u16 value);
/**
 *  \brief
 *      Set the <i>never visible</i> flag this sprite.<br>
 *      By setting it to TRUE the sprite is considered as always hidden<br>
 *      and won't be displayed.
 *
 *  \param sprite
 *      Sprite to set the <i>never visible</i> flag
 *  \param value
 *      The never visible flag value (<i>TRUE</i> or <i>FALSE</i>)
 *
 *  \see SPR_setAlwaysVisible(..)
 */
void SPR_setNeverVisible(Sprite *sprite, u16 value);

/**
 *  \brief
 *      Test if specified sprites are in collision.
 *
 *  \param sprite1
 *      first sprite.
 *  \param sprite2
 *      second sprite.
 *  \return
 *      TRUE if sprite1 and sprite2 are in collision, FALSE otherwise.
 */
//u16 SPR_testCollision(Sprite *sprite1, Sprite *sprite2);

/**
 *  \brief
 *      Clear all displayed sprites.<br/>
 *      This actually clear the list cache and send it to the hardware (VDP) at Vint.
 */
void SPR_clear();
/**
 *  \brief
 *      Update and display the specified list of sprite.<br/>
 *      This actually prepare the list cache and send it to the hardware (VDP) at Vint.
 *
 *  \param sprites
 *      sprites we want to prepare and display.
 *  \param num
 *      number of sprites in the list.
 */
void SPR_update(Sprite *sprites, u16 num);

///**
// *  \brief
// *      Release allocated resources for the specified list of sprite.<br/>
// *      This actually release all allocated VRAM for sprite tile data.
// *
// *  \param sprites
// *      sprites we want to release.
// *  \param num
// *      number of sprites to release.
// */
//void SPR_release(Sprite *sprites, u16 num);


#endif // _SPRITE_ENG_H_

