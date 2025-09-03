// *****************************************************************************
//  TMX Map Sample
//
// This sample demonstrates loading Tiled TMX maps and one method of setting
// tile priority through priority layers on both BG planes.
//
// Notes:
//   - Separate tileset for priority layers is not mandatory, and you can use any tile from any tileset,
//     but it is more convenient to use monotonous colored tiles that are individual for each layer.
//   - For simplicity, the example does not use fixed point types for player coordinates.
//
//  Controls: DPAD to move the player and observe camera scrolling.
//
//  written by werton playskin on 09/2025
//  graphics by Luis Zuno aka Ansimuz ansimuz.itch.io (CC0 license)
// *****************************************************************************

#include <genesis.h>
#include "maps.h"
#include "sprites.h"

#define MAP_WIDTH               1280
#define MAP_HEIGHT              592
#define SCREEN_WIDTH            320
#define SCREEN_HEIGTH           224

#define PLAYER_SPRITE_WIDTH     64
#define PLAYER_SPRITE_HEIGHT    56
#define PLAYER_MOVE_SPEED       2

// Background scroll scale for far/background layer (shift right)
#define BG_SCROLL_SHIFT         3

typedef struct
{
    Vect2D_s16 pos;
    Vect2D_s16 prevPos;
} Positions;

typedef struct
{
    Positions;
    Sprite *sprite;
} Player;

Positions camera;
Player player;

// Declare foreground and background map's pointers
Map *fgMap;
Map *bgMap;

// Forward declarations
_Noreturn void MainLoop(void);
void GameInit(void);
void CameraUpdate(void);
void InputUpdate(void);
bool PlayerPositionUpdate();

int main(bool hardReset)
{
    // do hard reset on soft reset to nullify global variables
    if (!hardReset)
        SYS_hardReset();
    
    GameInit();
    MainLoop();
    return 0; // unreachable, MainLoop is _Noreturn but keep signature
}

void GameInit()
{
    VDP_setScreenWidth320();
    
    // Set all palette to black
    PAL_setColors(0, (u16 *) palette_black, 64, CPU);
    
    // Set the initial VRAM tile index from which we will load the first tileset
    u16 nextToFillTileIndex = TILE_USER_INDEX;
    
    // Set the maximum number of tiles for the sprite engine,
    // I set it to 40 because the rest went to the map's tilesets,
    // which is very unoptimized for Genesis and consumes a lot of tiles
    SPR_initEx(40);
    
    // Place player at screen center minus sprite offsets
    player.pos.x = SCREEN_WIDTH / 2 - PLAYER_SPRITE_WIDTH / 2;
    player.pos.y = SCREEN_HEIGTH / 2 - PLAYER_SPRITE_HEIGHT / 2;
    
    // Create player sprite with proper palette and attributes
    player.sprite = SPR_addSprite(&spr_def_player, player.pos.x, player.pos.y, TILE_ATTR(PAL2, FALSE, FALSE, TRUE));
    
    // Load tileset and create foreground map
    VDP_loadTileSet(&fg_tileset, nextToFillTileIndex, DMA);
    fgMap = MAP_create(&fg_map_def, BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, nextToFillTileIndex));
    
    // Update tile index for next tileset
    nextToFillTileIndex += fg_tileset.numTile;
    
    // Load background tileset and create background map
    VDP_loadTileSet(&bg_tileset, nextToFillTileIndex, DMA);
    bgMap = MAP_create(&bg_map_def, BG_B, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, nextToFillTileIndex));
    
    // Initial camera positioning
    CameraUpdate();
    
    // Set palettes for foreground, background and player sprite palette
    PAL_setPalette(PAL0, fg_palette.data, DMA);
    PAL_setPalette(PAL1, bg_palette.data, DMA);
    PAL_setPalette(PAL2, spr_def_player.palette->data, DMA);
    
    // Update sprites and process VBlank
    SPR_update();
    SYS_doVBlankProcess();
}

_Noreturn void MainLoop()
{
    while (TRUE)
    {
        InputUpdate();              // Process player input
        if (PlayerPositionUpdate()) // Update player position
            CameraUpdate();         // Update camera position
        SPR_update();               // Update sprite positions
        SYS_doVBlankProcess();      // Wait for VBlank
    }
}

bool PlayerPositionUpdate()
{
    // If player didn't move, no need to update
    if (player.pos.x == player.prevPos.x && player.pos.y == player.prevPos.y)
        return FALSE;
    
    SPR_setPosition(player.sprite, player.pos.x - camera.pos.x, player.pos.y - camera.pos.y);
    
    return TRUE;
}

void CameraUpdate()
{
    // store previous camera position
    camera.prevPos = camera.pos;
    
    // Calculate camera position centered on player and clamp to map bounds
    // Use integer division for screen center; clamp ensures camera stays within map
    camera.pos.x = clamp(player.pos.x - SCREEN_WIDTH / 2, 0, MAP_WIDTH - SCREEN_WIDTH);
    camera.pos.y = clamp(player.pos.y - SCREEN_HEIGTH / 2, 0, MAP_HEIGHT - SCREEN_HEIGTH);
    
    // Scroll foreground directly and background scaled down (parallax) using BG_SCROLL_SHIFT
    MAP_scrollTo(fgMap, camera.pos.x, camera.pos.y);
    MAP_scrollTo(bgMap, camera.pos.x >> BG_SCROLL_SHIFT, camera.pos.y >> BG_SCROLL_SHIFT);
}

void InputUpdate()
{
    u16 value = JOY_readJoypad(JOY_1);
    
    // remember previous player position for movement and rendering logic
    player.prevPos = player.pos;
    
    // Move player position using defined speed constant and update sprite flip
    if (value & BUTTON_RIGHT)
    {
        player.pos.x += PLAYER_MOVE_SPEED;
        SPR_setHFlip(player.sprite, TRUE); // Flip sprite to face right
    }
    else if (value & BUTTON_LEFT)
    {
        player.pos.x -= PLAYER_MOVE_SPEED;
        SPR_setHFlip(player.sprite, FALSE); // Reset flip to face left
    }
    
    if (value & BUTTON_UP)
        player.pos.y -= PLAYER_MOVE_SPEED;
    else if (value & BUTTON_DOWN)
        player.pos.y += PLAYER_MOVE_SPEED;
}
