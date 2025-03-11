// *****************************************************************************
//  Share tiles between sprites example
//
//  Shared tiles between sprites is a technique that allows you not to load the same tiles
//  of each the similar sprites to the VRAM, but to use one set of tiles for the several sprite,
//  which reduces the consumption of VRAM
//
//  writen by werton playskin 03/2025
// *****************************************************************************

#include <genesis.h>
#include "res/res_sprite.h"

// Change to 0 for steaming frames, else all frames of animation to VRAM
#define LOAD_ALL_FRAMES_TO_VRAM     1

// number of replica sprites
#define REPLICA_COUNT               10

// define Sprite pointers for primary and replica sprites
Sprite *primarySprite;
Sprite *replicaSprites[REPLICA_COUNT];

// define empty pointer to 2D array of u16 for storing VRAM tile indexes of primary sprite
// (used for preloaded frames only)
u16 **frameIndexes;

// synchronizes the animation of the replicated sprites by the primary sprite
void SyncReplicaSpritesToSprite(Sprite *sprite, s16 tileIndex)
{
    for (u16 i = 0; i < REPLICA_COUNT; i++) {
        // sync animation index of replica sprite to primary sprite
        SPR_setAnim(replicaSprites[i], sprite->animInd);
        // sync VRAM tile index of replica sprite with VRAM tile index of primary sprite
        SPR_setVRAMTileIndex(replicaSprites[i], tileIndex);
    }
}

// updates the VRAM tile indexes for the primary sprite
// and sync replica sprites to primary sprite
// for FULL LOADED frames
void UpdateSpritesTileIndexesForFullLoadedFrames(Sprite *sprite)
{
    // get the VRAM tile index for the current frame of the primary sprite
    s16 tileIndex = (s16) frameIndexes[sprite->animInd][sprite->frameInd];
    
    // set VRAM tile index for primary sprite
    SPR_setVRAMTileIndex(sprite, tileIndex);
    
    // sync replica sprites to primary sprite
    SyncReplicaSpritesToSprite(sprite, tileIndex);
}

// sync replica sprites to primary sprite
// for STREAMED frames
void UpdateSpritesTileIndexesForStreamedFrames(Sprite *sprite)
{
    // get the VRAM tile index for the current frame of the primary sprite
    s16 tileIndex = (s16) (sprite->attribut & TILE_INDEX_MASK);
    
    // sync replica sprites to primary sprite
    SyncReplicaSpritesToSprite(sprite, tileIndex);
}


int main(bool hardReset)
{
    // screen initialization
    VDP_setScreenWidth320();
    
    // set all palette to black
    PAL_setColors(0, (u16 *) palette_black, 64, CPU);
    
    // init sprite engine with default parameters
    SPR_init();
    
    // create variable for containing number of tiles
    u16 numTile;

#if LOAD_ALL_FRAMES_TO_VRAM
    // load all frames of target SpriteDefinition to VRAM and got pointer to
    // dynamically allocated 2D array of VRAM tile indexes (frameIndexes[anim][frame])
    frameIndexes = SPR_loadAllFrames(&sonicSpriteDef, TILE_USER_INDEX, &numTile);
    
    // create a primary sprite with disabled settings - "SPR_FLAG_AUTO_VRAM_ALLOC" & "SPR_FLAG_AUTO_TILE_UPLOAD"
    // (default flag for SPR_addSprite is "SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD")
    primarySprite = SPR_addSpriteEx(&sonicSpriteDef, 0, 0, TILE_ATTR(PAL0, TRUE, FALSE, FALSE), SPR_FLAG_AUTO_VISIBILITY);
    
    // set up the callback, which will update the tile indexes of the sprites on frame changing
    SPR_setFrameChangeCallback(primarySprite, UpdateSpritesTileIndexesForFullLoadedFrames);
#else
    // create primary sprite with streamed frames
    primarySprite = SPR_addSprite(&sonicSpriteDef, 0, 0, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    
    // set up the callback, which will update the tile indexes of the sprites on frame changing
    SPR_setFrameChangeCallback(primarySprite, UpdateSpritesTileIndexesForStreamedFrames);
#endif
    
    // create replica sprites with settings of "auto vram allocation" & "auto tile upload" to off
    // and offset of X position by 30 pixels
    for (u16 i = 0; i < REPLICA_COUNT; i++) {
        replicaSprites[i] = SPR_addSpriteEx(&sonicSpriteDef, 30 * i, 50, TILE_ATTR(PAL0, TRUE, FALSE, FALSE), SPR_FLAG_AUTO_VISIBILITY);
    }
    
    // load palette
    PAL_setPalette(PAL0, sonicSpriteDef.palette->data, CPU);
    
    // set primary sprite animation (Sonic running animation)
    SPR_setAnim(primarySprite, 3);
    
    // main loop
    while (TRUE) {
        SPR_update();
        SYS_doVBlankProcess();
    }

#if LOAD_ALL_FRAMES_TO_VRAM
    // these lines are unreachable here, but don't forgot to free memory when frameTable is no longer needed,
    // otherwise you'll get a memory leak
    MEM_free(frameIndexes);
#endif
    
    // also, don't forget to free up the memory from the sprites.
    SPR_releaseSprite(primarySprite);
    
    for (u16 i = 0; i < REPLICA_COUNT; i++) {
        SPR_releaseSprite(replicaSprites[i]);
    }
    
    return 0;
}
