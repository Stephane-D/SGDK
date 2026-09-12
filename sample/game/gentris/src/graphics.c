#include <genesis.h>
#include "defs.h"
#include "global.h"
#include "graphics.h"


// Helper functions to apply block patterns and masks for tile generation
static inline void ApplyPatternFlat1(u16 tile)
{
    g_tileData[tile][1] |= 0xFFFFFF00;
    g_tileData[tile][2] |= 0xF0000000;
    g_tileData[tile][3] |= 0xF0000000;
    g_tileData[tile][4] |= 0xF0000000;
    g_tileData[tile][5] |= 0xF0000000;
    g_tileData[tile][6] |= 0xF0000000;
}

static inline void ApplyPatternFlat2(u16 tile)
{
    g_tileData[tile][1] |= 0xFFFFFF00;
    g_tileData[tile][2] |= 0xFFFFF000;
    g_tileData[tile][3] |= 0xFFFF0000;
    g_tileData[tile][4] |= 0xFFF00000;
    g_tileData[tile][5] |= 0xFF000000;
    g_tileData[tile][6] |= 0xF0000000;
}

static inline void ApplyPatternOld(u16 tile)
{
    g_tileData[tile][1] |= 0xFFFFFF00;
    g_tileData[tile][2] |= 0xFffff000;
    g_tileData[tile][3] |= 0xFf000000;
    g_tileData[tile][4] |= 0xFf000000;
    g_tileData[tile][5] |= 0xFf000000;
    g_tileData[tile][6] |= 0xF0000000;
}

// Generate and load tile graphics data for game display
void Graphics_GenerateTiles(void)
{
    // Clear tile data for tile 0 (transparent)
    for (u16 i = 0; i < 8; i++)
        g_tileData[0][i] = 0;
    
    // Generate tile data for tiles 1-7 (colored blocks) based on g_blockPatternType
    for (u16 tile = 1; tile <= 7; tile++)
    {
        u32 row = 0;
        u32 row2 = 0;
        
        // Generate colored row data using palette indices
        // first color - transparent
        for (u16 p = 1; p < 8; p++)
            row |= ((u32) (tile) << (p * 4));
        
        // Write rows to tile data
        for (u16 i = 1; i < 8; i++)
            g_tileData[tile][i] = row;
        
        // 0 color index - transparent
        g_tileData[tile][0] = 0x00000000;
        
        // Generate row2 data for different block patterns based on g_blockPatternType
        u32 mask1 = 0x000000F0;
        u32 mask2 = 0x000000F0;
        u32 mask3 = 0x000000F0;
        u32 mask4 = 0x000000F0;
        u32 mask5 = 0x000000F0;
        u32 mask6 = 0x000000F0;
        u32 mask7 = 0xFFFFFFF0;
        
        switch (g_game.blockPatternType)
        {
            case BLOCK_PTRN_FLAT1:
                ApplyPatternFlat1(tile);
                // masks remain default for FLAT1
                break;
            case BLOCK_PTRN_FLAT2:
                ApplyPatternFlat2(tile);
                mask1 = 0x000000F0;
                mask2 = 0x00000FF0;
                mask3 = 0x0000FFF0;
                mask4 = 0x000FFFF0;
                mask5 = 0x00FFFFF0;
                mask6 = 0x0FFFFFF0;
                mask7 = 0xFFFFFFF0;
                break;
            case BLOCK_PTRN_OLD:
                ApplyPatternOld(tile);
                mask1 = 0x00000000;
                mask2 = 0x000000F0;
                mask3 = 0x00000fF0;
                mask4 = 0x00000fF0;
                mask5 = 0x00000fF0;
                mask6 = 0x00ffffF0;
                mask7 = 0x0FFFFFF0;
                break;
            default:
                break;
        }
        
        for (u16 p = 1; p < 8; p++)
            row2 |= ((u32) (tile + 7) << (p * 4));
        
        g_tileData[tile][1] = (g_tileData[tile][1] & ~mask1) | (row2 & mask1);
        g_tileData[tile][2] = (g_tileData[tile][2] & ~mask2) | (row2 & mask2);
        g_tileData[tile][3] = (g_tileData[tile][3] & ~mask3) | (row2 & mask3);
        g_tileData[tile][4] = (g_tileData[tile][4] & ~mask4) | (row2 & mask4);
        g_tileData[tile][5] = (g_tileData[tile][5] & ~mask5) | (row2 & mask5);
        g_tileData[tile][6] = (g_tileData[tile][6] & ~mask6) | (row2 & mask6);
        g_tileData[tile][7] = (g_tileData[tile][7] & ~mask7) | (row2 & mask7);
    }
    
    // Generate tile data for border tiles (tiles 8-16)
    g_tileData[8][0] = 0x00000000;
    g_tileData[8][1] = 0xFFFFFFF0;
    g_tileData[8][7] = 0xFFFFFFF0;
    
    // Generate left and right edge tiles for tile 8
    for (u16 i = 2; i < 7; i++)
        g_tileData[8][i] = 0xF00000F0; // left & right edges
    
    // Generate glass tiles and other border tiles
    g_tileData[9][0] = 0x00000000;
    g_tileData[9][1] = 0x00000000;
    g_tileData[9][2] = 0xFFFFFFFF;
    g_tileData[9][3] = 0x00000000;
    g_tileData[9][4] = 0xFFFFFFFF;
    g_tileData[9][5] = 0x00000000;
    g_tileData[9][6] = 0x00000000;
    g_tileData[9][7] = 0x00000000;
    
    g_tileData[10][0] = 0x00F0F000;
    g_tileData[10][1] = 0x00F0F000;
    g_tileData[10][2] = 0x00F0F000;
    g_tileData[10][3] = 0x00F0F000;
    g_tileData[10][4] = 0x00F0F000;
    g_tileData[10][5] = 0x00F0F000;
    g_tileData[10][6] = 0x00F0F000;
    g_tileData[10][7] = 0x00F0F000;
    
    g_tileData[11][0] = 0x00000000;
    g_tileData[11][1] = 0x00000000;
    g_tileData[11][2] = 0x000000FF;
    g_tileData[11][3] = 0x00000F00;
    g_tileData[11][4] = 0x0000F00F;
    g_tileData[11][5] = 0x000F00F0;
    g_tileData[11][6] = 0x00F00F00;
    g_tileData[11][7] = 0x00F0F000;
    
    g_tileData[12][0] = 0x00000000;
    g_tileData[12][1] = 0x00000000;
    g_tileData[12][2] = 0xF0000000;
    g_tileData[12][3] = 0x0F000000;
    g_tileData[12][4] = 0x00F00000;
    g_tileData[12][5] = 0xF00F0000;
    g_tileData[12][6] = 0x0F00F000;
    g_tileData[12][7] = 0x00F0F000;
    
    g_tileData[13][0] = 0x00F00F00;
    g_tileData[13][1] = 0x000F00F0;
    g_tileData[13][2] = 0x0000F00F;
    g_tileData[13][3] = 0x00000F00;
    g_tileData[13][4] = 0x000000FF;
    g_tileData[13][5] = 0x00000000;
    g_tileData[13][6] = 0x00000000;
    g_tileData[13][7] = 0x00000000;
    
    g_tileData[14][0] = 0x0F00F000;
    g_tileData[14][1] = 0xF00F0000;
    g_tileData[14][2] = 0x00F00000;
    g_tileData[14][3] = 0x0F000000;
    g_tileData[14][4] = 0xF0000000;
    g_tileData[14][5] = 0x00000000;
    g_tileData[14][6] = 0x00000000;
    g_tileData[14][7] = 0x00000000;
    
    g_tileData[15][0] = 0x0FF000F0;
    g_tileData[15][1] = 0x0FF000F0;
    g_tileData[15][2] = 0x0FF000F0;
    g_tileData[15][3] = 0x0FF000F0;
    g_tileData[15][4] = 0x0FF000F0;
    g_tileData[15][5] = 0x0FF000F0;
    g_tileData[15][6] = 0x0FF000F0;
    g_tileData[15][7] = 0x0FF000F0;
    
    g_tileData[16][0] = 0xF000FF00;
    g_tileData[16][1] = 0xF000FF00;
    g_tileData[16][2] = 0xF000FF00;
    g_tileData[16][3] = 0xF000FF00;
    g_tileData[16][4] = 0xF000FF00;
    g_tileData[16][5] = 0xF000FF00;
    g_tileData[16][6] = 0xF000FF00;
    g_tileData[16][7] = 0xF000FF00;
    
    g_tileData[17][0] = 0x00000000;
    g_tileData[17][1] = 0xFFFFFFFF;
    g_tileData[17][2] = 0x00000000;
    g_tileData[17][3] = 0x00000000;
    g_tileData[17][4] = 0x00000000;
    g_tileData[17][5] = 0xFFFFFFFF;
    g_tileData[17][6] = 0xFFFFFFFF;
    g_tileData[17][7] = 0x00000000;
    
    g_tileData[18][0] = 0x0FF000F0;
    g_tileData[18][1] = 0x0FF000FF;
    g_tileData[18][2] = 0x0FF00000;
    g_tileData[18][3] = 0x0FFF0000;
    g_tileData[18][4] = 0x00FFF000;
    g_tileData[18][5] = 0x000FFFFF;
    g_tileData[18][6] = 0x0000FFFF;
    g_tileData[18][7] = 0x00000000;
    
    g_tileData[19][0] = 0xF000FF00;
    g_tileData[19][1] = 0xF000FF00;
    g_tileData[19][2] = 0x0000FF00;
    g_tileData[19][3] = 0x000FFF00;
    g_tileData[19][4] = 0x00FFF000;
    g_tileData[19][5] = 0xFFFF0000;
    g_tileData[19][6] = 0xFFF00000;
    g_tileData[19][7] = 0x00000000;
    
    // Left top
    g_tileData[20][0] = 0x00000000;
    g_tileData[20][1] = 0x00000000;
    g_tileData[20][2] = 0x00000000;
    g_tileData[20][3] = 0x00000000;
    g_tileData[20][4] = 0x00000000;
    g_tileData[20][5] = 0x00000000;
    g_tileData[20][6] = 0x0FFFFFF0;
    g_tileData[20][7] = 0x0FFFFFF0;
    
    // Right top
    g_tileData[21][0] = 0x00000000;
    g_tileData[21][1] = 0x00000000;
    g_tileData[21][2] = 0x00000000;
    g_tileData[21][3] = 0x00000000;
    g_tileData[21][4] = 0x00000000;
    g_tileData[21][5] = 0x00000000;
    g_tileData[21][6] = 0xFFFFFF00;
    g_tileData[21][7] = 0xFFFFFF00;
    
    g_tileData[22][0] = 0xAAAAAAAA;
    g_tileData[22][1] = 0xAAAAAAAA;
    g_tileData[22][2] = 0xAAAAAAAA;
    g_tileData[22][3] = 0xAAAAAAAA;
    g_tileData[22][4] = 0xAAAAAAAA;
    g_tileData[22][5] = 0xAAAAAAAA;
    g_tileData[22][6] = 0xAAAAAAAA;
    g_tileData[22][7] = 0xAAAAAAAA;
    
    g_tileData[23][0] = 0x00000000;
    g_tileData[23][1] = 0x00000000;
    g_tileData[23][2] = 0x00000000;
    g_tileData[23][3] = 0x00000000;
    g_tileData[23][4] = 0x00000000;
    g_tileData[23][5] = 0x00000000;
    g_tileData[23][6] = 0x00000000;
    g_tileData[23][7] = 0x00000000;
    
    // Load tile data into VRAM
    VDP_loadTileData((const u32*) g_tileData, TILE_USER, TILE_COUNT, DMA);
}
