// base SGDK include
#include <genesis.h>

// file header (not really useful here but just for the example)
#include "main.h"

// include our own resources
#include "res_gfx.h"
#include "res_snd.h"

static void joyEvent(u16 joy, u16 changed, u16 state);


int main(bool hardReset)
{
    // disable interrupt when accessing VDP
    SYS_disableInts();

    VDP_setTextPalette(PAL2);

    u16 vramIndex = TILE_USER_INDEX;

    vramIndex = loadDonut(vramIndex);

    // re enable interrupts
    SYS_enableInts();

    // set up the joy handler
    JOY_setEventHandler(&joyEvent);

    XGM_startPlay(mus_actraiser);

    PAL_setPalette(PAL2, spr_donut.palette->data, CPU);
    SPR_addSprite(&spr_donut, 0, 0, TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE, 0));
    //  Start !!!!
    while (TRUE)
    {
        SPR_update();
        // wait for the end of frame and do all the vblank process
        SYS_doVBlankProcess();
    }

    return 0;
}