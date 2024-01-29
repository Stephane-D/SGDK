#ifndef _CAMERA_H_
#define _CAMERA_H_


// camera position
extern s16 camPosX;
extern s16 camPosY;


void CAMERA_init(bool lowLevelMethod);

void CAMERA_centerOn(s16 posX, s16 posY);


#endif // _CAMERA_H_
