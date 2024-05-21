#pragma once

#include <genesis.h>

#include "xtypes.h"
#include "player.h"
#include "physics.h"
#include "levels.h"

extern Vect2D_s16 cameraPosition;

void setupCamera(Vect2D_u16 deadZoneCenter, u16 deadZoneWidth, u16 deadZoneHeight);
void updateCamera();