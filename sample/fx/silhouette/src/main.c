// *****************************************************************************
//  Silhouette Effect Sample
//
//  This demo showcases a silhouette effect (complete or partial clipping
//  of a sprite by another sprite) using a dot pattern mask sprite.
//  The implementation demonstrates how to hide sprites with a "spray effect"
//  by moving a patterned mask across them. The effect works in both horizontal and
//  vertical directions, with adjustable speed and jitter parameters.
//
//  Core Effect Requirements:
//  1. Masking Sprite: Must have LOW priority.
//  2. Masked Sprites (e.g., Sonic, enemy): Must have HIGH priority.
//  3. Background Plane(s) (BG_A or BG_B or both): Must have HIGH priority to hide the mask behind them.
//  4. Depth: The masking sprite's depth must be numerically LESS than the depth of the sprites it masks.
//
//  Controls:
//  - C Button: Toggle between a horizontal/vertical mask movement direction.
//  - A/B Buttons: Decrease/Increase mask movement speed.
//  - D-Pad: Adjust mask jitter along X and Y axes.
//  - Start Button: Toggle mask sprite's priority (useful for visually demonstrating how the effect works).
//
//  Written by werton playskin on 05/2025
// *****************************************************************************

#include <genesis.h>
#include "res/resources.h"

// --- Configuration Constants ---
// Mask jitter (shaking effect) parameters
#define MASK_MIN_JITTER                 0           // Minimum jitter amplitude (pixels)
#define MASK_MAX_JITTER                 9           // Maximum jitter amplitude (pixels)

// Mask movement speed parameters
#define MASK_MIN_SPEED                  FF32(0.0)   // Minimum speed for the mask (fixed point)
#define MASK_MAX_SPEED                  FF32(5.0)   // Maximum speed for the mask (fixed point)
#define MASK_SPEED_STEP                 FF32(0.1)   // Increment/decrement step for mask speed (fixed point)

// Character starting positions (pixels)
#define SONIC_START_POS_X               180         // Initial X position for Sonic
#define SONIC_START_POS_Y               162         // Initial Y position for Sonic
#define ENEMY_START_POS_X               80          // Initial X position for the enemy (Crab)
#define ENEMY_START_POS_Y               164         // Initial Y position for the enemy (Crab)

// Horizontal mask mode parameters (pixels)
#define HOR_MASK_START_POS_X            234         // Initial X position for the horizontal mask
#define HOR_MASK_START_POS_Y            154         // Initial Y position for the horizontal mask
#define HOR_MASK_MIN_POS_X              15          // X position at which the horizontal mask resets
#define HOR_MASK_ENEMY_HIDE_POS_X       34          // X position threshold to hide the enemy in horizontal mode
#define HOR_MASK_SONIC_HIDE_POS_X       132         // X position threshold to hide Sonic in horizontal mode

// Vertical mask mode parameters (pixels)
#define VERT_MASK_START_POS_X           69          // Initial X position for the vertical mask
#define VERT_MASK_START_POS_Y           60          // Initial Y position for the vertical mask
#define VERT_MASK_SECOND_POS_X          170         // X position for the vertical mask during its second processing stage
#define VERT_MASK_SECOND_POS_Y          60          // Y position for the vertical mask during its second processing stage
#define VERT_MASK_MAX_POS_Y             170         // Y position at which the vertical mask resets (completes its pass)
#define VERT_MASK_SWITCH_POS_Y          160         // Y position threshold to trigger the second stage of vertical masking

// UI Text Layout Constants
#define UI_TEXT_START_COL               1           // Starting column for drawing UI text on background plane
#define UI_MAX_LINES                    12          // Maximum number of lines for UI text
#define UI_MAX_LINE_LENGTH              40          // Maximum length of a single UI text line (characters)

// --- Structures ---
/**
 * @brief Represents a generic game object with a sprite and position.
 */
typedef struct
{
    Sprite *sprite;                     // Pointer to the SGDK Sprite object.
    V2ff32 pos;                         // Current position using fast fixed-point 2D vector (ff32).
    V2s16 offset;                       // Jitter offset (primarily used by the mask object for the shaking effect).
} GameObject;

// --- Global Game Variables ---

// Game objects instances
GameObject sonic;                       // Sonic the Hedgehog game object.
GameObject enemy;                       // Crab enemy game object.
GameObject dotPatternMaskObject;        // The mask game object, using a dot pattern sprite.

// Pointers to mask sprites (these are assigned from sprite definitions in InitGame)
Sprite *horizontalMaskSprite;           // Sprite used for the mask in horizontal mode.
Sprite *verticalMaskSprite;             // Sprite used for the mask in vertical mode.

// Game state variables
u16 maskJitterX = 0;                    // Current horizontal jitter amplitude for the mask.
u16 maskJitterY = 0;                    // Current vertical jitter amplitude for the mask.
ff32 maskMovingSpeed = FF32(1.0);       // Current movement speed of the mask.
bool isHorizontalMode = TRUE;           // Flag indicating if the mask is currently in horizontal (TRUE) or vertical (FALSE) mode.
bool isFirstSpriteProcessing = TRUE;    // Flag for managing the two-stage sprite processing in vertical mask mode.
bool isMaskHighPriority = FALSE;        // Flag to toggle the mask sprite's VDP priority (for demonstration).

// --- Function Prototypes ---
// Core game functions
void InitGame();
void MainLoop();
void ResetCharacterPositions();
void Sprites_ResetPosToHorizontalMode();
void Sprites_ResetPosToVerticalMode();
void Sprites_SetVisible();
void GameObject_SetPos(GameObject *object, ff32 x, ff32 y, bool isRelativePos);
void GameObject_Move(GameObject *object, u16 directionButton, ff32 speed);
void Mask_Update();
void Mask_UpdateHorizontalMode();
void Mask_UpdateVerticalMode();
static void Sprites_HideInHorizontalMode(s16 maskPosX);
static void Mask_ProcessVerticalStages(s16 maskPosY);
void Mask_JitterUpdate(u16 jitterAmplitudeX, u16 jitterAmplitudeY);
void Mask_SwitchMode();
static u16 AdjustJitterValue(u16 currentValue, s16 change, u16 minVal, u16 maxVal, s16 *offsetComponentToReset);
void Joy_EventHandler(u16 joy, u16 changed, u16 state);
void UI_Draw();

// --- Function Implementations ---
/**
 * @brief Main entry point of the application.
 * @param hardReset TRUE if the system was hard reset, FALSE for soft reset.
 */
int main(bool hardReset)
{
    // A hard reset ensures global variables are cleared after a soft reset (reset button press);
    // this is for convenience, not strictly necessary as SGDK might handle some of this.
    if (!hardReset)
        SYS_hardReset();
    
    InitGame(); // Initialize game resources and state
    MainLoop(); // Enter the main game loop
    
    return 0;
}

/**
 * @brief Initializes all game resources, sprites, palettes, and initial states.
 * This function sets up the VDP, loads graphics, and prepares objects for the game loop.
 */
void InitGame()
{
    // Load palettes for backgrounds and sprites
    PAL_setPalette(PAL0, imageBGB.palette->data, DMA); // Palette for background layer B
    PAL_setPalette(PAL1, imageBGA.palette->data, DMA); // Palette for background layer A
    PAL_setPalette(PAL2, sonicSpriteDef.palette->data, DMA); // Palette for sprites (Sonic, Enemy, Mask)
    
    // Initialize the SGDK sprite engine
    SPR_init();
    
    // Load and draw background images for BGA and BGB layers
    // CRITICAL: For the mask to be hidden by the background, BG_A, BG_B, or both must have high priority (PRIORITY = TRUE).
    VDP_drawImageEx(BG_B, &imageBGB, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_USER_INDEX), 0, 0, FALSE, DMA);
    VDP_drawImageEx(BG_A, &imageBGA,
                    TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, TILE_USER_INDEX + imageBGB.tileset->numTile),
                    0, 0, FALSE, DMA);
    
    // Create mask sprites. In this example, they are separate for horizontal and vertical modes,
    // but this separation is not strictly necessary for the effect itself if a single, larger mask sprite were used and manipulated.
    // CRITICAL: The MASK SPRITE must have low priority (PRIORITY = FALSE) to be obscured by high-priority sprites/backgrounds.
    horizontalMaskSprite = SPR_addSprite(&horMaskSpriteDef, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    verticalMaskSprite = SPR_addSprite(&vertMaskSpriteDef, 0, 0, TILE_ATTR(PAL2, FALSE, FALSE, FALSE));
    
    // Create sprites to be masked (animated Crab enemy and Sonic).
    // CRITICAL: Sprites to be masked (e.g., Sonic, enemy) must have high priority (PRIORITY = TRUE) to appear "in front" of the low-priority mask.
    sonic.sprite = SPR_addSprite(&sonicSpriteDef, 0, 0, TILE_ATTR(PAL2, TRUE, FALSE, FALSE)); // Initial position set later
    enemy.sprite = SPR_addSprite(&enemySpriteDef, 0, 0, TILE_ATTR(PAL2, TRUE, FALSE, FALSE)); // Initial position set later
    
    // Initially, hide the vertical mask sprite and set the horizontal mask as the active one.
    SPR_setVisibility(verticalMaskSprite, HIDDEN);
    dotPatternMaskObject.sprite = horizontalMaskSprite; // Start with horizontal mask sprite
    
    // Set sprite depths (Z-order). This is crucial for the silhouette effect.
    // CRITICAL: The MASK SPRITE's depth must be numerically less (closer to the "camera") than the depth of the sprites it masks.
    // However, due to priority, it will only show where high-priority objects are NOT present.
    SPR_setDepth(sonic.sprite, SPR_MIN_DEPTH + 1); // Sonic is further back than enemy (relative to each other)
    SPR_setDepth(enemy.sprite, SPR_MIN_DEPTH + 2); // Enemy is further back
    SPR_setDepth(horizontalMaskSprite, SPR_MIN_DEPTH); // Mask is closest (numerically smallest depth)
    SPR_setDepth(verticalMaskSprite, SPR_MIN_DEPTH);   // Same for vertical mask
    
    // Set initial positions for all sprites according to the horizontal mode default.
    Sprites_ResetPosToHorizontalMode();
    
    // Register the joypad event handler function.
    JOY_setEventHandler(Joy_EventHandler);
}

/**
 * @brief The main game loop where game logic is updated and rendering occurs each frame.
 */
void MainLoop()
{
    while (TRUE) // Loop forever
    {
        Mask_Update();          // Update mask logic (movement, hiding sprites)
        UI_Draw();              // Draw UI text information
        SPR_update();           // Update all hardware sprites on screen
        SYS_doVBlankProcess();  // Wait for VBlank and perform system tasks (like DMA)
    }
}

/**
 * @brief Resets the positions of the character sprites (Sonic and Enemy) to their defined starting points.
 */
void ResetCharacterPositions()
{
    GameObject_SetPos(&sonic, FF32(SONIC_START_POS_X), FF32(SONIC_START_POS_Y), FALSE /* absolute */);
    GameObject_SetPos(&enemy, FF32(ENEMY_START_POS_X), FF32(ENEMY_START_POS_Y), FALSE /* absolute */);
}

/**
 * @brief Resets all relevant sprite positions for the horizontal mask mode.
 * This includes characters and the mask itself.
 */
void Sprites_ResetPosToHorizontalMode()
{
    ResetCharacterPositions(); // Reset Sonic and Enemy positions
    // Set the horizontal mask to its starting position for this mode
    GameObject_SetPos(&dotPatternMaskObject, FF32(HOR_MASK_START_POS_X), FF32(HOR_MASK_START_POS_Y), FALSE /* absolute */);
}

/**
 * @brief Resets all relevant sprite positions for the vertical mask mode.
 * This includes characters, the mask, and the vertical processing stage flag.
 */
void Sprites_ResetPosToVerticalMode()
{
    ResetCharacterPositions(); // Reset Sonic and Enemy positions
    // Set the vertical mask to its starting position for this mode
    GameObject_SetPos(&dotPatternMaskObject, FF32(VERT_MASK_START_POS_X), FF32(VERT_MASK_START_POS_Y), FALSE /* absolute */);
    isFirstSpriteProcessing = TRUE; // Reset the flag for the two-stage vertical masking process
}

/**
 * @brief Makes all key game sprites (Sonic, Enemy, and the current Mask) visible.
 */
void Sprites_SetVisible()
{
    SPR_setVisibility(sonic.sprite, VISIBLE);
    SPR_setVisibility(enemy.sprite, VISIBLE);
    SPR_setVisibility(dotPatternMaskObject.sprite, VISIBLE);
}

/**
 * @brief Sets the position of a GameObject.
 * @param object Pointer to the GameObject to modify.
 * @param x The X coordinate (fixed point).
 * @param y The Y coordinate (fixed point).
 * @param isRelativePos If TRUE, x and y are added to the current position; otherwise, position is set absolutely.
 */
void GameObject_SetPos(GameObject *object, ff32 x, ff32 y, bool isRelativePos)
{
    if (isRelativePos)
    {
        object->pos.x += x;
        object->pos.y += y;
    }
    else
    {
        object->pos.x = x;
        object->pos.y = y;
    }
    // Apply the new position to the hardware sprite (converting ff32 to integer screen coordinates)
    SPR_setPosition(object->sprite, FF32_toInt(object->pos.x), FF32_toInt(object->pos.y));
}

/**
 * @brief Moves a GameObject based on a direction button and a given speed.
 * Typically used for the mask's movement.
 * @param object Pointer to the GameObject to move.
 * @param directionButton The button constant (e.g., BUTTON_LEFT) indicating movement direction.
 * @param speed The speed of movement (fixed point).
 */
void GameObject_Move(GameObject *object, u16 directionButton, ff32 speed)
{
    if (directionButton & BUTTON_RIGHT)
        GameObject_SetPos(object, speed, FF32(0), TRUE /* relative */);
    else if (directionButton & BUTTON_LEFT)
        GameObject_SetPos(object, -speed, FF32(0), TRUE /* relative */);
    
    if (directionButton & BUTTON_UP)
        GameObject_SetPos(object, FF32(0), -speed, TRUE /* relative */);
    else if (directionButton & BUTTON_DOWN)
        GameObject_SetPos(object, FF32(0), speed, TRUE /* relative */);
}

/**
 * @brief Updates the mask's jitter effect based on current jitter amplitudes.
 * This function should be called each frame to apply the shaking.
 * @param jitterAmplitudeX Maximum horizontal jitter offset.
 * @param jitterAmplitudeY Maximum vertical jitter offset.
 */
void Mask_JitterUpdate(u16 jitterAmplitudeX, u16 jitterAmplitudeY)
{
    // Increment internal offset counters for jitter
    dotPatternMaskObject.offset.x++;
    dotPatternMaskObject.offset.y++;
    
    // Wrap around the offset if it exceeds the current amplitude
    if (dotPatternMaskObject.offset.x > jitterAmplitudeX)
        dotPatternMaskObject.offset.x = 0;
    
    if (dotPatternMaskObject.offset.y > jitterAmplitudeY)
        dotPatternMaskObject.offset.y = 0;
    
    // Apply the base position plus the current jitter offset to the mask sprite
    SPR_setPosition(dotPatternMaskObject.sprite,
                    FF32_toInt(dotPatternMaskObject.pos.x) + dotPatternMaskObject.offset.x,
                    FF32_toInt(dotPatternMaskObject.pos.y) + dotPatternMaskObject.offset.y);
}

/**
 * @brief Switches the mask mode between horizontal and vertical.
 * It updates the active mask sprite, resets positions, and ensures visibility.
 */
void Mask_SwitchMode()
{
    isHorizontalMode = !isHorizontalMode; // Toggle the mode flag
    SPR_setVisibility(dotPatternMaskObject.sprite, HIDDEN); // Hide current mask before switching to prevent visual glitch
    
    if (isHorizontalMode)
    {
        Sprites_ResetPosToHorizontalMode();
        dotPatternMaskObject.sprite = horizontalMaskSprite; // Set active mask to horizontal
    }
    else
    {
        Sprites_ResetPosToVerticalMode();
        dotPatternMaskObject.sprite = verticalMaskSprite;   // Set active mask to vertical
    }
    
    Sprites_SetVisible(); // Make all relevant sprites (including the new mask) visible
}

/**
 * @brief Hides Sonic and/or Enemy sprites based on the horizontal mask's X position.
 * Static helper function for Mask_UpdateHorizontalMode.
 * @param maskPosX The current X position of the mask sprite.
 */
static void Sprites_HideInHorizontalMode(s16 maskPosX)
{
    // Hide Sonic if the mask has moved past its defined hiding threshold
    if (maskPosX <= HOR_MASK_SONIC_HIDE_POS_X)
        SPR_setVisibility(sonic.sprite, HIDDEN);
    
    // Hide the enemy if the mask has moved past its defined hiding threshold
    if (maskPosX <= HOR_MASK_ENEMY_HIDE_POS_X)
        SPR_setVisibility(enemy.sprite, HIDDEN);
}

/**
 * @brief Handles the logic for updating the mask in horizontal mode.
 * This includes movement, jitter, sprite hiding, and reset conditions.
 */
void Mask_UpdateHorizontalMode()
{
    // Move the mask to the left at the current speed
    GameObject_Move(&dotPatternMaskObject, BUTTON_LEFT, maskMovingSpeed);
    // Apply jitter effect
    Mask_JitterUpdate(maskJitterX, maskJitterY);
    
    s16 currentMaskPosX = SPR_getPositionX(dotPatternMaskObject.sprite);
    
    // Check if the mask has reached the leftmost boundary for reset
    if (currentMaskPosX <= HOR_MASK_MIN_POS_X)
    {
        Sprites_ResetPosToHorizontalMode(); // Reset positions for this mode
        Sprites_SetVisible();               // Ensure all sprites are visible after reset
    }
    else // If not resetting, proceed to hide sprites based on mask position
    {
        Sprites_HideInHorizontalMode(currentMaskPosX);
    }
}

/**
 * @brief Manages the two-stage sprite hiding process in vertical mask mode.
 * Static helper function for Mask_UpdateVerticalMode.
 * @param maskPosY The current Y position of the mask sprite.
 */
static void Mask_ProcessVerticalStages(s16 maskPosY)
{
    // This logic triggers when the mask reaches a certain Y threshold (VERT_MASK_SWITCH_POS_Y)
    if (maskPosY >= VERT_MASK_SWITCH_POS_Y)
    {
        if (isFirstSpriteProcessing) // Check if it's the first stage of this specific pass
        {
            // First stage: move the mask to a secondary X,Y position and hide the enemy sprite
            GameObject_SetPos(&dotPatternMaskObject, FF32(VERT_MASK_SECOND_POS_X), FF32(VERT_MASK_SECOND_POS_Y), FALSE /* absolute */);
            SPR_setVisibility(enemy.sprite, HIDDEN);
            isFirstSpriteProcessing = FALSE; // Advance to the second stage for the next relevant check
        }
        else
        {
            // Second stage (enemy is already hidden): hide the Sonic sprite
            SPR_setVisibility(sonic.sprite, HIDDEN);
        }
    }
}

/**
 * @brief Handles the logic for updating the mask in vertical mode.
 * This includes movement, jitter, two-stage sprite hiding, and reset conditions.
 */
void Mask_UpdateVerticalMode()
{
    // Move the mask downwards at the current speed
    GameObject_Move(&dotPatternMaskObject, BUTTON_DOWN, maskMovingSpeed);
    // Apply jitter effect
    Mask_JitterUpdate(maskJitterX, maskJitterY);
    
    s16 currentMaskPosY = SPR_getPositionY(dotPatternMaskObject.sprite);
    
    // Check if the mask has reached the bottommost boundary for a full reset
    if (currentMaskPosY >= VERT_MASK_MAX_POS_Y)
    {
        Sprites_ResetPosToVerticalMode(); // Reset positions for this mode (also resets isFirstSpriteProcessing)
        Sprites_SetVisible();             // Make all sprites visible again for the new pass
    }
    else // If not resetting, proceed with the stage-based sprite hiding logic
    {
        Mask_ProcessVerticalStages(currentMaskPosY);
    }
}

/**
 * @brief Main update function for the mask, called each frame.
 * It dispatches to either horizontal or vertical mode update logic.
 */
void Mask_Update()
{
    if (isHorizontalMode)
    {
        Mask_UpdateHorizontalMode();
    }
    else
    {
        Mask_UpdateVerticalMode();
    }
}

/**
 * @brief Adjusts a jitter amplitude value, ensuring it stays within min/max bounds.
 * Static helper function for Joy_EventHandler.
 * @param currentValue The current jitter amplitude.
 * @param change The amount to change the value by (+1 or -1).
 * @param minVal The minimum allowed value for the jitter amplitude.
 * @param maxVal The maximum allowed value for the jitter amplitude.
 * @param offsetComponentToReset Pointer to the mask's internal jitter offset (e.g., dotPatternMaskObject.offset.x) to reset to 0.
 * @return The new jitter amplitude value.
 */
static u16 AdjustJitterValue(u16 currentValue, s16 change, u16 minVal, u16 maxVal, s16 *offsetComponentToReset)
{
    s16 newValueSigned = (s16) currentValue + change;
    // Check if the new value is within the defined bounds
    if (newValueSigned >= (s16) minVal && newValueSigned <= (s16) maxVal)
    {
        // If an offset component is provided, reset it to prevent visual jumps when changing amplitude
        if (offsetComponentToReset)
            *offsetComponentToReset = 0;
        return (u16) newValueSigned;
    }
    return currentValue; // Return original value if change is out of bounds
}

/**
 * @brief Handles joypad input events to control mask parameters.
 * @param joy The joypad number (JOY_1, JOY_2, etc.).
 * @param changed A bitmask of buttons whose state has changed since the last call.
 * @param state A bitmask of currently pressed buttons.
 */
void Joy_EventHandler(u16 joy, u16 changed, u16 state)
{
    // Handle jitter adjustment for X axis using D-Pad Right/Left
    if (changed & state & BUTTON_RIGHT) // If Right is newly pressed
        maskJitterX = AdjustJitterValue(maskJitterX, 1, MASK_MIN_JITTER, MASK_MAX_JITTER, &dotPatternMaskObject.offset.x);
    else if (changed & state & BUTTON_LEFT) // If Left is newly pressed
        maskJitterX = AdjustJitterValue(maskJitterX, -1, MASK_MIN_JITTER, MASK_MAX_JITTER, &dotPatternMaskObject.offset.x);
    
    // Handle jitter adjustment for Y axis using D-Pad Up/Down
    if (changed & state & BUTTON_UP) // If Up is newly pressed
        maskJitterY = AdjustJitterValue(maskJitterY, 1, MASK_MIN_JITTER, MASK_MAX_JITTER, &dotPatternMaskObject.offset.y);
    else if (changed & state & BUTTON_DOWN) // If Down is newly pressed
        maskJitterY = AdjustJitterValue(maskJitterY, -1, MASK_MIN_JITTER, MASK_MAX_JITTER, &dotPatternMaskObject.offset.y);
    
    // Handle mask movement speed adjustment using A and B buttons
    if (changed & state & BUTTON_A) // If A is newly pressed (decrease speed)
    {
        maskMovingSpeed -= MASK_SPEED_STEP;
        if (maskMovingSpeed < MASK_MIN_SPEED) // Clamp to minimum speed
            maskMovingSpeed = MASK_MIN_SPEED;
    }
    else if (changed & state & BUTTON_B) // If B is newly pressed (increase speed)
    {
        maskMovingSpeed += MASK_SPEED_STEP;
        if (maskMovingSpeed > MASK_MAX_SPEED) // Clamp to maximum speed
            maskMovingSpeed = MASK_MAX_SPEED;
    }
    
    // Toggle mask direction mode (horizontal/vertical) using C button
    if (changed & state & BUTTON_C)
        Mask_SwitchMode();
    
    // Toggle mask sprite's VDP priority using Start button (for demonstration)
    if (changed & state & BUTTON_START)
    {
        isMaskHighPriority = !isMaskHighPriority;
        // Apply the new priority to both potential mask sprites
        SPR_setPriority(horizontalMaskSprite, isMaskHighPriority);
        SPR_setPriority(verticalMaskSprite, isMaskHighPriority);
    }
}

/**
 * @brief Draws the User Interface text on the screen.
 * Displays current mask parameters and controls.
 */
void UI_Draw()
{
    // Buffer for UI strings
    char str[UI_MAX_LINES][UI_MAX_LINE_LENGTH];
    u16 y = 0;
    // Format UI strings with current game state information
    sprintf(str[y++], "_______________ USAGE ________________");
    sprintf(str[y++], "    C:Change Direction");
    sprintf(str[y++], "  A/B:Speed -/+    ");
    sprintf(str[y++], "D-Pad:Jitter X/Y -/+");
    sprintf(str[y++], "Start:Toggle Mask Priority");
    sprintf(str[y++], "________________ Mask ________________");
    sprintf(str[y++], "Direction: %s", (isHorizontalMode) ? "HORIZONTAL " : "VERTICAL   ");
    // Display speed with one decimal place
    sprintf(str[y++], "    Speed: %01d.%01d   pix/frame", FF32_toInt(maskMovingSpeed),
            (u16) (mulu(FF32_frac(maskMovingSpeed), 10) >> FASTFIX32_FRAC_BITS));
    sprintf(str[y++], "   Jitter: X:%1d   Y:%1d  ", maskJitterX, maskJitterY);
    sprintf(str[y++], "      Pos: X:%03d Y:%03d  ", SPR_getPositionX(dotPatternMaskObject.sprite),
            SPR_getPositionY(dotPatternMaskObject.sprite));
    
    
    // Draw all formatted UI strings to background layer A
    for (u16 i = 0; i < UI_MAX_LINES; i++)
        VDP_drawTextBG(BG_A, str[i], UI_TEXT_START_COL, i);
}
