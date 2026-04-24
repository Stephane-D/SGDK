#include <genesis.h>
#include "globals.h"
#include "maps.h"
#include "player.h"
#include "camera.h"
#include "sprites.h"


static void Palettes_Init();
static void Map_Init();

// Set up game, prepare for the main loop
void Game_Init()
{
    // Set display mode to 320x224 (can be set to 256x224)
    VDP_setScreenWidth320();
    
    // Set the maximum number of tiles for the sprite engine.
    // I set it to 50 because these beautiful background tilesets are not optimized for Genesis and consume a lot of
    // VRAM tiles, while the sprite in the example is only one
    // In a real project, you should optimize the tileset by reducing the number of unique tiles
    SPR_initEx(50);
    
    // Initialize palettes, maps, player and camera
    Palettes_Init();
    Map_Init();
    Player_Init();
    Camera_Init();
    
    // Update sprites and process VBlank
    SPR_update();
    SYS_doVBlankProcess();
}

// Main loop
_Noreturn void Game_DoLoop()
{
    while (true)
    {
        // Process player input
        Player_HandleInput();
        
        // Update player state
        Player_UpdateState();
        
        // Update player position
        if (Player_UpdatePosition())
        {
            // Update camera and player sprite position
            // only if player moved to avoid unnecessary updates
            Camera_Update();
            Player_UpdateSprite();
        }
        
        // Update sprites with the Sprite Engine and do VBlank processing
        SPR_update();
        SYS_doVBlankProcess();
    }
}

// Load tilesets and create maps
static void Map_Init()
{
    // Set the initial VRAM tile index from which we will load the first tileset
    u16 nextToFillTileIndex = TILE_USER_INDEX;
    
    // Load tileset and create foreground map
    VDP_loadTileSet(&fg_tileset, nextToFillTileIndex, DMA);
    fgMap = MAP_create(&fg_map_def, BG_A, TILE_ATTR_FULL(PAL0, false, false, false, nextToFillTileIndex));
    
    // Update tile index for next tileset
    nextToFillTileIndex += fg_tileset.numTile;
    
    // Load background tileset and create background map
    VDP_loadTileSet(&bg_tileset, nextToFillTileIndex, DMA);
    bgMap = MAP_create(&bg_map_def, BG_B, TILE_ATTR_FULL(PAL1, false, false, false, nextToFillTileIndex));
}

static void Palettes_Init()
{
    // Set palettes for foreground, background, player sprite palette, player hurt palette (white from index 7 of palette_grey)
    PAL_setPalette(PAL0, fg_palette.data, DMA);
    PAL_setPalette(PAL1, bg_palette.data, DMA);
    PAL_setPalette(PAL2, spr_def_player_bb.palette->data, DMA);
    PAL_setPalette(PAL3, &palette_grey[7], DMA);
}


