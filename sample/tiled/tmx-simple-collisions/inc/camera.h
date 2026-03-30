#ifndef HEADER_CAMERA
#define HEADER_CAMERA

#include "typedefs.h"
#include "globals.h"


// Camera functions
void Camera_Init();
void Camera_Update();
void Camera_SetMaxScrollArea(u16 width, u16 height);

// Position getters
FORCE_INLINE s16 Camera_GetPosX(void)
{
    return camera.pos.x;
}

FORCE_INLINE s16 Camera_GetPosY(void)
{
    return camera.pos.y;
}

// Position setters
FORCE_INLINE void Camera_SetPos(s16 x, s16 y)
{
    camera.pos.x = x;
    camera.pos.y = y;
}

FORCE_INLINE void Camera_SetPosX(s16 x)
{
    camera.pos.x = x;
}

FORCE_INLINE void Camera_SetPosY(s16 y)
{
    camera.pos.y = y;
}

#endif //HEADER_CAMERA

