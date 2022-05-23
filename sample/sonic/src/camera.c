#include "genesis.h"

#include "camera.h"
#include "level.h"


// absolute camera position (pixel)
s16 camPosX;
s16 camPosY;

// maintain X button to use alternate MAP update mode
bool alternateScrollMethod;


// forward
static void setCameraPosition(s16 x, s16 y);


void CAMERA_init(void)
{
    // camera position (force refresh)
    camPosX = -1;
    camPosY = -1;

    alternateScrollMethod = FALSE;          // by default we use the easy MAP_scrollTo(..) method
}


void CAMERA_centerOn(s16 posX, s16 posY)
{
    // get entity position (pixel)
    s16 px = posX;
    s16 py = posY;
    // current sprite position on screen
    s16 px_scr = px - camPosX;
    s16 py_scr = py - camPosY;

    s16 npx_cam, npy_cam;

    // adjust new camera position
    if (px_scr > 240) npx_cam = px - 240;
    else if (px_scr < 40) npx_cam = px - 40;
    else npx_cam = camPosX;
    if (py_scr > 140) npy_cam = py - 140;
    else if (py_scr < 60) npy_cam = py - 60;
    else npy_cam = camPosY;

    // clip camera position
    if (npx_cam < 0) npx_cam = 0;
    else if (npx_cam > (MAP_WIDTH - 320)) npx_cam = (MAP_WIDTH - 320);
    if (npy_cam < 0) npy_cam = 0;
    else if (npy_cam > (MAP_HEIGHT - 224)) npy_cam = (MAP_HEIGHT - 224);

    // set new camera position
    setCameraPosition(npx_cam, npy_cam);
}

static void setCameraPosition(s16 x, s16 y)
{
    if ((x != camPosX) || (y != camPosY))
    {
        camPosX = x;
        camPosY = y;

        // alternate map update method ?
        if (alternateScrollMethod)
        {
            // update maps (convert pixel to metatile coordinate)
            LEVEL_updateMapAlternate(BG_A, bga, x >> 4, y >> 4);
            // scrolling is slower on BGB, no vertical scroll (should be consisten with updateVDPScroll())
            LEVEL_updateMapAlternate(BG_B, bgb, x >> 7, y >> 9);

            // request VDP scroll update at vsync
            VDP_setHorizontalScrollVSync(BG_A, -camPosX);
            VDP_setHorizontalScrollVSync(BG_B, (-camPosX) >> 3);
            VDP_setVerticalScrollVSync(BG_A, camPosY);
            VDP_setVerticalScrollVSync(BG_B, camPosY >> 5);
        }
        else
        {
            // scroll maps
            MAP_scrollTo(bga, x, y);
            // scrolling is slower on BGB
            MAP_scrollTo(bgb, x >> 3, y >> 5);

            // update it to avoid full map update on method change
            mapMetaTilePosX[BG_A] = x >> 4;
            mapMetaTilePosY[BG_A] = y >> 4;
            mapMetaTilePosX[BG_B] = x >> 7;
            mapMetaTilePosY[BG_B] = y >> 9;
        }
    }
}


void CAMERA_handleInput(u16 value)
{
    if (value & BUTTON_X) alternateScrollMethod = TRUE;
    else alternateScrollMethod = FALSE;
}