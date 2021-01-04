/******************************************************************************
  DEMO TEST SPOTLIGHT EFFECT (LINE SCROLL + SHADOW MODE)

******************************************************************************/
#include "genesis.h"

// CONSTANTS ==================================================================

// Some data to deal with graphical data
#define NUM_COLUMNS     40
#define NUM_ROWS        28
#define NUM_LINES       NUM_ROWS * 8
#define NUM_SPRITES     20

#define NUM_TILES_TILESET   2

// Places where we place tiles in Genesis VRAM
#define VRAM_POS_BRICK_A            TILE_USERINDEX + 0
#define VRAM_POS_BRICK_B            TILE_USERINDEX + 1
#define VRAM_POS_TILE_VOID          TILE_SYSTEMINDEX

// How many pixels bottom line is going to swing
#define SPOTLIGHT_SWING_RANGE       50

// Some other data
#define SPOTLIGHT_WIDTH             8
#define NUM_SPOTLIGHTS              NUM_COLUMNS / SPOTLIGHT_WIDTH

// TILESET ====================================================================
const u32 background_tileset[NUM_TILES_TILESET*8] =
{
    // TILE 1: brick left half
   0x11211111, 0x12222222, 0x12422222, 0x12242222, 0x12422222, 0x12222222, 0x12222222, 0x33333333,
   // TILE 2: brick right half
    0x11111113, 0x22222223, 0x22222223, 0x22222223, 0x22222223, 0x22222223, 0x22222223, 0x32333333
};

const u32 sprite_tileset[16] =
{
   0x000FF000, 0x00FFFF00, 0x0FFFFFF0, 0xFFFFFFFF, 0xFFFFFFFF, 0x0FFFFFF0, 0x00FFFF00, 0x000FF000,
   0x000EE000, 0x00EEEE00, 0x0EEEEEE0, 0xEEEEEEEE, 0xEEEEEEEE, 0x0EEEEEE0, 0x00EEEE00, 0x000EE000
};


// PALETTE ====================================================================
const u16 background_palette[16] = {
   0x0000,0x06CE,0x044E,0x0008,0x008C,0x0000,0x0000,0x0000,
   0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
};


// MACROS =====================================================================
#define DrawWall(); \
    for(row = 0; row < NUM_ROWS; row += 2) \
    for(column = 0; column < NUM_COLUMNS; column += 2) \
    { \
       VDP_setTileMapXY( BG_B, VRAM_POS_BRICK_A, column,   row); \
       VDP_setTileMapXY( BG_B, VRAM_POS_BRICK_B, column+1, row); \
       VDP_setTileMapXY( BG_B, VRAM_POS_BRICK_B, column,   row+1); \
       VDP_setTileMapXY( BG_B, VRAM_POS_BRICK_A, column+1, row+1); \
    }

#define DrawSpotlights(); \
    for(row = 0; row < NUM_ROWS; row++) \
    for(column = 0; column < NUM_SPOTLIGHTS; column++) \
    for(tile_spotlight = 0; tile_spotlight < SPOTLIGHT_WIDTH; tile_spotlight++) \
    { \
       VDP_setTileMapXY( BG_A, TILE_ATTR_FULL(PAL0, column%2, 0, 0, VRAM_POS_TILE_VOID), \
          (column * SPOTLIGHT_WIDTH) + tile_spotlight, row); \
    }

#define InitializeScrollTable(); \
    for(i = 0; i < NUM_LINES; i++) line_scroll_data[i] = FIX16(0);

#define InitializeSpeedTable(); \
    line_speed_data[0] = FIX16(0.05); \
    for(i = 1; i < NUM_LINES; i++) \
       line_speed_data[i] = fix16Add(line_speed_data[i-1], FIX16(0.02));

//=============================================================================
// MAIN =======================================================================
//=============================================================================
int main()
{
    // Local data _____________________________________________________________

    // Indexes
    u16 row, column;     // Horizontal and vertical tile placement index/coords
    u16 tile_spotlight;  // Horizontal tile counter for drawing spotlights
    u16 i;               // General counter

    // Line scroll buffers
    fix16 line_scroll_data[NUM_LINES];  // Current line scroll values
    fix16 line_speed_data[NUM_LINES];   // Line scroll speeds
    s16 aux[NUM_LINES];                 // Needed for VDP_setHorizontalScrollLine
    s16 spr_pos[NUM_SPRITES * 2];       // Sprites position
    s16 *sp;


    // Initialization lot _____________________________________________________

    // Scroll
    InitializeScrollTable();
    InitializeSpeedTable();

    // Init sprites position
    VDP_resetSprites();

    sp = spr_pos;
    for(i = 0; i < NUM_SPRITES; i++)
    {
        *sp = 20 + (i * 10);
        VDP_setSpriteFull((i * 2) + 0, *sp, 20 + (i * 8), SPRITE_SIZE(1, 1), TILE_ATTR_FULL(PAL3, 0, 0, 0, TILE_USERINDEX + 2), (i * 2) + 1);
        sp++;

        *sp = 300 - (i * 10);
        VDP_setSpriteFull((i * 2) + 1, *sp, 20 + (i * 8), SPRITE_SIZE(1, 1), TILE_ATTR_FULL(PAL3, 0, 0, 0, TILE_USERINDEX + 3), (i == (NUM_SPRITES - 1))?0:(i * 2) + 2);
        sp++;
    }


    // Process ________________________________________________________________

    // Screen setting
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);
    VDP_setHilightShadow(1);    // Hilight/shadow activation

    // Loading tile stuff and color data into VRAM/CRAM
    VDP_setPalette(PAL0, background_palette);
    VDP_loadTileData(background_tileset, TILE_USERINDEX, NUM_TILES_TILESET, 1);
    VDP_loadTileData(sprite_tileset, TILE_USERINDEX + 2, 2, 1);

    VDP_updateSprites(NUM_SPRITES, TRUE);

    // Drawing
    DrawWall();
    DrawSpotlights();


    // MAIN LOOP ______________________________________________________________
    while(1)
    {
        // update scroll position
        for(i = 0; i < NUM_LINES; i++)
        {
            // Sum the speed value
            line_scroll_data[i] = fix16Add(line_scroll_data[i], line_speed_data[i]);

            // Rebound when movement of bottom line reaches its left or right boundary
            if(line_scroll_data[NUM_LINES-1] >= FIX16(SPOTLIGHT_SWING_RANGE)
            || line_scroll_data[NUM_LINES-1] <= FIX16(-SPOTLIGHT_SWING_RANGE))
               line_speed_data[i] *= -1;

            // An auxiliar "regular integer" buffer is needed for VDP_setHorizontalScrollLine
            aux[i] = fix16ToInt(line_scroll_data[i]);

        }// end for(NUM_LINES)

        // Update sprites position
        sp = spr_pos;
        for(i = 0; i < NUM_SPRITES; i++)
        {
            *sp = *sp + (1 + (i >> 2));
            if (*sp > 300) *sp = 20;
            VDP_setSpritePosition((i * 2) + 0, *sp, 20 + (i * 8));
            sp++;

            *sp = *sp - (1 + (i >> 2));
            if (*sp < 20) *sp = 300;
            VDP_setSpritePosition((i * 2) + 1, *sp, 20 + (i * 8));
            sp++;
        }

        // send to VDP with DMA queue
        VDP_updateSprites(NUM_SPRITES, TRUE);

        // Drawing/movement speed down
        SYS_doVBlankProcess();

        // Set Horizontal Scroll and update sprites during blank period
        VDP_setHorizontalScrollLine(BG_A, 0, aux, NUM_LINES, 1);

    } // end main loop

    return 0;   // Ok ... we'll return something

} // end main()
