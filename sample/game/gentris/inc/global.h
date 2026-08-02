#ifndef HEADER_GLOBAL
#define HEADER_GLOBAL

#include <genesis.h>
#include "defs.h"
#include "typedefs.h"
#include "src/ui/menu.h"
#include "input.h"
#include "src/nfsm/nfsm.h"


typedef enum GameState
{
    ST_GAME_TITLE_MENU,
    ST_GAME_LOAD_RES,
    ST_GAME_GAMEPLAY,
    ST_GAME_PAUSED,
    ST_GAME_FINISHED,
    ST_GAME_OPTIONS_MENU,
    ST_GAME_GAME_OVER,
    ST_GAME_COUNT,
} GameState;

typedef struct
{
    NFSM fsm;
    NFSM_State states[ST_GAME_COUNT];
    bool isSoundOn;
    GameMode gameMode;                  // Current game mode (classic or 40-lines)
    s16 blockPatternType;               // Block pattern type
    u16 soundRowRemovingTone;           // Base frequency for row removal sound
} GameConfig;


// const
extern const u8 g_figureLines[FIG_COUNT][ROT_COUNT][FIG_LINES];             // Tetris piece shape data
extern const u16 g_figFallFrameDelay[];                                     // Frame delay per level (gravity)
extern const s16 g_glassShakeTable[4][GLASS_SHAKE_FRAMES_Y][GLASS_WIDTH];   // Screen shake animation table
extern const u16 g_palettes[64];                                            // VDP color palettes

// globals
extern GameConfig g_game;                           // Game state and configuration
extern Figure g_activeFig;                          // Currently falling figure
extern Joypad g_joypad[2];                          // Controller input state
extern Input g_input;                               // Current input state
extern Score g_score;                               // Current score and statistics
extern Stats g_stats;                               // Game statistics counters

extern MenuPage* g_currentMenu;                     // Pointer to current menu structure
extern MenuPage g_titleMenu;                        // Title menu structure
extern MenuPage g_optionsMenu;                      // Options menu structure
extern MenuPage g_gameOverMenu;                     // Game over menu structure
extern MenuPage g_inGameMenu;                       // Pause menu structure
extern MenuPage g_40LinesCompletionMenu;            // 40-line mode completion menu

extern u32 g_tileData[TILE_COUNT][8];               // Tile graphics data
extern u8 g_blockField[FIELD_H][FIELD_W];           // Game field grid (0 = empty, 1-7 = piece type)
extern u16 g_rowMask[FIELD_H];                      // Bitmask for filled blocks in each row
extern u8 g_dirtyRows[FIELD_H];                     // Rows that need redrawing flag
extern u8 g_figuresBag[7];                          // Shuffled bag of piece types for randomization
extern u8 g_bagIndex;                               // Current index in pieces bag
extern u16 g_fallDownCounter;                       // Frames until next piece falls

extern bool g_glassShakeEnabled;                    // Enable/disable screen shake effect
extern u16 g_glassShakeInd;                         // Current frame of shake animation
extern s16 g_glassShakeType;                        // Type of shake effect (drop type based)
extern u16 g_level;                                 // Current level (increases every 10 lines)
extern char g_timeStrBuf[18];                       // Time string buffer for display
extern bool g_canSpawnFigure;                       // Flag to spawn next figure
extern u16 g_rowsRemoveDownCounter;                 // Frames remaining in row removal animation
extern RowsBlock g_rowsBlockToRemove;               // Rows currently being removed
extern u16 g_rowsToRemoveCount;                     // Number of rows being removed
extern u16 g_palRowRemoving;                        // Palette for flashing rows
extern u16 g_nextFigType;                           // Next piece type to spawn


#endif // HEADER_GLOBAL

