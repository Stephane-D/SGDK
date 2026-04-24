#ifndef HEADER_PLAYER
#define HEADER_PLAYER

#include <genesis.h>
#include "defs.h"
#include "typedefs.h"
#include "globals.h"


// Global player instance
extern Player player;

// Player functions
void Player_Init();
void Player_Update();
void Player_HandleInput();
bool Player_UpdatePosition();
void Player_UpdateState();
void Player_UpdateSprite();


// Position getters
FORCE_INLINE s16 Player_GetPosX(void)
{
    return player.pos.x;
}

FORCE_INLINE s16 Player_GetPosY(void)
{
    return player.pos.y;
}

// Position setters
FORCE_INLINE void Player_SetPos(s16 x, s16 y)
{
    player.pos.x = x;
    player.pos.y = y;
}

FORCE_INLINE void Player_SetPosX(s16 x)
{
    player.pos.x = x;
}

FORCE_INLINE void Player_SetPosY(s16 y)
{
    player.pos.y = y;
}

// Velocity getters
FORCE_INLINE s16 Player_GetVelocityX(void)
{
    return player.velocity.x;
}

FORCE_INLINE s16 Player_GetVelocityY(void)
{
    return player.velocity.y;
}

// Velocity setters
FORCE_INLINE void Player_SetVelocity(s16 vx, s16 vy)
{
    player.velocity.x = vx;
    player.velocity.y = vy;
}

FORCE_INLINE void Player_SetVelocityX(s16 vx)
{
    player.velocity.x = vx;
}

FORCE_INLINE void Player_SetVelocityY(s16 vy)
{
    player.velocity.y = vy;
}

// Sprite setter
FORCE_INLINE void Player_SetSprite(Sprite *sprite)
{
    player.sprite = sprite;
}

// Get player sprite width
FORCE_INLINE u16 Player_GetWidth()
{
    return player.sprite->definition->w;
}

// Get player sprite height
FORCE_INLINE u16 Player_GetHeight()
{
    return player.sprite->definition->h;
}

// AABB setter
FORCE_INLINE void Player_SetAabb(Range_s16 aabb)
{
    player.aabb = aabb;
}

// Move speed setter
FORCE_INLINE void Player_SetMoveSpeed(s16 moveSpeed)
{
    player.moveSpeed = moveSpeed;
}

#endif //HEADER_PLAYER
