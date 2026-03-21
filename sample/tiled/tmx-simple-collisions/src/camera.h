#ifndef HEADER_CAMERA
#define HEADER_CAMERA

#include "typedefs.h"


// Global camera instance
extern Camera camera;
// Background scroll scale for far/background layer (shift right)
extern Map *fgMap;
extern Map *bgMap;

void Camera_SetMaxScrollArea(u16 width, u16 height);
void Camera_Update();

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

