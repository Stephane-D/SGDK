
// *****************************************************************************
//  Screen Wobble SGDK Sample
//
//    Helper functions
//
//  Written in 2020 by Andreas Dietrich
// *****************************************************************************

#include "helpers.h"

// -----------------------------------------------------------------------------
//  Palette functions
// -----------------------------------------------------------------------------

void fadeInBlue(u16 fromcol, u16 tocol, const u16 *pal, u16 numframeBlue, u16 numframeGreenRed)
{
    u16 palBlue[64];

    // Mask out non-blue part
    for (u16 i=0; i<(tocol-fromcol)+1; i++)
        palBlue[i] = pal[i] & 0xF00;

    // First fade in blue part, then green and red
    VDP_fadeIn(fromcol, tocol, palBlue, numframeBlue, FALSE);
    VDP_fadeTo(fromcol, tocol, pal, numframeGreenRed, FALSE);
}

// -----------------------------------------------------------------------------
//  Sprite functions
// -----------------------------------------------------------------------------

static u16        vdpSpriteIndexNext     = 0;
static u16        vdpSpriteTileIndexNext = 96;
static Vect2D_s16 spriteOffsets[80];

// -----------------------------------------------------------------------------

 u16 loadSpriteData(const SpriteDefinition* spriteDef)
 {
    const AnimationFrame* animationFrame = spriteDef->animations[0]->frames[0];

    const u32* tiles   = animationFrame->tileset->tiles;
    const u16  numTile = animationFrame->tileset->numTile;

    // Upload sprite tile data
    VDP_loadTileData(tiles, vdpSpriteTileIndexNext, numTile, DMA);

    // Determine tile index for next sprite
    u16 result = vdpSpriteTileIndexNext;
    vdpSpriteTileIndexNext += numTile;
    return result;
 }

// -----------------------------------------------------------------------------

void addSprite(s16 x, s16 y, u16 attribute)
{
    Vect2D_s16 offset = { x, y };
    spriteOffsets[vdpSpriteIndexNext] = offset;

    // Generate sprite list entry
    VDP_setSpriteFull( vdpSpriteIndexNext,
                       320, 0,
                       SPRITE_SIZE(4, 4),
                       attribute,
                       0 );

    // Link to previous sprite
    if (vdpSpriteIndexNext > 0)
        VDP_setSpriteLink(vdpSpriteIndexNext-1, vdpSpriteIndexNext);

    vdpSpriteIndexNext++;
}

// -----------------------------------------------------------------------------

void moveSprites(s16 x, s16 y)
{
    // Move compound sprite to (x,y)
    for (u16 i=0; i<vdpSpriteIndexNext; i++)
        VDP_setSpritePosition( i,
                               min(320, max(-32, spriteOffsets[i].x + x)),
                               min(224, max(-32, spriteOffsets[i].y + y)) );

    VDP_updateSprites(vdpSpriteIndexNext, DMA);
}

// -----------------------------------------------------------------------------
//  Menu functions
// -----------------------------------------------------------------------------

static u16       menuItemsNum    = 0;
static u16       menuLine        = 0;
static u16       menuLineOld     = 0;
static u16       menuButtonDelay = 0;
static u16       menuJoyStateOld = 0;
static char*     menuPointer     = NULL;
static MenuItem* menuItems       = NULL;
static char      menuLineBuffer[40];

// -----------------------------------------------------------------------------

void initMenu(MenuItem* items, u16 num)
{
    initMenuEx(items, num, "\x7F");
}

// -----------------------------------------------------------------------------

void initMenuEx(MenuItem* items, u16 num, char* pointer)
{
 
    menuItems    = items;
    menuItemsNum = num;
    menuPointer  = pointer;
    menuLine     = 0;
    menuLineOld  = 0;

    resetMenu();
}

// -----------------------------------------------------------------------------

void resetMenu()
{
    for (u16 i=0; i<menuItemsNum; i++)
    {
        // Generate menu text and draw
        MenuItem* item = menuItems + i;
        sprintf(menuLineBuffer, item->description, *item->vs16.value);
        VDP_drawText(menuLineBuffer, item->x, item->y);
    }
}

// -----------------------------------------------------------------------------

void updateMenuVDP()
{
    // Clear pointer tile
    MenuItem* item = menuItems + menuLineOld;
    VDP_drawText(" ", (item->x)-2, item->y);

    // Update current line tiles
    item = menuItems + menuLine;
    VDP_drawText(menuPointer, (item->x)-2, item->y);
    VDP_drawText(menuLineBuffer, item->x, item->y);
}

// -----------------------------------------------------------------------------

void updateMenuItems()
{
    MenuItem* item = menuItems + menuLine;
    menuLineOld = menuLine;

    // Manipulate values or move pointer
    const u16 joyState = JOY_readJoypad(JOY_1);
    if ((menuJoyStateOld != 0) && (menuButtonDelay > 0))
    {
        menuButtonDelay--;
    }
    else
    {
        if (joyState & BUTTON_A)
        {
            if (item->type == MIT_BOOL)
                *item->vbool.value = TRUE;
        }
        else if (joyState & BUTTON_LEFT)
        {
            if (item->type == MIT_S16)
                *item->vs16.value = max(*item->vs16.value - item->vs16.step, item->vs16.min);
        }
        else if (joyState & BUTTON_RIGHT)
        {
            if (item->type == MIT_S16)
                *item->vs16.value = min(*item->vs16.value + item->vs16.step, item->vs16.max);
        }
        else if (joyState & BUTTON_UP)
        {
            if (menuLine > 0)
                menuLine--;
        }
        else if (joyState & BUTTON_DOWN)
        {
            if (menuLine < menuItemsNum-1)
                menuLine++;
        }
        else
        {
            if (item->type == MIT_BOOL)
                *item->vbool.value = FALSE;
        }
        
        menuButtonDelay = (menuJoyStateOld == 0) ? 20 : 5;
    }
    menuJoyStateOld = joyState;

    // Generate updated text
    item = menuItems + menuLine;
    sprintf(menuLineBuffer, item->description, *item->vs16.value);
}
