#ifndef _MAP_H_
#define _MAP_H_

#include "../inc/tile_tools.h"


extern Plugin map;

void outMap(tilemap_* map, FILE* fs, FILE* fh, char* id, int global);


#endif // _MAP_H_

