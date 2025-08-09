// *****************************************************************************
//  TMX-Objects Sample
//
//  This sample shows how you can load object data from TMX files (Tiled map format).
//  It shows loading of built-in 'name', 'x', 'y', and various custom fields using Tiled
//  types 'int', 'float', 'bool', 'string', 'object', 'Enum', placing objects of same and
//  different types on different and identical layers and filtering of objects at loading.
//
//  Use DPAD to toggle the active object and the description of its statistics
//
//  written by werton playskin on 05/2025
// *****************************************************************************

#include <genesis.h>
#include "sprites.h"

// Enumeration and type declaration

// Sprite definition enumerations
typedef enum
{
    DEF_SONIC,
    DEF_BUZZ,
    DEF_CRAB,
    DEF_BOT,
    DEF_DISPLAY,
    DEF_COUNT
} SpriteDefEnum;

// Object type enumerations
typedef enum
{
    TYPE_PLAYER,
    TYPE_ENEMY,
    TYPE_ITEM,
    TYPE_COUNT,
} ObjectType;

// ---------------------- TMX Data Types used for import data from .tmx file and stored in rom --------------------------
// Base object tmx data properties used for inheritance
typedef struct
{
    char *name;                 // Display name
    f32 x;                      // X position (fixed point)
    f32 y;                      // Y position (fixed point)
    ObjectType type;            // Object type index
    SpriteDefEnum sprDefInd;    // Sprite definition index
    u8 pal;                     // Palette index
    bool flipH;                 // Horizontal flip flag
    bool flipV;                 // Horizontal flip flag
    bool priority;              // priority flag
    bool enabled;               // state flag (not used just for example)
} TMX_BaseObjectData;

// Extended object data for items
typedef struct
{
    TMX_BaseObjectData;         // Base object properties
    s16 hp;                     // Hit points
} TMX_ItemData;

// Extended object data for actors
typedef struct
{
    TMX_BaseObjectData;         // Base object properties
    char *phrase;               // Dialogue text
    f32 speed;                  // Movement speed
    s16 hp;                     // Hit points
    void *target;               // Target reference
} TMX_ActorData;

// ----------------------------- Game data types -------------------------------------------
// Game item object with sprite and tmx item data
typedef struct
{
    union
    {
        TMX_ItemData data;      // Named access
        TMX_ItemData;           // Anonymous access
    };
    Sprite *sprite;             // Sprite reference
} GameItem;

// Game actor object with sprite and tmx actor data
typedef struct
{
    union
    {
        TMX_ActorData data;     // Named access
        TMX_ActorData;          // Anonymous access
    };
    Sprite *sprite;             // Sprite reference
} GameActor;
// ------------------------------------------------------------------------------------------

#include "objects.h"

// Game constants
#define PLAYER_COUNT            (sizeof(playersData)/sizeof(playersData[0]))
#define ENEMY_COUNT             (sizeof(enemiesData)/sizeof(enemiesData[0]))
#define ITEM_COUNT              (sizeof(itemsData)/sizeof(itemsData[0]))
#define OBJECTS_COUNT           (ENEMY_COUNT+ITEM_COUNT+PLAYER_COUNT)
#define STAT_LINES              20
#define TEXT_BUFFER             40

// Stringification macro
#define STR(x) #x

// Game state variables
static GameActor players[PLAYER_COUNT];
static GameActor enemies[ENEMY_COUNT];
static GameItem items[ITEM_COUNT];
static u16 selectedObjectIndex = 0;
TMX_BaseObjectData *objectsList[OBJECTS_COUNT];

// Sprite definition names
const char sprDefNames[DEF_COUNT][20] = {
    [DEF_SONIC] = STR(DEF_SONIC),
    [DEF_BUZZ] = STR(DEF_BUZZ),
    [DEF_CRAB] = STR(DEF_CRAB),
    [DEF_BOT] = STR(DEF_BOT),
    [DEF_DISPLAY] = STR(DEF_DISPLAY),
};

// Object type names
const char objectTypeNames[TYPE_COUNT][20] = {
    [TYPE_PLAYER] = STR(TYPE_PLAYER),
    [TYPE_ENEMY] = STR(TYPE_ENEMY),
    [TYPE_ITEM] = STR(TYPE_ITEM),
};

// Sprite definitions
const SpriteDefinition *spriteDefs[OBJECTS_COUNT] = {
    [DEF_SONIC] = &sprDefSonic,
    [DEF_BUZZ] = &sprDefBuzz,
    [DEF_CRAB] = &sprDefCrab,
    [DEF_BOT] = &sprDefBot,
    [DEF_DISPLAY] = &sprDefDisplay,
};

// Forward declarations
static void Game_Init();
static void Joy_Handler(u16 joy, u16 changed, u16 state);
static void GameItem_Init(GameItem *obj, const TMX_ItemData *data, const SpriteDefinition *sprDef);
static void GameActor_Init(GameActor *obj, const TMX_ActorData *data, const SpriteDefinition *sprDef);
static void UI_DrawCursor(const char *symbol1, const char *symbol2);
static u16 UI_DrawBaseObjectData(const TMX_BaseObjectData *object);
static void UI_DrawActorData(const TMX_ActorData *actor);
static void UI_DrawItemData(const TMX_ItemData *item);
static void UI_DrawData(const TMX_BaseObjectData *object);

// Entry point
int main(bool hardReset)
{
    if (!hardReset)
        SYS_hardReset();
    
    Game_Init();
    
    while (TRUE)
    {
        SPR_update();
        SYS_doVBlankProcess();
    }
    
    return 0;
}

// Initialize game state
static void Game_Init()
{
    SPR_init();
    
    // Initialize enemies
    for (u16 i = 0; i < ENEMY_COUNT; i++)
    {
        GameActor_Init((GameActor *)&enemies[i], enemiesData[i], spriteDefs[enemiesData[i]->sprDefInd]);
        // Add enemies to a global object list
        objectsList[i] = (TMX_BaseObjectData *)&enemies[i];
    }
    
    // Initialize items
    for (u16 i = 0; i < ITEM_COUNT; i++)
    {
        GameItem_Init((GameItem *)&items[i], itemsData[i], spriteDefs[itemsData[i]->sprDefInd]);
        // Add items to a global object list
        objectsList[ENEMY_COUNT + i] = (TMX_BaseObjectData *)&items[i];
    }
    
    // Initialize players
    for (u16 i = 0; i < PLAYER_COUNT; i++)
    {
        GameActor_Init((GameActor *)&players[i], playersData[i], spriteDefs[playersData[i]->sprDefInd]);
        // Add players to a global object list
        objectsList[ENEMY_COUNT + ITEM_COUNT + i] = (TMX_BaseObjectData *)&players[i];
    }
    
    // Set Sonic animation to 1
    SPR_setAnim(players[0].sprite, 1);
    
    // Set up input handler
    JOY_setEventHandler(Joy_Handler);
    
    // Setup UI
    VDP_setTextPalette(PAL2);
    VDP_drawText("USE DPAD TO SWITCH CHARACTER", 6, 0);
    UI_DrawData((const TMX_BaseObjectData *)objectsList[selectedObjectIndex]);
    UI_DrawCursor(">>", "<<");
}

// Initialize a game item object
static void GameItem_Init(GameItem *obj, const TMX_ItemData *data, const SpriteDefinition *sprDef)
{
    obj->data = *data;
    
    PAL_setPalette(obj->data.pal, sprDef->palette->data, DMA);
    obj->sprite = SPR_addSprite(sprDef,
                                F32_toInt(obj->data.x),
                                F32_toInt(obj->data.y),
                                TILE_ATTR(obj->data.pal, obj->data.priority, obj->data.flipV, obj->data.flipH));
}

// Initialize a game actor object
static void GameActor_Init(GameActor *obj, const TMX_ActorData *data, const SpriteDefinition *sprDef)
{
    obj->data = *data;
    
    PAL_setPalette(obj->data.pal, sprDef->palette->data, DMA);
    obj->sprite = SPR_addSprite(sprDef,
                                F32_toInt(obj->data.x),
                                F32_toInt(obj->data.y),
                                TILE_ATTR(obj->data.pal, obj->data.priority, obj->data.flipV, obj->data.flipH));
}

// Handle joypad input events
static void Joy_Handler(u16 joy, u16 changed, u16 state)
{
    // Clear current cursor
    UI_DrawCursor("  ", "  ");
    
    // Handle direction input
    if (changed & state & BUTTON_LEFT)
        selectedObjectIndex = (selectedObjectIndex == OBJECTS_COUNT - 1) ? 0 : selectedObjectIndex + 1;
    else if (changed & state & BUTTON_RIGHT)
        selectedObjectIndex = (selectedObjectIndex == 0) ? OBJECTS_COUNT - 1 : (u16) (selectedObjectIndex - 1);
    
    // Update displayed data
    UI_DrawData((const TMX_BaseObjectData *)objectsList[selectedObjectIndex]);
    
    // Draw new cursor
    UI_DrawCursor(">>", "<<");
}

// Draw an array of strings on screen
void UI_DrawStringsArray(const char *text, u16 fromY, u16 length)
{
    VDP_setTextPalette(PAL1);
    for (u16 i = 0; i < length; i++)
        VDP_drawTextBG(BG_A, text + i * TEXT_BUFFER, 0, 2 + fromY + i);
}

// Draw base object statistics
static u16 UI_DrawBaseObjectData(const TMX_BaseObjectData *object)
{
    static char text[STAT_LINES][TEXT_BUFFER];
    u16 y = 0;
    
    sprintf(text[y++], " _______ TMX DATA _______");
    sprintf(text[y++], " Name:     %-11s", object->name);
    sprintf(text[y++], " Type:     %-15s", objectTypeNames[object->type]);
    sprintf(text[y++], " Enabled:  %-5s", object->enabled ? "TRUE" : "FALSE");
    sprintf(text[y++], " Pos:      X:%03ld, Y:%03ld", F32_toInt(object->x), F32_toInt(object->y));
    sprintf(text[y++], " SprDefInd:%-15s", sprDefNames[object->sprDefInd]);
    sprintf(text[y++], " PalInd:   %d", object->pal);
    sprintf(text[y++], " Prio:     %-5s", object->priority ? "TRUE" : "FALSE");
    sprintf(text[y++], " FlipH:    %-5s", object->flipH ? "TRUE" : "FALSE");
    sprintf(text[y++], " FlipV:    %-5s", object->flipV ? "TRUE" : "FALSE");
    
    UI_DrawStringsArray((const char *)text, 0, y);
    return y;
}

// Draw actor-specific statistics
static void UI_DrawActorData(const TMX_ActorData *actor)
{
    static char text[STAT_LINES][TEXT_BUFFER];
    u16 y = UI_DrawBaseObjectData((const TMX_BaseObjectData *)actor);
    
    sprintf(text[y++], " Target:   %s(ptr:%p) %-10s",
            (actor->target != NULL) ? ((TMX_ActorData *)actor->target)->name : "NONE",
            actor->target, "");
    sprintf(text[y++], " Speed:    %02ld.%d %-25s",
            F32_toInt(actor->speed),
            (u16)(mulu(F32_frac(actor->speed), 100) >> FIX32_FRAC_BITS), "");
    sprintf(text[y++], " HP:       %-29d", actor->hp);
    sprintf(text[y++], " Phrase:   %-70s", actor->phrase);
    y++;
    sprintf(text[y++], " ______________________________________");
    
    UI_DrawStringsArray((const char *)text, 0, y);
}

// Draw item-specific statistics
static void UI_DrawItemData(const TMX_ItemData *item)
{
    static char text[STAT_LINES][TEXT_BUFFER];
    u16 y = UI_DrawBaseObjectData((const TMX_BaseObjectData *)item);
    
    sprintf(text[y++], " HP:       %-29d", item->hp);
    sprintf(text[y++], " ______________________________________");
    sprintf(text[y++], "%-40s", "");
    sprintf(text[y++], "%-40s", "");
    sprintf(text[y++], "%-40s", "");
    sprintf(text[y++], "%-40s", "");
    
    UI_DrawStringsArray((const char *)text, 0, y);
}

// Draw appropriate data based on an object type
static void UI_DrawData(const TMX_BaseObjectData *object)
{
    switch (object->type)
    {
        case TYPE_PLAYER:
        case TYPE_ENEMY:
            UI_DrawActorData((const TMX_ActorData *)object);
            break;
        
        case TYPE_ITEM:
            UI_DrawItemData((const TMX_ItemData *)object);
            break;
            
        default:
            break;
    }
}

// Draw selection cursor around a selected object
static void UI_DrawCursor(const char *symbol1, const char *symbol2)
{
    const TMX_BaseObjectData *object = objectsList[selectedObjectIndex];
    const SpriteDefinition *sprDef = spriteDefs[object->sprDefInd];
    
    s16 x1 = (F32_toInt(object->x) >> 3) - (sprDef->w >> 4);
    s16 x2 = (F32_toInt(object->x) >> 3) + (sprDef->w >> 3) + (sprDef->w >> 4) - 1;
    s16 y = (F32_toInt(object->y) >> 3) + (sprDef->h >> 4);
    
    VDP_setTextPalette(PAL3);
    VDP_drawTextBG(BG_B, symbol1, x1, y);
    VDP_drawTextBG(BG_B, symbol2, x2, y);
}