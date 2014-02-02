#ifndef _TILESET_H_
#define _TILESET_H_

#include "../inc/tile_tools.h"


extern Plugin tileset;

void outTileset(tileset_* tileset, FILE* fs, FILE* fh, char* id, int global);


#endif // _TILESET_H_
