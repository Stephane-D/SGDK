// *****************************************************************************
//  Sprite masking example
//
//  Sprite masking is a technique that allows you to mask (hide) parts of selected sprites
//  that intersect with a horizontal rectangle/line of a given height and width in the screen width.
//
//  To do this, a masking sprite is used (of the required height and minimum width of 8 pixels),
//  which is placed outside the visible part of the screen. The parts of the selected Sprites (how to see rule #4)
//  that are in the visible part of the screen on the same horizontal line with the masking sprite and
//  have a depth greater than the masking sprite will be hidden.
//
//  THE NEXT RULES MUST be FOLLOWED:
//  1. masking sprite must have X POSITION == -128
//  2. there must be at least 1 sprite (helper sprite) at the same Y POSITION as the masking sprite, and X POSITION > X POSITION than masking sprite
//  3. this helper sprite must have SPRITE DEPTH < SPRITE DEPTH of masking sprite
//  4. masking sprite must have SPRITE DEPTH > SPRITE DEPTH of sprites that must be masked (otherwise, these sprites will not be masked)
//
//  writen by werton playskin 03/2025
// *****************************************************************************

#include <genesis.h>
#include "res/resources.h"

// number of sprites in row and column
#define SONIC_SPRITES_ROW_COUNT         5
#define SONIC_SPRITES_COLUMN_COUNT      3

// define Sprite pointers for masking
Sprite *spriteMaskHelper;
Sprite *spriteMask;

// vars for masking sprite Y position and movement offset
s16 maskSpritesPosY = 0;
s16 maskSpritesMovementOffsetY = 2;


int main(bool hardReset)
{
    // screen initialization
    VDP_setScreenWidth320();
    
    // set all palette to black
    PAL_setColors(0, (u16 *) palette_black, 64, CPU);
    
    // load palette
    PAL_setPalette(PAL0, bgImage.palette->data, CPU);
    PAL_setPalette(PAL1, sonicSpriteDef.palette->data, CPU);
    
    // init sprite engine with default parameters
    SPR_init();
    
    // load and draw BG Image
    VDP_drawImage(BG_B, &bgImage, 0, 0);
    
    // create 3 * 8 Sonic sprites and placed them in middle of screen
    for (u16 column = 0; column < SONIC_SPRITES_COLUMN_COUNT; column++) {
        for (u16 row = 0; row < SONIC_SPRITES_ROW_COUNT; row++) {
            Sprite *sprite = SPR_addSprite(&sonicSpriteDef, (column + 1) * 70, row * 40 + 8, TILE_ATTR(PAL1, TRUE, FALSE, FALSE));
            
            // set the sprite depth of sprites in column 1 (exclude first and last row) to minimal depth
            // (these sprites will not be masked according to rule 4 described below)
            if (column == 1 && row > 0 && row < SONIC_SPRITES_ROW_COUNT - 1)
                SPR_setDepth(sprite, SPR_MIN_DEPTH);
        }
    }
    
    // create mask sprite and helper mask sprite according to the rules described above (repeat for clarity)
    // --------------------------------------------------------------------------------------------------------------------
    // SPRITE MASKING RULES:
    // 1. spriteMask must have X POSITION == -128
    // 2. there must be at least 1 sprite (let it be spriteMaskHelper) at the same Y POSITION as the spriteMask, and X POSITION > X POSITION than spriteMask
    // 3. spriteMaskHelper must have SPRITE DEPTH < SPRITE DEPTH of spriteMask
    // 4. spriteMask must have SPRITE DEPTH > SPRITE DEPTH of sprites that must be masked (otherwise, these sprites will not be masked)
    // --------------------------------------------------------------------------------------------------------------------
    spriteMask = SPR_addSprite(&spriteMaskDef, -128, 100, TILE_ATTR(PAL1, TRUE, FALSE, FALSE));
    spriteMaskHelper = SPR_addSprite(&spriteMaskDef, -127, 100, TILE_ATTR(PAL1, TRUE, FALSE, FALSE));
    
    SPR_setDepth(spriteMask, SPR_MIN_DEPTH + 1);
    SPR_setDepth(spriteMaskHelper, SPR_MIN_DEPTH);
    
    // main loop
    while (TRUE) {
        
        // changing maskSpritesPosY from the top of the screen to the bottom and back
        maskSpritesPosY += maskSpritesMovementOffsetY;
        
        if (maskSpritesPosY > VDP_getScreenHeight() || maskSpritesPosY < -spriteMaskDef.h) {
            maskSpritesMovementOffsetY = -maskSpritesMovementOffsetY;
        }
        
        // moving the spriteMask && spriteMaskHelper along the Y axis simultaneously
        SPR_setPosition(spriteMaskHelper, -127, maskSpritesPosY);
        SPR_setPosition(spriteMask, -128, maskSpritesPosY);
        
        
        SPR_update();
        SYS_doVBlankProcess();
    }
    
    return 0;
}
