#include <genesis.h>

#include "gfx.h"
#include "sprite.h"
#include "sound.h"
#include "dma.h"

#define SFX_JUMP            64
#define SFX_ROLL            65
#define SFX_STOP            66

#define ANIM_STAND          0
#define ANIM_WAIT           1
#define ANIM_WALK           2
#define ANIM_RUN            3
#define ANIM_BRAKE          4
#define ANIM_UP             5
#define ANIM_CROUNCH        6
#define ANIM_ROLL           7

#define MAX_SPEED_MAX       FIX32(20L)
#define MAX_SPEED_MIN       FIX32(1L)
#define MAX_SPEED_DEFAULT   FIX32(8L)

#define RUN_SPEED           FIX32(6L)
#define BRAKE_SPEED         FIX32(2L)

#define JUMP_SPEED_MIN      FIX32(4L)
#define JUMP_SPEED_MAX      FIX32(22L)
#define JUMP_SPEED_DEFAULT  FIX32(7.8L)

#define GRAVITY_MIN         FIX32(0.15)
#define GRAVITY_MAX         FIX32(0.8)
#define GRAVITY_DEFAULT     FIX32(0.32)

#define ACCEL               FIX32(0.1)
#define DE_ACCEL            FIX32(0.15)

#define MAP_WIDTH           10240
#define MAP_HEIGHT          1280

#define MIN_POSX            FIX32(10L)
#define MAX_POSX            FIX32(MAP_WIDTH - 100)
#define MAX_POSY            FIX32(MAP_HEIGHT - 356)


// forward
static void handleInput();
static void joyEvent(u16 joy, u16 changed, u16 state);

static void setSpritePosition(Sprite* sprite, s16 posX, s16 posY);

static void updateBarsVisitility();
static void updateBar(Sprite* bar, f32 min, f32 max, f32 current);
static void updatePhysic();
static void updateAnim();

static void updateCameraPosition();
static void setCameraPosition(s16 x, s16 y);

static void updateMap(VDPPlane plane, Map* map, s16 xt, s16 yt);
static void updateVDPScroll();

static void frameChanged(Sprite* sprite);


// 42 * 32 = complete tilemap update; * 2 as we have 2 full plans to update potentially
// used for alternate map update mode
u16 tilemapBuf[42 * 32 * 2];
u16 bufOffset;

// player (sonic) sprite
Sprite* player;
// enemies sprites
Sprite* enemies[2];

// Speed, Jump and Gravity interface
Sprite* bars[3];

// maps (BGA and BGB) position (tile) for alternate method
s16 mapMetaTilePosX[2];
s16 mapMetaTilePosY[2];
// maps (BGA and BGB)
Map *bgb;
Map *bga;

// absolute camera position (pixel)
s16 camPosX;
s16 camPosY;
// require scroll update
bool scrollNeedUpdate;

// physic variables
fix32 maxSpeed;
fix32 jumpSpeed;
fix32 gravity;

// position and movement variables
fix32 posX;
fix32 posY;
fix32 movX;
fix32 movY;
s16 xOrder;
s16 yOrder;

// enemies positions and move direction
fix32 enemiesPosX[2];
fix32 enemiesPosY[2];
s16 enemiesXOrder[2];

// animation index table for enemies (static VRAM loading)
u16** sprTileIndexes[2];
// BG start tile index
u16 bgBaseTileIndex[2];

// maintain X button to use alternate MAP update mode
bool alternateScrollMethod;
bool paused;

int main(u16 hard)
{
    u16 palette[64];
    u16 ind;
    u16 numTile;

    // initialization
    VDP_setScreenWidth320();

    // init SFX
    XGM_setPCM(SFX_JUMP, sonic_jump_sfx, sizeof(sonic_jump_sfx));
    XGM_setPCM(SFX_ROLL, sonic_roll_sfx, sizeof(sonic_roll_sfx));
    XGM_setPCM(SFX_STOP, sonic_stop_sfx, sizeof(sonic_stop_sfx));
    // start music
    XGM_startPlay(sonic_music);

    // init sprite engine with default parameters
    SPR_init();

    // set all palette to black
    VDP_setPaletteColors(0, (u16*) palette_black, 64);

    // load background tilesets in VRAM
    ind = TILE_USERINDEX;
    bgBaseTileIndex[0] = ind;
    VDP_loadTileSet(&bga_tileset, ind, DMA);
    ind += bga_tileset.numTile;
    bgBaseTileIndex[1] = ind;
    VDP_loadTileSet(&bgb_tileset, ind, DMA);
    ind += bgb_tileset.numTile;

    // initialize variables
    bufOffset = 0;

    alternateScrollMethod = FALSE;          // by default we use the easy MAP_scrollTo(..) method
    paused = FALSE;

    // BGB/BGA tile position (force refresh)
    mapMetaTilePosX[0] = -42;
    mapMetaTilePosY[0] = 0;
    mapMetaTilePosX[1] = -42;
    mapMetaTilePosY[1] = 0;
    // camera position (force refresh)
    camPosX = -1;
    camPosY = -1;
    scrollNeedUpdate = FALSE;

    // default speeds
    maxSpeed = MAX_SPEED_DEFAULT;
    jumpSpeed = JUMP_SPEED_DEFAULT;
    gravity = GRAVITY_DEFAULT;

    // set main sprite position (camera position may be adjusted depending it)
    posX = FIX32(48L);
    posY = MAX_POSY;
    movX = FIX32(0);
    movY = FIX32(0);
    xOrder = 0;
    yOrder = 0;

    // enemies position
    enemiesPosX[0] = FIX32(300L);
    enemiesPosY[0] = MAX_POSY - FIX32(100);
    enemiesPosX[1] = FIX32(128L);
    enemiesPosY[1] = MAX_POSY + FIX32(5);
    enemiesXOrder[0] = -1;
    enemiesXOrder[1] = 1;

    // init backgrounds
    bga = MAP_create(&bga_map, BG_A, TILE_ATTR_FULL(0, FALSE, FALSE, FALSE, bgBaseTileIndex[0]));
    bgb = MAP_create(&bgb_map, BG_B, TILE_ATTR_FULL(0, FALSE, FALSE, FALSE, bgBaseTileIndex[1]));

    // init scrolling
    updateCameraPosition();
    if (scrollNeedUpdate)
    {
        updateVDPScroll();
        scrollNeedUpdate = FALSE;
    }

    // update camera position
    SYS_doVBlankProcess();
    // reset tilemap buffer position after update
    bufOffset = 0;

    // init sonic sprite
    player = SPR_addSprite(&sonic_sprite, fix32ToInt(posX) - camPosX, fix32ToInt(posY) - camPosY, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    // init enemies sprites
    enemies[0] = SPR_addSprite(&enemy01_sprite, fix32ToInt(enemiesPosX[0]) - camPosX, fix32ToInt(enemiesPosY[0]) - camPosY, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    enemies[1] = SPR_addSprite(&enemy02_sprite, fix32ToInt(enemiesPosX[1]) - camPosX, fix32ToInt(enemiesPosY[1]) - camPosY, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));

    // Speed, Jump and Gravity setting interface
    bars[0] = SPR_addSprite(&sbar_sprite, 10, 180, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    bars[1] = SPR_addSprite(&jbar_sprite, 18, 180, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    bars[2] = SPR_addSprite(&gbar_sprite, 26, 180, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));

    // disable auto tile upload for enemies sprites as we will pre-load all animation frams in VRAM for them
    SPR_setAutoTileUpload(enemies[0], FALSE);
    SPR_setAutoTileUpload(enemies[1], FALSE);
    // set frame change callback for enemies so we can update tile index easily
    SPR_setFrameChangeCallback(enemies[0], &frameChanged);
    SPR_setFrameChangeCallback(enemies[1], &frameChanged);

    // pre-load all animation frames into VRAM for enemies
    sprTileIndexes[0] = SPR_loadAllFrames(&enemy01_sprite, ind, &numTile);
    ind += numTile;
    sprTileIndexes[1] = SPR_loadAllFrames(&enemy02_sprite, ind, &numTile);
    ind += numTile;

    // store enemy 'sprTileIndexes' table index in 'data' field (can be used freely)
    enemies[0]->data = 0;
    enemies[1]->data = 1;

    // update BAR sprites
    updateBar(bars[0], MAX_SPEED_MIN, MAX_SPEED_MAX, maxSpeed);
    updateBar(bars[1], JUMP_SPEED_MIN, JUMP_SPEED_MAX, jumpSpeed);
    updateBar(bars[2], GRAVITY_MIN, GRAVITY_MAX, gravity);
    updateBarsVisitility();

    SPR_update();

    // prepare palettes (BGB image contains the 4 palettes data)
    memcpy(&palette[0], palette_all.data, 64 * 2);
//    memcpy(&palette[16], bga_image.palette->data, 16 * 2);
//    memcpy(&palette[32], sonic_sprite.palette->data, 16 * 2);
//    memcpy(&palette[48], enemies_sprite.palette->data, 16 * 2);

    // fade in
    PAL_fadeIn(0, (4 * 16) - 1, palette, 20, FALSE);

    JOY_setEventHandler(joyEvent);

    // just to monitor frame CPU usage
    SYS_showFrameLoad(TRUE);

//    reseted = FALSE;

    while(TRUE)
    {
        handleInput();

        if (!paused)
        {
            // update internal sprite position
            updatePhysic();
            updateAnim();
        }

        // update sprites
        SPR_update();

        // sync frame and do vblank process
        SYS_doVBlankProcess();
        // reset tilemap buffer position after update
        bufOffset = 0;

        // needed only for alternate MAP update method
        if (scrollNeedUpdate)
        {
            updateVDPScroll();
            scrollNeedUpdate = FALSE;
        }

//        KLog_U1("CPU usage = ", SYS_getCPULoad());
    }

    // release maps
    MEM_free(bga);
    MEM_free(bgb);

    return 0;
}


static void updateBarsVisitility()
{
    if (paused)
    {
        SPR_setVisibility(bars[0], VISIBLE);
        SPR_setVisibility(bars[1], VISIBLE);
        SPR_setVisibility(bars[2], VISIBLE);
    }
    else
    {
        SPR_setVisibility(bars[0], HIDDEN);
        SPR_setVisibility(bars[1], HIDDEN);
        SPR_setVisibility(bars[2], HIDDEN);
    }
}

static void updateBar(Sprite* bar, f32 min, f32 max, f32 current)
{
    f32 levelf;
    s16 leveli;

    levelf = fix32Mul(current, FIX32(16));
    levelf = fix32Div(levelf, (max - min));
    levelf -= min;

    leveli = fix32ToInt(levelf);
    if (leveli < 0) leveli = 0;
    else if (leveli > 16) leveli = 16;

    SPR_setFrame(bar, leveli);
}

static void updatePhysic()
{
    u16 i;

    // sonic physic
    if (xOrder > 0)
    {
        movX += ACCEL;
        // going opposite side, quick breaking
        if (movX < 0) movX += ACCEL;

        if (movX >= maxSpeed) movX = maxSpeed;
    }
    else if (xOrder < 0)
    {
        movX -= ACCEL;
        // going opposite side, quick breaking
        if (movX > 0) movX -= ACCEL;

        if (movX <= -maxSpeed) movX = -maxSpeed;
    }
    else
    {
        if ((movX < FIX32(0.1)) && (movX > FIX32(-0.1)))
            movX = 0;
        else if ((movX < FIX32(0.3)) && (movX > FIX32(-0.3)))
            movX -= movX >> 2;
        else if ((movX < FIX32(1)) && (movX > FIX32(-1)))
            movX -= movX >> 3;
        else
            movX -= movX >> 4;
    }

    posX += movX;
    posY += movY;

    if (movY)
    {
        if (posY > MAX_POSY)
        {
            posY = MAX_POSY;
            movY = 0;
        }
        else movY += gravity;
    }

    if (posX >= MAX_POSX)
    {
        posX = MAX_POSX;
        movX = 0;
    }
    else if (posX <= MIN_POSX)
    {
        posX = MIN_POSX;
        movX = 0;
    }

    // enemies physic
    if (enemiesXOrder[0] > 0) enemiesPosX[0] += FIX32(1.5);
    else enemiesPosX[0] -= FIX32(1.5);
    if (enemiesXOrder[1] > 0) enemiesPosX[1] += FIX32(0.7);
    else enemiesPosX[1] -= FIX32(0.7);
    for(i = 0; i < 2; i++)
    {
        if ((enemiesPosX[i] >= MAX_POSX) || (enemiesPosX[i] <= MIN_POSX))
            enemiesXOrder[i] = -enemiesXOrder[i];
    }

    // update camera position (*after* player sprite position has been updated)
    updateCameraPosition();

    // set sprites position
    setSpritePosition(player, fix32ToInt(posX) - camPosX, fix32ToInt(posY) - camPosY);
    setSpritePosition(enemies[0], fix32ToInt(enemiesPosX[0]) - camPosX, fix32ToInt(enemiesPosY[0]) - camPosY);
    setSpritePosition(enemies[1], fix32ToInt(enemiesPosX[1]) - camPosX, fix32ToInt(enemiesPosY[1]) - camPosY);
}

static void setSpritePosition(Sprite* sprite, s16 x, s16 y)
{
    // clip out of screen sprites
    if ((x < -100) || (x > 320) || (y < -100) || (y > 240)) SPR_setVisibility(sprite, HIDDEN);
    else
    {
        SPR_setVisibility(sprite, VISIBLE);
        SPR_setPosition(sprite, x, y);
    }
}

static void updateAnim()
{
    // jumping
    if (movY) SPR_setAnim(player, ANIM_ROLL);
    else
    {
        if (((movX >= BRAKE_SPEED) && (xOrder < 0)) || ((movX <= -BRAKE_SPEED) && (xOrder > 0)))
        {
            if (player->animInd != ANIM_BRAKE)
            {
                SND_startPlayPCM_XGM(SFX_STOP, 1, SOUND_PCM_CH2);
                SPR_setAnim(player, ANIM_BRAKE);
            }
        }
        else if ((movX >= RUN_SPEED) || (movX <= -RUN_SPEED))
            SPR_setAnim(player, ANIM_RUN);
        else if (movX != 0)
            SPR_setAnim(player, ANIM_WALK);
        else
        {
            if (yOrder < 0)
                SPR_setAnim(player, ANIM_UP);
            else if (yOrder > 0)
                SPR_setAnim(player, ANIM_CROUNCH);
            else
                SPR_setAnim(player, ANIM_STAND);
        }
    }

    if (movX > 0) SPR_setHFlip(player, FALSE);
    else if (movX < 0) SPR_setHFlip(player, TRUE);

    // enemies
    if (enemiesXOrder[0] > 0) SPR_setHFlip(enemies[0], TRUE);
    else SPR_setHFlip(enemies[0], FALSE);
//    for(i = 0; i < 2; i++)
//    {
//        if (enemiesXOrder[i] > 0) SPR_setHFlip(sprites[i + 1], TRUE);
//        else SPR_setHFlip(sprites[i + 1], FALSE);
//    }
}

static void updateCameraPosition()
{
    // get player position (pixel)
    s16 px = fix32ToInt(posX);
    s16 py = fix32ToInt(posY);
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
            updateMap(BG_A, bga, x >> 4, y >> 4);
            // scrolling is slower on BGB, no vertical scroll (should be consisten with updateVDPScroll())
            updateMap(BG_B, bgb, x >> 7, y >> 9);

            // request VDP scroll update
            scrollNeedUpdate = TRUE;
        }
        else
        {
            // scroll maps
            MAP_scrollTo(bga, x, y);
            // scrolling is slower on BGB
            MAP_scrollTo(bgb, x >> 3, y >> 5);
        }

        // always store it to avoid full map update on method change
        mapMetaTilePosX[BG_A] = x >> 4;
        mapMetaTilePosY[BG_A] = y >> 4;
        mapMetaTilePosX[BG_B] = x >> 7;
        mapMetaTilePosY[BG_B] = y >> 9;
    }
}

// this is just to show how use the MAP_getTilemapRect(..) method
// if we weed to actually access tilemap data and do manual tilemap update to VDP
static void updateMap(VDPPlane plane, Map* map, s16 xmt, s16 ymt)
{
    // BGA = 0; BGB = 1
    s16 cxmt = mapMetaTilePosX[plane];
    s16 cymt = mapMetaTilePosY[plane];
    s16 deltaX = xmt - cxmt;
    s16 deltaY = ymt - cymt;

    // no update --> exit
    if ((deltaX == 0) && (deltaY == 0)) return;

    // clip to 21 metatiles column max (full screen update)
    if (deltaX > 21)
    {
        cxmt += deltaX - 21;
        deltaX = 21;
        deltaY = 0;
    }
    // clip to 21 metatiles column max (full screen update)
    else if (deltaX < -21)
    {
        cxmt += deltaX + 21;
        deltaX = -21;
        deltaY = 0;
    }
    // clip to 16 metatiles row max (full screen update)
    else if (deltaY > 16)
    {
        cymt += deltaY - 16;
        deltaY = 16;
        deltaX = 0;
    }
    // clip to 16 metatiles row max (full screen update)
    else if (deltaY < -16)
    {
        cymt += deltaY + 16;
        deltaY = -16;
        deltaX = 0;
    }

    if (deltaX > 0)
    {
        // update on right
        cxmt += 21;

        // need to update map column on right
        while(deltaX--)
        {
            MAP_getTilemapRect(map, cxmt, ymt, 1, 16, TRUE, tilemapBuf + bufOffset);
            VDP_setTileMapDataColumnFast(plane, tilemapBuf + bufOffset, (cxmt * 2) + 0, ymt * 2, 16 * 2, DMA_QUEUE);
            // next column
            bufOffset += 16 * 2;
            VDP_setTileMapDataColumnFast(plane, tilemapBuf + bufOffset, (cxmt * 2) + 1, ymt * 2, 16 * 2, DMA_QUEUE);
            // next column
            bufOffset += 16 * 2;
            cxmt++;
        }
    }
    else
    {
        // need to update map column on left
        while(deltaX++)
        {
            cxmt--;
            MAP_getTilemapRect(map, cxmt, ymt, 1, 16, TRUE, tilemapBuf + bufOffset);
            VDP_setTileMapDataColumnFast(plane, tilemapBuf + bufOffset, (cxmt * 2) + 0, ymt * 2, 16 * 2, DMA_QUEUE);
            // next column
            bufOffset += 16 * 2;
            VDP_setTileMapDataColumnFast(plane, tilemapBuf + bufOffset, (cxmt * 2) + 1, ymt * 2, 16 * 2, DMA_QUEUE);
            // next column
            bufOffset += 16 * 2;
        }
    }

    if (deltaY > 0)
    {
        // update at bottom
        cymt += 16;

        // need to update map row on bottom
        while(deltaY--)
        {
            MAP_getTilemapRect(map, xmt, cymt, 21, 1, FALSE, tilemapBuf + bufOffset);
            VDP_setTileMapDataRow(plane, tilemapBuf + bufOffset, (cymt * 2) + 0, (xmt * 2), 21 * 2, DMA_QUEUE);
            // next row
            bufOffset += 21 * 2;
            VDP_setTileMapDataRow(plane, tilemapBuf + bufOffset, (cymt * 2) + 1, (xmt * 2), 21 * 2, DMA_QUEUE);
            // next row
            bufOffset += 21 * 2;
            cymt++;
        }
    }
    else
    {
        // need to update map row on top
        while(deltaY++)
        {
            cymt--;
            MAP_getTilemapRect(map, xmt, cymt, 21, 1, FALSE, tilemapBuf + bufOffset);
            VDP_setTileMapDataRow(plane, tilemapBuf + bufOffset, (cymt * 2) + 0, (xmt * 2), 21 * 2, DMA_QUEUE);
            // next row
            bufOffset += 21 * 2;
            VDP_setTileMapDataRow(plane, tilemapBuf + bufOffset, (cymt * 2) + 1, (xmt * 2), 21 * 2, DMA_QUEUE);
            // next row
            bufOffset += 21 * 2;
        }
    }

    mapMetaTilePosX[plane] = xmt;
    mapMetaTilePosY[plane] = ymt;
}

static void updateVDPScroll()
{
    VDP_setHorizontalScroll(BG_A, -camPosX);
    VDP_setHorizontalScroll(BG_B, (-camPosX) >> 3);
    VDP_setVerticalScroll(BG_A, camPosY);
    VDP_setVerticalScroll(BG_B, camPosY >> 5);
}

static void frameChanged(Sprite* sprite)
{
    // get enemy index (stored in data field)
    u16 enemyIndex = sprite->data;
    // get VRAM tile index for this animation of this sprite
    u16 tileIndex = sprTileIndexes[enemyIndex][sprite->animInd][sprite->frameInd];
    // manually set tile index for the current frame (preloaded in VRAM)
    SPR_setVRAMTileIndex(sprite, tileIndex);
}

static void handleInput()
{
    u16 value = JOY_readJoypad(JOY_1);

    // game is paused ? adjust physics settings
    if (paused)
    {
        if (value & BUTTON_RIGHT)
        {
            maxSpeed += FIX32(0.2);
            if (maxSpeed > MAX_SPEED_MAX) maxSpeed = MAX_SPEED_MAX;
            updateBar(bars[0], MAX_SPEED_MIN, MAX_SPEED_MAX, maxSpeed);
        }
        else if (value & BUTTON_LEFT)
        {
            maxSpeed -= FIX32(0.2);
            if (maxSpeed < MAX_SPEED_MIN) maxSpeed = MAX_SPEED_MIN;
            updateBar(bars[0], MAX_SPEED_MIN, MAX_SPEED_MAX, maxSpeed);
        }

        if (value & BUTTON_UP)
        {
            jumpSpeed += FIX32(0.3);
            if (jumpSpeed > JUMP_SPEED_MAX) jumpSpeed = JUMP_SPEED_MAX;
            updateBar(bars[1], JUMP_SPEED_MIN, JUMP_SPEED_MAX, jumpSpeed);
        }
        else if (value & BUTTON_DOWN)
        {
            jumpSpeed -= FIX32(0.3);
            if (jumpSpeed < JUMP_SPEED_MIN) jumpSpeed = JUMP_SPEED_MIN;
            updateBar(bars[1], JUMP_SPEED_MIN, JUMP_SPEED_MAX, jumpSpeed);
        }

        if (value & BUTTON_A)
        {
            gravity -= FIX32(0.005);
            if (gravity < GRAVITY_MIN) gravity = GRAVITY_MIN;
            updateBar(bars[2], GRAVITY_MIN, GRAVITY_MAX, gravity);
        }
        else if (value & BUTTON_B)
        {
            gravity += FIX32(0.005);
            if (gravity > GRAVITY_MAX) gravity = GRAVITY_MAX;
            updateBar(bars[2], GRAVITY_MIN, GRAVITY_MAX, gravity);
        }
    }
    // can affect gameplay
    else
    {
        if (value & BUTTON_UP) yOrder = -1;
        else if (value & BUTTON_DOWN) yOrder = +1;
        else yOrder = 0;

        if (value & BUTTON_LEFT) xOrder = -1;
        else if (value & BUTTON_RIGHT) xOrder = +1;
        else xOrder = 0;

        if (value & BUTTON_X) alternateScrollMethod = TRUE;
        else alternateScrollMethod = FALSE;
    }
}

static void joyEvent(u16 joy, u16 changed, u16 state)
{
    // START button state changed --> pause / unpause
    if (changed & state & BUTTON_START)
    {
        paused = !paused;
//        // change scroll method when pressing pause
//        if (paused)
//            alternateScrollMethod = !alternateScrollMethod;

        updateBarsVisitility();
    }

    // can't do more in paused state
    if (paused) return;

    if (changed & state & (BUTTON_A | BUTTON_B | BUTTON_C))
    {
        if (movY == 0)
        {
            movY = -jumpSpeed;
            SND_startPlayPCM_XGM(SFX_JUMP, 1, SOUND_PCM_CH2);
        }
    }
}
