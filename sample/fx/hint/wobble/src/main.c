
// *****************************************************************************
//  Screen Wobble SGDK Sample
//
//    Main file
//
//  Written in 2020 by Andreas Dietrich
// *****************************************************************************

// -----------------------------------------------------------------------------
//  Includes
// -----------------------------------------------------------------------------

// SGDK
#include <genesis.h>

// Resources
#include "resources.h"

// Helpers
#include "helpers.h"

// -----------------------------------------------------------------------------
//  Constants
// -----------------------------------------------------------------------------

#define MENU_OFFSET_X          8
#define MENU_OFFSET_Y          8

#define AMPLITUDE_DEF          0.6250
#define AMPLITUDE_MIN          0.0000
#define AMPLITUDE_MAX          2.0000
#define AMPLITUDE_STEP         0.0625

#define WAVE_SPEED_DEF         1.0000
#define WAVE_SPEED_MIN         0.0000
#define WAVE_SPEED_MAX        10.0000
#define WAVE_SPEED_STEP        0.1250

#define ANGULAR_VELOCITY_DEF   4.0000
#define ANGULAR_VELOCITY_MIN   0.0000
#define ANGULAR_VELOCITY_MAX  16.0000
#define ANGULAR_VELOCITY_STEP  0.5000

// -----------------------------------------------------------------------------
//  Types
// -----------------------------------------------------------------------------

// Menu states
typedef enum MenuState
{
    MS_ON,
    MS_OFF,
    MS_FADEIN,
    MS_FADEOUT
}
MenuState;

// *****************************************************************************
//
//  Global variables
//
// *****************************************************************************

// Menu state
MenuState menuState       = MS_ON;
s16       menuX           = 0;
s16       menuV           = 1;

// Wave parameters
fix16     amplitude       = FIX16(AMPLITUDE_DEF);
fix16     waveSpeed       = FIX16(WAVE_SPEED_DEF);
fix16     angularVelocity = FIX16(ANGULAR_VELOCITY_DEF);
bool      resetWave       = FALSE;
bool      resetParameters = FALSE;

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
        for (s16 x=-w; x<40; x+=w)
        {
            // Shift each image row 3 tiles further
            const s16 shift = (3 * y/h) % w;
            VDP_drawImageEx( BG_B,
                             &image_sgdk_logo,
                             TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX),
                             x+shift, y,
                             FALSE,
                             DMA );
        }
}

// -----------------------------------------------------------------------------

void setupSprites()
{
    inline u16 attr(bool v, bool h, u16 idx) { return TILE_ATTR_FULL(PAL3, 0, v, h, idx); }

    // Backdrop building blocks
    const u16 idxCorner = loadSpriteData(&sprite_MenuBackdrop_Corner);
    const u16 idxEdgeH  = loadSpriteData(&sprite_MenuBackdrop_EdgeH);
    const u16 idxEdgeV  = loadSpriteData(&sprite_MenuBackdrop_EdgeV);
    const u16 idxCenter = loadSpriteData(&sprite_MenuBackdrop_Center);

    // First sprite row
    addSprite(  0,  0, attr(FALSE, FALSE, idxCorner));
    addSprite( 32,  0, attr(FALSE, FALSE, idxEdgeH) );
    addSprite( 64,  0, attr(FALSE, FALSE, idxEdgeH) );
    addSprite( 96,  0, attr(FALSE, FALSE, idxEdgeH) );
    addSprite(128,  0, attr(FALSE, FALSE, idxEdgeH) );
    addSprite(136,  0, attr(FALSE, FALSE, idxEdgeH) );
    addSprite(168,  0, attr(FALSE,  TRUE, idxCorner));

    // Second sprite row
    addSprite(  0, 32, attr(FALSE, FALSE, idxEdgeV) );
    addSprite( 32, 32, attr(FALSE, FALSE, idxCenter));
    addSprite( 64, 32, attr(FALSE, FALSE, idxCenter));
    addSprite( 96, 32, attr(FALSE, FALSE, idxCenter));
    addSprite(128, 32, attr(FALSE, FALSE, idxCenter));
    addSprite(136, 32, attr(FALSE, FALSE, idxCenter));
    addSprite(168, 32, attr(FALSE,  TRUE, idxEdgeV) );

    // Third sprite row
    addSprite(  0, 64, attr( TRUE, FALSE, idxCorner));
    addSprite( 32, 64, attr( TRUE, FALSE, idxEdgeH) );
    addSprite( 64, 64, attr( TRUE, FALSE, idxEdgeH) );
    addSprite( 96, 64, attr( TRUE, FALSE, idxEdgeH) );
    addSprite(128, 64, attr( TRUE, FALSE, idxEdgeH) );
    addSprite(136, 64, attr( TRUE, FALSE, idxEdgeH) );
    addSprite(168, 64, attr( TRUE,  TRUE, idxCorner));

    // Center sprites
    moveSprites(MENU_OFFSET_X*8, MENU_OFFSET_Y*8);
}

// -----------------------------------------------------------------------------

void setupMenu()
{
    // Setup plane
    VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, 0), 0, 0, 64, 28);
    VDP_setTextPlane(BG_A);
    VDP_setTextPriority(1);

    // Header / footer
    VDP_drawText("Wave Menu",             MENU_OFFSET_X+1, MENU_OFFSET_Y+1);
    VDP_drawText("START: Show/Hide Menu", MENU_OFFSET_X+1, MENU_OFFSET_Y+10);

    // Init and draw menu
    static MenuItem menuItems[5] =
    {
        { MENU_OFFSET_X+3, MENU_OFFSET_Y+3, "Amplitude        %04X", {{(void*)&amplitude,       FIX16(AMPLITUDE_MIN       ), FIX16(AMPLITUDE_MAX       ), FIX16(AMPLITUDE_STEP       )}}, MIT_S16  },
        { MENU_OFFSET_X+3, MENU_OFFSET_Y+4, "Wave Speed       %04X", {{(void*)&waveSpeed,       FIX16(WAVE_SPEED_MIN      ), FIX16(WAVE_SPEED_MAX      ), FIX16(WAVE_SPEED_STEP      )}}, MIT_S16  },
        { MENU_OFFSET_X+3, MENU_OFFSET_Y+5, "Angular Velocity %04X", {{(void*)&angularVelocity, FIX16(ANGULAR_VELOCITY_MIN), FIX16(ANGULAR_VELOCITY_MAX), FIX16(ANGULAR_VELOCITY_STEP)}}, MIT_S16  },
        { MENU_OFFSET_X+3, MENU_OFFSET_Y+7, "Reset Wave",            {{(void*)&resetWave,       0,0,0                                                                                 }}, MIT_BOOL },
        { MENU_OFFSET_X+3, MENU_OFFSET_Y+8, "Reset Parameters",      {{(void*)&resetParameters, 0,0,0                                                                                 }}, MIT_BOOL }
    };
    initMenu(menuItems, 5);
 }

// -----------------------------------------------------------------------------

void updateMenuState()
{
    // Update menu positions
    const u16 joyState = JOY_readJoypad(JOY_1);
    switch (menuState)
    {
        case MS_ON: // Menu active, can be operated
            updateMenuItems();
            if (joyState & BUTTON_START)
                menuState = MS_FADEOUT;
            break;

        case MS_OFF: // Menu inactive, waiting for START
            if (joyState & BUTTON_START)
                menuState = MS_FADEIN;
            break;

        case MS_FADEIN: // Decelerate incoming menu
            if (menuX > 0)
                menuX -= --menuV;
            else if (!(joyState & BUTTON_START))
                menuState = MS_ON;
            break;

        case MS_FADEOUT: // Accelerate outgoing menu
            if (menuX + MENU_OFFSET_X*8 < 320)
                menuX += menuV++;
            else if (!(joyState & BUTTON_START))
                menuState = MS_OFF;
            break;
    }
}

// -----------------------------------------------------------------------------

void updateMenuDisplay()
{
    // Update menu item graphics
    updateMenuVDP();

    // Move backdrop sprites and menu text
    moveSprites(menuX + MENU_OFFSET_X*8, MENU_OFFSET_Y*8);
    VDP_setHorizontalScroll(BG_A, min(320 - MENU_OFFSET_X*8, menuX));
}

// *****************************************************************************
//
//  Main
//
// *****************************************************************************

int main()
{
    // -------------------------------------------------------------------------
    //  Interrupt handlers
    // -------------------------------------------------------------------------

    static u8    lineDisplay  = 0; // line position on display screen
    static fix16 lineGraphics = 0; // line position in graphics texture
    static s16   lineBuffer[224];

    void HIntHandler()
    {
        // Set line to display
        VDP_setVerticalScroll(BG_B, fix16ToInt(lineGraphics) - lineDisplay);

        // Determine next graphics line to display (+1 means image is unscaled)
        lineGraphics += lineBuffer[lineDisplay++];
    }
    void VIntHandler()
    {
        // Make sure HInt always starts with line 0
        lineDisplay = lineGraphics = 0;
    }

    // -------------------------------------------------------------------------
    //  Main thread
    // -------------------------------------------------------------------------

    //
    // Initalization
    //

    // Setup VDP
    VDP_setHilightShadow(1);
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setPaletteColors(0, palette_black, 64);
    VDP_loadFontData(tileset_Font_Namco_Gradient.tiles, 96, CPU);

    // Setup graphics
    setupBackground();
    setupMenu();
    setupSprites();

    // Fade in graphics
    fadeInBlue(0,  15, palette_Font_Namco_Gradient.data, 8, 16);
    fadeInBlue(16, 31, palette_sgdk_logo.data, 8, 16);

    // Initialize line buffer (unscaled)
    memsetU16((u16*)lineBuffer, FIX16(1.0), 224);

    // Setup interrupt handlers
    SYS_disableInts();
    {
        VDP_setHIntCounter(0);
        VDP_setHInterrupt(1);
        SYS_setHIntCallback(HIntHandler);
        SYS_setVIntCallback(VIntHandler);
    }
    SYS_enableInts();

    //
    // Display loop
    //

    fix16 wave  = 0;
    fix16 angle = 0;

    while (TRUE)
    {
        updateMenuState();     // Update menu text (not accessing VDP)
        SYS_doVBlankProcess(); // Wait for vblank
        updateMenuDisplay();   // Update VDP

        // Wave reset handling
        if (resetWave || resetParameters)
        {
            memsetU16((u16*)lineBuffer, FIX16(1.0), 224);

            if (resetParameters)
            {
                amplitude       = FIX16(AMPLITUDE_DEF);
                waveSpeed       = FIX16(WAVE_SPEED_DEF);
                angularVelocity = FIX16(ANGULAR_VELOCITY_DEF);

                // Resetting the menu takes longer than vblank, make sure the handlers don't interfere
                SYS_disableInts();
                resetMenu();
                SYS_enableInts();
            }
            continue;
        }
        
        // Determine how many lines the wave advances
        wave += waveSpeed;
        u16 steps = fix16ToInt(wave);

        // Shift buffer to advance wave
        memcpy(lineBuffer, lineBuffer+steps, (224-steps)*2);

        // Compute new wave values and insert into buffer
        for (; steps > 0; steps--, wave -= FIX16(1.0))
        {
            angle += angularVelocity;
            lineBuffer[224-steps] = fix16Add( FIX16(1.0), 
                                              fix16Mul( amplitude,
                                                        sinFix16( fix16ToInt(angle) + 512 ) ) );
        }
    }
}
