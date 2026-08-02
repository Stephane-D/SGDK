#ifndef HEADER_TYPEDEFS
#define HEADER_TYPEDEFS

#include <genesis.h>


typedef enum GameState GameState;

// Game mode selection
typedef enum
{
    MODE_CLASSIC = 0,   // Endless classic Tetris mode
    MODE_40LINES,       // Time attack - clear 40 lines as fast as possible
} GameMode;

// Direction enumeration for collision detection
typedef enum
{
    DIR_NONE = 0,       // No collision
    DIR_LEFT,           // Collision on left side
    DIR_RIGHT,          // Collision on right side
    DIR_UP,             // Collision above
    DIR_DOWN,           // Collision below
} Direction;

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
    u16 nextFixTimer;   // Timer for next fix event
    u16 tetris[5];      // Count of 0-4 line clears (tetris[4] = 4-line clear count)
    u16 combo;          // Current combo counter (consecutive line clears)
    u16 maxCombo;       // Maximum combo achieved
    u32 seconds;        // Elapsed time in seconds
    u32 secondsStart;   // Game start time
} Stats;

#endif // HEADER_TYPEDEFS
