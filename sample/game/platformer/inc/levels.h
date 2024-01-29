#pragma once

#include <genesis.h>

#include "global.h"
#include "types.h"

typedef struct {
	s8 top;
	s8 bottom;
	s8 left;
	s8 right;
}Level;

void loadLevel();

extern u16 currentCurrencySpawnerEntity;