#include "../inc/levelgenerator.h"

#include "../res/resources.h"
#include "../inc/global.h"
#include "../inc/map.h"

//Dynamic 2D array where we store the collision map data
//We could read that directly from ROM but in the long term it's cleaner and/or more performant
u8** currentMap;

//Downlscaled size of the map in order to match the collision map size
Vect2D_u16 roomTileSize;

void freeCollisionMap() {
	//We have to free the collision map data in this way in order to avoid memory leaks
	for (u16 i = 0; i < roomTileSize.y; i++) {
		MEM_free(currentMap[i]);
	}
	MEM_free(currentMap);
}

void generateCollisionMap(const u8 map[][48]) {
	roomSize = newAABB(0, 768, 0, 768);

	//Each tile is 16x16px so we divide 768(size of the room in pixels) / 16, we use bitshifts because it is more performant than divisions
	roomTileSize = newVector2D_u16(768 >> 4, 768 >> 4);

	//To store a 2D array with pointers we have to do it in that way
	currentMap = (u8**)MEM_alloc(roomTileSize.y * sizeof(u8*));
	for (u16 i = 0; i < roomTileSize.y; i++) {
		currentMap[i] = (u8*)MEM_alloc(roomTileSize.x);
		memcpy(currentMap[i], map[i], roomTileSize.x);
	}
}

u16 getTileValue(s16 x, s16 y) {
	if (x >= roomTileSize.x || x < 0 || y < 0 || y >= roomTileSize.y)
		return 0;

	//If the position is inside the collision map return the value of the tile from it
	return currentMap[y][x];
}