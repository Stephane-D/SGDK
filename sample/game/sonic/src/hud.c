#include "genesis.h"

#include "hud.h"

#include "player.h"

#include "res_sprite.h"


// Speed, Jump and Gravity interface
Sprite* bars[3];

// forward
static void updateBar(Sprite* bar, f32 min, f32 max, f32 current);


u16 HUD_init(u16 vramIndex)
{
    // Speed, Jump and Gravity setting interface
    bars[0] = SPR_addSprite(&sbar_sprite, 10, 180, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    bars[1] = SPR_addSprite(&jbar_sprite, 18, 180, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    bars[2] = SPR_addSprite(&gbar_sprite, 26, 180, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));

    // update BAR sprites
    updateBar(bars[0], MAX_SPEED_MIN, MAX_SPEED_MAX, maxSpeed);
    updateBar(bars[1], JUMP_SPEED_MIN, JUMP_SPEED_MAX, jumpSpeed);
    updateBar(bars[2], GRAVITY_MIN, GRAVITY_MAX, gravity);

    return vramIndex;
}

void HUD_setVisibility(bool visible)
{
    if (visible)
    {
        SPR_setVisibility(bars[0], VISIBLE);
        SPR_setVisibility(bars[1], VISIBLE);
        SPR_setVisibility(bars[2], VISIBLE);
    }
    else
    {
        SPR_setVisibility(bars[0], HIDDEN);
        SPR_setVisibility(bars[1], HIDDEN);
        SPR_setVisibility(bars[2], HIDDEN);
    }
}

void HUD_handleInput(u16 value)
{
    if (value & BUTTON_RIGHT)
    {
        maxSpeed += FIX32(0.2);
        if (maxSpeed > MAX_SPEED_MAX) maxSpeed = MAX_SPEED_MAX;
        updateBar(bars[0], MAX_SPEED_MIN, MAX_SPEED_MAX, maxSpeed);
    }
    else if (value & BUTTON_LEFT)
    {
        maxSpeed -= FIX32(0.2);
        if (maxSpeed < MAX_SPEED_MIN) maxSpeed = MAX_SPEED_MIN;
        updateBar(bars[0], MAX_SPEED_MIN, MAX_SPEED_MAX, maxSpeed);
    }

    if (value & BUTTON_UP)
    {
        jumpSpeed += FIX32(0.3);
        if (jumpSpeed > JUMP_SPEED_MAX) jumpSpeed = JUMP_SPEED_MAX;
        updateBar(bars[1], JUMP_SPEED_MIN, JUMP_SPEED_MAX, jumpSpeed);
    }
    else if (value & BUTTON_DOWN)
    {
        jumpSpeed -= FIX32(0.3);
        if (jumpSpeed < JUMP_SPEED_MIN) jumpSpeed = JUMP_SPEED_MIN;
        updateBar(bars[1], JUMP_SPEED_MIN, JUMP_SPEED_MAX, jumpSpeed);
    }

    if (value & BUTTON_A)
    {
        gravity -= FIX32(0.005);
        if (gravity < GRAVITY_MIN) gravity = GRAVITY_MIN;
        updateBar(bars[2], GRAVITY_MIN, GRAVITY_MAX, gravity);
    }
    else if (value & BUTTON_B)
    {
        gravity += FIX32(0.005);
        if (gravity > GRAVITY_MAX) gravity = GRAVITY_MAX;
        updateBar(bars[2], GRAVITY_MIN, GRAVITY_MAX, gravity);
    }
}

static void updateBar(Sprite* bar, f32 min, f32 max, f32 current)
{
    f32 levelf;
    s16 leveli;

    levelf = fix32Mul(current, FIX32(16));
    levelf = fix32Div(levelf, (max - min));
    levelf -= min;

    leveli = fix32ToInt(levelf);
    if (leveli < 0) leveli = 0;
    else if (leveli > 16) leveli = 16;

    SPR_setFrame(bar, leveli);
}
