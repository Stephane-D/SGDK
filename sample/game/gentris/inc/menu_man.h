#ifndef HEADER_MENU_MAN
#define HEADER_MENU_MAN


void Menu_OnTitle_Show(void);
void Menu_OnInGame_Show(void);
void Menu_OnGameOver_Show(void);
void Menu_40Lines_Show(void);

void Menu_OnTitle_Hide(void);
void Menu_OnInGame_Hide(void);
void Menu_OnGameOver_Hide(void);
void Menu_40Lines_Hide(void);
void Menu_OnOptions_Show(void);
void Menu_OnOptions_Hide(void);

void TitleMenuState_OnUpdate(void);
void OptionsState_OnUpdate(void);
void PauseMenuState_OnUpdate(void);
void GameOverMenuState_OnUpdate(void);
void CompletionMenuState_OnUpdate(void);

void Menu_InGame_Show(void);

#endif // HEADER_MENU_MAN
