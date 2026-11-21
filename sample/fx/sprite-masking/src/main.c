// *****************************************************************************
//  Sprite Masking Sample
//
//  Sprite masking hides parts of selected sprites that intersect a horizontal
//  band (line) of a given height across the screen width.
//
//  It works by placing a masking sprite (height as needed, minimum width 8 px)
//  outside the visible screen area. Any selected sprite (see rule 4) that lies
//  on the same horizontal line as the masking sprite (and helper sprite) and
//  has greater depth will be hidden.
//
//  RULES:
//  1. Masking sprite X position must be -128.
//  2. At least one helper sprite must share the same Y position as the masking
//     sprite, but have a different X position.
//  3. The helper sprite must have lower depth than the masking sprite.
//  4. The masking sprite must have lower depth than the sprites to be masked
//     (otherwise they will not be masked).
//
//  Written by werton playskin, 03/2025
// *****************************************************************************

#include <genesis.h>
#include "res/resources.h"

// Sprite grid dimensions
#define SONIC_SPRITES_ROW_COUNT         5
#define SONIC_SPRITES_COLUMN_COUNT      3

// Sprite pointers for masking
Sprite *spriteMaskHelper;
Sprite *spriteMask;

// Mask Y position and movement offset
s16 maskSpritesPosY = 0;
s16 maskSpritesMovementOffsetY = 2;


int main(bool hardReset)
{
    // Initialize screen width (320 px)
    VDP_setScreenWidth320();
    
    // Clear all palettes to black
    PAL_setColors(0, (u16 *) palette_black, 64, CPU);
    
    // Load palettes
    PAL_setPalette(PAL0, bgImage.palette->data, CPU);
    PAL_setPalette(PAL1, sonicSpriteDef.palette->data, CPU);
    
    // Initialize sprite engine
    SPR_init();
    
    // Draw background image
    VDP_drawImage(BG_B, &bgImage, 0, 0);
    
    // Create a 3x5 grid of Sonic sprites
    for (u16 column = 0; column < SONIC_SPRITES_COLUMN_COUNT; column++)
    {
        for (u16 row = 0; row < SONIC_SPRITES_ROW_COUNT; row++)
        {
            Sprite *sprite = SPR_addSprite(&sonicSpriteDef, (column + 1) * 70, row * 40 + 8, TILE_ATTR(PAL1, true, false, false));
            

            if (column == 1 && row > 0 && row < SONIC_SPRITES_ROW_COUNT - 1)
            {            
                // Set depth of sprites in column 1 (excluding first and last row) to minimum depth.
                // These sprites will not be masked (see rule 4).                
                SPR_setDepth(sprite, SPR_MIN_DEPTH);
            }
            else
            {
                // Set depth of sprites in column 0 and 2 to middle depth.
                // These sprites will be masked (see rule 4).   
                SPR_setDepth(sprite, 0);
            }
        }
    }
    
    // Create mask and helper sprites following the rules above.
    // (See the top of the file for the full rules.)
    spriteMask = SPR_addSprite(&spriteMaskDef, -128, 100, TILE_ATTR(PAL1, true, false, false));
    spriteMaskHelper = SPR_addSprite(&spriteMaskDef, -127, 100, TILE_ATTR(PAL1, true, false, false));

    // Set the depth of the masking sprite greater than the depth of the helper sprite and less than the depth of the masked sprites
    SPR_setDepth(spriteMask, SPR_MIN_DEPTH + 1);
    SPR_setDepth(spriteMaskHelper, SPR_MIN_DEPTH);
    
    // Main loop
    while (true)
    {
        
        // Move mask Y position from top to bottom and back
        maskSpritesPosY += maskSpritesMovementOffsetY;
        
        if (maskSpritesPosY > VDP_getScreenHeight() || maskSpritesPosY < -spriteMaskDef.h)
        {
            maskSpritesMovementOffsetY = -maskSpritesMovementOffsetY;
        }
        
        // Move mask and helper sprites together along Y
        SPR_setPosition(spriteMaskHelper, -127, maskSpritesPosY);
        SPR_setPosition(spriteMask, -128, maskSpritesPosY);
        
        SPR_update();
        SYS_doVBlankProcess();
    }
    
    return 0;
}
