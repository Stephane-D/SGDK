#include <genesis.h>
#include "global.h"
#include "defs.h"
#include "types.h"
#include "game.h"
#include "menu_man.h"


MenuPage g_40LinesCompletionMenu =
    {
        .desk = {
            .state = UI_HIDDEN,
            .offsetX = 0,
            .x = MENU_INGAME_X,
            .y = MENU_INGAME_Y + 1,
            .width = MENU_WIDTH,
            .height = 8,
            .bg = WINDOW,
            .pal = PAL0,
            .prio = true,
            .titleText = " SUCCESS in ",
        },
        
        .isCycledCursor = true,
        .startToConfirm = true,
        .OnHideCallback = Menu_40Lines_Hide,
        .OnShowCallback = Menu_40Lines_Show,
        .menuItems = {
            {
                10, 3, "Restart",
                {{.value = 0,                            0, 0,  0, .Callback = Game_Restart}},                          MIT_S16
            },
            
            {
                10, 4, "Exit to Title",
                {{.value = 0,                            0, 0,  0, .Callback = Game_Reset}},                          MIT_S16
            },
        },
    };



