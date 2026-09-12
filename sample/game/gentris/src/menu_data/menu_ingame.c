#include <genesis.h>
#include "global.h"
#include "defs.h"
#include "types.h"
#include "game.h"
#include "menu_man.h"


MenuPage g_inGameMenu =
    {
        .desk = {
            .state = UI_HIDDEN,
            .offsetX = 0,
            .x = MENU_INGAME_X,
            .y = MENU_INGAME_Y,
            .width = MENU_WIDTH,
            .height = MENU_HEIGHT,
            .bg = WINDOW,
            .pal = PAL0,
            .prio = true,
            .titleText = " PAUSED ",
        },
        
        .isCycledCursor = true,
        .OnHideCallback = Menu_OnInGame_Hide,
        .OnShowCallback = Menu_OnInGame_Show,
        .menuItems = {
            {8, 3, "Shake             %s",
                {.boolValue = {(void*) &(g_game.g_glassShakeEnabled), true}},
                MIT_BOOL
            },
            {8, 5, "Sound             %s",
                {.boolValue = {(void*) &(g_game.isSoundOn), true}},
                MIT_BOOL
            },
            {8, 8, "Exit to Title",
                {{.value = 0, 0, 0, 0, .Callback = Game_Reset}},
                MIT_S16
            },
        },
    };



