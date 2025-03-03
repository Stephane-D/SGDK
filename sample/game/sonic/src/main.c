#include <genesis.h>

#include "player.h"
#include "entities.h"
#include "hud.h"
#include "level.h"
#include "camera.h"

#include "res_gfx.h"
#include "res_sound.h"


bool paused;

// forward
static void handleInput();
static void joyEvent(u16 joy, u16 changed, u16 state);
static void vblank();


int main(bool hard)
{
    u16 palette[64];
    u16 ind;

    paused = FALSE;

    // initialization
    VDP_setScreenWidth320();
    // set all palette to black
    PAL_setColors(0, (u16*) palette_black, 64, CPU);

    // need to increase a bit DMA buffer size to init both plan tilemap and sprites
    DMA_setBufferSize(10000);
    DMA_setMaxTransferSize(10000);

    // init sprite engine with default parameters
    SPR_init();

    ind = TILE_USER_INDEX;
    ind += LEVEL_init(ind);
    CAMERA_init();
    ind += PLAYER_init(ind);
    ind += ENTITIES_init(ind);
    ind += HUD_init(ind);

    // set camera position
    CAMERA_centerOn(160, 100);
    // update sprite
    SPR_update();
    // and init map
    SYS_doVBlankProcess();

    // start music
    XGM2_play(sonic_music);
    XGM2_fadeIn(30);

    // can restore default DMA buffer size
    DMA_setBufferSizeToDefault();
    DMA_setMaxTransferSizeToDefault();

    // prepare palettes (BGB image contains the 4 palettes data)
    memcpy(&palette[0], palette_all.data, 64 * 2);
//    memcpy(&palette[16], bga_image.palette->data, 16 * 2);
//    memcpy(&palette[32], sonic_sprite.palette->data, 16 * 2);
//    memcpy(&palette[48], enemies_sprite.palette->data, 16 * 2);

    // fade in
    PAL_fadeIn(0, (4 * 16) - 1, palette, 20, TRUE);

    // set joy and vblank handler
    JOY_setEventHandler(joyEvent);
    SYS_setVBlankCallback(vblank);

    // just to monitor frame CPU usage
    SYS_showFrameLoad(TRUE);

    while(TRUE)
    {
        // first
        handleInput();

        if (!paused)
        {
            // update player first
            PLAYER_update();
            // then set camera from player position
            CAMERA_centerOn(F32_toInt(posX), F32_toInt(posY));

            // then we can update entities
            ENTITIES_update();

            // better to do it separately, when camera position is up to date
            PLAYER_updateScreenPosition();
            ENTITIES_updateScreenPosition();
        }

        // update sprites
        SPR_update();

        // sync frame and do vblank process
        SYS_doVBlankProcess();
    }

    // release maps
    MEM_free(bga);
    MEM_free(bgb);

    return 0;
}


static void handleInput()
{
    u16 value = JOY_readJoypad(JOY_1);

    // game is paused ?
    if (paused)
    {
        // adjust physics settings
        HUD_handleInput(value);
    }
    else
    {
        // can affect gameplay
        PLAYER_handleInput(value);
    }
}

static void joyEvent(u16 joy, u16 changed, u16 state)
{
    // START button state changed --> pause / unpause
    if (changed & state & BUTTON_START)
    {
        paused = !paused;
        HUD_setVisibility(paused);
    }

    // can't do more in paused state
    if (paused) return;

    // handle player / camera / level joy actions
    PLAYER_doJoyAction(joy, changed, state);
    CAMERA_doJoyAction(joy, changed, state);
    LEVEL_doJoyAction(joy, changed, state);
}

static void vblank()
{
    // handle vblank stuff
    LEVEL_onVBlank();
}