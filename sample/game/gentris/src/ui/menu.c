#include "menu.h"
#include "desk.h"

static void updateActiveItem(MenuPage* menu);
inline static MenuItem* getMenuItem(u8 itemIndex);
inline static void drawMenuItemText(MenuPage* menu, u8 itemIndex, char* text);


static u8 menuItemsCount = 0;
static u8 menuActiveLine = 0;
static u8 menuLineOld = 0;
static char* menuPointer = NULL;
static MenuItem* menuItems = NULL;
static char menuLineBuffer[30];


// Count number of active menu items in the menu array
u16 Menu_GetItemsCount(MenuPage* menu)
{
    u8 arrayLength = sizeof(menu->menuItems) / sizeof(menu->menuItems[0]);
    u8 itemCount = 0;
    
    for (u8 i = 0; i < arrayLength; ++i)
    {
        if (menu->menuItems[i].type != NULL)
            itemCount++;
    }
    
    return itemCount;
}

// Redraw entire menu including desk background and all menu items
void Menu_Redraw(MenuPage* menu)
{
    Desk_Redraw(&menu->desk);
    Menu_RedrawItems(menu, menu->menuItems, Menu_GetItemsCount(menu));
}

// Initialize menu - redraw if already shown
void Menu_Create(MenuPage* menu)
{
    if (menu->state == UI_SHOWED)
        Menu_Redraw(menu);
}

// Handle input events - show menu or update items based on current state
void Menu_OnInputCallback(MenuPage* menu, u16 joypad, u16 changed, u16 state)
{
    if (menu->state == UI_HIDDEN)
    {
        Menu_Show(menu);
    }
    else if (menu->state == UI_SHOWED)
    {
        Menu_UpdateItems(menu, joypad, changed, state);
    }
}

// GameplayState_OnUpdate menu state (empty stub for state management)
void Menu_UpdateState(MenuPage* menu)
{
    if (menu->state == UI_HIDDEN)
        return;
}

// GameplayState_OnUpdate menu display
void Menu_UpdateDisplay(MenuPage* menu)
{
    if (menu->state == UI_HIDDEN)
        return;
    
    updateActiveItem(menu);
}

// Reset menu selection state to first item
void ResetState(MenuPage* menu)
{
    menuActiveLine = 0;
    menuLineOld = 0;
}

// Hide menu, clear input handler, and call hide callback
void Menu_Hide(MenuPage* menu)
{
    Desk_Hide(&menu->desk);
    
    menu->state = UI_HIDDEN;
    
    JOY_setEventHandler(NULL);
    
    if (NULL != menu->OnHideCallback)
        menu->OnHideCallback();
}

// Display menu on screen - call show callback, reset state, redraw, and set input handler
void Menu_Show(MenuPage* menu)
{
    if (NULL != menu->OnShowCallback)
        menu->OnShowCallback();
    
    ResetState(menu);
    
    Desk_Show(&menu->desk);
    
    menu->state = UI_SHOWED;
    Menu_Redraw(menu);
    
    VDP_setTextPlane(menu->desk.bg);
    
    if (menu->OnInputCallback)
        JOY_setEventHandler(menu->OnInputCallback);
    
    SPR_update();
    SYS_doVBlankProcess();
}

bool Menu_IsShowed(MenuPage* menu)
{
    return menu->state == UI_SHOWED;
}

void Menu_RedrawItems(MenuPage* menu, MenuItem* items, u16 itemCount)
{
    Menu_InitEx(menu, items, itemCount, "\x7F");
}

void Menu_InitEx(MenuPage* menu, MenuItem* items, u16 itemCount, char* pointer)
{
    menuItems = items;
    menuItemsCount = itemCount;
    menuPointer = pointer;
    menuActiveLine = 0;
    menuLineOld = 0;
    resetMenu(menu);
}

inline static MenuItem* getMenuItem(u8 itemIndex)
{
    return menuItems + itemIndex;
}

inline static void drawMenuItemText(MenuPage* menu, u8 itemIndex, char* text)
{
    MenuItem* item = getMenuItem(itemIndex);

    s16 x = menu->desk.x + menu->desk.offsetX / 8;
    s16 y = menu->desk.y + menu->desk.offsetY / 8;

    VDP_drawTextBG(menu->desk.bg, text, x + item->x, y + item->y);
}

void Menu_SetPosition(MenuPage* menu, s16 x, s16 y)
{
    menu->desk.x = x;
    menu->desk.y = y;
}

void fillMenuItemTextBuffer(MenuPage* menu, u8 itemIndex)
{
    MenuItem* item = getMenuItem(itemIndex);
    
    switch (item->type)
    {
        case MIT_S16:
            sprintf(menuLineBuffer, item->description, *item->s16Value.value);
            break;
        
        case MIT_U8:
            sprintf(menuLineBuffer, item->description, *item->u8Value.value);
            break;
        
        case MIT_TEXT:
            sprintf(menuLineBuffer, item->description, item->textValue.valueOpt[*item->textValue.index]);
            break;
        
        case MIT_BOOL:
            if (item->boolValue.isShowed)
            {
                if (*item->boolValue.value)
                    sprintf(menuLineBuffer, item->description, "ON ");
                else
                    sprintf(menuLineBuffer, item->description, "OFF");
            }
            else
                sprintf(menuLineBuffer, item->description, "");
            
            break;
        
        case MIT_S16_OPTIONS:
            sprintf(menuLineBuffer, item->description, item->optionValue.valueOpt[*item->optionValue.index]);
            break;
        
        case MIT_S16_ARRAY:
            if (strnlen(item->arrayValue.optText[0], 2) == 0)
                sprintf(menuLineBuffer, item->description, item->arrayValue.valueOpt[item->arrayValue.index]);
            else
                sprintf(menuLineBuffer, item->description, item->arrayValue.optText[item->arrayValue.index]);
            
            break;
        
        case MIT_FUNC:
            break;
    }
}

void resetMenu(MenuPage* menu)
{
    for (u16 i = 0; i < menuItemsCount; i++)
    {
        // Generate inGameMenu textValue and draw
        fillMenuItemTextBuffer(menu, i);
        drawMenuItemText(menu, i, menuLineBuffer);
    }
    fillMenuItemTextBuffer(menu, menuActiveLine);
}

static void updateActiveItem(MenuPage* menu)
{
    s16 x = menu->desk.x + menu->desk.offsetX / 8;
    s16 y = menu->desk.y + menu->desk.offsetY / 8;
    
    // Clear pointer tile
    MenuItem* item = menuItems + menuLineOld;
    VDP_drawTextBG(menu->desk.bg, " ", (x + item->x) - 2, y + item->y);
    
    // Calculate current line item ptr
    item = menuItems + menuActiveLine;
    // Draw pointer tile
    VDP_drawTextBG(menu->desk.bg, menuPointer, (x + item->x) - 2, y + item->y);
    // Redraw current line text
    drawMenuItemText(menu, menuActiveLine, menuLineBuffer);
}

void Menu_UpdateItems(MenuPage* menu, u16 joypad, u16 changed, u16 state)
{
    MenuItem* item = menuItems + menuActiveLine;
    menuLineOld = menuActiveLine;
    
    u16 buttons = BUTTON_B;
    
    if (menu->startToConfirm)
        buttons += BUTTON_START;
    
    if (state & changed & buttons)
    {
        switch (item->type)
        {
            case MIT_S16:
                if (item->s16Value.Callback)
                {
                    Menu_Hide(menu);
                    item->s16Value.Callback(*item->s16Value.value);
                }
                break;
            
            case MIT_U8:
                if (item->u8Value.Callback)
                {
                    Menu_Hide(menu);
                    item->u8Value.Callback(*item->u8Value.value);
                }
                break;
            
            case MIT_BOOL:
                *item->boolValue.value = !(*item->boolValue.value);
                break;
            
            case MIT_TEXT:
                break;
            
            case MIT_S16_OPTIONS:
                break;
            
            case MIT_S16_ARRAY:
                if (item->arrayValue.Callback)
                {
                    Menu_Hide(menu);
                    item->arrayValue.Callback(*item->arrayValue.value);
                }
                break;
            
            case MIT_FUNC:
                break;
        }
    }
    else if (state & changed & BUTTON_LEFT)
    {
        switch (item->type)
        {
            case MIT_S16:
                *item->s16Value.value = max(*item->s16Value.value - item->s16Value.step, item->s16Value.min);
                break;
            
            case MIT_U8:
                *item->u8Value.value = max(*item->u8Value.value - item->u8Value.step, item->u8Value.min);
                break;
            
            case MIT_BOOL:
                *item->boolValue.value = !(*item->boolValue.value);
                break;
            
            case MIT_TEXT:
                *item->textValue.index = *item->textValue.index - 1;
                
                if (*item->textValue.index < 0)
                    *item->textValue.index = item->textValue.optCount - 1;
                
                break;
            
            case MIT_S16_OPTIONS:
                *item->optionValue.index = *item->optionValue.index - 1;
                
                if (*item->optionValue.index < 0)
                    *item->optionValue.index = item->optionValue.optCount - 1;
                
                break;
            
            case MIT_S16_ARRAY:
                item->arrayValue.index--;
                
                if (item->arrayValue.index < 0)
                    item->arrayValue.index = item->arrayValue.optCount - 1;
                
                *item->arrayValue.value = item->arrayValue.valueOpt[item->arrayValue.index];
                break;
            
            case MIT_FUNC:
                break;
        }
    }
    else if (state & changed & BUTTON_RIGHT)
    {
        switch (item->type)
        {
            case MIT_S16:
                *item->s16Value.value = min(*item->s16Value.value + item->s16Value.step, item->s16Value.max);
                break;
            
            case MIT_U8:
                *item->u8Value.value = min(*item->u8Value.value + item->u8Value.step, item->u8Value.max);
                break;
            
            case MIT_BOOL:
                *item->boolValue.value = !(*item->boolValue.value);
                break;
            
            case MIT_TEXT:
                *item->textValue.index = *item->textValue.index + 1;
                
                if (*item->textValue.index > item->textValue.optCount - 1)
                    *item->textValue.index = 0;
                
                break;
            
            case MIT_S16_OPTIONS:
                *item->optionValue.index = *item->optionValue.index + 1;
                
                if (*item->optionValue.index > item->optionValue.optCount - 1)
                    *item->optionValue.index = 0;
                
                break;
            
            case MIT_S16_ARRAY:
                item->arrayValue.index++;
                
                if (item->arrayValue.index > item->arrayValue.optCount - 1)
                    item->arrayValue.index = 0;
                
                *item->arrayValue.value = item->arrayValue.valueOpt[item->arrayValue.index];
                break;
            
            case MIT_FUNC:
                break;
        }
    }
    else if (state & changed & BUTTON_UP)
    {
        if (menuActiveLine > 0)
            menuActiveLine--;
        else if (menu->isCycledCursor)
            menuActiveLine = menuItemsCount - 1;
    }
    else if (state & changed & BUTTON_DOWN)
    {
        if (menuActiveLine < menuItemsCount - 1)
            menuActiveLine++;
        else if (menu->isCycledCursor)
            menuActiveLine = 0;
    }

    fillMenuItemTextBuffer(menu, menuActiveLine);
}

