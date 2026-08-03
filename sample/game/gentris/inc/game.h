#ifndef HEADER_GAME
#define HEADER_GAME

#include "typedefs.h"

void Game_Init(void);
void Game_MainLoop(void);
void Game_SetPaused(bool state);
void Game_Reset(s16 noUse);
void Game_ResetStats(s16 noUse);
void Game_RestartClassic(s16 noUse);
void Game_Restart40(s16 noUse);
void Game_Restart(s16 noUse);
void GameState_SetOptionsMenu(s16 noUse);
void GameState_SetTitleMenu(s16 noUse);

#endif // HEADER_GAME

