/**
 *  \file sprite_eng.h
 *  \brief Sprite engine
 *  \author Stephane Dallongeville
 *  \date 10/2013
 *
 * Sprite engine providing advanced sprites manipulation and operations.<br>
 * This unit use both the tile cache engine (see tilecache.h file for more info)<br>
 * and the Sega Genesis VDP sprite capabilities (see vdp_spr.h file for more info).
 */

#ifndef _SPRITE_ENG_H_
#define _SPRITE_ENG_H_

#include "vdp_pal.h"
#include "vdp_tile.h"
#include "vdp_spr.h"


/**
 *  \brief
 *      Bouding box collision type (Box structure)
 */
#define COLLISION_TYPE_BOX      1
/**
 *  \brief
 *      Round circle collision type (Circle structure)
 */
#define COLLISION_TYPE_CIRCLE   2


/**
 *  \brief
 *      Enable automatic visibility calculation
 */
#define SPR_FLAG_AUTO_VISIBILITY        0x4000
/**
 *  \brief
 *      Enable fast visibility calculation (only meaningful if SPR_FLAG_AUTO_VISIBILITY is used)
 */
#define SPR_FLAG_FAST_AUTO_VISIBILITY   0x2000
/**
 *  \brief
 *      Enable automatic VRAM allocation
 */
#define SPR_FLAG_AUTO_VRAM_ALLOC        0x1000
/**
 *  \brief
 *      Enable automatic hardware sprite allocation
 */
#define SPR_FLAG_AUTO_SPRITE_ALLOC      0x0800
/**
 *  \brief
 *      Enable automatic upload of sprite tiles data into VRAM
 */
#define SPR_FLAG_AUTO_TILE_UPLOAD       0x0400
/**
 *  \brief
 *      Enable automatic Y sorting
 */
#define SPR_FLAG_AUTO_YSORTING          0x0200
/**
 *  \brief
 *      Enable 'always on top' state so the sprite always stay above others sprites whatever is sorting order
 */
#define SPR_FLAG_ALWAYS_ON_TOP          0x0100
/**
 *  \brief
 *      Mask for sprite flags
 */
#define SPR_FLAGS_MASK                  (SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_FAST_AUTO_VISIBILITY | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_SPRITE_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_AUTO_YSORTING | SPR_FLAG_ALWAYS_ON_TOP)

/**
 *  \brief
 *      Sprite visibility enumeration
 */
typedef enum
{
    VISIBLE,        /**< Sprite is visible (no computation needed) */
    HIDDEN,         /**< Sprite is hidden (no computation needed) */
    AUTO_FAST,      /**< Automatic visibility calculation FAST (computation made on global meta sprite) */
    AUTO_SLOW,      /**< Automatic visibility calculation SLOW (computation made per hardware sprite) */
} SpriteVisibility;

/**
 *  \struct Collision
 *  \brief
 *      Collision definition union.
 *
 *  \param type
 *      Collision type:<br>
 *      Allowed values are #COLLISION_TYPE_BOX or #COLLISION_TYPE_CIRCLE.
 *  \param box
 *      Box definition if type = #COLLISION_TYPE_BOX
 *  \param circle
 *      Circle definition if type = #COLLISION_TYPE_CIRCLE
 *  \param inner
 *      if current collision is verified the we test inner for more precise collisions if needed
 *  \param next
 *      if current collision is not verified then we test next for next collision if needed
 */
typedef struct _collision
{
    u16 type;
    union
    {
        Box box;
        Circle circle;
    } norm;
    union
    {
        Box box;
        Circle circle;
    } hflip;
    union
    {
        Box box;
        Circle circle;
    } vflip;
    union
    {
        Box box;
        Circle circle;
    } hvflip;
    void* inner;
    void* next;
} Collision;

/**
 *  \brief
 *      VDP sprite info structure for sprite resource definition.
 *
 *  \param y
 *      Y offset for this VDP sprite relative to global Sprite position plus 0x80 (0x80 = 0 = no offset)
 *  \param size
 *      sprite size (see SPRITE_SIZE macro)
 *  \param numTile
 *      number of tile for this VDP sprite (should be coherent with the given size field)
 *  \param x
 *      X offset for this VDP sprite relative to global Sprite position plus 0x80 (0x80 = 0 = no offset)
 */
typedef struct
{
    s16 y;          // respect VDP sprite field order
    u16 size;
    s16 x;
    u16 numTile;
}  VDPSpriteInf;


/**
 *  \brief
 *      Sprite animation frame structure.
 *
 *  \param numSprite
 *      number of VDP sprite which compose this frame
 *  \param vdpSpritesInf
 *      pointer to an array of VDP sprites info composing the frame (followed by H/V/HV flipped versions)
 *  \param collision
 *      collision structure
 *  \param tileset
 *      tileset containing tiles for this animation frame (ordered for sprite)
 *  \param w
 *      frame width in pixel
 *  \param h
 *      frame height in pixel
 *  \param timer
 *      active time for this frame (in 1/60 of second)
 */
typedef struct
{
    u16 numSprite;
    VDPSpriteInf **vdpSpritesInf;
    Collision *collision;
    TileSet *tileset;
    s16 w;
    s16 h;
    u16 timer;
} AnimationFrame;

/**
 *  \brief
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
 *      frame sequence index for loop (last index if no loop)
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
 *  \brief
 *      Sprite definition structure.
 *
 *  \param palette
 *      Default palette data
 *  \param numAnimation
 *      number of animation for this sprite
 *  \param animations
 *      animation definitions
 *  \param maxNumTile
 *      maximum number of tile used by a single animation frame (used for VRAM tile space allocation)
 *  \param maxNumSprite
 *      maximum number of VDP sprite used by a single animation frame (used for VDP sprite allocation)
 *
 *  Contains all animations for a Sprite and internal informations.
 */
typedef struct
{
    Palette *palette;
    u16 numAnimation;
    Animation **animations;
    u16 maxNumTile;
    u16 maxNumSprite;
} SpriteDefinition;

/**
 *  \brief
 *      Sprite structure used by the Sprite Engine to store state for a sprite.<br>
 *      WARNING: always use the #SPR_addSprite(..) method to allocate Sprite object.<br>
 *
 *  \param status
 *      Internal state and automatic allocation information (internal)
 *  \param spriteDef
 *      Sprite definition pointer
 *  \param animation
 *      Animation pointer cache (internal)
 *  \param frame
 *      AnimationFrame pointer cache (internal)
 *  \param animInd
 *      current animation index (internal)
 *  \param frameInd
 *      current frame animation index (internal)
 *  \param seqInd
 *      current frame animation sequence index (internal)
 *  \param timer
 *      timer for current frame (internal)
 *  \param x
 *      current sprite X position on screen
 *  \param y
 *      current sprite Y position on screen
 *  \param ylong
 *      32 bit version of Y used for Y sorting (internal)
 *  \param aot
 *      ALWAYS_ON_TOP state (internal)
 *  \param attribut
 *      sprite specific attribut and allocated VRAM tile index (see TILE_ATTR_FULL() macro)
 *  \param visibility
 *      visibility information of current frame for each VDP sprite (max = 16)
 *  \param VDPSpriteIndex
 *      index of first allocated VDP sprite (0 when no yet allocated)<br>
 *      Number of allocated VDP sprite is defined by definition->maxNumSprite
 *  \param frameNumSprite
 *      the number of VDP sprite used by the current frame (internal)
 *  \param lastVDPSprite
 *      Pointer to last VDP sprite used by this Sprite (used internally to update link between sprite)
 *  \param data
 *      this is a free field for user data, use it for whatever you want (flags, pointer...)
 *  \param prev
 *      pointer on previous Sprite in list
 *  \param next
 *      pointer on next Sprite in list
 *
 *  Used to manage an active sprite in game condition.
 */
typedef struct _Sprite
{
    u16 status;
    u16 visibility;
    const SpriteDefinition *definition;
    Animation *animation;
    AnimationFrame *frame;
    s16 animInd;
    s16 frameInd;
    s16 seqInd;
    u16 timer;
    s16 x;
    union
    {
        s32 ylong;
        struct
        {
            s16 aot;
            s16 y;
        };
    };
    u16 attribut;
    u16 VDPSpriteIndex;
    u16 frameNumSprite;
    VDPSprite *lastVDPSprite;
    u32 data;
    struct _Sprite *prev;
    struct _Sprite *next;
} Sprite;


/**
 *  \brief
 *      Callback for the sprite sorting method SPR_sort(..)
 *
 * This callback is used to compare 2 sprite objects.<br>
 * Return value should be:<br>
 * negatif if s1 is before s2<br>
 * 0 if s1 is equal to s2<br>
 * positif if s1 is after s2
 */
typedef s16 _spriteComparatorCallback(Sprite* s1, Sprite* s2);


/**
 *  \brief
 *      Init the Sprite engine with specified VRAM allocation and unpacking buffer size.
 *
 *  \param maxSprite
 *      Maximum number of sprite the Sprite Engine can handle, higher value requires more memory (maximum accepted = <i>127</i>).<br>
 *      If set to 0 the default value is used (40 sprites)
 *  \param vramSize
 *      size (in tile) of the VRAM region for the automatic VRAM tile allocation.<br>
 *      If set to 0 the default size is used (384 tiles)
 *  \param unpackBufferSize
 *      size of the buffer for unpacking sprite tilesets.<br>
 *      the buffer should be big enough to contains all unpacked tileset ready to be send to VRAM.<br>
 *      If set to 0 the default size is used (256 tiles)
 *
 *      Initialize the sprite engine.<br>
 *      This allocates a VRAM region for sprite tiles, memory for tileset unpacking and initialize
 *      hardware sprite allocation system.
 *
 *  \see SPR_end()
 */
void SPR_init(u16 maxSprite, u16 vramSize, u16 unpackBufferSize);
/**
 *  \brief
 *      End the Sprite engine.
 *
 *      End the sprite engine and release attached resources.<br>
 *      This releases the allocated VRAM region, memory for unpacking and hardware sprite.
 */
void SPR_end();
/**
 *  \brief
 *      FALSE if sprite cache engine is not initialized, TRUE otherwise.
 */
u16 SPR_isInitialized();

/**
 *  \brief
 *      Reset the Sprite engine.<br>
 *
 *      This method releases all allocated sprites and their resources.
 */
void SPR_reset();

/**
 *  \brief
 *      Adds a new sprite and returns it.
 *
 *  \param spriteDef
 *      the SpriteDefinition data to assign to this sprite.
 *  \param x
 *      default X position.
 *  \param y
 *      default Y position.
 *  \param attribut
 *      sprite attribut (see TILE_ATTR() macro).
 *  \param spriteIndex
 *      index of the first sprite in the VDP sprite table used to display this Sprite (should be > 0 and < 128).<br>
 *      IMPORTANT: this value is used only if you use manual VDP Sprite allocation (see the <i>flags</i> parameter).<br>
 *  \param flags
 *      specific settings for this sprite:<br>
 *      #SPR_FLAG_AUTO_VISIBILITY = Enable automatic sprite visibility calculation (you can also use SPR_setVisibility(..) method).<br>
 *      #SPR_FLAG_FAST_AUTO_VISIBILITY = Enable fast computation for the automatic visibility calculation (disabled by default)<br>
 *          If you set this flag the automatic visibility calculation will be done globally for the (meta) sprite and not per internal
 *          hardware sprite. This result in faster visibility computation at the expense of some waste of hardware sprite.
 *          You can set the automatic visibility computation by using SPR_setVisibility(..) method.<br>
 *      #SPR_FLAG_AUTO_VRAM_ALLOC = Enable automatic VRAM allocation (enabled by default)<br>
 *          If you don't set this flag you will have to manually define VRAM tile index position for this sprite with the <i>attribut</i> parameter or by using the #SPR_setVRAMTileIndex(..) method<br>
 *      #SPR_FLAG_AUTO_SPRITE_ALLOC = Enable automatic hardware/VDP sprite allocation (enabled by default)<br>
 *          If you don't set this flag you will have to manually define the hardware sprite table index to reserve with the <i>spriteIndex</i> parameter or by using the #SPR_setSpriteTableIndex(..) method<br>
 *      #SPR_FLAG_AUTO_TILE_UPLOAD = Enable automatic upload of sprite tiles data into VRAM (enabled by default)<br>
 *          If you don't set this flag you will have to manually upload tiles data of sprite into the VRAM.<br>
 *      #SPR_FLAG_AUTO_YSORTING = Enable automatic Y sorting for this sprite so it will always appear in front of sprites with lower Y position.<br>
 *          If you don't set this flag you can still use SPR_sortOnY() to do Y sorting on the whole sprite list.<br>
 *      #SPR_FLAG_ALWAYS_ON_TOP = Enable 'always on top' this sprite so it will always appear above others sprites whatever sorting order is.<br>
 *      <br>
 *      It's recommended to use the following default settings:<br>
 *      SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_SPRITE_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD<br>
 *  \return the new sprite or <i>NULL</i> if the operation failed (some logs can be generated in the KMod console in this case)
 *
 *      By default the sprite uses the provided flags setting for automatic resources allocation and sprite visibility computation.<br>
 *      If auto visibility is not enabled then sprite is considered as not visible by default (see SPR_setVisibility(..) method).<br>
 *      You can release all sprite resources by using SPR_releaseSprite(..) or SPR_reset(..).
 *
 *  \see SPR_addSprite(..)
 *  \see SPR_releaseSprite(..)
 */
Sprite* SPR_addSpriteEx(const SpriteDefinition *spriteDef, s16 x, s16 y, u16 attribut, u16 spriteIndex, u16 flags);
/**
 *  \brief
 *      Adds a new sprite with auto resource allocation enabled and returns it.
 *
 *  \param spriteDef
 *      the SpriteDefinition data to assign to this sprite.
 *  \param x
 *      default X position.
 *  \param y
 *      default Y position.
 *  \param attribut
 *      sprite attribut (see TILE_ATTR() macro).
 *  \return the new sprite or <i>NULL</i> if the operation failed (some logs can be generated in the KMod console in this case)
 *
 *      By default the sprite uses automatic resources allocation (VRAM and hardware sprite) and visibility is set to ON.<br>
 *      You can change these defaults settings later by calling SPR_setVRAMTileIndex(..), SPR_setSpriteTableIndex(..), SPR_setAutoTileUpload(..) and SPR_setVisibility(..) methods.<br>
 *      You can release all sprite resources by using SPR_releaseSprite(..) or SPR_reset(..).
 *
 *  \see SPR_addSpriteEx(..)
 *  \see SPR_releaseSprite(..)
 */
Sprite* SPR_addSprite(const SpriteDefinition *spriteDef, s16 x, s16 y, u16 attribut);
/**
 *  \brief
 *      Release the specified sprite (no more visible and release its resources).
 *
 *  \param sprite
 *      Sprite to release
 *
 *      This method release resources for the specified Sprite object and remove it from the screen at next SPR_update() call.
 *
 *  \see SPR_releasesSprite(..)
 */
void SPR_releaseSprite(Sprite* sprite);
/**
 *  \brief
 *      Returns the number of active sprite (number of sprite added with SPR_addSprite(..) method).
 */
u16 SPR_getNumActiveSprite();

/**
 *  \brief
 *      Set the Sprite Definition.
 *
 *  \param sprite
 *      Sprite to set definition for.
 *  \param spriteDef
 *      the SpriteDefinition data to assign to this sprite.
 *
 *   Set the Sprite Definition for this sprite.<br>
 *   By default the first frame of the first animation from Sprite Definition is loaded.
 *
 *  \return FALSE if auto resource allocation failed, TRUE otherwise.
 */
u16 SPR_setDefinition(Sprite *sprite, const SpriteDefinition *spriteDef);
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
 *      Set sprite Horizontal Flip attribut.
 *
 *  \param sprite
 *      Sprite to set attribut for
 *  \param value
 *      The horizontal flip attribut value (TRUE or FALSE)
 */
void SPR_setHFlip(Sprite *sprite, u16 value);
/**
 *  \brief
 *      Set sprite Vertical Flip attribut.
 *
 *  \param sprite
 *      Sprite to set attribut for
 *  \param value
 *      The vertical flip attribut value (TRUE or FALSE)
 */
void SPR_setVFlip(Sprite *sprite, u16 value);
/**
 *  \brief
 *      Set sprite Palette index to use.
 *
 *  \param sprite
 *      Sprite to set attribut for
 *  \param value
 *      The palette index to use for this sprite (PAL0, PAL1, PAL2 or PAL3)
 */
void SPR_setPalette(Sprite *sprite, u16 value);
/**
 *  \brief
 *      Set sprite Priority attribut.
 *
 *  \param sprite
 *      Sprite to set attribut for
 *  \param value
 *      The priority attribut value (TRUE or FALSE)
 */
void SPR_setPriorityAttribut(Sprite *sprite, u16 value);

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
 *      Set the VRAM tile position reserved for this sprite.
 *
 *  \param sprite
 *      Sprite to set the VRAM tile position for
 *  \param value
 *      the tile position in VRAM where we will upload the sprite tiles data.<br>
 *      Use <b>-1</b> for auto allocation.<br>
 *  \return FALSE if auto allocation failed (can happen only if sprite is currently active), TRUE otherwise
 *
 *  By default the Sprite Engine auto allocate VRAM for sprites tiles but you can force
 *  manual allocation and fix the sprite tiles position in VRAM with this method.
 */
u16 SPR_setVRAMTileIndex(Sprite *sprite, s16 value);
/**
 *  \brief
 *      Set the VDP sprite index to use for this sprite.
 *
 *  \param sprite
 *      Sprite to set the VDP Sprite index for
 *  \param value
 *      the index of the first sprite in the VDP sprite table used to display this Sprite (should be > 0 and < 128).<br>
 *      Use <b>-1</b> for auto allocation.<br>
 *  \return FALSE if auto allocation failed (can happen only if sprite is currently active), TRUE otherwise
 *
 *  By default the Sprite Engine auto allocate VDP sprite but you can force
 *  manual allocation and fix the index of the first VDP sprite to use with this method.<br>
 *  If you set the index manually you need to ensure you have enough available contiguous VDP sprites at this
 *  index so it can fit the current sprite requirement in VDP sprite.
 *  <b>WARNING: you cannot use sprite 0 as it is internally reserved.</b>
 */
u16 SPR_setSpriteTableIndex(Sprite *sprite, s16 value);
/**
 *  \brief
 *      Enable/disable the automatic upload of sprite tiles data into VRAM.
 *
 *  \param sprite
 *      Sprite we want to enable/disable auto tile upload for
 *  \param value
 *      TRUE to enable the automatic upload of sprite tiles data into VRAM.<br>
 *      FALSE to disable it (mean you have to handle that on your own).<br>
 */
void SPR_setAutoTileUpload(Sprite *sprite, u16 value);
/**
 *  \brief
 *      Enable/disable the automatic Y sorting for this sprite so it will always appear in front of sprites will lower Y position.<br>
 *      Note that you can also use SPR_sortOnY() to do manual Y sorting on the whole sprite list.
 *
 *  \param sprite
 *      Sprite we want to enable/disable Y sorting for
 *  \param value
 *      TRUE to enable the automatic Y sorting for this sprite<br>
 *      FALSE to disable it, you can still use the SPR_sort(..) method.<br>
 *
 *  \see SPR_sort(..)
 */
void SPR_setYSorting(Sprite *sprite, u16 value);
/**
 *  \brief
 *      Enable/disable 'always on top' state for this sprite so it will always appear above others sprites whatever sorting order is.<br>the automatic Y sorting for this sprite
 *      so it will always appear in front of sprites will lower Y position.
 *
 *  \param sprite
 *      Sprite we want to enable/disable 'always on top' state
 *  \param value
 *      TRUE to enable the 'always on top' state for this sprite<br>
 *      FALSE to disable it
 *
 *  \see SPR_sort(..)
 *  \see SPR_setYSorting(..)
 */
void SPR_setAlwaysOnTop(Sprite *sprite, u16 value);
/**
 *  \brief
 *      Set the <i>visibility</i> state for this sprite.
 *
 *  \param sprite
 *      Sprite to set the <i>visibility</i> information
 *  \param value
 *      Visibility value to set.<br>
 *      SpriteVisibility.VISIBLE       = sprite is visible<br>
 *      SpriteVisibility.HIDDEN        = sprite is not visible<br>
 *      SpriteVisibility.AUTO_FAST     = visibility is automatically computed from sprite position (global visibility)<br>
 *      SpriteVisibility.AUTO_SLOW     = visibility is automatically computed from sprite position (per hardware sprite visibility)<br>
 */
void SPR_setVisibility(Sprite *sprite, SpriteVisibility value);
/**
 *  \deprecated Use #SPR_setVisibility(..) method instead.
 */
void SPR_setAlwaysVisible(Sprite *sprite, u16 value);
/**
 *  \deprecated Use #SPR_setVisibility(..) method instead.
 */
void SPR_setNeverVisible(Sprite *sprite, u16 value);
/**
 *  \brief
 *      Update the internal <i>visibility</i> state for this sprite (when AUTO visibility is enabled).<br>
 *
 *  \param sprite
 *      Sprite to compute <i>visibility</i> state
 *  \return TRUE if sprite is visible, FALSE otherwise
 *
 *  This method computes the visibility state for the specified sprite if the AUTO visibility is enabled (see SPR_setVisibility(..) method).<br>
 *  Note that sprite visibility is automatically computed when you call SPR_update() so you need to use this method
 *  only if you want to know if the sprite is visible or not before the next SPR_update() call.
 *
 *  \see SPR_setVisibility(..)
 */
u16 SPR_computeVisibility(Sprite *sprite);

// /**
// *  \brief
// *      Test if specified sprites are in collision.
// *
// *  \param sprite1
// *      first sprite.
// *  \param sprite2
// *      second sprite.
// *  \return
// *      TRUE if sprite1 and sprite2 are in collision, FALSE otherwise.
// */
//u16 SPR_testCollision(Sprite *sprite1, Sprite *sprite2);

/**
 *  \brief
 *      Clear all displayed sprites.
 *
 *  This method allow to quickly hide all sprites (without releasing their resources).<br>
 *  Sprites can be displayed again just by calling SPR_update().
 */
void SPR_clear();
/**
 *  \brief
 *      Update and display the active list of sprite.
 *
 *  This actually updates all internal active sprites states and prepare the sprite list
 *  cache to send it to the hardware (VDP) at Vint.
 *
 *  \see #SPR_addSprite(..)
 */
void SPR_update();
/**
 *  \brief
 *      Sort the sprites to define display order.
 *
 *  This method uses the given comparator callback to sort the whole list of Sprite and so define
 *  the display order of the sprites.<br>
 *  If the comparator callback is set to NULL then by default Y sorting is performed.<br>
 *  This method can take a long time, use it carefully !
 *
 *  \param comparator
 *      the comparator callback used to compare sprites.<br>
 *      The comparator is responsible to take care of the ALWAYS_ON_TOP sprite state (using <i>aot</i> sprite field).<br>
 *      It should return a value < 0 if sprite 1 is below sprite 2 and a value > 0 in the opposite case.<br>
 *      If order doesn't matter it can return 0.
 *
 *  \see #SPR_sortOnYPos()
 */
void SPR_sort(_spriteComparatorCallback* comparator);
/**
 *  \brief
 *      Sort the sprites by their Y position to define display order (same as SPR_sort(NULL))
 *
 *  \see #SPR_sort(..)
 */
void SPR_sortOnYPos();

/**
 *  \brief
 *      Log the profil informations (when enabled) in the KMod message window.
 */
void SPR_logProfil();
/**
 *  \brief
 *      Log the sprites informations (when enabled) in the KMod message window.
 */
void SPR_logSprites();


#endif // _SPRITE_ENG_H_

