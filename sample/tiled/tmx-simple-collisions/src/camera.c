#include <genesis.h>
#include "camera.h"
#include "defs.h"
#include "player.h"
#include "tile.h"

// Set max scroll area based on map size and screen size to prevent scrolling beyond map bounds
void Camera_SetMaxScrollArea(u16 mapWidth, u16 mapHeight)
{
    camera.scrollSize.x = mapWidth * MAP_TILE_SIZE - VDP_getScreenWidth();
    camera.scrollSize.y = mapHeight * MAP_TILE_SIZE - VDP_getScreenHeight();
}

// Camera update: center on player, clamp to map bounds, and scroll maps with parallax
void Camera_Update()
{
    // Calculate camera position centered on player and clamp to map bounds
    Camera_SetPosX(clamp(Player_GetPosX() + Player_GetWidth() / 2 - VDP_getScreenWidth() / 2, 0, camera.scrollSize.x));
    Camera_SetPosY(clamp(Player_GetPosY() + Player_GetHeight() / 2 - VDP_getScreenHeight() / 2, 0, camera.scrollSize.y));
    
    // Scroll foreground directly and background scaled down (parallax)
    MAP_scrollTo(fgMap, Camera_GetPosX(), Camera_GetPosY());
    MAP_scrollTo(bgMap, Camera_GetPosX() >> BG_SCROLL_SHIFT, Camera_GetPosY() >> BG_SCROLL_SHIFT);
}


