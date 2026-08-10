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

void StateClbk_TitleMenuOnUpdate(void);
void StateClbk_OptionsOnUpdate(void);
void StateClbk_PauseMenuOnUpdate(void);
void StateClbk_GameOverMenuOnUpdate(void);
void StateClbk_CompletionMenuOnUpdate(void);

void StateClbk_TitleMenuOnEnter(void);
void StateClbk_TitleMenuOnExit(void);
void StateClbk_OptionsOnEnter(void);
void StateClbk_OptionsOnExit(void);

void Menu_InGame_Show(void);

#endif // HEADER_MENU_MAN
