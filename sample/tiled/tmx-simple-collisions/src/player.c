#include <genesis.h>
#include "typedefs.h"
#include "player.h"
#include "camera.h"
#include "tile_collision.h"
#include "sprites.h"

static bool HandleTileCollision(GameObject *gameObject, TileType tileType);

void Player_Init()
{
    // Place player at screen center minus sprite offsets
    Player_SetPos(VDP_getScreenWidth() / 2 - spr_def_player_bb.w, VDP_getScreenHeight() / 2 - spr_def_player_bb.h);
    
    // Create player sprite with proper palette and attributes
    Player_SetSprite(SPR_addSprite(&spr_def_player_bb, Player_GetPosX(), Player_GetPosY(), TILE_ATTR(PAL2, FALSE, FALSE, TRUE)));
    
    // Set On tile collision handler callback
    player.OnTileCollision = HandleTileCollision;
}

// Update player position based on velocity and handle tile collision
bool Player_UpdatePosition()
{
    // If player didn't move, no need to update
    if (!Player_GetVelocityX() && !Player_GetVelocityY())
        return FALSE;
    
    // Limiting the player's velocity by checking for collisions with tiles
    // (with cast the Player to GameObject from which it was inherited)
    GameObject_RestrictVelocityByTileCollision((GameObject *) &player);
    
    // Apply player velocity
    Player_SetPosX(Player_GetPosX() + Player_GetVelocityX());
    Player_SetPosY(Player_GetPosY() + Player_GetVelocityY());
    
    return TRUE;
}

// Update player sprite position based on camera position to ensure it stays in the correct place on screen
void Player_UpdateSpritePosition()
{
    SPR_setPosition(player.sprite, Player_GetPosX() - Camera_GetPosX(), Player_GetPosY() - Camera_GetPosY());
}

// Handle player input and set velocity accordingly, also update sprite flip based on horizontal movement direction
void Player_HandleInput()
{
    u16 value = JOY_readJoypad(JOY_1);
    
    // reset player velocity before processing input to ensure it stops when no direction is pressed
    Player_SetVelocity(0, 0);
    
    // Move player position using defined speed constant and update sprite flip
    if (value & BUTTON_RIGHT)
    {
        Player_SetVelocityX(PLAYER_MOVE_SPEED);
        SPR_setHFlip(player.sprite, TRUE); // Flip sprite to face right
    }
    else if (value & BUTTON_LEFT)
    {
        Player_SetVelocityX(-PLAYER_MOVE_SPEED);
        SPR_setHFlip(player.sprite, FALSE); // Reset flip to face left
    }
    
    if (value & BUTTON_UP)
        Player_SetVelocityY(-PLAYER_MOVE_SPEED);
    else if (value & BUTTON_DOWN)
        Player_SetVelocityY(PLAYER_MOVE_SPEED);
}

// Set player state and perform state enter action
void Player_SetState(PlayerState state)
{
    player.state = state;
    
    switch (state)
    {
        case PL_STATE_NORMAL:
            SPR_setPalette(player.sprite, PAL2);
            break;
        
        case PL_STATE_HURT:
            player.hurtStateDelay = PLAYER_HURT_DELAY_FRAMES;
            SPR_setPalette(player.sprite, PAL3);
            break;
        
        default:
            break;
    }
}

// Update player state
void Player_UpdateState()
{
    switch (player.state)
    {
        case PL_STATE_NORMAL:
            break;
        
        case PL_STATE_HURT:
            if (player.hurtStateDelay & 1)
                SPR_setVisibility(player.sprite, VISIBLE);
            else
                SPR_setVisibility(player.sprite, HIDDEN);
            
            if (--player.hurtStateDelay == 0)
                Player_SetState(PL_STATE_NORMAL);
            break;
        
        default:
            break;
    }
}

// Update player states and sprite position based on camera
void Player_Update()
{
    Player_UpdateState();
    Player_UpdateSpritePosition();
}

// Handling tile collision events based on their type
static bool HandleTileCollision(GameObject *gameObject, TileType tileType)
{
    switch (tileType)
    {
        case TILE_EMPTY:
            return false;
        
        case TILE_SOLID:
            return true;
        
        case TILE_DAMAGER:
            if (player.state != PL_STATE_HURT)
                Player_SetState(PL_STATE_HURT);
            return true;
        
        default:
            return false;
    }
}