#ifndef _LEVEL_H_
#define _LEVEL_H_


#define MAP_WIDTH           10240
#define MAP_HEIGHT          1280

#define MIN_POSX            FIX32(10L)
#define MAX_POSX            FIX32(MAP_WIDTH - 100)
#define MAX_POSY            FIX32(MAP_HEIGHT - 356)


extern Map *bgb;
extern Map *bga;

extern s16 mapMetaTilePosX[2];
extern s16 mapMetaTilePosY[2];


u16 LEVEL_init(u16 vramIndex);

void LEVEL_updateMapAlternate(VDPPlane plane, Map* map, s16 xmt, s16 ymt);
void LEVEL_updateVDPScroll();

void LEVEL_onVBlank(void);
void LEVEL_handleInput(u16 value);


#endif // _LEVEL_H_
