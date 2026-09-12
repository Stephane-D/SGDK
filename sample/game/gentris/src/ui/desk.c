#include <genesis.h>
#include "desk.h"
#include "window.h"
#include "defs.h"


// Release sprite resources used by desk (mask sprites)
void Desk_FreeSprites(Desk* desk)
{
    if (desk->maskSprite)
        SPR_releaseSprite(desk->maskSprite);
    if (desk->maskSpriteHelper)
        SPR_releaseSprite(desk->maskSpriteHelper);
    desk->maskSprite = NULL;
    desk->maskSpriteHelper = NULL;
}

// Redraw desk background - fill with border tiles and title text
void Desk_Redraw(Desk* desk)
{
    s16 y = desk->y + desk->offsetY / 8;
    s16 h = desk->height;

    u8 layer = desk->bg;
    u8 pal = desk->pal;
    bool prio = desk->prio;

    VDP_fillTileMapRect(layer, TILE_ATTR_FULL(PAL3, prio, false, false, TILE_USER + 22), 0, desk->y, SCREEN_TILES_WIDTH, h);
    VDP_fillTileMapRect(BG, TILE_ATTR_FULL(PAL3, prio, false, false, TILE_USER + 22), 0, desk->y, SCREEN_TILES_WIDTH, h);

    VDP_fillTileMapRect(layer, TILE_ATTR_FULL(pal, prio, false, false, BORDER_THINK_TOP), 0, y, SCREEN_TILES_WIDTH, 1);
    VDP_fillTileMapRect(layer, TILE_ATTR_FULL(pal, prio, true, false, BORDER_THINK_TOP), 0, y + h - 1, SCREEN_TILES_WIDTH, 1);

    u8 titleLength = strnlen(desk->titleText, SCREEN_TILES_WIDTH);

    VDP_drawTextBG(desk->bg, desk->titleText, VDP_getScreenWidth() / (2 * 8) - titleLength / 2 - 1, y);
}

// Set vertical position of desk and redraw if visible
void Desk_SetTilePosY(Desk* desk, s16 posY)
{
    desk->y = posY;

    if (desk->state == UI_SHOWED)
        Desk_Redraw(desk);
}

// Hide desk - disable window, call hide callback, and free sprite resources
void Desk_Hide(Desk* desk)
{
    if (desk->state == UI_DISABLED)
        return;

    desk->state = UI_HIDDEN;

    Window_SetHidden();

    if (NULL != desk->OnHideCallback)
        desk->OnHideCallback();

    Desk_FreeSprites(desk);

    SPR_update();
}

// Show desk - call show callback, setup text properties, redraw, show sprites, enable window
void Desk_Show(Desk* desk)
{
    if (desk->state == UI_DISABLED)
        return;

    if (NULL != desk->OnShowCallback)
        desk->OnShowCallback();

    desk->state = UI_SHOWED;

    VDP_setTextPalette(desk->pal);
    VDP_setTextPlane(desk->bg);
    VDP_setTextPriority(desk->prio);

    Desk_Redraw(desk);

    SPR_setVisibility(desk->maskSprite, VISIBLE);
    SPR_setVisibility(desk->maskSpriteHelper, VISIBLE);

    Window_SetCentered(0, desk->y, 16, desk->height);

    SYS_doVBlankProcess();
}

// Handle input events - hide desk on B button press when waiting for input
void Desk_OnInputCallback(Desk* desk, u16 joypad, u16 changed, u16 state)
{
    switch (desk->state)
    {
        case UI_SHOWED:
            if (changed & state & BUTTON_B && desk->waitInput)
                Desk_Hide(desk);
            break;

        case UI_HIDDEN:
        case UI_DISABLED:
            break;
    }
}
