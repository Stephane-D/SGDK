#pragma once

#include <genesis.h>

#include "physics.h"

#define TILEMAP_PLANE BG_A

#define PLAYER_PALETTE PAL1
#define LEVEL_PALETTE PAL0

#define GROUND_TILE 1
#define LADDER_TILE 2
#define ONE_WAY_PLATFORM_TILE 4

typedef struct {
	u16 joy;
	u16 changed;
	u16 state;
} InputType;

extern InputType input;

extern const fix16 gravityScale;

extern Map* bga;

extern AABB roomSize;
extern AABB playerBounds;

extern u16 VDPTilesFilled;