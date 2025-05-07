// *****************************************************************************
//  Silhouette Effect Sample
//
//  This demo showcases a silhouette effect (complete or partial clipping
//  of a sprite by another sprite) using a dot pattern mask sprite.
//  The implementation demonstrates how to hide sprites with a "spray effect"
//  by moving a patterned mask across them. The effect works in both horizontal and
//  vertical directions, with adjustable speed and jitter parameters.
//
//  The effect requires that the masking sprite has a low priority,
//  the masking sprites and one or both layers of the background plane
//  have a high priority. Also, the masking sprite must have a lower
//  depth than the masking sprites.
//
//  Controls:
//  - C Button: Toggle between horizontal/vertical mask movement direction
//  - A/B Buttons: Decrease/Increase mask movement speed
//  - D-Pad: Adjust mask jitter by axes
//  - Start Button: Toggle mask visibility priority
//
//
//  writen by werton playskin on 05/2025
// *****************************************************************************

#include <genesis.h>
#include "res/resources.h"

// Mask jitter range (shaking effect)
#define MASK_MIN_JITTER                 0
#define MASK_MAX_JITTER                 9

// Mask movement speed parameters
#define MASK_MIN_SPEED                  FF32(0)
#define MASK_MAX_SPEED                  FF32(5)
#define MASK_SPEED_STEP                 FF32(0.1)

// Character starting positions
#define SONIC_START_POS_X               180
#define SONIC_START_POS_Y               162
#define ENEMY_START_POS_X               80
#define ENEMY_START_POS_Y               164

// Horizontal mask parameters
#define HOR_MASK_START_POS_X            234
#define HOR_MASK_START_POS_Y            154
#define HOR_MASK_MIN_POS_X              15
#define HOR_MASK_ENEMY_HIDE_POS_X       34
#define HOR_MASK_SONIC_HIDE_POS_X       132

// Vertical mask parameters
#define VERT_MASK_START_POS_X           69
#define VERT_MASK_START_POS_Y           60
#define VERT_MASK_SECOND_POS_X          170
#define VERT_MASK_SECOND_POS_Y          60
#define VERT_MASK_MAX_POS_Y             170
#define VERT_MASK_SWITCH_POS_Y          160

// Game object structure
typedef struct
{
    Sprite *sprite;                     // Pointer to sprite object
    V2ff32 pos;                         // Current position (fast fixed point vector)
    V2s16 offset;                       // Jitter offset
} GameObject;

// Game objects
GameObject sonic;                       // Sonic game object
GameObject enemy;                       // Crab enemy game object
GameObject dotPatternMask;              // Mask game object

// Mask sprites
Sprite *horMask;                        // Mask sprite for horizontal mode
Sprite *vertMask;                       // Mask sprite for vertical mode

// Game state variables
u16 maskJitterX = 0;                    // Horizontal jitter amplitude
u16 maskJitterY = 0;                    // Vertical jitter amplitude
ff32 maskMovingSpeed = FF32(1.0);       // Current mask movement speed
bool isHorizontalMode = TRUE;           // Current mask direction mode
bool isFistSpriteProcessing = TRUE;     // Current sprite processing flag for vertical mode
bool isMaskHighPrio = FALSE;            // Mask visibility priority

// Function prototypes
void InitGame();
void MainLoop();
void Sprites_ResetPosToHorizontalMode();
void Sprites_ResetPosToVerticalMode();
void Sprites_SetVisible();
void Object_SetPos(GameObject *object, ff32 x, ff32 y, bool isRelativePos);
void Object_Move(GameObject *object, u16 value);
void Mask_Update();
void Mask_Jitter(u16 x, u16 y);
void Mask_Switch();
void Joy_EventHandler(u16 joy, u16 changed, u16 state);
void UI_Draw();


int main(bool hardReset)
{
    // Hard reset is used to avoid manually resetting global variables after
    // a soft reset (pressing the reset button), it's not necessary, just for convenience.
    if (!hardReset)
        SYS_hardReset();
    
    InitGame();
    MainLoop();
    
    return 0;
}

// Initializes sprites, and resources
void InitGame()
{
    // Load palettes
    PAL_setPalette(PAL0, imageBGB.palette->data, DMA);
    PAL_setPalette(PAL1, imageBGA.palette->data, DMA);
    PAL_setPalette(PAL2, sonicSpriteDef.palette->data, DMA);
    
    // Initialize sprite engine
    SPR_init();
    
    // Load and draw background images for BGA and BGB layers
    // !!!The PRIORITY of BGA or BGB or both must be set to TRUE, in order to hide the mask behind itself!!!
    VDP_drawImageEx(BG_B, &imageBGB, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_USER_INDEX), 0, 0, FALSE, DMA);
    VDP_drawImageEx(BG_A, &imageBGA,
                    TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, TILE_USER_INDEX + imageBGB.tileset->numTile),
                    0, 0, FALSE, DMA);
    
    // Create mask sprites (in this example, they are for horizontal and vertical modes, but this is not necessary)
    // !!!The MASK SPRITE must have a PRIORITY set to FALSE!!!
    horMask = SPR_addSprite(&horMaskSpriteDef, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    vertMask = SPR_addSprite(&vertMaskSpriteDef, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    
    // Create masking sprites of animated enemy Crab and Sonic
    // !!!The MASKING SPRITES must have a PRIORITY set to TRUE!!!
    sonic.sprite = SPR_addSprite(&sonicSpriteDef, 128, 20, TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
    enemy.sprite = SPR_addSprite(&enemySpriteDef, 128, 20, TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
    
    // Hide vertical mask initially
    SPR_setVisibility(vertMask, HIDDEN);
    dotPatternMask.sprite = horMask;
    
    // Set sprite depths (Z-order).
    // !!!The MASK SPRITE must have a depth LESS than MASKING SPRITES!!!
    SPR_setDepth(sonic.sprite, SPR_MIN_DEPTH + 1);
    SPR_setDepth(enemy.sprite, SPR_MIN_DEPTH + 2);
    SPR_setDepth(horMask, SPR_MIN_DEPTH);
    SPR_setDepth(vertMask, SPR_MIN_DEPTH);
    
    // Initialize positions for horizontal mode
    Sprites_ResetPosToHorizontalMode();
    
    // Set up joypad event handler
    JOY_setEventHandler(Joy_EventHandler);
}

// Main game loop
void MainLoop()
{
    while (TRUE)
    {
        Mask_Update();
        UI_Draw();
        SPR_update();
        SYS_doVBlankProcess();
    }
}

// Sets the position of a game object
void Object_SetPos(GameObject *object, ff32 x, ff32 y, bool isRelativePos)
{
    if (isRelativePos)
        object->pos = (V2ff32) {object->pos.x + x, object->pos.y + y};
    else
        object->pos = (V2ff32) {x, y};
    
    SPR_setPosition(object->sprite, FF32_toInt(object->pos.x), FF32_toInt(object->pos.y));
}

// Moves an object based on input constants direction
void Object_Move(GameObject *object, u16 value)
{
    if (value & BUTTON_RIGHT)
        Object_SetPos(object, maskMovingSpeed, FF32(0), TRUE);
    else if (value & BUTTON_LEFT)
        Object_SetPos(object, -maskMovingSpeed, FF32(0), TRUE);
    
    if (value & BUTTON_UP)
        Object_SetPos(object, FF32(0), -maskMovingSpeed, TRUE);
    else if (value & BUTTON_DOWN)
        Object_SetPos(object, FF32(0), maskMovingSpeed, TRUE);
}

// Makes all relevant sprites visible
void Sprites_SetVisible()
{
    SPR_setVisibility(sonic.sprite, VISIBLE);
    SPR_setVisibility(enemy.sprite, VISIBLE);
    SPR_setVisibility(dotPatternMask.sprite, VISIBLE);
}

// Resets all sprite positions for horizontal mode
void Sprites_ResetPosToHorizontalMode()
{
    Object_SetPos(&sonic, FF32(SONIC_START_POS_X), FF32(SONIC_START_POS_Y), FALSE);
    Object_SetPos(&enemy, FF32(ENEMY_START_POS_X), FF32(ENEMY_START_POS_Y), FALSE);
    Object_SetPos(&dotPatternMask, FF32(HOR_MASK_START_POS_X), FF32(HOR_MASK_START_POS_Y), FALSE);
}

// Resets all sprite positions for vertical mode
void Sprites_ResetPosToVerticalMode()
{
    Object_SetPos(&sonic, FF32(SONIC_START_POS_X), FF32(SONIC_START_POS_Y), FALSE);
    Object_SetPos(&enemy, FF32(ENEMY_START_POS_X), FF32(ENEMY_START_POS_Y), FALSE);
    Object_SetPos(&dotPatternMask, FF32(VERT_MASK_START_POS_X), FF32(VERT_MASK_START_POS_Y), FALSE);
}

// Applies jitter effect to the mask
void Mask_Jitter(u16 x, u16 y)
{
    dotPatternMask.offset.x++;
    dotPatternMask.offset.y++;
    
    if (dotPatternMask.offset.x > x)
        dotPatternMask.offset.x = 0;
    
    if (dotPatternMask.offset.y > y)
        dotPatternMask.offset.y = 0;
    
    SPR_setPosition(dotPatternMask.sprite,
                    FF32_toInt(dotPatternMask.pos.x) + dotPatternMask.offset.x,
                    FF32_toInt(dotPatternMask.pos.y) + dotPatternMask.offset.y);
}

// Switches between horizontal and vertical mask modes
void Mask_Switch()
{
    isHorizontalMode = !isHorizontalMode;
    isFistSpriteProcessing = TRUE;
    SPR_setVisibility(dotPatternMask.sprite, HIDDEN);
    
    if (dotPatternMask.sprite == horMask)
    {
        Sprites_ResetPosToVerticalMode();
        dotPatternMask.sprite = vertMask;
    }
    else
    {
        Sprites_ResetPosToHorizontalMode();
        dotPatternMask.sprite = horMask;
    }
    
    Sprites_SetVisible();
}

// Updates mask position and effect logic
void Mask_Update()
{
    if (isHorizontalMode)
    {
        // Horizontal mode logic
        Object_Move(&dotPatternMask, BUTTON_LEFT);
        Mask_Jitter(maskJitterX, maskJitterY);
        
        // Reset when mask reaches left edge
        if (SPR_getPositionX(dotPatternMask.sprite) <= HOR_MASK_MIN_POS_X)
        {
            Sprites_ResetPosToHorizontalMode();
            Sprites_SetVisible();
        }
        
        // Hide sprites as mask passes them
        if (SPR_getPositionX(dotPatternMask.sprite) <= HOR_MASK_SONIC_HIDE_POS_X)
            SPR_setVisibility(sonic.sprite, HIDDEN);
        
        if (SPR_getPositionX(dotPatternMask.sprite) <= HOR_MASK_ENEMY_HIDE_POS_X)
            SPR_setVisibility(enemy.sprite, HIDDEN);
    }
    else
    {
        // Vertical mode logic
        Object_Move(&dotPatternMask, BUTTON_DOWN);
        Mask_Jitter(maskJitterX, maskJitterY);
        
        // Reset when mask reaches bottom
        if (SPR_getPositionY(dotPatternMask.sprite) >= SONIC_START_POS_X)
            Sprites_ResetPosToVerticalMode();
        
        // Special vertical mode processing
        if (SPR_getPositionY(dotPatternMask.sprite) >= VERT_MASK_SWITCH_POS_Y)
        {
            if (isFistSpriteProcessing)
            {
                // Move mask to second position and hide enemy
                Object_SetPos(&dotPatternMask, FF32(VERT_MASK_SECOND_POS_X), FF32(VERT_MASK_SECOND_POS_Y), FALSE);
                SPR_setVisibility(enemy.sprite, HIDDEN);
                isFistSpriteProcessing = FALSE;
            }
            else
            {
                // Hide sonic
                SPR_setVisibility(sonic.sprite, HIDDEN);
            }
        }
        
        // Complete vertical pass - reset everything
        if (SPR_getPositionY(dotPatternMask.sprite) >= VERT_MASK_MAX_POS_Y)
        {
            SPR_setVisibility(sonic.sprite, VISIBLE);
            SPR_setVisibility(enemy.sprite, VISIBLE);
            Object_SetPos(&dotPatternMask, FF32(VERT_MASK_START_POS_X), FF32(VERT_MASK_START_POS_Y), FALSE);
            isFistSpriteProcessing = TRUE;
        }
    }
}

// Handles joypad input events
void Joy_EventHandler(u16 joy, u16 changed, u16 state)
{
    // Handle jitter adjustment
    if (changed & state & BUTTON_RIGHT)
    {
        if (maskJitterX < MASK_MAX_JITTER)
        {
            dotPatternMask.offset.x = 0;
            maskJitterX++;
        }
    }
    else if (changed & state & BUTTON_LEFT)
    {
        if (maskJitterX > MASK_MIN_JITTER)
        {
            dotPatternMask.offset.x = 0;
            maskJitterX--;
        }
    }
    
    if (changed & state & BUTTON_UP)
    {
        if (maskJitterY < MASK_MAX_JITTER)
        {
            dotPatternMask.offset.y = 0;
            maskJitterY++;
        }
    }
    else if (changed & state & BUTTON_DOWN)
    {
        if (maskJitterY > MASK_MIN_JITTER)
        {
            dotPatternMask.offset.y = 0;
            maskJitterY--;
        }
    }
    
    // Handle speed adjustment
    if (changed & state & BUTTON_A)
    {
        maskMovingSpeed -= MASK_SPEED_STEP;
        if (maskMovingSpeed < MASK_MIN_SPEED)
        {
            maskMovingSpeed = MASK_MIN_SPEED;
        }
    }
    
    if (changed & state & BUTTON_B)
    {
        maskMovingSpeed += MASK_SPEED_STEP;
        if (maskMovingSpeed > MASK_MAX_SPEED)
        {
            maskMovingSpeed = MASK_MAX_SPEED;
        }
    }
    
    // Toggle mask direction
    if (changed & state & BUTTON_C)
        Mask_Switch();
    
    // Toggle mask priority
    if (changed & state & BUTTON_START)
    {
        isMaskHighPrio = !isMaskHighPrio;
        SPR_setPriority(horMask, isMaskHighPrio);
        SPR_setPriority(vertMask, isMaskHighPrio);
    }
}

// Draws the UI text
void UI_Draw()
{
    char str[9][40];
    
    // Format UI strings
    sprintf(str[0], "Use:C button to change direction");
    sprintf(str[1], "    B & A button to +/- speed");
    sprintf(str[2], "    DPAD to +/- mask hor/vert jitter");
    sprintf(str[3], "    START button to show/hide the mask");
    sprintf(str[4], "---------------- Mask ----------------");
    sprintf(str[5], "    Direction:  %s", (isHorizontalMode) ? "HORIZONTAL " : "VERTICAL   ");
    sprintf(str[6], "        Speed:  %01d.%01d   pix/frame", FF32_toInt(maskMovingSpeed),
            (u16) (mulu(FF32_frac(maskMovingSpeed), 10) >> FASTFIX32_FRAC_BITS));
    sprintf(str[7], "       Jitter:  X:%1d   Y:%1d  ", maskJitterX, maskJitterY);
    sprintf(str[8], "          Pos:  X:%03d Y:%03d  ", SPR_getPositionX(dotPatternMask.sprite),
            SPR_getPositionY(dotPatternMask.sprite));
    
    // Draw all UI strings
    for (u16 y = 0; y < 9; y++)
        VDP_drawTextBG(BG_A, str[y], 1, y);
}
