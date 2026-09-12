#ifndef HEADER_GAME
#define HEADER_GAME

#include "typedefs.h"

void Game_Init(void);
void Game_MainLoop(void);
void Game_SetPaused(bool state);
void Game_Reset(s16 noUse);
void Game_ModeRestartClassic(s16 noUse);
void Game_ModeRestart40(s16 noUse);
void Game_Restart(s16 noUse);
void Game_SetStateOptionsMenu(s16 noUse);
void Game_SetStateTitleMenu(s16 noUse);

#endif // HEADER_GAME

