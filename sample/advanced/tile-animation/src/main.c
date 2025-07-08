// *****************************************************************************
//  Tile Animation Sample
//
//  This example shows one way how you can implement animated tiles on BG plane.
//  To do this, we perform the following actions:
//   - On the tile map, instead of tiles that should be animated, we draw tile placeholders (these should be unique
//     tiles for each type of animation and not used for other purposes on the tile map).
//   - Prepare the map tileset so that the tile placeholders are located at the top of the tileset
//     (this will allow you to place the tile placeholders at the beginning of the VRAM user area when loading the tileset)
//   - Create separate tilesets (image files) for each tile animation frame
//     (tiles in the image should be located in accordance with the positions of their placeholders)
//
//  additional explanations in resources.res
//
//  writen by werton playskin 03/2025
// *****************************************************************************

#include <genesis.h>
#include "resources.h"

// define number of frames in tile animation
#define TILE_FRAMES_NUM             2

// define frame delay for tile animation
#define TILE_FRAME_DELAY            10

// define width and height of tile map in pixels (resolution of map image)
#define MAP_WIDTH                   864
#define MAP_HEIGHT                  320

// auxiliary macros
#define CAMERA_MAX_POS_X            (MAP_WIDTH - VDP_getScreenWidth())
#define CAMERA_MAX_POS_Y            (MAP_HEIGHT - VDP_getScreenHeight())

// defining camera Type (just for convenience)
typedef struct
{
    V2s16 pos;
    V2s16 prevPos;
} Camera;

// array of animated tileset definitions
const TileSet *tile_anim[] =
    {
        &animated_tileset_frame_0,
        &animated_tileset_frame_1
    };

// define camera
Camera camera;

// define pointer to BG Map
Map *bgaMap;

// define current frame index for tiles
s16 tileFrameIndex;

// define auxiliary frame counter
s16 frameTicks;


// just camera control
void CameraMoveByOffset(s16 x, s16 y)
{
    camera.pos.x += x;
    camera.pos.y += y;
    
    if (camera.pos.x < 0)
        camera.pos.x = 0;
    else if (camera.pos.x > CAMERA_MAX_POS_X)
        camera.pos.x = CAMERA_MAX_POS_X;
    
    if (camera.pos.y < 0)
        camera.pos.y = 0;
    else if (camera.pos.y > CAMERA_MAX_POS_Y)
        camera.pos.y = CAMERA_MAX_POS_Y;
    
    if ((camera.pos.x != camera.prevPos.x) || (camera.pos.y != camera.prevPos.y))
    {
        MAP_scrollTo(bgaMap, camera.pos.x, camera.pos.y);
        camera.prevPos = camera.pos;
    }
}

// processing the input for the camera
void HandleInput()
{
    u16 value = JOY_readJoypad(JOY_1);
    
    if (value & BUTTON_RIGHT)
        CameraMoveByOffset(4, 0);
    else if (value & BUTTON_LEFT)
        CameraMoveByOffset(-4, 0);
    
    if (value & BUTTON_UP)
        CameraMoveByOffset(0, -4);
    else if (value & BUTTON_DOWN)
        CameraMoveByOffset(0, 4);
}

// updating tileset of animated tiles
void TilesAnimationUpdate()
{
    if (frameTicks-- == 0)
    {
        // reset frame counter
        frameTicks = TILE_FRAME_DELAY;
        
        // increase frame index
        tileFrameIndex++;
        
        // reset frame index to 0 if reached last frame + 1
        if (tileFrameIndex == TILE_FRAMES_NUM)
            tileFrameIndex = 0;
        
        // load next frame of animated tileset to beginning of VRAM map aria
        VDP_loadTileSet(tile_anim[tileFrameIndex], bgaMap->baseTile, DMA);
    }
}

int main(bool hardReset)
{
    // do hard reset on soft reset (just for convenience)
    if (!hardReset)
        SYS_hardReset();
    
    // load map tileset
    VDP_loadTileSet(&bg_tileset, TILE_USER_INDEX, DMA);
    
    // create map
    bgaMap = MAP_create(&map_def, BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX));
    
    // load tileset of animated tiles (first frame)
    VDP_loadTileSet(tile_anim[tileFrameIndex], bgaMap->baseTile, DMA);
    
    // set camera start position
    CameraMoveByOffset(240, 72);
   
    // fade in PAL0 from black to bg palette
    PAL_fadeIn(0, 15, bg_pal.data, 30, TRUE);
    
    // main cycle
    while (TRUE)
    {
        // control camera
        HandleInput();
        
        // update tileset of animated tiles
        TilesAnimationUpdate();
        
        // do vblank process
        SYS_doVBlankProcess();
    }
    return (0);
}
