#include <genesis.h>
#include "defs.h"
#include "global.h"
#include "types.h"
#include "game.h"
#include "menu_man.h"


MenuPage g_optionsMenu =
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
            .titleText = "  OPTIONS ",
        },
        
        .isCycledCursor = true,
        .startToConfirm = true,
        .OnHideCallback = Menu_OnOptions_Hide,
        .OnShowCallback = Menu_OnOptions_Show,
        .menuItems =
            {
                {8,  3,  "Block pattern %s",
                    {.arrayValue = {
                        .value = (void*) &g_game.blockPatternType, .valueOpt = {BLOCK_PTRN_FLAT1, BLOCK_PTRN_FLAT2, BLOCK_PTRN_OLD},
                        .optText = {"< Flat1 >", "< Flat2 >", "< Old >  "}, .optCount = 3, .index = 0, .Callback = NULL
                    }},                                                           MIT_S16_ARRAY
                },
                {8,  5,  "Shake           %s",
                    {.boolValue = {(void*) &(g_game.g_glassShakeEnabled), true}}, MIT_BOOL
                },
                {8,  7,  "Sound           %s",
                    {.boolValue = {(void*) &(g_game.isSoundOn), true}},           MIT_BOOL
                },
                {
                 11, 10, "   Exit",
                    {{.value = 0, 0, 0, 0, .Callback = Game_SetStateTitleMenu}},  MIT_S16
                },
            },
    };



