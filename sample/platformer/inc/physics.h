#pragma once

#include <genesis.h>

#include "types.h"

u16 getTileLeftEdge(u16 x);
u16 getTileRightEdge(u16 x);
u16 getTileTopEdge(u16 y);
u16 getTileBottomEdge(u16 y);
AABB getTileBounds(s16 x, s16 y);
Vect2D_u16 posToTile(Vect2D_s16 position);