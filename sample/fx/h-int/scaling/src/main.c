
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

    // Fill screen assuming 32 by 40 tiles
    for (u16 y=0; y<32; y+=h)
    {
        for (s16 x=-w; x<40; x+=w)
        {
            VDP_drawImageEx( BG_B,
                             &image_sgdk_logo,
                             TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USERINDEX),
                             x, y,
                             FALSE,
                             DMA );
        }
    }
}



// -------------------------------------------------------------------------
//  Interrupt handlers
// -------------------------------------------------------------------------

static u8    lineDisplay  = 0;          // line position on display screen
static fix16 lineGraphics = 0;          // line position in graphics texture
static fix16 scroll       = 0;          // scrolling offset
static fix16 scale        = FIX16(6.0); // scaling factor

void HIntHandler()
{
    // Set line to display
    VDP_setVerticalScroll(BG_B, fix16ToInt(lineGraphics) - lineDisplay);

    // Determine next graphics line to display (+1 means image is unscaled)
    lineGraphics += scale;

    // Count raster lines
    lineDisplay++;

    // Decrease scaling factor each line
    scale -= max(scale >> 6, FIX16(0.02));
}

void VIntHandler()
{
    // Make sure HInt always starts with line 0
    lineDisplay = 0;

    // Reset first line we want to display
    lineGraphics = scroll;

    // Decrease scrolling offset, reset after 64 lines
    scroll = (scroll - FIX16(1)) % FIX16(64);

    // Reset scaling factor
    scale = FIX16(6.0);
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
    VDP_setPaletteColors(0, palette_black, 64);

    // Setup graphics
    setupBackground();

    // Setup interrupt handlers
    SYS_disableInts();
    {
        VDP_setHIntCounter(0);
        VDP_setHInterrupt(1);
        SYS_setHIntCallback(HIntHandler);
        SYS_setVIntCallback(VIntHandler);
    }
    SYS_enableInts();

    // Fade in graphics
    PAL_fadeIn(0, 15, palette_sgdk_logo.data, 32, FALSE);

    //
    // Display loop
    //

    while (TRUE)
    {
        // Empty, because all work is done by the interrupt handlers
        SYS_doVBlankProcess();
    }
}
