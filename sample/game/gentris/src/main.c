// *****************************************************************************
// Gentris - game Sample
//
// This example demonstrates how to implement a classic Tetris game clone
//
// Notes:
//   - game scene management using a Finite State Machine (nfsm module).
//   - custom UI module for creating menus (title menu, options, pause menu,
//   game over menu etc.).
//   - all graphics and sound effects are generated procedurally from code,
//   no external assets or Rescomp conversion used.
//   - two game modes: Classic (endless) and 40 Lines (time attack).
//
// Controls:
//   DPAD Left/Right - Move piece horizontally
//   DPAD Down       - Soft drop (accelerate falling)
//   DPAD Up         - Hard drop (instant placement)
//   Button A        - Rotate clockwise
//   Button B        - Rotate counter-clockwise
//   Button Start    - Pause/In-game menu
//
//  written by werton playskin, 08/2026
// *****************************************************************************

#include <genesis.h>
#include "game.h"


int main(bool hardReset)
{
    // Do hard reset on soft reset to nullify global variables
    if (!hardReset)
        SYS_hardReset();

    // Initialize game system and data
    Game_Init();
    // Start the main game loop
    Game_MainLoop();

    return 0;
}

