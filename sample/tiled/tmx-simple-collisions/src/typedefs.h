#ifndef HEADER_TYPEDEFS
#define HEADER_TYPEDEFS

#include <maths.h>

typedef struct
{
    s16 left;
    s16 top;
    s16 right;
    s16 bottom;
} AABB_s16;

typedef struct
{
    Vect2D_s16 leftTop;
    Vect2D_s16 rightBottom;
} Range_s16;

typedef enum
{
    TILE_EMPTY = 0,
    TILE_SOLID,
    TILE_DAMAGER,
} TileType;

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

//
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

typedef enum
{
    PL_STATE_NORMAL,
    PL_STATE_HURT,
} PlayerState;

// Player struct, inheriting from GameObject
typedef struct
{
    GameObject;
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
