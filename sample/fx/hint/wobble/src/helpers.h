
// *****************************************************************************
//  Screen Wobble SGDK Sample
//
//    Helper functions
//
//  Written in 2020 by Andreas Dietrich
// *****************************************************************************

// -----------------------------------------------------------------------------
//  Includes
// -----------------------------------------------------------------------------

// SGDK
#include <genesis.h>

// -----------------------------------------------------------------------------
//  Types
// -----------------------------------------------------------------------------

typedef enum MenuItemType
{
    MIT_S16,
    MIT_BOOL
}
MenuItemType;

typedef struct MenuItemS16
{
    s16* value;
    s16  min;
    s16  max;
    s16  step;
}
MenuItemS16;

typedef struct MenuItemBool
{
    bool* value;
}
MenuItemBool;

typedef struct MenuItem
{
    u16   x;
    u16   y;

    char* description;

    union
    {
        MenuItemS16  vs16;
        MenuItemBool vbool;
    };
    MenuItemType type;
}
MenuItem;

// -----------------------------------------------------------------------------
//  Functions
// -----------------------------------------------------------------------------

// Palettte
extern void fadeInBlue(u16 fromcol, u16 tocol, const u16 *pal, u16 numframeBlue, u16 numframeGreenRed);

// Sprites
extern u16  loadSpriteData(const SpriteDefinition* spriteDef);
extern void addSprite(s16 x, s16 y, u16 attribute);
extern void moveSprites(s16 x, s16 y);

// Menu
extern void initMenu(MenuItem* items, u16 num);
extern void initMenuEx(MenuItem* items, u16 num, char* pointer);
extern void resetMenu();
extern void updateMenuVDP();
extern void updateMenuItems();
