#include "genesis.h"

#include "entities.h"

#include "level.h"
#include "camera.h"
#include "utils.h"

#include "res_sprite.h"


#define NUM_ENEMY       2


// enemies sprites
Sprite* enemies[NUM_ENEMY];

// enemies positions and move direction
fix32 enemiesPosX[NUM_ENEMY];
fix32 enemiesPosY[NUM_ENEMY];
s16 enemiesXOrder[NUM_ENEMY];

// animation index table for enemies (static VRAM loading)
u16** sprTileIndexes[NUM_ENEMY];


// forward
static void frameChanged(Sprite* sprite);


u16 ENTITIES_init(u16 vramIndex)
{
    u16 ind;
    u16 numTile;

    // enemies position
    enemiesPosX[0] = FIX32(300L);
    enemiesPosY[0] = MAX_POSY - FIX32(100);
    enemiesPosX[1] = FIX32(128L);
    enemiesPosY[1] = MAX_POSY + FIX32(5);
    enemiesXOrder[0] = -1;
    enemiesXOrder[1] = 1;

    // init enemies sprites
    enemies[0] = SPR_addSprite(&enemy01_sprite, 0, 0, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    enemies[1] = SPR_addSprite(&enemy02_sprite, 0, 0, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));

    // disable auto tile upload for enemies sprites as we will pre-load all animation frams in VRAM for them
    SPR_setAutoTileUpload(enemies[0], FALSE);
    SPR_setAutoTileUpload(enemies[1], FALSE);
    // set frame change callback for enemies so we can update tile index easily
    SPR_setFrameChangeCallback(enemies[0], &frameChanged);
    SPR_setFrameChangeCallback(enemies[1], &frameChanged);

    // pre-load all animation frames into VRAM for enemies
    ind = vramIndex;
    sprTileIndexes[0] = SPR_loadAllFrames(&enemy01_sprite, ind, &numTile);
    ind += numTile;
    sprTileIndexes[1] = SPR_loadAllFrames(&enemy02_sprite, ind, &numTile);
    ind += numTile;

    // store enemy 'sprTileIndexes' table index in 'data' field (can be used freely)
    enemies[0]->data = 0;
    enemies[1]->data = 1;

    // return end VRAM index (static allocation)
    return ind;
}


void ENTITIES_update(void)
{
    // enemies 'physic'
    if (enemiesXOrder[0] > 0) enemiesPosX[0] += FIX32(1.5);
    else enemiesPosX[0] -= FIX32(1.5);
    if (enemiesXOrder[1] > 0) enemiesPosX[1] += FIX32(0.7);
    else enemiesPosX[1] -= FIX32(0.7);

    // update internal state
    for(u16 i = 0; i < NUM_ENEMY; i++)
    {
        if ((enemiesPosX[i] >= MAX_POSX) || (enemiesPosX[i] <= MIN_POSX))
            enemiesXOrder[i] = -enemiesXOrder[i];
    }

    // then update animation from internal state (only first enemy require flip here)
    if (enemiesXOrder[0] > 0) SPR_setHFlip(enemies[0], TRUE);
    else SPR_setHFlip(enemies[0], FALSE);
//    for(i = 0; i < 2; i++)
//    {
//        if (enemiesXOrder[i] > 0) SPR_setHFlip(sprites[i + 1], TRUE);
//        else SPR_setHFlip(sprites[i + 1], FALSE);
//    }
}


void ENTITIES_updateScreenPosition()
{
    for(u16 i = 0; i < NUM_ENEMY; i++)
        setSpritePosition(enemies[i], fix32ToInt(enemiesPosX[i]) - camPosX, fix32ToInt(enemiesPosY[i]) - camPosY);
}


static void frameChanged(Sprite* sprite)
{
    // get enemy index (stored in data field)
    u16 enemyIndex = sprite->data;
    // get VRAM tile index for this animation of this sprite
    u16 tileIndex = sprTileIndexes[enemyIndex][sprite->animInd][sprite->frameInd];
    // manually set tile index for the current frame (preloaded in VRAM)
    SPR_setVRAMTileIndex(sprite, tileIndex);
}