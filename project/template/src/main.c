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
    SPR_init();
    JOY_setEventHandler(&joyEvent);
    // re enable interrupts
    SYS_enableInts();

    XGM_startPlay(mus_actraiser);


    SPR_addSprite(
            &spr_donut,                                                 // sprite data
            144, // x
            96, // y
            TILE_ATTR(PAL1, FALSE, FALSE, FALSE));  
            
    while(TRUE){        
        SYS_doVBlankProcess();
        SPR_update();
    }

    return 0;
}



static void joyEvent(u16 joy, u16 changed, u16 state)
{
}