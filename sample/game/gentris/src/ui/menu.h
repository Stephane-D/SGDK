#ifndef HEADER_UI_MENU
#define HEADER_UI_MENU

#include <genesis.h>
#include "desk.h"

#define MENU_MAX_ITEMS                      10
#define MENU_ITEM_MAX_OPTIONS               10
#define MENU_ITEM_TEXT_MAX_LENGTH           10

typedef void (* OneArgCallback)(s16 arg);
typedef void (* OneArgCallbackU8)(u8 arg);
typedef void (* InputCallback)(u16 joy, u16 changed, u16 state);

typedef enum MenuItemType
{
    MIT_S16 = 1,
    MIT_U8,
    MIT_BOOL,
    MIT_TEXT,
    MIT_S16_OPTIONS,
    MIT_S16_ARRAY,
    MIT_FUNC,
} MenuItemType;

typedef struct MenuItemS16
{
    s16* value;
    s16 min;
    s16 max;
    s16 step;
    OneArgCallback Callback;
} MenuItemS16;

typedef struct MenuItemU8
{
    u8* value;
    u8 min;
    u8 max;
    u8 step;
    OneArgCallbackU8 Callback;
} MenuItemU8;

typedef struct MenuItemBool
{
    bool* value;
    bool isShowed;
} MenuItemBool;

typedef struct MenuItemText
{
    s16* index;
    char valueOpt[MENU_ITEM_MAX_OPTIONS][MENU_ITEM_TEXT_MAX_LENGTH];
    u16 optCount;
} MenuItemText;

typedef struct
{
    s16* index;
    s16 valueOpt[MENU_ITEM_MAX_OPTIONS];
    u16 optCount;
} MenuItemS16Options;

typedef struct
{
    s16* value;
    s16 index;
    s16 valueOpt[MENU_ITEM_MAX_OPTIONS];
    char optText[MENU_ITEM_MAX_OPTIONS][MENU_ITEM_TEXT_MAX_LENGTH];
    u16 optCount;
    OneArgCallback Callback;
} MenuItemS16Array;

typedef struct MenuItem
{
    u16 x;
    u16 y;
    
    char* description;
    
    union
    {
        MenuItemS16 s16Value;
        MenuItemU8 u8Value;
        MenuItemBool boolValue;
        MenuItemText textValue;
        MenuItemS16Options optionValue;
        MenuItemS16Array arrayValue;
    };
    
    MenuItemType type;
} MenuItem;

typedef struct
{
    Desk desk;
    UIState state;
    bool isCycledCursor;
    MenuItem menuItems[MENU_MAX_ITEMS];
    Callback OnHideCallback;
    Callback OnShowCallback;
    InputCallback OnInputCallback;
    bool startToConfirm;
} MenuPage;

void Menu_RedrawItems(MenuPage* menu, MenuItem* items, u16 itemCount);
void Menu_SetPosition(MenuPage* menu, s16 x, s16 y);
void Menu_InitEx(MenuPage* menu, MenuItem* items, u16 itemCount, char* pointer);
void resetMenu(MenuPage* menu);
void updateMenuVDP(MenuPage* menu);
void Menu_UpdateItems(MenuPage* menu, u16 joypad, u16 changed, u16 state);
void Menu_OnInputCallback(MenuPage* menu, u16 joypad, u16 changed, u16 state);
void Menu_UpdateState(MenuPage* menu);
void Menu_UpdateDisplay(MenuPage* menu);
void Menu_Create(MenuPage* menu);
void Menu_Show(MenuPage* menu);
bool Menu_IsShowed(MenuPage* menu);
void Menu_Redraw(MenuPage* menu);
void Menu_Hide(MenuPage* menu);

#endif // HEADER_UI_MENU

