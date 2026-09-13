// base SGDK include
#include <genesis.h>

// file header (not really useful here but just for the example)
#include "main.h"

// include our own resources
#include "res_gfx.h"
#include "res_snd.h"


#define TABLE_LEN       220
#define MAX_DONUT       40
#define MAX_DONUT_ANIM  8

// forward declarations
static u16 loadStarField(u16 vramIndex);
static u16 loadDonut(u16 vramIndex);
static void animateStarfield(void);
static void animateDonut(void);

static void joyEvent(u16 joy, u16 changed, u16 state);

// scrolling tables
s16 scroll_PLAN_B[TABLE_LEN];
fix16 scroll_PLAN_B_F[TABLE_LEN];
fix16 scroll_speed[TABLE_LEN];

// donut sprites
Sprite* sprites[MAX_DONUT];
u16 animVramIndexes[MAX_DONUT_ANIM];

// donut animation variables
fix16 donutPhase;
fix16 donutPhaseSpeed;
fix16 donutAmplitude;
s16 donutAngleStep;
fix16 donutAmplitudeStep;


int main(bool hardReset)
{
    // disable interrupt when accessing VDP
    SYS_disableInts();

    VDP_setTextPalette(PAL2);

    u16 vramIndex = TILE_USER_INDEX;

    // load starfield image and donut sprite
    vramIndex = loadStarField(vramIndex);
    vramIndex = loadDonut(vramIndex);

    // re enable interrupts
    SYS_enableInts();

    // set up the joy handler
    JOY_setEventHandler(&joyEvent);

    XGM_startPlay(mus_actraiser);

    donutPhase = FIX16(0);
    donutPhaseSpeed = FIX16(2);
    donutAmplitude = FIX16(50);

    donutAngleStep = (360 * 2) / MAX_DONUT;
    donutAmplitudeStep = FIX16(3);

    //  Start !!!!
    while (TRUE)
    {
        // read joypad 1
        u16 button = JOY_readJoypad(JOY_1);

        // increase / decrease donut phase speed
        if (button & BUTTON_LEFT) donutPhaseSpeed -= FIX16(0.1);
        if (button & BUTTON_RIGHT) donutPhaseSpeed += FIX16(0.1);
        if (button & BUTTON_UP) donutAmplitude += FIX16(1);
        if (button & BUTTON_DOWN) donutAmplitude -= FIX16(1);

        animateStarfield();
        animateDonut();

        VDP_showCPULoad(0, 0);

        // update sprites
        SPR_update();
        // wait for the end of frame and do all the vblank process
        SYS_doVBlankProcess();
    }

    return 0;
}

static u16 loadStarField(u16 vramIndex)
{
    // Draw the foreground
    VDP_drawImageEx(BG_B, &img_starfield, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, vramIndex), 0, 0, TRUE, FALSE);
    vramIndex += img_starfield.tileset->numTile;

    // Set the proper scrolling mode (line by line)
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);

    // Create the scrolling offset table
    s16 s = 1;
    for(s16 i = 0; i < TABLE_LEN; i++)
    {
        s16 ns;

        scroll_PLAN_B_F[i] = FIX16(0);
        do
        {
            ns = -((random() & 0x7F) + 10);
        }
        while (ns == s);
        scroll_speed[i] = ns;
        s = ns;
    }

    return vramIndex;
}

static u16 loadDonut(u16 vramIndex)
{
    // load tilesets
    Animation* anim = spr_donut.animations[0];
    for(s16 i = 0; i < anim->numFrame; i++)
    {
        TileSet* tileset = anim->frames[i]->tileset;
        VDP_loadTileSet(tileset, vramIndex, TRUE);
        animVramIndexes[i] = vramIndex;
        vramIndex += tileset->numTile;
    }

    // init the sprite engine
    SPR_init();

    for(s16 i = 0; i < MAX_DONUT; i++)
    {
        // create sprite
        Sprite* spr = SPR_addSprite(&spr_donut, 0, 0, TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE, 0));

        // disable auto tile upload (all tiles are already loaded in VRAM)
        SPR_setAutoTileUpload(spr, FALSE);
        // set fixed tile index (disable automatic VRAM allocation)
        SPR_setVRAMTileIndex(spr, animVramIndexes[0]);
        // default position (out of visible area)
        SPR_setPosition(spr, 0, -64);
        // set depth
        //if ((i & 7) == 7) SPR_setDepth(spr, -1);
        //else SPR_setDepth(spr, spr->y);

        sprites[i] = spr;
    }

    // load sprite palette
    PAL_setPalette(PAL2, spr_donut.palette->data, CPU);
    // first update
    SPR_update();

    return vramIndex;
}


static void animateStarfield()
{
    for(s16 i = 0; i < TABLE_LEN; i++)
    {
        scroll_PLAN_B_F[i] += scroll_speed[i];
        scroll_PLAN_B[i] = F16_toInt(scroll_PLAN_B_F[i]);
    }

    // send hscroll table to VDP using DMA queue (will be done on vblank by SYS_doVBlankProcess())
   VDP_setHorizontalScrollLine(BG_B, 2, scroll_PLAN_B, TABLE_LEN, DMA_QUEUE);
}

static void animateDonut()
{
    // start angle and amplitude
    s16 angle = F16_toInt(donutPhase);
    fix16 amplitude = donutAmplitude;

    for(s16 i = 0; i < MAX_DONUT; i++)
    {
        Sprite *spr = sprites[i];
        // vtimer is the current frame counter
        const u16 frame = ((vtimer >> 2) + i) & 0x7;
        const fix16 donutAngleF = FIX16(angle);
        const fix16 x = F16_mul(F16_cos(donutAngleF), amplitude);
        const fix16 y = F16_mul(F16_sin(donutAngleF), amplitude / 2);

        SPR_setPosition(spr, (160 - 16) + F16_toInt(x), (112 - 16) + F16_toInt(y));
        //if (spr->depth != -1) SPR_setDepth(spr, spr->y);
        SPR_setFrame(spr, frame);
        SPR_setVRAMTileIndex(spr, animVramIndexes[frame]);

        // increment current angle and amplitude
        angle += donutAngleStep;
        // normalize angle
        if (angle < 0) angle += 360;
        else if (angle >= 360) angle -= 360;
        amplitude += donutAmplitudeStep;
    }

    donutPhase += donutPhaseSpeed;
    donutPhase = F16_normalizeAngle(donutPhase);
}

static void joyEvent(u16 joy, u16 changed, u16 state)
{
    if (joy == JOY_1)
    {
        if (changed & state & BUTTON_A) donutAngleStep += 2;
        if (changed & state & BUTTON_X) donutAngleStep -= 2;
        if (changed & state & BUTTON_B) donutAmplitudeStep += FIX16(1);
        if (changed & state & BUTTON_Y) donutAmplitudeStep -= FIX16(1);

        // normalize angle step
        if (donutAngleStep < 0) donutAngleStep += 360;
        else if (donutAngleStep >= 360) donutAngleStep -= 360;
    }
}

