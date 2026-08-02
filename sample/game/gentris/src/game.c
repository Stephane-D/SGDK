#include <genesis.h>
#include "defs.h"
#include "typedefs.h"
#include "global.h"
#include "input.h"
#include "graphics.h"
#include "menu_man.h"
#include "game.h"
#include "glass.h"
#include "figure.h"
#include "hud.h"


// Macro to simplify state transitions in the game FSM
#define GAME_SET_STATE(STATE_ENUM_ID)  NFSM_TransitToState(&g_game.fsm, &g_game.states[STATE_ENUM_ID])

// Function prototypes for internal static functions
static void ResetGameSettings(void);
static void GameConfig_Init(void);
static void Stats_Update(void);
static void UpdatePhysics(void);
static void RowsRemovingUpdate(void);
static void ResetField(void);
static void GameOver(void);
static void InitSystem(void);
static void InitData(void);
static void ResetStates(void);
static void TitleMenuState_OnEnter(void);
static void TitleMenuState_OnExit(void);
static void OptionsState_OnEnter(void);
static void OptionsState_OnExit(void);
static void LoadResState_OnEnter(void);


// Initialize game system and data
void Game_Init(void)
{
    GameConfig_Init();
    InitSystem();
    InitData();
}

// Main game loop
void Game_MainLoop(void)
{
    while (true)
        NFSM_Update(&g_game.fsm);
}

// Set game paused state and switch between paused and main loop states
void Game_SetPaused(bool state)
{
    if (state)
        GAME_SET_STATE(ST_GAME_PAUSED);
    else
        GAME_SET_STATE(ST_GAME_GAMEPLAY);
}

// Perform hard reset of the system
void Game_Reset(s16 noUse)
{
    SYS_hardReset();
}

// Restart game with field reset and stats cleared
void Game_ResetStats(s16 noUse)
{
    ResetField();
    g_activeFig.x = 3;
    g_activeFig.y = 0;
    g_activeFig.rot = 0;
    g_input.dropType = NO_DROP;

    g_stats.maxCombo = 0;
    g_stats.combo = 0;
    g_score.lines = 0;
    g_score.score = 0;
    g_level = 0;
    memset(g_stats.tetris, 0, sizeof(g_stats.tetris));
    
    g_stats.secondsStart = getTime(0);
}

// Restart game in classic mode
void Game_RestartClassic(s16 noUse)
{
    GAME_SET_STATE(ST_GAME_LOAD_RES);
    Game_ResetStats(NULL);
    g_game.gameMode = MODE_CLASSIC;
}

// Restart game in 40-lines mode
void Game_Restart40(s16 noUse)
{
    GAME_SET_STATE(ST_GAME_LOAD_RES);
    Game_ResetStats(NULL);
    g_game.gameMode = MODE_40LINES;
    VDP_drawText("       ", BORDER_STAT_X + 3, BORDER_STAT_Y + 17);
    HUD_TimeDraw();
}

// Restart game
void Game_Restart(s16 noUse)
{
    GAME_SET_STATE(ST_GAME_GAMEPLAY);
    Game_ResetStats(NULL);
    Glass_RedrawDirty();
    HUD_ScoreRedraw();
}

// Set game state to title menu
void GameState_SetTitleMenu(s16 noUse)
{
    GAME_SET_STATE(ST_GAME_TITLE_MENU);
}

// Set game state to options menu
void GameState_SetOptionsMenu(s16 noUse)
{
    GAME_SET_STATE(ST_GAME_OPTIONS_MENU);
}

// Gameplay loop update - handles input, physics, rendering
static void GameplayState_OnUpdate(void)
{
    // Update game statistics and timers
    Stats_Update();
    
    // Update input state and handle user actions
    Input_HoldingUpdate();
    
    // Update figure position and check for collisions
    UpdatePhysics();
    
    // Update screen shake effect if enabled
    Glass_UpdateShake();
    
    // Update row removal animation and check for completed rows
    RowsRemovingUpdate();
    
    // Redraw dirty rows in the game field
    Glass_RedrawDirty();
    
    // Draw the currently active falling figure
    Figure_DrawActive();
    
    // Update system state
    SYS_doVBlankProcess();
}

// Initialize Sega Genesis hardware - Z80, sound, video settings
static void InitSystem(void)
{
    // Initialize Z80 co-processor and PSG sound system
    Z80_init();
    
    // Initialize PSG sound system
    PSG_reset();
    
    // Set VDP screen width to 320 pixels and configure scrolling mode
    VDP_setScreenWidth320();
    
    // Set VDP scrolling mode to tile-based horizontal and column-based vertical
    VDP_setScrollingMode(HSCROLL_TILE, VSCROLL_COLUMN);
    
    // Set VDP text plane and palette
    VDP_setTextPlane(FG);
    
    // Set VDP text palette
    VDP_setTextPalette(PAL0);
    
    // Fade out all palettes
    PAL_fadeOutAll(1, false);
    
    // Set VDP background color to black
    VDP_setBackgroundColor(0);
}

// Initialize game data - create menus and reset game states
static void InitData(void)
{
    Menu_Create(&g_titleMenu);
    Menu_Create(&g_inGameMenu);
    Menu_Create(&g_gameOverMenu);
    ResetStates();
}

// Setup game state machine with callbacks for each game state
static void ResetStates(void)
{
    JOY_setEventHandler(OnInputCallback_InGame);
//    ResetGameSettings();

    // Init game states
    // Title menu state
    g_game.states[ST_GAME_TITLE_MENU].OnEnter = TitleMenuState_OnEnter;
    g_game.states[ST_GAME_TITLE_MENU].OnUpdate = TitleMenuState_OnUpdate;
    g_game.states[ST_GAME_TITLE_MENU].OnExit = TitleMenuState_OnExit;
    
    // Options menu state
    g_game.states[ST_GAME_OPTIONS_MENU].OnEnter = OptionsState_OnEnter;
    g_game.states[ST_GAME_OPTIONS_MENU].OnUpdate = OptionsState_OnUpdate;
    g_game.states[ST_GAME_OPTIONS_MENU].OnExit = OptionsState_OnExit;
    
    // Load resources state
    g_game.states[ST_GAME_LOAD_RES].OnEnter = LoadResState_OnEnter;
    g_game.states[ST_GAME_LOAD_RES].OnUpdate = NULL;
    g_game.states[ST_GAME_LOAD_RES].OnExit = NULL;
    g_game.states[ST_GAME_LOAD_RES].nextState = &g_game.states[ST_GAME_GAMEPLAY];
    
    // Gameplay state
    g_game.states[ST_GAME_GAMEPLAY].OnEnter = NULL;
    g_game.states[ST_GAME_GAMEPLAY].OnUpdate = GameplayState_OnUpdate;
    g_game.states[ST_GAME_GAMEPLAY].OnExit = NULL;
    
    // Paused menu state
    g_game.states[ST_GAME_PAUSED].OnEnter = NULL;
    g_game.states[ST_GAME_PAUSED].OnUpdate = PauseMenuState_OnUpdate;
    g_game.states[ST_GAME_PAUSED].OnExit = NULL;
    
    // 40Lines game finished menu state
    g_game.states[ST_GAME_FINISHED].OnEnter = NULL;
    g_game.states[ST_GAME_FINISHED].OnUpdate = CompletionMenuState_OnUpdate;
    g_game.states[ST_GAME_FINISHED].OnExit = NULL;
    
    // Game over menu state
    g_game.states[ST_GAME_GAME_OVER].OnEnter = NULL;
    g_game.states[ST_GAME_GAME_OVER].OnUpdate = GameOverMenuState_OnUpdate;
    g_game.states[ST_GAME_GAME_OVER].OnExit = NULL;

    // Set initial state to title menu
    GAME_SET_STATE(ST_GAME_TITLE_MENU);
}

// Title menu on start callback - display title menu
static void TitleMenuState_OnEnter(void)
{
    Menu_Show(&g_titleMenu);
    PAL_fadeInAll(g_palettes, MENU_FADE_DURATION, false);
    Input_Enable();
}

// Transition from title menu - fade out, load tiles, initialize game field
static void TitleMenuState_OnExit(void)
{
    Input_Disable();
    PAL_fadeOutAll(MENU_FADE_DURATION, false);
}

// Options menu on start callback - display options menu
static void OptionsState_OnEnter(void)
{
    Menu_Show(&g_optionsMenu);
    PAL_fadeInAll(g_palettes, MENU_FADE_DURATION, false);
    Input_Enable();
}

// Options menu on exit callback - fade out, display title menu
static void OptionsState_OnExit(void)
{
    Input_Disable();
    PAL_fadeOutAll(MENU_FADE_DURATION, false);
}

// Gameplay on start callback - display gameplay
static void LoadResState_OnEnter(void)
{
    Graphics_GenerateTiles();
    
    Glass_DrawBorder();
    HUD_ScoreRedraw();
    VDP_setBackgroundColor(19);
    
    ResetField();
    Figure_ShuffleBag();
    
    g_nextFigType = Figure_GetNextType();
    Figure_Spawn();
    
    PAL_fadeInAll(g_palettes, MENU_FADE_DURATION, false);
    Input_Enable();
}

// GameplayState_OnUpdate game statistics (next fix timer)
static void Stats_Update(void)
{
    g_stats.nextFixTimer++;
}

// Handle figure gravity and collision - move figure down each frame
static void UpdatePhysics(void)
{
    if (g_activeFig.fixed)
        return;
    
    // Decrease fall counter and apply soft drop acceleration
    if (g_fallDownCounter)
        g_fallDownCounter--;
    
    // Accelerate fall when soft drop is active
    if (g_input.dropType)
        g_fallDownCounter = (g_fallDownCounter < 25) ? 0 : g_fallDownCounter - 25;
    
    // Process figure falling when counter reaches zero
    if (!g_fallDownCounter)
    {
        g_fallDownCounter = g_figFallFrameDelay[g_level];
        g_activeFig.y++;

        if (g_game.gameMode == MODE_40LINES)
            HUD_TimeDraw();
        
        // Check if figure hit obstacle after moving down
        if (Figure_IsCollided(&g_activeFig))
        {
            // Move figure back up and mark as grounded
            g_activeFig.y--;
            g_activeFig.grounded = true;
            Figure_FixRemoveSpawn();
        }
        else
        {
            g_activeFig.grounded = false;
            g_activeFig.fixTimer = 0;
        }
    }
    else
    {
        // Increment timer while grounded - used to delay figure fixing
        if (g_activeFig.grounded)
            g_activeFig.fixTimer++;
    }
}

// Handle row removal animation and scoring
static void RowsRemovingUpdate(void)
{
    static u8 flashCounter = 0;
    
    if (g_rowsRemoveDownCounter)
    {
        g_rowsRemoveDownCounter--;

        // Alternate palette for flashing effect during row removal
        flashCounter = (flashCounter == 2) ? 0 : flashCounter + 1;
        
        if (flashCounter == 0)
            g_palRowRemoving = (g_palRowRemoving > PAL2) ? PAL0 : g_palRowRemoving + 1;

        // Calculate score based on number of consecutive lines removed (Tetris scoring)
        switch (g_rowsToRemoveCount)
        {
            case 1:
                g_score.target = SCORE_BASE_SINGLE * (g_level + 1);
                break;
            case 2:
                g_score.target = SCORE_BASE_DOUBLE * (g_level + 1);
                break;
            case 3:
                g_score.target = SCORE_BASE_TRIPLE * (g_level + 1);
                break;
            case 4:
                g_score.target = SCORE_BASE_TETRIS * (g_level + 1);
                break;
            default:
                break;
        }

        g_score.step = g_score.target / ROW_REMOVE_FX_DELAY;
        g_score.score += g_score.step;
        g_score.counter += g_score.step;

        // Play row removal sound if enabled
        if (g_game.isSoundOn)
        {
            if (g_rowsToRemoveCount < 4)
            {
                if (g_rowsRemoveDownCounter == ROW_REMOVE_FX_DELAY / 9 * 1)
                    PSG_setEnvelope(1, PSG_ENVELOPE_MIN);
                else
                    PSG_setTone(1, g_game.soundRowRemovingTone + (g_rowsRemoveDownCounter + 10) * 30);
            }
            else
            {
                PSG_setTone(1, g_game.soundRowRemovingTone - (g_rowsRemoveDownCounter + 10) * 497);
            }
            
            if (g_rowsRemoveDownCounter == ROW_REMOVE_FX_DELAY / 2)
                PSG_setEnvelope(3, PSG_ENVELOPE_MIN);
        }


        if (!g_rowsRemoveDownCounter)
        {
            // Add final score increment to reach target
            g_score.score += g_score.target - g_score.counter;

            PSG_reset();

            u16 removedLines = Figure_TryRemoveLines();

            // GameplayState_OnUpdate level and allow spawning next figure if rows were removed
            if (removedLines)
            {
                g_score.lines += removedLines;
                g_level = g_score.lines / NEXT_LEVEL_LINES;
                g_canSpawnFigure = true;
            }
            memset(&g_rowsBlockToRemove, 0, sizeof(g_rowsBlockToRemove));

            // Check if 40-line mode is completed
            if (g_glassShakeInd == GLASS_SHAKE_FRAMES_Y && g_score.lines >= LINES_40_MODE && g_game.gameMode == MODE_40LINES)
            {
                sprintf(g_timeStrBuf, "SUCCESS in %ld secs", g_stats.seconds);
                g_40LinesCompletionMenu.desk.titleText = g_timeStrBuf;

                Menu_Show(&g_40LinesCompletionMenu);
                GAME_SET_STATE(ST_GAME_FINISHED);
            }
        }
        
        HUD_ScoreRedraw();
    }
    
    // Spawn next figure if removal animation is complete
    if (g_canSpawnFigure)
    {
        g_canSpawnFigure = false;
        Figure_Spawn();

        FigCollision fiCol = Figure_GetCollision(&g_activeFig);
        if (fiCol.type == COL_FIGURE)
            GameOver();
    }
}

// Clear playing field and reset level/score data
static void ResetField(void)
{
    memset(g_blockField, 0, sizeof(g_blockField));
    memset(g_rowMask, 0, sizeof(g_rowMask));
    memset(g_dirtyRows, 1, sizeof(g_dirtyRows));
    
    g_score.lines = 0;
    g_level = 0;
}

// End game and show game over menu
static void GameOver()
{
    // force offscreen
    if (Figure_IsCollided(&g_activeFig))
        g_activeFig.y = -10;

    PSG_reset();

    GAME_SET_STATE(ST_GAME_GAME_OVER);
    Menu_Show(&g_gameOverMenu);
}

// Initialize game settings - enable input and sound
static void ResetGameSettings(void)
{
    Input_Enable();
    g_game.isSoundOn = true;
    g_game.soundRowRemovingTone = DEFAULT_ROW_REMOVAL_TONE;
}

// Initialize game configuration defaults and zero global game struct
static void GameConfig_Init(void)
{
    // Clear whole structure to ensure deterministic defaults
    memset(&g_game, 0, sizeof(g_game));

    // sensible defaults
    g_game.isSoundOn = true;
    g_game.gameMode = MODE_CLASSIC;
    g_game.blockPatternType = BLOCK_PTRN_FLAT1;
    g_game.soundRowRemovingTone = DEFAULT_ROW_REMOVAL_TONE;
    // states and fsm will be initialized in ResetStates()/NFSM init flow
}

