#ifndef _LEVEL_H_
#define _LEVEL_H_


#define MAP_WIDTH           10240
#define MAP_HEIGHT          1280

#define MIN_POSX            5000
#define MAX_POSX            (MAP_WIDTH - 2000)
#define MIN_POSY            200
#define MAX_POSY            (MAP_HEIGHT - 356)


extern Map *bgb;
extern Map *bga;

extern s16 mapMetaTilePosX[2];
extern s16 mapMetaTilePosY[2];


u16 LEVEL_init(u16 vramIndex);
void LEVEL_end();

void LEVEL_updateMapAlternate(VDPPlane plane, Map* map, s16 xmt, s16 ymt);

void LEVEL_onVBlank(void);


#endif // _LEVEL_H_
