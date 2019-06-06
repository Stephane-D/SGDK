/**
 *  \file sprite_eng.h
 *  \brief Sprite engine
 *  \author Stephane Dallongeville
 *  \date 10/2013
 *
 * Sprite engine providing advanced sprites manipulation and operations.<br>
 * This unit use both the vram memory manager (see vram.h file for more info)<br>
 * and the Sega Genesis VDP sprite capabilities (see vdp_spr.h file for more info).
 */

#ifndef _SPRITE_ENG_H_
#define _SPRITE_ENG_H_

#include "vdp_tile.h"
#include "vdp_spr.h"
#include "pal.h"


/**
 *  \brief
 *      No collision type
 */
#define COLLISION_TYPE_NONE     0
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
 *      Special flag to indicate that we want to add the sprite at position 0 (head) in the list<br>
 *      instead of adding it in last position (default)
 */
#define SPR_FLAG_INSERT_HEAD                    0x4000

/**
 *  \brief
 *      Disable delaying of frame update when we are running out of DMA capacity.<br>
 *      By default we delay the frame update when DMA is already full so the frame update happen as soon we have enough DMA capacity to do it.
 *      This flag forces frame update to always happen immediately but that may cause graphical glitches.
 */
#define SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE   0x2000
/**
 *  \brief
 *      Enable automatic visibility calculation
 */
#define SPR_FLAG_AUTO_VISIBILITY                0x1000
/**
 *  \brief
 *      Enable fast visibility calculation (only meaningful if SPR_FLAG_AUTO_VISIBILITY is used)
 */
#define SPR_FLAG_FAST_AUTO_VISIBILITY           0x0800
/**
 *  \brief
 *      Enable automatic VRAM allocation
 */
#define SPR_FLAG_AUTO_VRAM_ALLOC                0x0400
/**
 *  \brief
 *      Enable automatic hardware sprite allocation
 */
#define SPR_FLAG_AUTO_SPRITE_ALLOC              0x0200
/**
 *  \brief
 *      Enable automatic upload of sprite tiles data into VRAM
 */
#define SPR_FLAG_AUTO_TILE_UPLOAD               0x0100
/**
 *  \brief
 *      Mask for sprite flag
 */
#define SPR_FLAG_MASK                           (SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE | SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_FAST_AUTO_VISIBILITY | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_SPRITE_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD)

/**
 *  \brief
 *      Minimum depth for a sprite (always above others sprites)
 */
#define SPR_MIN_DEPTH       (-0x8000)
/**
 *  \brief
 *      Maximum depth for a sprite (always below others sprites)
 */
#define SPR_MAX_DEPTH       0x7FFF

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
 *  \brief
 *      Simple Box structure for collision
 *
 *  \param x
 *      X position (left)
 *  \param y
 *      Y position (top)
 *  \param w
 *      width
 *  \param h
 *      heigth
 */
typedef struct
{
    s8 x;
    s8 y;
    u8 w;
    u8 h;
} BoxCollision;

/**
 *  \brief
 *      Simple Circle structure (can be used for collision detection)
 *
 *  \param x
 *      X center position
 *  \param y
 *      Y center position
 *  \param ray
 *      circle ray
 */
typedef struct
{
    s8 x;
    s8 y;
    u16 ray;
} CircleCollision;

/**
 *  \struct Collision
 *  \brief
 *      Collision definition union.
 *
 *  \param typeHit
 *      Collision type for hit collision:<br>
 *      Allowed values are #COLLISION_TYPE_BOX or #COLLISION_TYPE_CIRCLE.
 *  \param typeAttack
 *      Collision type for attack collision (can be used as alternative hit collision):<br>
 *      Allowed values are #COLLISION_TYPE_BOX or #COLLISION_TYPE_CIRCLE.
 *  \param box
 *      BoxCollision definition if type = #COLLISION_TYPE_BOX
 *  \param circle
 *      CircleCollision definition if type = #COLLISION_TYPE_CIRCLE
 *  \param inner
 *      if current collision is verified then we test inner for more precise collision if needed
 *  \param next
 *      if current collision is not verified then we test next for next collision if needed
 */
typedef struct _collision
{
    u8 typeHit;
    u8 typeAttack;
    union
    {
        BoxCollision box;
        CircleCollision circle;
    } hit;
    union
    {
        BoxCollision box;
        CircleCollision circle;
    } attack;
} Collision;

/**
 *  \brief
 *      Single VDP sprite info structure for sprite animation frame.
 *
 *  \param numTile
 *      number of tile for this VDP sprite (should be coherent with the given size field)
 *  \param offsetY
 *      Y offset for this VDP sprite relative to global Sprite position
 *  \param size
 *      sprite size (see SPRITE_SIZE macro)
 *  \param offsetX
 *      X offset for this VDP sprite relative to global Sprite position
 */
typedef struct
{
    u8 numTile;
    s8 offsetY;          // respect VDP sprite field order
    u8 size;
    s8 offsetX;
}  FrameVDPSprite;

/**
 *  \brief
 *      Frame information structure (used to store frame info for base, H-Flip, V-Flip and HV-Flip version of animation frame)
 *
 *  \param frameSprites
 *      pointer to an array of VDP sprites info composing the frame
 *  \param collision
 *      collision structure
 */
typedef struct
{
    FrameVDPSprite** frameVDPSprites;
    Collision* collision;
} FrameInfo;

/**
 *  \brief
 *      Sprite animation frame structure.
 *
 *  \param numSprite
 *      number of VDP sprite which compose this frame
 *  \param frameInfos
 *      frame information for [base, hflip, vflip, hvflip] version of the sprite
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
    u8 numSprite;               // we use u8 to not waste ROM space
    u8 w;
    u8 h;
    u8 timer;
    FrameInfo frameInfos[4];
    TileSet* tileset;           // TODO: have a tileset per VDP sprite (when rescomp will be optimized for better LZ4W compression)
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
    AnimationFrame** frames;
    u16 length;
    u8* sequence;
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
    Palette* palette;
    u16 numAnimation;
    Animation** animations;
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
 *  \param depth
 *      current sprite depth (Z) position used for Z sorting
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
    const SpriteDefinition* definition;
    Animation* animation;
    AnimationFrame* frame;
    FrameInfo* frameInfo;
    s16 animInd;
    s16 frameInd;
    s16 seqInd;
    u16 timer;
    s16 x;
    s16 y;
    s16 depth;
    u16 attribut;
    u16 VDPSpriteIndex;
    u16 spriteToHide;
    VDPSprite* lastVDPSprite;
    u32 data;
    struct _Sprite* prev;
    struct _Sprite* next;
} Sprite;


/**
 *  \brief
 *      Initialize the Sprite engine with default parameters.
 *
 *      Initialize the sprite engine using default parameters:<br>
 *      512 tiles reserved in VRAM and 320 tiles in memory for the decompression buffer.<br>
 *      This also initialize the hardware sprite allocation system.
 *
 *  \see SPR_initEx()
 *  \see SPR_end()
 */
void SPR_init();
/**
 *  \brief
 *      Init the Sprite engine with specified advanced parameters (VRAM allocation size and decompression buffer size).
 *
 *  \param vramSize
 *      size (in tile) of the VRAM region for the automatic VRAM tile allocation.<br>
 *      If set to 0 the default size is used (512 tiles)
 *  \param unpackBufferSize
 *      size of the buffer for unpacking sprite tilesets.<br>
 *      the buffer should be big enough to contains all unpacked tileset ready to be send to VRAM.<br>
 *      If set to 0 the default size is used (320 tiles)
 *
 *      Initialize the sprite engine.<br>
 *      This allocates a VRAM region for sprite tiles, memory for tileset unpacking and initialize
 *      hardware sprite allocation system.
 *
 *  \see SPR_init()
 *  \see SPR_end()
 */
void SPR_initEx(u16 vramSize, u16 unpackBufferSize);
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
bool SPR_isInitialized();

/**
 *  \brief
 *      Reset the Sprite engine.<br>
 *
 *      This method releases all allocated sprites and their resources.
 */
void SPR_reset();

/**
 *  \brief
 *      Adds a new sprite with specified parameters and returns it.
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
 *      index of the first sprite in the VDP sprite table used to display this Sprite (should be in [1..79] range.<br>
 *      IMPORTANT: this value is used only if you use manual VDP Sprite allocation (see the <i>flags</i> parameter).<br>
 *  \param flag
 *      specific settings for this sprite:<br>
 *      #SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE = Disable delaying of frame update when we are running out of DMA capacity.<br>
 *          If you set this flag then sprite frame update always happen immediately but may lead to some graphical glitches (tiles data and sprite table data not synchronized).
 *          You can use SPR_setDelayedFrameUpdate(..) method to change this setting.<br>
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
 *          If you don't set this flag you will have to manually upload tiles data of sprite into the VRAM (you can change this setting using #SPR_setAutoTileUpload(..) method).<br>
 *      <br>
 *      It's recommended to use the following default settings:<br>
 *      SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_SPRITE_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD<br>
 *  \return the new sprite or <i>NULL</i> if the operation failed (some logs can be generated in the KMod console in this case)
 *
 *      By default the sprite uses the provided flag setting for automatic resources allocation and sprite visibility computation.<br>
 *      If auto visibility is not enabled then sprite is considered as not visible by default (see SPR_setVisibility(..) method).<br>
 *      You can release all sprite resources by using SPR_releaseSprite(..) or SPR_reset(..).<br>
 *      IMPORTANT NOTE: sprite allocation can fail (return NULL) when you are using auto VRAM allocation (SPR_FLAG_AUTO_VRAM_ALLOC) even if there is enough VRAM available,<br>
 *      this can happen because of the VRAM fragmentation. You can use #SPR_addSpriteExSafe(..) method instead so it take care about VRAM fragmentation.
 *
 *  \see SPR_addSprite(..)
 *  \see SPR_addSpriteExSafe(..)
 *  \see SPR_releaseSprite(..)
 */
Sprite* SPR_addSpriteEx(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut, u16 spriteIndex, u16 flag);
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
 *      You can release all sprite resources by using SPR_releaseSprite(..) or SPR_reset(..).<br>
 *      IMPORTANT NOTE: sprite allocation can fail (return NULL) because of automatic VRAM allocation even if there is enough VRAM available,
 *      this can happen because of the VRAM fragmentation.<br> You can use #SPR_addSpriteSafe(..) method instead so it take care about VRAM fragmentation.
 *
 *  \see SPR_addSpriteEx(..)
 *  \see SPR_addSpriteSafe(..)
 *  \see SPR_releaseSprite(..)
 */
Sprite* SPR_addSprite(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut);
/**
 *  \brief
 *      Adds a new sprite with specified parameters and returns it.
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
 *      IMPORTANT: this value is used only if you use manual VDP Sprite allocation (see the <i>flag</i> parameter).<br>
 *  \param flag
 *      specific settings for this sprite:<br>
 *      #SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE = Disable delaying of frame update when we are running out of DMA capacity.<br>
 *          If you set this flag then sprite frame update always happen immediately but may lead to some graphical glitches (tiles data and sprite table data not synchronized).
 *          You can use SPR_setDelayedFrameUpdate(..) method to change this setting.<br>
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
 *          If you don't set this flag you will have to manually upload tiles data of sprite into the VRAM (you can change this setting using #SPR_setAutoTileUpload(..) method).<br>
 *      <br>
 *      It's recommended to use the following default settings:<br>
 *      SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_SPRITE_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD<br>
 *  \return the new sprite or <i>NULL</i> if the operation failed (some logs can be generated in the KMod console in this case)
 *
 *      By default the sprite uses the provided flag setting for automatic resources allocation and sprite visibility computation.<br>
 *      If auto visibility is not enabled then sprite is considered as not visible by default (see SPR_setVisibility(..) method).<br>
 *      You can release all sprite resources by using SPR_releaseSprite(..) or SPR_reset(..).
 *
 *  \see SPR_addSpriteSafe(..)
 *  \see SPR_addSpriteEx(..)
 *  \see SPR_releaseSprite(..)
 */
Sprite* SPR_addSpriteExSafe(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut, u16 spriteIndex, u16 flag);
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
 *      You can release all sprite resources by using SPR_releaseSprite(..) or SPR_reset(..).<
 *
 *  \see SPR_addSpriteSafeEx(..)
 *  \see SPR_addSprite(..)
 *  \see SPR_releaseSprite(..)
 */
Sprite* SPR_addSpriteSafe(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut);

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
 *      Returns the number of active sprite (number of sprite added with SPR_addSprite(..) or SPR_addSpriteEx(..) methods).
 */
u16 SPR_getNumActiveSprite();
/**
 *  \brief
 *      Defragment allocated VRAM for sprites, that can help when sprite allocation fail (SPR_addSprite(..) or SPR_addSpriteEx(..) return <i>NULL</i>).
 */
void SPR_defragVRAM();

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
bool SPR_setDefinition(Sprite* sprite, const SpriteDefinition* spriteDef);
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
void SPR_setPosition(Sprite* sprite, s16 x, s16 y);
/**
 *  \brief
 *      Set sprite Horizontal Flip attribut.
 *
 *  \param sprite
 *      Sprite to set attribut for
 *  \param value
 *      The horizontal flip attribut value (TRUE or FALSE)
 */
void SPR_setHFlip(Sprite* sprite, u16 value);
/**
 *  \brief
 *      Set sprite Vertical Flip attribut.
 *
 *  \param sprite
 *      Sprite to set attribut for
 *  \param value
 *      The vertical flip attribut value (TRUE or FALSE)
 */
void SPR_setVFlip(Sprite* sprite, u16 value);
/**
 *  \brief
 *      Set sprite Palette index to use.
 *
 *  \param sprite
 *      Sprite to set attribut for
 *  \param value
 *      The palette index to use for this sprite (PAL0, PAL1, PAL2 or PAL3)
 */
void SPR_setPalette(Sprite* sprite, u16 value);
/**
 *  \brief
 *      Set sprite Priority attribut.
 *
 *  \param sprite
 *      Sprite to set attribut for
 *  \param value
 *      The priority attribut value (TRUE or FALSE)
 */
void SPR_setPriorityAttribut(Sprite* sprite, u16 value);
/**
 *  \brief
 *      Set sprite depth (for sprite display ordering)
 *
 *  \param sprite
 *      Sprite to set depth for
 *  \param value
 *      The depth value (SPR_MIN_DEPTH to set always on top)
 *
 *  Sprite having lower depth are display in front of sprite with higher depth.<br>
 *  The sprite is *immediately* sorted when its depth value is changed.
 */
void SPR_setDepth(Sprite* sprite, s16 value);
/**
 *  \brief
 *      Same as #SPR_setDepth(..)
 */
void SPR_setZ(Sprite* sprite, s16 value);
/**
 *  \deprecated Use SPR_setDepth(SPR_MIN_DEPTH) instead
 */
void SPR_setAlwaysOnTop(Sprite* sprite, u16 value);
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
void SPR_setAnimAndFrame(Sprite* sprite, s16 anim, s16 frame);
/**
 *  \brief
 *      Set current sprite animation.
 *
 *  \param sprite
 *      Sprite to set animation for
 *  \param anim
 *      animation index to set.
 */
void SPR_setAnim(Sprite* sprite, s16 anim);
/**
 *  \brief
 *      Set current sprite frame.
 *
 *  \param sprite
 *      Sprite to set frame for
 *  \param frame
 *      frame index to set.
 */
void SPR_setFrame(Sprite* sprite, s16 frame);
/**
 *  \brief
 *      Pass to the next sprite frame.
 *
 *  \param sprite
 *      Sprite to pass to next frame for
 */
void SPR_nextFrame(Sprite* sprite);

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
bool SPR_setVRAMTileIndex(Sprite* sprite, s16 value);
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
bool SPR_setSpriteTableIndex(Sprite* sprite, s16 value);
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
void SPR_setAutoTileUpload(Sprite* sprite, bool value);
/**
 *  \brief
 *      Enable/disable the delayed frame update.
 *
 *  \param sprite
 *      Sprite we want to enable/disable delayed frame update
 *  \param value
 *      TRUE to enable the delayed frame update when DMA is running out of transfert capacity.<br>
 *      FALSE to disable it (sprite frame is always updated immediately but that may cause graphical glitches).<br>
 *
 *  \see #SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE
 */
void SPR_setDelayedFrameUpdate(Sprite* sprite, bool value);
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
void SPR_setVisibility(Sprite* sprite, SpriteVisibility value);
/**
 *  \deprecated Use #SPR_setVisibility(..) method instead.
 */
void SPR_setAlwaysVisible(Sprite* sprite, u16 value);
/**
 *  \deprecated Use #SPR_setVisibility(..) method instead.
 */
void SPR_setNeverVisible(Sprite* sprite, u16 value);
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
bool SPR_computeVisibility(Sprite* sprite);

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
//u16 SPR_testCollision(Sprite* sprite1, Sprite* sprite2);

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
 *      Log the profil informations (when enabled) in the KMod message window.
 */
void SPR_logProfil();
/**
 *  \brief
 *      Log the sprites informations (when enabled) in the KMod message window.
 */
void SPR_logSprites();


#endif // _SPRITE_ENG_H_
