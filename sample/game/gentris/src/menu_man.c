#include <genesis.h>
#include "defs.h"
#include "global.h"
#include "game.h"
#include "menu_man.h"
#include "input.h"

// Display in-game menu if input is enabled
void Menu_InGame_Show(void)
{
    if (Input_IsEnabled())
        Menu_Show(&g_inGameMenu);
}

// Callback when the in-game menu is shown
void Menu_OnInGame_Show(void) {
    g_currentMenu = &g_inGameMenu;
    JOY_setEventHandler(OnMenuInputCallback);
    Game_SetPaused(true);
}

// Callback when the in-game menu is hidden
void Menu_OnInGame_Hide(void) {
    JOY_setEventHandler(OnInputCallback_InGame);
    Game_SetPaused(false);
    VDP_fillTileMapRect(BG, TILE_ATTR_FULL(PAL3, false, false, false, TILE_USER + 23), 0, g_inGameMenu.desk.y, SCREEN_TILES_WIDTH,
                        g_inGameMenu.desk.height);
}

// Callback when the game over menu is shown
void Menu_OnGameOver_Show(void) {
    g_currentMenu = &g_gameOverMenu;
    JOY_setEventHandler(OnMenuInputCallback);
}

// Callback when the game over menu is hidden
void Menu_OnGameOver_Hide(void) {
    JOY_setEventHandler(OnInputCallback_InGame);
    VDP_fillTileMapRect(BG, TILE_ATTR_FULL(PAL3, false, false, false, TILE_USER + 23), 0, g_gameOverMenu.desk.y, SCREEN_TILES_WIDTH,
                        g_gameOverMenu.desk.height);
}

// Callback when the title menu is shown
void Menu_OnTitle_Show(void) {
    g_currentMenu = &g_titleMenu;
    JOY_setEventHandler(OnMenuInputCallback);
}

// Callback when the title menu is hidden
void Menu_OnTitle_Hide(void) {
    JOY_setEventHandler(OnInputCallback_InGame);
    VDP_fillTileMapRect(BG, TILE_ATTR_FULL(PAL3, false, false, false, TILE_USER + 23), 0, g_titleMenu.desk.y, SCREEN_TILES_WIDTH,
                        g_titleMenu.desk.height);
}

// Callback when the title menu is shown
void Menu_OnOptions_Show(void) {
    g_currentMenu = &g_optionsMenu;
    JOY_setEventHandler(OnMenuInputCallback);
}

// Callback when the title menu is hidden
void Menu_OnOptions_Hide(void) {
    JOY_setEventHandler(OnInputCallback_InGame);
    VDP_fillTileMapRect(BG, TILE_ATTR_FULL(PAL3, false, false, false, TILE_USER + 23), 0, g_titleMenu.desk.y, SCREEN_TILES_WIDTH,
                        g_titleMenu.desk.height);
}

// Show 40-line completion menu and set input handler
void Menu_40Lines_Show(void) {
    g_currentMenu = &g_40LinesCompletionMenu;
    JOY_setEventHandler(OnMenuInputCallback);
}

// Hide 40-line menu and restore game input handler
void Menu_40Lines_Hide(void) {
    JOY_setEventHandler(OnInputCallback_InGame);
    VDP_fillTileMapRect(BG, TILE_ATTR_FULL(PAL3, false, false, false, TILE_USER + 23), 0, g_40LinesCompletionMenu.desk.y, SCREEN_TILES_WIDTH,
                        g_40LinesCompletionMenu.desk.height);
}

// Render in-game menu
void PauseMenuState_OnUpdate(void)
{
    Menu_UpdateDisplay(&g_inGameMenu);
    SYS_doVBlankProcess();
}

// Render game over menu
void GameOverMenuState_OnUpdate(void)
{
    Menu_UpdateDisplay(&g_gameOverMenu);
    SYS_doVBlankProcess();
}

// Render title menu
void TitleMenuState_OnUpdate(void)
{
    Menu_UpdateDisplay(&g_titleMenu);
    SYS_doVBlankProcess();
}

// Render options menu
void OptionsState_OnUpdate(void)
{
    Menu_UpdateDisplay(&g_optionsMenu);
    SYS_doVBlankProcess();
}

// Render 40-line completion menu
void CompletionMenuState_OnUpdate(void)
{
    Menu_UpdateDisplay(&g_40LinesCompletionMenu);
    SYS_doVBlankProcess();
}
