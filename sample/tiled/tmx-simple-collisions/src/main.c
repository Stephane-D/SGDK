// *****************************************************************************
// TMX Simple Collisions Sample
//
// This example demonstrates how to implement a simple collision system using the Tiled TMX tile layer as a collision map,
// as well as a simple rectangular collision controller (without inclined surfaces).
//
// Notes:
//   - TileMap is used as a structure for storing a collision map, which is not very optimal for this purpose.
//     For large maps, it can consume too much memory, especially if you are going load it into RAM to make a destructible map.
//   - The collision map is defined in the TMX map as a tile layer with the same size as the foreground layer,
//     where each tile represents a collision type (e.g. empty space, solid tile, damager tile).
//   - For simplicity, fixed point types are not used for player coordinates, and a finite state machine is not used to process player states.
//   - The example is not optimized in terms of performance or memory usage, it is just a simple demonstration
//     of a concept that focuses on simplicity and code readability.
//
//  Controls: DPAD to move the player and observe camera scrolling.
//
//  written by werton playskin on 03/2026
//  graphics by Luis Zuno aka Ansimuz ansimuz.itch.io (CC0 license)
// *****************************************************************************

#include <genesis.h>
#include "defs.h"
#include "typedefs.h"
#include "maps.h"
#include "player.h"
#include "camera.h"
#include "tile_collision.h"
#include "sprites.h"
#include "game.h"

// Global variables
Camera camera;     // Defined here, declared extern in camera.h
Player player;     // Defined here, declared extern in player.h

// Declare foreground and background map's pointers
Map *fgMap;     // Defined here, declared extern in camera.h
Map *bgMap;     // Defined here, declared extern in camera.h


int main(bool hardReset)
{
    // do hard reset on soft reset to nullify global variables
    if (!hardReset)
        SYS_hardReset();

    Game_Init();
    Game_DoLoop();
    return 0;
}



