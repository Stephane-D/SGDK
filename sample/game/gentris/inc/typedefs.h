#ifndef HEADER_TYPEDEFS
#define HEADER_TYPEDEFS

#include "src/nfsm/nfsm.h"
#include "input.h"
#include "src/ui/menu.h"
#include "defs.h"
#include <genesis.h>

// Direction enumeration for collision detection
typedef enum
{
    DIR_NONE = 0,       // No collision
    DIR_LEFT,           // Collision on left side
    DIR_RIGHT,          // Collision on right side
    DIR_UP,             // Collision above
    DIR_DOWN,           // Collision below
} Direction;

// Game mode selection
typedef enum
{
    MODE_CLASSIC = 0,   // Endless classic Tetris mode
    MODE_40LINES,       // Time attack - clear 40 lines as fast as possible
} GameMode;

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
    GameMode gameMode;                  // Current game mode (classic or 40-lines)
    s16 blockPatternType;               // Block pattern type
    u16 soundRowRemovingTone;           // Base frequency for row removal sound
    bool isSoundOn;                     // Enable/disable sound effects
    bool g_glassShakeEnabled;           // Enable/disable screen shake effect
} GameConfig;

// Collision type enumeration
typedef enum
{
    COL_NONE = 0,       // No collision
    COL_BORDER,         // Collision with field borders
    COL_FIGURE,         // Collision with other figures
} CollisionType;

// Collision result data
typedef struct
{
    CollisionType type;  // Type of collision detected
    Direction dir;       // Direction where collision occurred
} FigCollision;

// Active falling figure state
typedef struct
{
    s8 x;               // X position in field
    s8 y;               // Y position in field
    u8 type;            // Figure type (0-6 for 7 piece types)
    u8 rot;             // Rotation state (0-3)
    u8 fixTimer;        // Frames until figure locks in place
    bool grounded;      // True if figure is touching bottom/other pieces
    bool fixed;         // True if figure has been placed permanently
    bool canSpawn;      // Flag to spawn next figure
    u16 nextFigType;    // Next figure type to spawn
} Figure;

// Block of rows to be removed
typedef struct
{
    u8 lines[4];        // Row indices to be removed (up to 4 complete rows)
} RowsBlock;

// Current input state
typedef struct
{
    bool enabled;       // Input processing enabled
    u8 dropType;        // Current drop type (NO_DROP, SOFT_DROP, HARD_DROP)
    Direction colDir;   // Last collision direction
} Input;

// Scoring information
typedef struct
{
    u16 score;          // Current total score
    u16 step;           // Score increment per frame during row removal
    u16 target;         // Target score for current row removal
    u16 counter;        // Score accumulator during row removal
    u16 lines;          // Total lines cleared
} Score;

// Game statistics
typedef struct
{
    u16 figuresFixed;   // Total figures placed
    u16 tetris[5];      // Count of 0-4 line clears (tetris[4] = 4-line clear count)
    u16 combo;          // Current combo counter (consecutive line clears)
    u16 maxCombo;       // Maximum combo achieved
    u32 seconds;        // Elapsed time in seconds
    u32 secondsStart;   // Game start time
    u16 level;        // Current level
} Stats;


#endif // HEADER_TYPEDEFS
