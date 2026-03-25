#include "player.h"
#include "camera.h"
#include "tile_collision.h"
#include "sprites.h"
#include "globals.h"


static bool HandleTileCollision(GameObject *gameObject, TileType tileType);

// Initialize player properties, position and sprite, and set tile collision handler callback
void Player_Init()
{
    // Set player bounding box for collision detection
    Player_SetAabb((Range_s16) {
        {PLAYER_AABB_LEFT, PLAYER_AABB_TOP},
        {PLAYER_AABB_RIGHT, PLAYER_AABB_BOTTOM}
    });
    
    // Place player at screen center minus sprite offsets
    Player_SetPos((s16)(VDP_getScreenWidth() / 2 - spr_def_player_bb.w), (s16)(VDP_getScreenHeight() / 2 - spr_def_player_bb.h));
    
    // Create player sprite with proper palette and attributes
    Player_SetSprite(SPR_addSprite(&spr_def_player_bb, Player_GetPosX(), Player_GetPosY(), TILE_ATTR(PAL2, false, false, true)));
    
    // Set On tile collision handler callback
    player.OnTileCollision = HandleTileCollision;
}

// Update player states and sprite position based on camera
void Player_Update()
{
    Player_UpdateState();
    Player_UpdateSprite();
}


// Handle player input and set velocity accordingly, also update sprite flip based on horizontal movement direction
void Player_HandleInput()
{
    u16 value = JOY_readJoypad(JOY_1);
    
    // reset player velocity before processing input to ensure it stops when no direction is pressed
    Player_SetVelocity(0, 0);
    
    // If A button are pressed, set player move speed to slow
    if (value & (BUTTON_A))
        Player_SetMoveSpeed(PLAYER_MOVE_SPEED_SLOW);
    
    // If any of the B or C buttons are pressed, set player move speed to fast
    if (value & (BUTTON_B | BUTTON_C))
        Player_SetMoveSpeed(PLAYER_MOVE_SPEED_FAST);
    
    // If none of the A, B or C buttons are pressed, set player move speed to normal
    if (!(value & (BUTTON_A | BUTTON_B | BUTTON_C)))
        Player_SetMoveSpeed(PLAYER_MOVE_SPEED_NORMAL);
    
    // Move player position using defined speed constant and update sprite flip
    if (value & BUTTON_RIGHT)
    {
        Player_SetVelocityX(player.moveSpeed);
        SPR_setHFlip(player.sprite, true); // Flip sprite to face right
    }
    else if (value & BUTTON_LEFT)
    {
        Player_SetVelocityX((s16) -player.moveSpeed);
        SPR_setHFlip(player.sprite, false); // Reset flip to face left
    }
    
    if (value & BUTTON_UP)
        Player_SetVelocityY((s16) -player.moveSpeed);
    else if (value & BUTTON_DOWN)
        Player_SetVelocityY(player.moveSpeed);
}

// Set player state and perform state enter action
void Player_SetState(PlayerState state)
{
    player.state = state;
    
    switch (state)
    {
        case PL_STATE_NORMAL:
            player.hurtStateDelay = 0;
            SPR_setVisibility(player.sprite, VISIBLE);
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

// Update player position based on velocity and handle tile collision
bool Player_UpdatePosition()
{
    // If player didn't move, no need to update
    if (!Player_GetVelocityX() && !Player_GetVelocityY())
        return false;
    
    // Limiting the player's velocity by checking for collisions with tiles
    // (with cast the Player to GameObject from which it was inherited)
    GameObject_RestrictVelocityByTileCollision((GameObject *) &player);
    
    // Apply player velocity
    Player_SetPosX((s16)(Player_GetPosX() + Player_GetVelocityX()));
    Player_SetPosY((s16)(Player_GetPosY() + Player_GetVelocityY()));
    
    return true;
}

// Update player sprite position based on camera position to ensure it stays in the correct place on screen
void Player_UpdateSprite()
{
    SPR_setPosition(player.sprite, (s16)(Player_GetPosX() - Camera_GetPosX()), (s16)(Player_GetPosY() - Camera_GetPosY()));
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

// Handling tile collision events based on their type
static bool HandleTileCollision(GameObject *gameObject, TileType tileType)
{
    // For TILE_EMPTY, no collision response is needed
    switch (tileType)
    {
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