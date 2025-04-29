// *****************************************************************************
//
//  Console example
//
//  Written in 2022 by Andreas Dietrich
//
//  To build this example set MODULE_CONSOLE to 1 in config.h and rebuild the
//  SGDK library (build_lib.bat).
//
// *****************************************************************************

// *****************************************************************************
//
//  Includes
//
// *****************************************************************************

// SGDK
#include <genesis.h>

#if (MODULE_CONSOLE == 0)
#error "Set MODULE_CONSOLE to 1 in config.h and rebuild the SGDK library."
#endif

// Resources
#include "resources.h"

// stb
#ifndef STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_IMPLEMENTATION
#endif
#ifndef STB_SPRINTF_NOSTD
#define STB_SPRINTF_NOSTD
#endif
#ifndef STB_SPRINTF_NOUNALIGNED
#define STB_SPRINTF_NOUNALIGNED
#endif

#include "ext/stb/stb_sprintf.h"

// *****************************************************************************
//
//  Defines
//
// *****************************************************************************

// VDP addresses
#define HSCROLL_TABLE_ADDR 0xA800
#define SPRITE_LIST_ADDR   0xAC00
#define WINDOW_ADDR        0xB000
#define BGA_ADDR           0xC000
#define BGB_ADDR           0xE000

// *****************************************************************************
//
//  Subroutines
//
// *****************************************************************************

void setupBackground()
{
    // Background pattern size in tiles
    const u16 w = 64/8;
    const u16 h = 64/8;

    // Fill screen
    for (s16 x=0; x<16*w; x+=w)
        for (s16 y=-h; y<3*h; y+=h)
        {
            // Shift each image column 3 tiles further
            const s16 shift = 3*(x/w);
            VDP_drawImageEx(
                BG_B,
                &image_R_Type_BG,
                TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX),
                x, y+shift,
                FALSE,
                DMA
            );
        }
}

// -----------------------------------------------------------------------------

void setupForeground()
{
    // Foreground image occupies complete plane
    VDP_drawImageEx(
        BG_A,
        &image_R_Type_FG,
        TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX+64),
        0, 0,
        FALSE,
        DMA
    );
}

// -----------------------------------------------------------------------------

void setupWindow()
{
    // Clear visible window area on the right (16x28 tiles)
    VDP_fillTileMapRect(
        WINDOW,
        TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, 8),
        40-16,
        0,
        16,
        28
    );
}

// -----------------------------------------------------------------------------

void waitPressSTART()
{
    // Wait until START gets released
    while (JOY_readJoypad(JOY_1) & BUTTON_START)
        SYS_doVBlankProcess();

    // Wait until START is pressed
    while (!(JOY_readJoypad(JOY_1) & BUTTON_START))
        SYS_doVBlankProcess();
}

// *****************************************************************************
//
//  Demos
//
// *****************************************************************************

void demoBasics()
{
    CON_write("\n");
    CON_write("------------\n");
    CON_write("Console Test\n");
    CON_write("------------\n");
    CON_write("\n");
    CON_write("using stb_sprintf\n");
    CON_write("(http://github.com/nothings/stb)\n");
    CON_write(" allowed types: sc uidBboXx p AaGgEef n\n");
    CON_write(" lengths      : h ll j z t I64 I32 I\n");
    CON_write("\n");
    CON_write("supported control characters:\n");
    CON_write(" \\b : backspace\n");
    CON_write(" \\n : new line (line feed)\n");
    CON_write(" \\r : carriage return\n");
    CON_write(" \\t : horizontal tab\n");
    CON_write(" \\v : vertical tab\n");

    CON_setCursorPosition(5, 26);
    CON_write("Press <START> for printf demo\n");

    waitPressSTART();
}

// -----------------------------------------------------------------------------

void demoControlCharacters()
{
    CON_clear();

    CON_write("\nFirst a carriage return & new line\n");
    CON_write("then print an integer %d", 101);
    CON_write("\nstep\vstep\vstep\vstep\vstep\vstep\vstep\n");
    CON_write("   <- The number five goes here\r 5\n");
    CON_write("\n0         1         2         3\n");
    CON_write("0123456789012345678901234567890123456789\n");
    CON_write("T\tT\tT\tT\tT\n");
    CON_write("\n0\b22\b\b1");
    CON_write("\n%e \b %g \b %g\n" , 1234567.0, 1234567.0, 123456.0);

    CON_setCursorPosition(5, 26);
    CON_write("Press <START> for window demo\n");

    waitPressSTART();
}

// -----------------------------------------------------------------------------

void demoWindow()
{
    // Blank screen
    PAL_setColors(0, palette_black, 64, CPU);

    // Setup text
    VDP_loadFontData(tileset_Font_Namco_Opaque.tiles, 96, CPU);
    VDP_setTextPalette(PAL1);
    VDP_setTextPlane(WINDOW);

    // Prepare tile maps
    setupBackground();
    setupForeground();
    setupWindow();

    // Trigger fade in
    PAL_fadeIn( 1, 15, image_R_Type_BG.palette->data+1, 16, TRUE);
    PAL_setPaletteColors (16, &palette_Font_Namco_Opaque, CPU);

    // Hide window
    bool windowVisible = FALSE;

    // Use DMA_QUEUE to not interfere with othe DMA transfers
    CON_setTransferMethod(DMA_QUEUE);

    do
    {
        for (s16 x=0; x<(128*8); x++)
        {
            SYS_doVBlankProcess();

            // Horizontal scroll values
            const s16 xb = -x/2;
            const s16 xa = -x;

            VDP_setHorizontalScroll(BG_B, xb);
            VDP_setHorizontalScroll(BG_A, xa);

            // Wait a while then gradually show window
            if (x>144 && !windowVisible)
            {
                // Set window size
                const s16 w = x - 144;
                VDP_setWindowHPos(TRUE, 20-w);

                // Window fully visible
                if (w == 8)
                {
                    // Print header
                    CON_setConsoleSize((40-16)+1, 1, 14, 27);
                    CON_write(" <START> for\n");
                    CON_write(" assert test\n");

                    // Place console area below header
                    CON_setConsoleSize((40-16)+1, 4, 14, 24);
                    windowVisible = TRUE;
                }
            }

            // Write scroll values
            if (windowVisible)
            {
                CON_write("A:%04hd B:%04hd\n", -xa, -xb);

                // *************************************************************
                // Simulated assert
                // *************************************************************

                const u16 joyState = JOY_readJoypad(JOY_1);
                if (joyState & BUTTON_START)
                {
                    // Simulate wrong x value
                    x = 1025;
                }

                assert(x < 1024);

                // *************************************************************
            }
        }
    }
    while (TRUE);
}

// *****************************************************************************
//
//  Main
//
// *****************************************************************************

int main()
{
    // Setup screen
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    VDP_setPlaneSize(128, 32, TRUE);

    // VDP memory layout
    VDP_setBGAAddress         ( BGA_ADDR           );
    VDP_setBGBAddress         ( BGB_ADDR           );
    VDP_setWindowAddress      ( WINDOW_ADDR        );
    VDP_setSpriteListAddress  ( SPRITE_LIST_ADDR   );
    VDP_setHScrollTableAddress( HSCROLL_TABLE_ADDR );

    // Use stbsp_vsnprintf() as print callback to provide more options.
    // See http://github.com/nothings/stb for more details.
    //
    // allowed types: sc uidBboXx p AaGgEef n
    // lengths      : h ll j z t I64 I32 I\n");
    //
    CON_setVsnprintf(stbsp_vsnprintf);

    // Run demos
    demoBasics();
    demoControlCharacters();
    demoWindow();
}
