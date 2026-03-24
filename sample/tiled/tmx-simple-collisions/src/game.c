#include <genesis.h>
#include "defs.h"
#include "maps.h"
#include "typedefs.h"
#include "player.h"
#include "camera.h"
#include "sprites.h"

void Game_Init()
{
    VDP_setScreenWidth320();

    // Set the maximum number of tiles for the sprite engine.
    // I set it to 50 because these beautiful background tilesets are not optimized for Genesis and consume a lot of
    // VRAM tiles, while the sprite in the example is only one
    // In a real project, you should optimize the tileset by reducing the number of unique tiles
    SPR_initEx(50);
    
    // Set max scroll area based on map size and screen size to prevent scrolling beyond map bounds
    Camera_SetMaxScrollArea(collision_map.w, collision_map.h);
    
    // Init player sprite, position and set collision callback
    Player_Init();
    
    // Set the initial VRAM tile index from which we will load the first tileset
    u16 nextToFillTileIndex = TILE_USER_INDEX;
    
    // Load tileset and create foreground map
    VDP_loadTileSet(&fg_tileset, nextToFillTileIndex, DMA);
    fgMap = MAP_create(&fg_map_def, BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, nextToFillTileIndex));
    
    // Update tile index for next tileset
    nextToFillTileIndex += fg_tileset.numTile;

    // Load background tileset and create background map
    VDP_loadTileSet(&bg_tileset, nextToFillTileIndex, DMA);
    bgMap = MAP_create(&bg_map_def, BG_B, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, nextToFillTileIndex));
    
    // Initial camera positioning
    Camera_Update();
    
    // Set palettes for foreground, background and player sprite palette
    PAL_setPalette(PAL0, fg_palette.data, DMA);
    PAL_setPalette(PAL1, bg_palette.data, DMA);
    PAL_setPalette(PAL2, spr_def_player_bb.palette->data, DMA);
    PAL_setPalette(PAL3, &palette_grey[7], DMA);

    // Set player bounding box for collision detection
    Player_SetAabb((Range_s16) {
        {PLAYER_AABB_LEFT, PLAYER_AABB_TOP},
        {PLAYER_AABB_RIGHT, PLAYER_AABB_BOTTOM}
    });
    
    // Update sprites and process VBlank
    SPR_update();
    SYS_doVBlankProcess();
}

// Main loop
_Noreturn void Game_DoLoop()
{
    while (TRUE)
    {
        Player_HandleInput();       // Process player input
        Player_UpdatePosition();    // Update player position
        Camera_Update();            // Update camera position
        Player_Update();            // Update player state & sprite
        
        SPR_update();               // Update sprites
        SYS_doVBlankProcess();      // Wait for VBlank processes
    }
}
