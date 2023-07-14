#ifndef _CAMERA_H_
#define _CAMERA_H_


// camera position
extern s16 camPosX;
extern s16 camPosY;


void CAMERA_init(void);

void CAMERA_centerOn(s16 posX, s16 posY);

void CAMERA_doJoyAction(u16 joy, u16 changed, u16 state);


#endif // _CAMERA_H_
