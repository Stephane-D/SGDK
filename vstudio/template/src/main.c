#include <genesis.h>

#include "gfx.h"
#include "sprite.h"
#include "sound.h"


int main()
{
    u16 palette[64];
    u16 ind;

    // disable interrupt when accessing VDP
    SYS_disableInts();

    // initialization
    VDP_setScreenWidth320();

    // init sprites engine
    SPR_init(16, 256, 256);

    // set all palette to black
    VDP_setPaletteColors(0, (u16*) palette_black, 64);

    SYS_enableInts();


    // fade in
    VDP_fadeIn(0, (4 * 16) - 1, palette, 20, FALSE);

    JOY_setEventHandler(joyEvent);


    while(TRUE)
    {
        SPR_update();

        VDP_waitVSync();
    }

    return 0;
}