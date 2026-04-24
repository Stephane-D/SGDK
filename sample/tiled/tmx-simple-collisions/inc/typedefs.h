#ifndef HEADER_TYPEDEFS
#define HEADER_TYPEDEFS

#include <maths.h>

// AABB helper struct
typedef struct
{
    s16 left;
    s16 top;
    s16 right;
    s16 bottom;
} AABB_s16;

// Range helper struct, used to store left-top and right-bottom coordinates of a rectangle area
typedef struct
{
    Vect2D_s16 leftTop;
    Vect2D_s16 rightBottom;
} Range_s16;

// Tile types for collision detection, defined in the TMX map's collision layer as tile indexes
typedef enum
{
    TILE_ERROR = -1,
    TILE_EMPTY = 0,
    TILE_SOLID,
    TILE_DAMAGER,
} TileType;

// Directions for tile collision checks and movement restriction
typedef enum
{
    DIR_LEFT,
    DIR_RIGHT,
} HorDirection;

typedef enum
{
    DIR_UP,
    DIR_DOWN,
} VertDirection;

typedef enum
{
    DIR_LEFT_UP,
    DIR_LEFT_DOWN,
    DIR_RIGHT_UP,
    DIR_RIGHT_DOWN,
} DiagDirection;

// Forward declaration of GameObject to be used in function pointer typedef
typedef struct GameObject GameObject;

// Base struct for game objects (player, enemies, items, etc.)
typedef struct GameObject
{
    Sprite *sprite;
    Vect2D_s16 pos;
    Vect2D_s16 velocity;
    Range_s16 aabb;
    bool (*OnTileCollision)(GameObject *gameObject, TileType tileType);
} GameObject;

// Player states for handling different behaviors (e.g. normal, hurt)
typedef enum
{
    PL_STATE_NORMAL,
    PL_STATE_HURT,
} PlayerState;

// Player struct, inheriting from GameObject
typedef struct
{
    GameObject;
    s16 moveSpeed;
    u16 hurtStateDelay;
    PlayerState state;
} Player;

// Camera struct, representing the viewport in the game world
typedef struct
{
    Vect2D_s16 pos;
    Vect2D_s16 offset;
    Vect2D_s16 scrollSize;
} Camera;

#endif //HEADER_TYPEDEFS
