#include "genesis.h"


int main(bool hardReset)
{
    VDP_drawText("Hello world !", 12, 12);

    while(TRUE)
    {
        // nothing to do here
        // ...

        // always call this method at the end of the frame
        SYS_doVBlankProcess();
    }

    return 0;
}
