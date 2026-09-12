#ifndef HEADER_DEFS
#define HEADER_DEFS

// Game field position and dimensions (in tiles)
#define FIELD_X                         16
#define FIELD_Y                         3
#define FIELD_W                         10
#define FIELD_H                         22

// Glass display area dimensions (includes borders)
#define GLASS_X                         (FIELD_X)
#define GLASS_Y                         (FIELD_Y - 1)
#define GLASS_W                         (FIELD_W)
#define GLASS_H                         (FIELD_H + 2)

// Next piece preview box position
#define PREVIEW_X                       (GLASS_X + GLASS_W + 5)
#define PREVIEW_Y                       (GLASS_Y + 2)

// Score display panel position
#define SCORE_X                         PREVIEW_X
#define SCORE_Y                         (PREVIEW_Y + 6)

// Right side statistics panel position
#define BORDER_STAT_X                   2
#define BORDER_STAT_Y                   GLASS_Y
#define BORDER_STAT_W                   10


// Number of different Tetris piece types (I, O, T, S, Z, J, L)
#define FIG_COUNT                       7
// Number of rotation states for each piece
#define ROT_COUNT                       4
// Height of piece data array
#define FIG_LINES                       4
// Tile index for empty space
#define TILE_EMPTY                      (TILE_USER + 0)
// Tile index for colored blocks
#define TILE_BLOCK                      (TILE_USER + 1)

// Tile index for top border
#define BORDER_THINK_TOP                (TILE_USER + 9)

// Tile indices for glass borders
#define TILE_GLASS_LEFT                 (TILE_USER + 15)
#define TILE_GLASS_RIGHT                (TILE_USER + 16)
#define TILE_GLASS_BOTTOM               (TILE_USER + 17)
#define TILE_GLASS_LEFT_COR             (TILE_USER + 18)
#define TILE_GLASS_RIGHT_COR            (TILE_USER + 19)
#define TILE_GLASS_LEFT_TOP             (TILE_USER + 20)
#define TILE_GLASS_RIGHT_TOP            (TILE_USER + 21)

// Total number of unique tiles loaded
#define TILE_COUNT                      25

// Maximum sprites for active falling piece
#define FIG_SPRITES                     4

// Screen shake animation frame count
#define GLASS_SHAKE_FRAMES_Y            29
// Duration of screen shake effect
#define SOUND_LENGTH_SHAKE              6
// Glass width for shake table lookup
#define GLASS_WIDTH                     12
// Duration of row removal animation
#define ROW_REMOVE_FX_DELAY             40

// Drop types for input
#define NO_DROP                         0
#define SOFT_DROP                       1
#define HARD_DROP                       2

// Screen shake type constants
#define MID_SHAKE                       1

// VDP layer assignments for foreground and background
#define FG                              BG_A
#define BG                              BG_B

// Lines required per level up
#define NEXT_LEVEL_LINES                10

// Menu position and dimensions constants
#define MENU_TITLE_X                    3
#define MENU_TITLE_Y                    8
#define MENU_INGAME_X                   3
#define MENU_INGAME_Y                   7
#define MENU_WIDTH                      26
#define MENU_HEIGHT                     11
#define MENU_FADE_DURATION              5

#define BLOCK_PTRN_FLAT1                0
#define BLOCK_PTRN_FLAT2                1
#define BLOCK_PTRN_OLD                  2

// Additional common constants
#define SCREEN_TILES_WIDTH              40   // Width of screen in tiles (used for VDP operations)

// 40-lines mode constants
#define LINES_40_MODE                   40   // Number of lines to clear in 40-lines mode

// Default audio constants
#define DEFAULT_ROW_REMOVAL_TONE        100  // Default base tone for row removal sound

// Scoring base values (per number of lines cleared) before level multiplier
#define SCORE_BASE_SINGLE               40
#define SCORE_BASE_DOUBLE               100
#define SCORE_BASE_TRIPLE               300
#define SCORE_BASE_TETRIS               1200

#endif // HEADER_DEFS
