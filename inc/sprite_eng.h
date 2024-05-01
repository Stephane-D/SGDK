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

#if     !LEGACY_SPRITE_ENGINE

#ifndef _SPRITE_ENG_H_
#define _SPRITE_ENG_H_

#include "vdp_tile.h"
#include "vdp_spr.h"
#include "pal.h"
#include "pool.h"


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
 *      Special flag to indicate that we want to add the sprite at position 0 (head / top) in the list<br>
 *      instead of adding it in last / bottom position (default)
 */
#define SPR_FLAG_INSERT_HEAD                    0x4000
/**
 *  \brief
 *      Disable animation auto loop..<br>
 *      By default animation always restart fater the last frame has been played.
 *      This flag prevent the animation to restart and so the animation end on the last frame forever (see #SPR_getAnimationDone(..))
 */
#define SPR_FLAG_DISABLE_ANIMATION_LOOP         0x2000
/**
 *  \brief
 *      Disable delaying of frame update when we are running out of DMA capacity.<br>
 *      By default we delay the frame update when DMA is already full so the frame update happen as soon we have enough DMA capacity to do it.
 *      This flag forces frame update to always happen immediately but that may cause graphical glitches.
 */
#define SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE   0x1000
/**
 *  \brief
 *      Enable automatic VRAM allocation
 */
#define SPR_FLAG_AUTO_VRAM_ALLOC                0x0800
/**
 *  \brief
 *      Enable automatic upload of sprite tiles data into VRAM
 */
#define SPR_FLAG_AUTO_TILE_UPLOAD               0x0400
/**
 *  \brief
 *      Enable automatic visibility calculation
 */
#define SPR_FLAG_AUTO_VISIBILITY                0x0200
/**
 *  \brief
 *      Enable fast visibility calculation (only meaningful if SPR_FLAG_AUTO_VISIBILITY is used).<br>
 *      If you set this flag the automatic visibility calculation will be done globally for the (meta) sprite and not per internal
 *      hardware sprite. This result in faster visibility computation at the expense of using extra (wasting) hardware sprites.
 */
#define SPR_FLAG_FAST_AUTO_VISIBILITY           0x0100

/**
 *  \brief
 *      Mask for sprite flag
 */
#define SPR_FLAG_MASK                           (SPR_FLAG_INSERT_HEAD | SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_FAST_AUTO_VISIBILITY)

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
    AUTO_FAST,      /**< Automatic visibility calculation - FAST (computation made on global meta sprite) */
    AUTO_SLOW,      /**< Automatic visibility calculation - SLOW (computation made per hardware sprite) */
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
 *  \param offsetY
 *      Y offset for this VDP sprite relative to global Sprite position
 *  \param offsetYFlip
 *      Y offset (flip version) for this VDP sprite relative to global Sprite position
 *  \param size
 *      sprite size (see SPRITE_SIZE macro)
 *  \param offsetX
 *      X offset for this VDP sprite relative to global Sprite position
 *  \param offsetXFlip
 *      X offset (flip version) for this VDP sprite relative to global Sprite position
 *  \param numTile
 *      number of tile for this VDP sprite (should be coherent with the given size field)
 */
typedef struct
{
    u8 offsetY;          // respect VDP sprite field order, may help
    u8 offsetYFlip;
    u8 size;
    u8 offsetX;
    u8 offsetXFlip;
    u8 numTile;
}  FrameVDPSprite;

/**
 *  \brief
 *      Sprite animation frame structure.
 *
 *  \param numSprite
 *      number of VDP sprite which compose this frame
 *  \param timer
 *      active time for this frame (in 1/60 of second)
 *  \param tileset
 *      tileset containing tiles for this animation frame (ordered for sprite)
 *  \param collision
 *      collision structure (not used currently)
 *  \param frameSprites
 *      array of VDP sprites info composing the frame
 */
typedef struct
{
    u8 numSprite;
    u8 timer;
    TileSet* tileset;                   // TODO: have a tileset per VDP sprite --> probably not a good idea performance wise
    Collision* collision;               // Require many DMA queue operations and fast DMA flush as well, also bring extra computing in calculating delayed update
    FrameVDPSprite frameVDPSprites[];
} AnimationFrame;

/**
 *  \brief
 *      Sprite animation structure.
 *
 *  \param numFrame
 *      number of different frame for this animation (max = 255)
 *  \param loop
 *      frame index for loop (last index if no loop)
 *  \param frames
 *      frames composing the animation
 */
typedef struct
{
    u8 numFrame;
    u8 loop;
    AnimationFrame** frames;
} Animation;

/**
 *  \brief
 *      Sprite definition structure.
 *
 *  \param w
 *      frame cell width in pixel
 *  \param h
 *      frame cell height in pixel
 *  \param palette
 *      default palette data
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
    u16 w;
    u16 h;
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
 *  \param visibility
 *      visibility information of current frame for each VDP sprite (max = 16)
 *  \param spriteDef
 *      Sprite definition pointer
 *  \param onFrameChange
 *      Custom callback on frame change event (see #SPR_setFrameChangeCallback(..) method)
 *  \param animation
 *      Animation pointer cache (internal)
 *  \param frame
 *      AnimationFrame pointer cache (internal)
 *  \param animInd
 *      current animation index (internal)
 *  \param frameInd
 *      current frame animation index (internal)
 *  \param timer
 *      timer for current frame (internal)
 *  \param x
 *      current sprite X position on screen offseted by 0x80 (internal VDP position)
 *  \param y
 *      current sprite Y position on screen offseted by 0x80 (internal VDP position)
 *  \param depth
 *      current sprite depth (Z) position used for Z sorting
 *  \param attribut
 *      sprite specific attribut and allocated VRAM tile index (see TILE_ATTR_FULL() macro)
 *  \param data
 *      this is a free field for user data, use it for whatever you want (flags, pointer...)
 *  \param prev
 *      pointer on previous Sprite in list
 *  \param next
 *      pointer on next Sprite in list
 *
 *  Used to manage an active sprite in game condition.
 */
typedef struct Sprite
{
    u16 status;
    u16 visibility;
    const SpriteDefinition* definition;
    void (*onFrameChange)(struct Sprite* sprite);
    Animation* animation;
    AnimationFrame* frame;
    s16 animInd;
    s16 frameInd;
    s16 timer;
    s16 x;
    s16 y;
    s16 depth;
    u16 attribut;
    u32 data;
    struct Sprite* prev;
    struct Sprite* next;
} Sprite;

/**
 *  \brief
 *      Sprite frame change event callback.<br>
 *
 *  \param sprite
 *      The sprite for which frame just changed.
 *
 *      This event occurs onframe change process during #SPR_update() call (CAUTION: sprite->status field is not up to date at this point).<br>
 *      It let opportunity to the developer to apply special behavior or process when sprite frame just changed:<br>
 *      for instance we can disable animation looping by setting sprite->timer to 0 when we meet the last animation frame.
 */
typedef void FrameChangeCallback(Sprite* sprite);

/**
 * Sprites object pool for the sprite engine
 */
extern Pool* spritesPool;
/**
 * First allocated sprite (NULL if no sprite allocated)
 */
extern Sprite* firstSprite;
/**
 * Last allocated sprite (NULL if no sprite allocated)
 */
extern Sprite* lastSprite;
/**
 * Allocated VRAM (in tile) for Sprite Engine
 */
extern u16 spriteVramSize;


/**
 *  \brief
 *      Initialize the Sprite engine with default parameters.
 *
 *      Initialize the sprite engine using default parameters (420 reserved tiles in VRAM).<br>
 *      This also initialize the hardware sprite allocation system.
 *
 *  \see SPR_initEx(void)
 *  \see SPR_end(void)
 */
void SPR_init(void);
/**
 *  \brief
 *      Init the Sprite engine with specified advanced parameters (VRAM allocation size and decompression buffer size).
 *
 *  \param vramSize
 *      size (in tile) of the VRAM region for the automatic VRAM tile allocation.<br>
 *      If set to 0 the default size is used (420 tiles)
 *
 *      Initialize the sprite engine.<br>
 *      This allocates a VRAM region for sprite tiles and initialize hardware sprite allocation system.
 *
 *  \see SPR_init(void)
 *  \see SPR_end(void)
 */
void SPR_initEx(u16 vramSize);
/**
 *  \brief
 *      End the Sprite engine.
 *
 *      End the sprite engine and release attached resources.<br>
 *      This releases the allocated VRAM region and hardware sprites.
 */
void SPR_end(void);
/**
 *  \brief
 *      FALSE if sprite cache engine is not initialized, TRUE otherwise.
 */
bool SPR_isInitialized(void);

/**
 *  \brief
 *      Reset the Sprite engine.<br>
 *
 *      This method releases all allocated sprites and their resources.
 */
void SPR_reset(void);

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
 *      #SPR_FLAG_AUTO_TILE_UPLOAD = Enable automatic upload of sprite tiles data into VRAM (enabled by default)<br>
 *          If you don't set this flag you will have to manually upload tiles data of sprite into the VRAM (you can change this setting using #SPR_setAutoTileUpload(..) method).<br>
 *      #SPR_FLAG_INSERT_HEAD = Allow to insert the sprite at the start/head of the list.<br>
 *          When you use this flag the sprite will be inserted at the head of the list making it top most (equivalent to #SPR_setDepth(#SPR_MIN_DEPTH))<br>
 *          while default insertion position is at the end of the list (equivalent to #SPR_setDepth(#SPR_MAX_DEPTH))<br>
 *      <br>
 *      It's recommended to use the following default settings:<br>
 *      SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD<br>
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
Sprite* SPR_addSpriteEx(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut, u16 flag);
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
 *      You can change these defaults settings later by calling SPR_setVRAMTileIndex(..), SPR_setAutoTileUpload(..) and SPR_setVisibility(..) methods.<br>
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
 *  \param flag
 *      specific settings for this sprite:<br>
 *      #SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE = Disable delaying of frame update when we are running out of DMA capacity.<br>
 *          If you set this flag then sprite frame update always happen immediately but may lead to some graphical glitches (tiles data and sprite table data not synchronized).
 *          You can use SPR_setDelayedFrameUpdate(..) method to change this setting.<br>
 *      #SPR_FLAG_AUTO_VISIBILITY = Enable automatic sprite visibility calculation (you can also use SPR_setVisibility(..) method).<br>
 *      #SPR_FLAG_FAST_AUTO_VISIBILITY = Enable fast computation for the automatic visibility calculation (disabled by default)<br>
 *          If you set this flag the automatic visibility calculation will be done globally for the (meta) sprite and not per internal
 *          hardware sprite. This result in faster visibility computation at the expense of some waste of hardware sprite (scanline limit).
 *          You can set the automatic visibility computation by using SPR_setVisibility(..) method.<br>
 *      #SPR_FLAG_AUTO_VRAM_ALLOC = Enable automatic VRAM allocation (enabled by default)<br>
 *          If you don't set this flag you will have to manually define VRAM tile index position for this sprite with the <i>attribut</i> parameter or by using the #SPR_setVRAMTileIndex(..) method<br>
 *      #SPR_FLAG_AUTO_TILE_UPLOAD = Enable automatic upload of sprite tiles data into VRAM (enabled by default)<br>
 *          If you don't set this flag you will have to manually upload tiles data of sprite into the VRAM (you can change this setting using #SPR_setAutoTileUpload(..) method).<br>
 *      #SPR_FLAG_INSERT_HEAD = Allow to insert the sprite at the start/head of the list.<br>
 *          When you use this flag the sprite will be inserted at the head of the list making it top most (equivalent to #SPR_setDepth(#SPR_MIN_DEPTH))<br>
 *          while default insertion position is at the end of the list (equivalent to #SPR_setDepth(#SPR_MAX_DEPTH))<br>
 *      <br>
 *      It's recommended to use the following default settings:<br>
 *      SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD<br>
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
Sprite* SPR_addSpriteExSafe(const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut, u16 flag);
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
 *      You can change these defaults settings later by calling SPR_setVRAMTileIndex(..), SPR_setAutoTileUpload(..) and SPR_setVisibility(..) methods.<br>
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
u16 SPR_getNumActiveSprite(void);
/**
 *  \brief
 *      Returns the (maximum) number of used VDP sprite from current active sprites (sum of maximum hardware sprite usage from all active sprites).
 */
u16 SPR_getUsedVDPSprite(void);
/**
 *  \brief
 *      Returns the current remaining free VRAM (in tile) for the sprite engine.
 */
u16 SPR_getFreeVRAM(void);
/**
 *  \brief
 *      Return the current largest free VRAM block size (in tile) for the sprite engine.
 */
u16 SPR_getLargestFreeVRAMBlock(void);

/**
 *  \brief
 *      Indicate that we want to prevent adding a new sprite if there is possibly not enough hardware sprite to display it
 *      (counting the maximum hardware sprite usage from all active sprites) and so this ensures to always display all of them.<br>
 *      By default the sprite engine allow to add a new sprite even if we may run out of hardware sprite to display all of them,
 *      that can lead to some sprite disappearence in some situations but ultimately it allows to handle more active sprites at once.
 *
 *  \see SPR_disableVDPSpriteChecking(..)
 *  \see SPR_getNumVDPSprite(..)
 */
void SPR_enableVDPSpriteChecking();
/**
 *  \brief
 *      Indicate that we allow the sprite engine to add a new sprite even if we may run out of hardware sprite to display all of them,
 *      that can lead to some sprite disappearence in some situation but ultimately it allows to handle more active sprites at once (default behavior).
 *
 *  \see SPR_enableVDPSpriteChecking(..)
 *  \see SPR_getNumVDPSprite(..)
 */
void SPR_disableVDPSpriteChecking();
/**
 *  \brief
 *      Defragment allocated VRAM for sprites, that can help when sprite allocation fail (SPR_addSprite(..) or SPR_addSpriteEx(..) return <i>NULL</i>).
 */
void SPR_defragVRAM(void);
/**
 *  \brief
 *      Load all frames of SpriteDefinition (using DMA) at specified VRAM tile index and return the indexes table.<br>
 *      <b>WARNING: This function should be call at init/loading time as it can be quite long (several frames)</b>
 *
 *  \param sprDef
 *      the SpriteDefinition we want to load frame data in VRAM.
 *  \param index
 *      the tile position in VRAM where we will upload all sprite frame tiles data.
 *  \param totalNumTile
 *      if not NULL then the function will store here the total number of tile used to load all animation frames.
 *
 *   Load all frames of spriteDef (using DMA) at specified VRAM tile index and return the indexes table.<br>
 *   The returned index table is a dynamically allocated 2D table[anim][frame] so you need to release it using #MEM_free(..)
 *   when you don't need the table anymore.<br>
 *   You can use the frame change callback (see #SPR_setFrameChangeCallback(..)) to automatically update the VRAM index using the indexes table:<br>
 *   <code>frameIndexes = SPR_loadAllFrames(sprite->definition, ind);<br>
 *   SPR_setFrameChangeCallback(sprite, &frameChanged);<br>
 *   ....<br>
 *   void frameChanged(Sprite* sprite)<br>
 *   {<br>
 *       u16 tileIndex = frameIndexes[sprite->animInd][sprite->frameInd];<br>
 *       SPR_setVRAMTileIndex(sprite, tileIndex);<br>
 *   }</code>
 *
 *  \return the 2D indexes table or NULL if there is not enough memory to allocate the table.
 *  \see SPR_setFrameChangeCallback(...);
 */
u16** SPR_loadAllFrames(const SpriteDefinition* sprDef, u16 index, u16* totalNumTile);

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
 *      Get sprite position X.
 */
s16 SPR_getPositionX(Sprite* sprite);
/**
 *  \brief
 *      Get sprite position Y.
 */
s16 SPR_getPositionY(Sprite* sprite);
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
void SPR_setHFlip(Sprite* sprite, bool value);
/**
 *  \brief
 *      Set sprite Vertical Flip attribut.
 *
 *  \param sprite
 *      Sprite to set attribut for
 *  \param value
 *      The vertical flip attribut value (TRUE or FALSE)
 */
void SPR_setVFlip(Sprite* sprite, bool value);
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
void SPR_setPriority(Sprite* sprite, bool value);
/**
 *  \deprecated
 *      Use #SPR_setPriority(..) instead
 */
#define SPR_setPriorityAttribut(sprite, value)      _Pragma("GCC error \"This method is deprecated, use SPR_setPriority(..) instead.\"")
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
 *  \brief
 *      Set sprite depth so it remains above others sprite - same as SPR_setDepth(SPR_MIN_DEPTH)
 *
 *  \param sprite
 *      Sprite to change depth for
 *
 *  \see SPR_setDepth(Sprite*)
 */
void SPR_setAlwaysOnTop(Sprite* sprite);
/**
 *  \brief
 *      Set sprite depth so it remains behind others sprite - same as SPR_setDepth(SPR_MAX_DEPTH)
 *
 *  \param sprite
 *      Sprite to change depth for
 *
 *  \see SPR_setDepth(Sprite*)
 */
void SPR_setAlwaysAtBottom(Sprite* sprite);

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
 *      Enable/disable auto animation (default is on).<br>
 *
 *  \param sprite
 *      Sprite we want to enable/disable auto animation.
 *  \param value
 *      TRUE to enable auto animation (default), FALSE otherwise
 *
 *  \see #SPR_getAutoAnimation(Sprite*)
 */
void SPR_setAutoAnimation(Sprite* sprite, bool value);
/**
 *  \brief
 *      Return TRUE if auto animation is enabled, FALSE otherwise.
 *
 *  \see #SPR_setAnimationLoop(Sprite*, bool)
 */
bool SPR_getAutoAnimation(Sprite* sprite);
/**
 *  \brief
 *      Enable/disable animation loop (default is on).<br>
 *      When disable the sprite will stay on the last animation frame when animation ended instead of restarting it.
 *
 *  \param sprite
 *      Sprite we want to enable/disable animation loop for.
 *  \param value
 *      TRUE to enable animation loop (default), FALSE otherwise
 *
 *  \see SPR_FLAG_DISABLE_ANIMATION_LOOP
 *  \see #SPR_getAnimationDone(Sprite*)
 */
void SPR_setAnimationLoop(Sprite* sprite, bool value);
/**
 *  \brief
 *      Return TRUE if animation ended / looped.<br>
 *      This can be used with the frame change callback (see #SPR_setFrameChangeCallback(..)) to detect
 *      the end of sprite animation and do appropriate action if required.
 *
 *  \see SPR_FLAG_DISABLE_ANIMATION_LOOP
 *  \see #SPR_setAnimationLoop(Sprite*, bool)
 */
bool SPR_getAnimationDone(Sprite* sprite);

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
 *      TRUE to enable the delayed frame update when DMA is running out of transfert capacity (default).<br>
 *      FALSE to disable it. In which case sprite frame is always updated immediately but that may cause graphical glitches (update during active display).<br>
 *
 *  \see #SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE
 */
void SPR_setDelayedFrameUpdate(Sprite* sprite, bool value);
/**
 *  \brief
 *      Set the frame change event callback for this sprite.
 *
 *  \param sprite
 *      Sprite we want to set the frame change callback
 *  \param callback
 *      the callback (function pointer) to call when we just changed the animation frame for this sprite.
 *
 *      The callback (if not NULL) will be called on frame change process during #SPR_update() call (CAUTION: sprite->status field is not up to date at this point).<br>
 *      It let opportunity to the developer to apply special behavior or process when sprite frame just changed:<br>
 *      for instance we can disable animation looping by setting sprite->timer to 0 when we meet the last animation frame.
 *
 *  \see #FrameChangeCallback
 */
void SPR_setFrameChangeCallback(Sprite* sprite, FrameChangeCallback* callback);

/**
 *  \brief
 *      Return the <i>visibility</i> state for this sprite.<br>
 *      WARNING: this is different from SPR_isVisible(..) method, possible value are:<br>
 *      SpriteVisibility.VISIBLE        = sprite is visible<br>
 *      SpriteVisibility.HIDDEN         = sprite is not visible<br>
 *      SpriteVisibility.AUTO_FAST      = visibility is automatically computed - global visibility (only using sprite X position)<br>
 *      SpriteVisibility.AUTO_SLOW      = visibility is automatically computed - per hardware sprite visibility (only using sprite X position)<br>
 *
 *  \param sprite
 *      Sprite to return <i>visibility</i> state
 *
 *  \return
 *       the <i>visibility</i> state for this sprite.
 *
 *  \see SPR_isVisible(...)
 *  \see SPR_setVisibility(...)
 */
SpriteVisibility SPR_getVisibility(Sprite* sprite);
/**
 *  \brief
 *      Return the visible state for this sprite (meaningful only if AUTO visibility is enabled, see #SPR_setVisibility(..) method).<br>
 *
 *  \param sprite
 *      Sprite to return <i>visible</i> state
 *  \param recompute
 *      Force visibility computation.<br>
 *      Only required if SPR_update() wasn't called since last sprite position change (note that can force the frame update processing).
 *
 *  Note that only the sprite X position is used to determine if it's visible (see #SPR_setVisibility(..) method for more information)

 *  \return
 *      the <i>visible</i> state for this sprite.
 *
 *  \see SPR_setVisibility(...)
 */
bool SPR_isVisible(Sprite* sprite, bool recompute);

/**
 *  \brief
 *      Set the <i>visibility</i> state for this sprite.
 *
 *  \param sprite
 *      Sprite to set the <i>visibility</i> information
 *  \param value
 *      Visibility value to set.<br>
 *      SpriteVisibility.VISIBLE        = sprite is visible<br>
 *      SpriteVisibility.HIDDEN         = sprite is not visible<br>
 *      SpriteVisibility.AUTO_FAST      = visibility is automatically computed - global visibility (only using sprite X position)<br>
 *      SpriteVisibility.AUTO_SLOW      = visibility is automatically computed - per hardware sprite visibility (only using sprite X position)<br>
 *
 *      The only interest in having a sprite hidden / not visible is to avoid having it consuming scanline sprite budget:<br>
 *      The VDP is limited to a maximum of 20 sprites or 320 pixels of sprite per scanline (16 sprites/256 px in H32 mode).<br>
 *      If we reach this limit, following sprites won't be renderer so it's important to try to optimize the number of sprites on a single scanline.<br>
 *      When a sprite is set to not visible (automatically or manually), its position is set offscreen *vertically* so it won't eat
 *      anymore the scanline sprite rendering budget, only X position test is required for that so a sprite is determined not visible only using its X coordinate.
 *
 *  \see SPR_computeVisibility(...)
 *  \see SPR_getVisibility(...)
 *  \see SPR_isVisible(...)
 */
void SPR_setVisibility(Sprite* sprite, SpriteVisibility value);
/**
 *  \deprecated Use #SPR_setVisibility(..) method instead.
 */
#define SPR_setAlwaysVisible(sprite, value)     _Pragma("GCC error \"This method is deprecated, use SPR_setVisibility(..) instead.\"")
/**
 *  \deprecated Use #SPR_setVisibility(..) method instead.
 */
#define SPR_setNeverVisible(sprite, value)      _Pragma("GCC error \"This method is deprecated, use SPR_setVisibility(..) instead.\"")
/**
 *  \deprecated Use SPR_isVisible(..) instead.
 */
#define SPR_computeVisibility(sprite)           _Pragma("GCC error \"This method is deprecated, use SPR_isVisible(sprite, TRUE) instead.\"")

/**
 *  \brief
 *      Clear all displayed sprites.
 *
 *  This method allow to quickly hide all sprites (without releasing their resources).<br>
 *  Sprites can be displayed again just by calling SPR_update().
 */
void SPR_clear(void);
/**
 *  \brief
 *      Update and display the active list of sprite.
 *
 *  This actually updates all internal active sprites states and prepare the sprite list
 *  cache to send it to the hardware (VDP) at Vint.
 *
 *  \see #SPR_addSprite(..)
 */
void SPR_update(void);

/**
 *  \brief
 *      Log the profil informations (when enabled) in the KMod message window.
 */
void SPR_logProfil(void);
/**
 *  \brief
 *      Log the sprites informations (when enabled) in the KMod message window.
 */
void SPR_logSprites(void);


#endif // _SPRITE_ENG_H_

#endif