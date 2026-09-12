#ifndef HEADER_UI_MENU
#define HEADER_UI_MENU

#include <genesis.h>
#include "desk.h"

#define MENU_MAX_ITEMS                      10
#define MENU_ITEM_MAX_OPTIONS               10
#define MENU_ITEM_TEXT_MAX_LENGTH           10

// Callback function pointer types for menu item actions
typedef void (* OneArgCallback)(s16 arg);

// Callback function pointer type for menu item actions with unsigned 8-bit argument
typedef void (* OneArgCallbackU8)(u8 arg);

// Callback function pointer type for menu input events
typedef void (* InputCallback)(u16 joy, u16 changed, u16 state);

// Menu item types enumeration
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

// Menu item structure for signed 16-bit integer values
typedef struct MenuItemS16
{
    s16* value;
    s16 min;
    s16 max;
    s16 step;
    OneArgCallback Callback;
} MenuItemS16;

// Menu item structure for unsigned 8-bit integer values
typedef struct MenuItemU8
{
    u8* value;
    u8 min;
    u8 max;
    u8 step;
    OneArgCallbackU8 Callback;
} MenuItemU8;

// Menu item structure for boolean values
typedef struct MenuItemBool
{
    bool* value;
    bool isShowed;
} MenuItemBool;

// Menu item structure for text options
typedef struct MenuItemText
{
    s16* index;
    char valueOpt[MENU_ITEM_MAX_OPTIONS][MENU_ITEM_TEXT_MAX_LENGTH];
    u16 optCount;
} MenuItemText;

// Menu item structure for signed 16-bit integer options
typedef struct
{
    s16* index;
    s16 valueOpt[MENU_ITEM_MAX_OPTIONS];
    u16 optCount;
} MenuItemS16Options;

// Menu item structure for signed 16-bit integer array options
typedef struct
{
    s16* value;
    s16 index;
    s16 valueOpt[MENU_ITEM_MAX_OPTIONS];
    char optText[MENU_ITEM_MAX_OPTIONS][MENU_ITEM_TEXT_MAX_LENGTH];
    u16 optCount;
    OneArgCallback Callback;
} MenuItemS16Array;

// Menu item structure representing a single menu entry
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

// Menu page structure representing a complete menu with items and callbacks
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

// Function declarations for menu operations
void Menu_RedrawItems(MenuPage* menu, MenuItem* items, u16 itemCount);

// Function to set the position of the menu on the screen
void Menu_SetPosition(MenuPage* menu, s16 x, s16 y);

// Function to initialize the menu with items and a pointer for selection
void Menu_InitEx(MenuPage* menu, MenuItem* items, u16 itemCount, char* pointer);

// Function to reset the menu items and redraw them
void resetMenu(MenuPage* menu);

// Function to update menu items based on user input
void Menu_UpdateItems(MenuPage* menu, u16 joypad, u16 changed, u16 state);

// Callback function for handling menu input events
void Menu_OnInputCallback(MenuPage* menu, u16 joypad, u16 changed, u16 state);

// Function to reset the menu state to its initial configuration
void Menu_UpdateState(MenuPage* menu);

// Function to update the menu display, including redrawing items and handling input
void Menu_UpdateDisplay(MenuPage* menu);

// Function to create and initialize a menu page with default settings
void Menu_Create(MenuPage* menu);

// Function to hide the menu and call the hide callback if defined
void Menu_Show(MenuPage* menu);

// Function to check if the menu is currently displayed on the screen
bool Menu_IsShowed(MenuPage* menu);

// Function to redraw the menu items and update the display
void Menu_Redraw(MenuPage* menu);

// Function to hide the menu, clear input handler, and call the hide callback
void Menu_Hide(MenuPage* menu);

#endif // HEADER_UI_MENU

