
// *****************************************************************************
//  Scaling Example
//
//  Written in 2021 by Andreas Dietrich
// *****************************************************************************

// -----------------------------------------------------------------------------
//  Includes
// -----------------------------------------------------------------------------

// SGDK
#include <genesis.h>

// Resources
#include "resources.h"

// *****************************************************************************
//
//  Subroutines
//
// *****************************************************************************

void setupBackground()
{
    const u16 w = 64/8;
    const u16 h = 64/8;

    VDP_loadTileSet(image_sgdk_logo.tileset, TILE_USER_INDEX, CPU);
    // Fill screen assuming 32 by 40 tiles
    for (u16 y=0; y<32; y+=h)
    {
        for (u16 x=0; x<40; x+=w)
        {
            VDP_setTileMapEx(BG_B,
                             image_sgdk_logo.tilemap,
                             TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX),
                             x, y,
                             0, 0, w, h,
                             CPU);
        }
    }
}



// -------------------------------------------------------------------------
//  Interrupt handlers
// -------------------------------------------------------------------------

static vu16  lineDisplay   = 0;             // line position on display screen
static vfix16 lineGraphics = 0;             // line position in graphics texture
static fix16 scroll        = 0;             // scrolling offset
static vfix16 scale        = FIX16(6.0);    // scaling factor


HINTERRUPT_CALLBACK HIntHandler()
{
    // Set line to display
    VDP_setVerticalScroll(BG_B, F16_toInt(lineGraphics) - lineDisplay);

    // Determine next graphics line to display (+1 means image is unscaled)
    lineGraphics += scale;

    // Count raster lines
    lineDisplay++;

    // Decrease scaling factor each line
    scale -= max(scale >> 6, FIX16(0.02));
}

void VBlankHandler()
{
    // Reset to line 0
    lineDisplay = 0;

    // Reset first line we want to display
    lineGraphics = scroll;

    // Decrease scrolling offset, reset after 64 lines
    scroll = (scroll - FIX16(1)) % FIX16(64);

    // Reset scaling factor
    scale = FIX16(6.0);

    // Reset v-scroll
    VDP_setVerticalScroll(BG_B, F16_toInt(lineGraphics) - lineDisplay);
 }

// *****************************************************************************
//
//  Main
//
// *****************************************************************************

int main()
{
    //
    // Initalization
    //

    // Setup VDP
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    PAL_setColors(0, palette_black, 64, CPU);

    // Setup graphics
    setupBackground();

    // Setup interrupt handlers
    SYS_disableInts();
    {
        SYS_setVBlankCallback(VBlankHandler);
        SYS_setHIntCallback(HIntHandler);
        VDP_setHIntCounter(0);
        VDP_setHInterrupt(1);
    }
    SYS_enableInts();

    // Fade in graphics
    PAL_fadeIn(0, 15, image_sgdk_logo.palette->data, 32, FALSE);

    //
    // Display loop
    //

    while (TRUE)
    {
        // Empty, because all work is done by the interrupt/vblank handlers
        SYS_doVBlankProcess();
    }
}
