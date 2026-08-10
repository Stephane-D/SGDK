#include <genesis.h>
#include "window.h"

Window windowCentral;

// H-Interrupt callback - update window position at specified scanline during frame
HINTERRUPT_CALLBACK hIntHandler()
{
    SYS_disableInts();
    VDP_setWindowVPos(false, windowCentral.rowBottom);
    SYS_enableInts();
}

// VBlank interrupt callback - reset window position at start of frame
void vBlankHandler()
{
    SYS_disableInts();
    VDP_setWindowVPos(false, 0);
    SYS_enableInts();
}

// Enable horizontal window with H-Interrupt callbacks for split-screen display
void Window_SetCentered(const u8 x, const u8 y, const u8 width, const u8 height)
{
    // Calculate center position on screen
    windowCentral.columnLeft = x;
    windowCentral.rowTop = y;
    windowCentral.columnRight = x + width;
    windowCentral.rowBottom = y + height;
    windowCentral.firstScanline = y * 8 - 1;
    windowCentral.lastScanline = windowCentral.rowBottom * 8;
    windowCentral.enabled = true;

    SYS_disableInts();
    VDP_setHInterrupt(true);
    VDP_setHIntCounter(windowCentral.firstScanline);
    SYS_setVBlankCallback(vBlankHandler);
    SYS_setHIntCallback(hIntHandler);
    SYS_enableInts();
}

// Disable horizontal interrupt and window display
void Window_SetHidden()
{
    SYS_disableInts();
    VDP_setHInterrupt(false);
    SYS_setVBlankCallback(NULL);
    SYS_setHIntCallback(NULL);
    SYS_enableInts();
}