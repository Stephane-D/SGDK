#include <genesis.h>
#include "res/res_sprite.h"

// number of replica sprites
#define REPLICA_COUNT     10

// define Sprite pointers for primary and replica sprites
Sprite *primarySprite;
Sprite *replicaSprites[REPLICA_COUNT];

// define empty pointer to 2D array of u16 for storing VRAM tile indexes of primary sprite
u16 **frameIndexes;

// this function updates the VRAM tile index for the current frame of the primary and replicated sprites,
// and synchronizes the animation of the replicated sprites with the primary sprite
void SyncReplicaSpritesToPrimarySprite(Sprite *sprite)
{
    // get the VRAM tile index for the current frame of the primary sprite
    s16 tileIndex = (s16) frameIndexes[sprite->animInd][sprite->frameInd];
    
    // set VRAM tile index for primary sprite
    SPR_setVRAMTileIndex(sprite, tileIndex);
    
    for (u16 i = 0; i < REPLICA_COUNT; i++) {
        // sync VRAM tile index of replica sprite with VRAM tile index of primary sprite
        SPR_setVRAMTileIndex(replicaSprites[i], tileIndex);
        
        // sync animation index of replica sprite to primary sprite
        SPR_setAnim(replicaSprites[i], sprite->animInd);
    }
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
    
    // load all frames of target SpriteDefinition to VRAM and got pointer to
    // dynamically allocated 2D array of VRAM tile indexes (like frameIndexes[anim][frame])
    frameIndexes = SPR_loadAllFrames(&sonicSpriteDef, TILE_USER_INDEX, &numTile);
    
    // create primary sprite with settings of "auto vram allocation" & "auto tile upload" to off
    // (default flag for SPR_addSprite is SPR_FLAG_AUTO_VISIBILITY | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_AUTO_TILE_UPLOAD)
    primarySprite = SPR_addSpriteEx(&sonicSpriteDef, 0, 0, TILE_ATTR(PAL0, TRUE, FALSE, FALSE), SPR_FLAG_AUTO_VISIBILITY);
    
    // create replica sprites with settings of "auto vram allocation" & "auto tile upload" to off
    // and offset of X position by 30 pixels
    for (u16 i = 0; i < REPLICA_COUNT; i++) {
        replicaSprites[i] = SPR_addSpriteEx(&sonicSpriteDef, 30 * i, 50, TILE_ATTR(PAL0, TRUE, FALSE, FALSE), SPR_FLAG_AUTO_VISIBILITY);
    }
    
    // set primarySprite on frame change callback
    SPR_setFrameChangeCallback(primarySprite, SyncReplicaSpritesToPrimarySprite);
    
    // load palette
    PAL_setPalette(PAL0, sonicSpriteDef.palette->data, CPU);
    
    // set primary sprite animation (Sonic running animation)
    SPR_setAnim(primarySprite, 3);
    
    // main loop
    while (TRUE) {
        SPR_update();
        SYS_doVBlankProcess();
    }
    
    // these lines are unreachable here, but don't forgot to free memory when frameTable is no longer needed,
    // otherwise you'll get a memory leak
    MEM_free(frameIndexes);
    
    // also, don't forget to free up the memory from the sprites.
    SPR_releaseSprite(primarySprite);
    
    for (u16 i = 0; i < REPLICA_COUNT; i++) {
        SPR_releaseSprite(replicaSprites[i]);
    }
    
    return 0;
}
