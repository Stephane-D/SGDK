#include "genesis.h"

#include "player.h"

#include "level.h"
#include "camera.h"
#include "utils.h"

#include "res_sprite.h"
#include "res_sound.h"


#define ANIM_STAND          0
#define ANIM_WAIT           1
#define ANIM_WALK           2
#define ANIM_RUN            3
#define ANIM_BRAKE          4
#define ANIM_UP             5
#define ANIM_CROUNCH        6
#define ANIM_ROLL           7

#define RUN_SPEED           FIX32(6L)
#define BRAKE_SPEED         FIX32(2L)

#define ACCEL               FIX32(0.1)
#define DE_ACCEL            FIX32(0.15)


// player (sonic) sprite
Sprite* player;

// physic variables
fix32 maxSpeed;
fix32 jumpSpeed;
fix32 gravity;

// position and movement variables
fix32 posX;
fix32 posY;
fix32 movX;
fix32 movY;
s16 xOrder;
s16 yOrder;


u16 PLAYER_init(u16 vramIndex)
{
    // default speeds
    maxSpeed = MAX_SPEED_DEFAULT;
    jumpSpeed = JUMP_SPEED_DEFAULT;
    gravity = GRAVITY_DEFAULT;

    // set main sprite position (camera position may be adjusted depending it)
    posX = FIX32(48L);
    posY = MAX_POSY;
    movX = FIX32(0);
    movY = FIX32(0);
    xOrder = 0;
    yOrder = 0;

    // init sonic sprite
    player = SPR_addSprite(&sonic_sprite, 0, 0, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));

    // do not used static vram allocation here
    return vramIndex;
}


void PLAYER_update(void)
{
    // sonic physic, uupdate movement first
    if (xOrder > 0)
    {
        movX += ACCEL;
        // going opposite side, quick breaking
        if (movX < 0) movX += ACCEL;

        if (movX >= maxSpeed) movX = maxSpeed;
    }
    else if (xOrder < 0)
    {
        movX -= ACCEL;
        // going opposite side, quick breaking
        if (movX > 0) movX -= ACCEL;

        if (movX <= -maxSpeed) movX = -maxSpeed;
    }
    else
    {
        if ((movX < FIX32(0.1)) && (movX > FIX32(-0.1)))
            movX = 0;
        else if ((movX < FIX32(0.3)) && (movX > FIX32(-0.3)))
            movX -= movX >> 2;
        else if ((movX < FIX32(1)) && (movX > FIX32(-1)))
            movX -= movX >> 3;
        else
            movX -= movX >> 4;
    }

    // update position from movement
    posX += movX;
    posY += movY;

    // apply gravity if needed
    if (movY)
    {
        if (posY > MAX_POSY)
        {
            posY = MAX_POSY;
            movY = 0;
        }
        else movY += gravity;
    }
    // clip x pos
    if (posX >= MAX_POSX)
    {
        posX = MAX_POSX;
        movX = 0;
    }
    else if (posX <= MIN_POSX)
    {
        posX = MIN_POSX;
        movX = 0;
    }

    // finally update sprite state from internal state
    if (movY) SPR_setAnim(player, ANIM_ROLL);
    else
    {
        if (((movX >= BRAKE_SPEED) && (xOrder < 0)) || ((movX <= -BRAKE_SPEED) && (xOrder > 0)))
        {
            if (player->animInd != ANIM_BRAKE)
            {
                XGM2_playPCM(sonic_stop_sfx, sizeof(sonic_stop_sfx), SOUND_PCM_CH3);
                SPR_setAnim(player, ANIM_BRAKE);
            }
        }
        else if ((movX >= RUN_SPEED) || (movX <= -RUN_SPEED))
            SPR_setAnim(player, ANIM_RUN);
        else if (movX != 0)
            SPR_setAnim(player, ANIM_WALK);
        else
        {
            if (yOrder < 0)
                SPR_setAnim(player, ANIM_UP);
            else if (yOrder > 0)
                SPR_setAnim(player, ANIM_CROUNCH);
            else
                SPR_setAnim(player, ANIM_STAND);
        }
    }

    if (movX > 0) SPR_setHFlip(player, FALSE);
    else if (movX < 0) SPR_setHFlip(player, TRUE);
}

void PLAYER_updateScreenPosition(void)
{
    setSpritePosition(player, F32_toInt(posX) - camPosX, F32_toInt(posY) - camPosY);
}


void PLAYER_handleInput(u16 value)
{
    if (value & BUTTON_UP) yOrder = -1;
    else if (value & BUTTON_DOWN) yOrder = +1;
    else yOrder = 0;

    if (value & BUTTON_LEFT) xOrder = -1;
    else if (value & BUTTON_RIGHT) xOrder = +1;
    else xOrder = 0;
}

void PLAYER_doJoyAction(u16 joy, u16 changed, u16 state)
{
    if (changed & state & (BUTTON_A | BUTTON_B | BUTTON_C))
    {
        if (movY == 0)
        {
            movY = -jumpSpeed;
            XGM2_playPCMEx(sonic_jump_sfx, sizeof(sonic_jump_sfx), SOUND_PCM_CH2, 15, TRUE, FALSE);
        }
    }
}

