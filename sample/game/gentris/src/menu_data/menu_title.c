#include <genesis.h>
#include "defs.h"
#include "global.h"
#include "types.h"
#include "game.h"
#include "menu_man.h"


MenuPage g_titleMenu =
    {
        .desk = {
            .state = UI_HIDDEN,
            .offsetX = 0,
            .x = MENU_TITLE_X,
            .y = MENU_TITLE_Y,
            .width = MENU_WIDTH,
            .height = MENU_HEIGHT,
            .bg = WINDOW,
            .pal = PAL0,
            .prio = true,
            .titleText = " GENTRIS ",
        },
        
        .isCycledCursor = true,
        .startToConfirm = true,
        .OnHideCallback = Menu_OnTitle_Hide,
        .OnShowCallback = Menu_OnTitle_Show,
        .menuItems = {
            {
                11, 4, "Classic Mode",
                {{.value = 0,                            0, 0,  0, .Callback = Game_RestartClassic}},                          MIT_S16
            },
            
            {
                11, 6, "40Lines Mode",
                {{.value = 0,                            0, 0,  0, .Callback = Game_Restart40}},                          MIT_S16
            },
            {
                11, 8, "  Options",
                {{.value = 0,                            0, 0,  0, .Callback = GameState_SetOptionsMenu}}, MIT_S16
            },

        },
    };



