#ifndef HEADER_UI_DESK
#define HEADER_UI_DESK

#include <types.h>
#include <sprite_eng.h>

typedef void (* Callback)(void);

typedef enum UIState
{
    UI_HIDDEN,
    UI_SHOWED,
    UI_DISABLED,
} UIState;

typedef struct
{
    Sprite* maskSprite;
    Sprite* maskSpriteHelper;
    char* titleText;
    Callback OnHideCallback;
    Callback OnShowCallback;
    UIState state;
    s16 offsetX;
    s16 offsetY;
    s16 x;
    s16 y;
    u16 width;
    u16 height;
    u8 bg;
    u8 pal;
    bool prio;
    bool waitInput;
} Desk;

void Desk_OnInputCallback(Desk* desk, u16 joypad, u16 changed, u16 state);
void Desk_FreeSprites(Desk* desk);
void Desk_Redraw(Desk* desk);
void Desk_Hide(Desk* desk);
void Desk_Show(Desk* desk);
void Desk_SetTilePosY(Desk* desk, s16 posY);

#endif // HEADER_UI_DESK

