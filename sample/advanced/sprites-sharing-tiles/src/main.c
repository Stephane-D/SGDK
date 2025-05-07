// *****************************************************************************
//  Sprite sharing tiles sample
//
//  Sprite sharing tiles is a technique that allows you not to load the same tiles
//  of each the similar sprites to the VRAM, but to use one set of tiles for the several sprite,
//  which reduces the consumption of VRAM.
//
//  There is an option  - for streamed frames to VRAM or for fully loaded
//  sprite's frames in VRAM (edit LOAD_ALL_FRAMES_TO_VRAM for switch)
//
//  Writen by werton playskin on 03/2025
// *****************************************************************************

#include <genesis.h>
#include "res/res_sprite.h"

// Change to 0 for streaming frames, else all frames will be loaded to VRAM
#define LOAD_ALL_FRAMES_TO_VRAM     0

// Number of child sprites
#define CHILD_COUNT                 10

// Define Sprite pointers for master and child sprites
Sprite *masterSprite;
Sprite *childSprites[CHILD_COUNT];

// Define empty pointer to 2D array of u16 for storing VRAM tile indexes of master sprite
// (used for preloaded frames only)
u16 **frameIndexes;

// Functions prototypes
void InitGame();
void ReleaseResources();
void UpdateChildSprites(Sprite *sprite);

int main(bool hardReset)
{
    // Do hard reset on soft reset (just for convenience)
    if (!hardReset)
        SYS_hardReset();
    
    // Init system and load resources
    InitGame();
    
    // Main loop
    while (TRUE)
    {
        SPR_update();
        SYS_doVBlankProcess();
    }
    
    // Free up memory
    // Note: Currently unreachable due to infinite main loop
    ReleaseResources();
    
    return 0;
}

void InitGame()
{
    // Init sprite engine with default parameters
    SPR_init();
    
#if LOAD_ALL_FRAMES_TO_VRAM
    // Create variable for containing number of tiles
    u16 numTile;
    
    // Load all frames of target SpriteDefinition to VRAM and got pointer to
    // dynamically allocated 2D array of VRAM tile indexes (frameIndexes[anim][frame])
    frameIndexes = SPR_loadAllFrames(&sonicSpriteDef, TILE_USER_INDEX, &numTile);
    
    // Create a master sprite with disabled settings - "SPR_FLAG_AUTO_VRAM_ALLOC" & "SPR_FLAG_AUTO_TILE_UPLOAD"
    // (default flag for SPR_addSprite is "SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD")
    masterSprite = SPR_addSpriteEx(&sonicSpriteDef, 0, 0, TILE_ATTR(PAL0, TRUE, FALSE, FALSE), SPR_FLAG_AUTO_VISIBILITY);
    
    // Set up the callback, which will update the tile indexes of the sprites on frame changing
    SPR_setFrameChangeCallback(masterSprite, UpdateChildSprites);
#else
    // Create master sprite with streamed frames
    masterSprite = SPR_addSprite(&sonicSpriteDef, 0, 0, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
        
    // Set up the callback, which will update the tile indexes of the sprites on frame changing
    SPR_setFrameChangeCallback(masterSprite, UpdateChildSprites);
#endif
    
    // Create child sprites with settings of "auto vram allocation" & "auto tile upload" to off
    // and offset of X position by 30 pixels
    for (u16 i = 0; i < CHILD_COUNT; i++)
        childSprites[i] = SPR_addSpriteEx(&sonicSpriteDef, 30 * i, 50, TILE_ATTR(PAL0, TRUE, FALSE, FALSE), SPR_FLAG_AUTO_VISIBILITY);
    
    // Load palette
    PAL_setPalette(PAL0, sonicSpriteDef.palette->data, DMA);
    
    // Set master sprite animation (Sonic running animation)
    SPR_setAnim(masterSprite, 3);
}

// Synchronizes the animation of the child sprites by the master sprite
void SyncChildSpritesToMaster(Sprite *sprite, s16 tileIndex)
{
    for (u16 i = 0; i < CHILD_COUNT; i++)
    {
        // Sync animation index of child sprite to master sprite
        SPR_setAnim(childSprites[i], sprite->animInd);
        // Sync VRAM tile index of child sprite with VRAM tile index of master sprite
        SPR_setVRAMTileIndex(childSprites[i], tileIndex);
    }
}

#if LOAD_ALL_FRAMES_TO_VRAM
// Updates the VRAM tile indexes for the master sprite
// and sync child sprites to master sprite
// !!!For FULLY loaded frames in VRAM!!!
void UpdateChildSprites(Sprite *sprite)
{
    // Get the VRAM tile index for the current frame of the master sprite
    s16 tileIndex = (s16) frameIndexes[sprite->animInd][sprite->frameInd];
    
    // Set VRAM tile index for master sprite
    SPR_setVRAMTileIndex(sprite, tileIndex);
    
    // Sync child sprites to master sprite
    SyncChildSpritesToMaster(sprite, tileIndex);
}
#else

// Sync child sprites to master sprite
// !!!For STREAMED frames to VRAM!!!
void UpdateChildSprites(Sprite *sprite)
{
    // Extracting VRAM tile index from the sprite attributes of the master sprite
    s16 tileIndex = (s16) (sprite->attribut & TILE_INDEX_MASK);
    
    // Sync child sprites to master sprite
    SyncChildSpritesToMaster(sprite, tileIndex);
}
#endif

// Cleanup function to free allocated resources
void ReleaseResources()
{
#if LOAD_ALL_FRAMES_TO_VRAM
    // Free up memory when frameTable is no longer needed,
    MEM_free(frameIndexes);
#endif
    
    // Release all master sprite
    SPR_releaseSprite(masterSprite);
    
    // Release all child sprites
    for (u16 i = 0; i < CHILD_COUNT; i++)
        SPR_releaseSprite(childSprites[i]);
}
