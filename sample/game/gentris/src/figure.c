#include <genesis.h>
#include "defs.h"
#include "typedefs.h"
#include "global.h"
#include "input.h"
#include "hud.h"
#include "figure.h"
#include "sound.h"


// Render active falling figure as sprites on screen
void Figure_DrawActive(void)
{
    u8 used = 0;
    u8 line;
    s16 posX, posY;
    u16 attr;
    u16 spriteShiftY = 0;

    const u8* shape = g_figureLines[g_activeFig.type][g_activeFig.rot];

    // Iterate through 4x4 grid and render visible blocks of active figure
    for (u16 row = 0; row < 4; row++)
    {
        line = shape[row];
        for (u16 col = 0; col < 4; col++)
        {
            // Check if block is set in this position
            if (line & (1 << col))
            {
                posX = (FIELD_X + g_activeFig.x + col) * 8;

                if (g_game.g_glassShakeEnabled)
                    spriteShiftY = g_glassShakeTable[g_glassShakeType][g_glassShakeInd][0];

                posY = (FIELD_Y + g_activeFig.y + row) * 8 - spriteShiftY;

                attr = TILE_ATTR_FULL(PAL0, false, false, false, TILE_BLOCK + g_activeFig.type);

                // link = next sprite in chain (last -> 0)
                u8 link = (used < FIG_SPRITES - 1) ? (used + 1) : 0;
                VDP_setSpriteFull(used, posX, posY, SPRITE_SIZE(1, 1), attr, link);
                used++;
                if (used >= FIG_SPRITES)
                    break;
            }
        }
        if (used >= FIG_SPRITES)
            break;
    }

    // Others sprites hide behind screen, but save the links
    for (u16 i = used; i < FIG_SPRITES; i++)
    {
        u8 link = (i < FIG_SPRITES - 1) ? (i + 1) : 0;
        VDP_setSpriteFull(i, 0, -128, SPRITE_SIZE(1, 1), 0, link);
    }

    VDP_updateSprites(FIG_SPRITES, DMA_QUEUE);
}

// Move figure down until collision, mark as fixed with hard drop effect
void Figure_DropHard(void)
{
    while (!Figure_IsCollided(&g_activeFig))
        g_activeFig.y++;

    // Move back up one position to avoid embedding in obstacle
    g_activeFig.y--;

    g_input.dropType = HARD_DROP;
    Figure_FixRemoveSpawn();

}

// Fix current figure in place, check for complete rows, update stats and spawn next figure
void Figure_FixRemoveSpawn(void)
{
    Figure_Fix();
    
    g_activeFig.canSpawn = true;

    g_rowsToRemoveCount = Figure_GetRowsBlockToRemove(&g_rowsBlockToRemove);

    // GameplayState_OnUpdate combo counter and statistics
    if (g_rowsToRemoveCount == 0)
        g_stats.combo = 0;
    if (g_rowsToRemoveCount > 0)
        g_stats.combo ++;

    g_stats.tetris[g_rowsToRemoveCount]++;
    g_stats.maxCombo = (g_stats.combo > g_stats.maxCombo)? g_stats.combo: g_stats.maxCombo;

    // Trigger row removal animation if rows are complete
    if (g_rowsBlockToRemove.lines[0])
    {
        g_rowsRemoveDownCounter = ROW_REMOVE_FX_DELAY;
        g_activeFig.canSpawn = false;
        g_score.counter = 0;
        Sound_PlayRowRemoved();
    }
}

// Initialize next falling figure with type from bag, reset position and rotation
void Figure_Spawn(void)
{
    Input_ResetAllJoyStates(JOY_1_INDEX);

    Input_Enable();
    
    g_activeFig.type = g_activeFig.nextFigType;
    g_activeFig.nextFigType = Figure_GetNextType();
    g_activeFig.rot = 0;
    g_activeFig.x = 3;   // centered in 10-wide field
    g_activeFig.y = 0;
    g_activeFig.fixTimer = 0;
    g_activeFig.grounded = false;
    g_activeFig.fixed = false;
    
    g_palRowRemoving = PAL0;
    
    HUD_DrawNextFigure();
}

// Find rows that are completely filled and need to be removed
u16 Figure_GetRowsBlockToRemove(RowsBlock* rowsBlock)
{
    u16 index = 0;
    u16 fullMask = (1 << FIELD_W) - 1;

    for (s16 y = g_activeFig.y + 4; y >= g_activeFig.y; y--)
    {
        if (g_rowMask[y] != fullMask)
            continue;

        rowsBlock->lines[index] = y;
        index++;
    }
    return index;
}

// Remove complete rows by shifting rows above down and clearing top row
u16 Figure_TryRemoveLines(void)
{
    u16 lines = 0;
    // Mask for full row
    u16 fullMask = (1 << FIELD_W) - 1;
    
    for (s16 y = g_activeFig.y + 4; y >= g_activeFig.y; y--)
    {
        if (y >= FIELD_H)
            continue;
        
        // If row is full, remove it and shift rows above down
        if (g_rowMask[y] == fullMask)
        {
            // Shift rows above removed row down
            for (s16 yy = y; yy > 0; yy--)
            {
                g_rowMask[yy] = g_rowMask[yy - 1];
                memcpy(g_blockField[yy], g_blockField[yy - 1], FIELD_W);
                // Mark row as dirty for redrawing
                g_dirtyRows[yy] = 1;
            }
            
            // Clear top row
            g_rowMask[0] = 0;
            memset(g_blockField[0], 0, FIELD_W);
            // Mark row as dirty for redrawing too
            g_dirtyRows[0] = 1;

            lines++;
            y++; // re-check same row index
        }
    }
    return lines;
}

// Place current figure on the game field permanently
void Figure_Fix(void)
{
    g_activeFig.fixed = true;
    g_glassShakeInd = 1;
    g_glassShakeType = g_input.dropType;
    g_stats.figuresFixed++;

    Input_Disable();
    
    if (!g_input.enabled)
        g_input.dropType = NO_DROP;

    Sound_PlayFigureFixed();
    
    // Add figure blocks to field, updating row masks and dirty rows
    for (u16 row = 0; row < 4; row++)
    {
        u16 mask = 0;
        s16 fy = g_activeFig.y + row;

        // Skip rows above field
        if (fy < 0)
            continue;

        u8 line = g_figureLines[g_activeFig.type][g_activeFig.rot][row];

        if (!line)
            continue;

        // Handle figure position relative to field boundaries
        if (g_activeFig.x < 0)
            mask = line >> (-g_activeFig.x);
        else
            mask = (u16) line << g_activeFig.x;

        g_rowMask[fy] |= mask;

        for (u16 col = 0; col < FIELD_W; col++)
        {
            if (mask & (1 << col)) g_blockField[fy][col] = g_activeFig.type + 1;
        }

        g_dirtyRows[fy] = 1;
    }
}

// Shuffle figure bag using Fisher-Yates algorithm for random figure selection
void Figure_ShuffleBag(void)
{
    g_bagIndex = 0;

    for (u16 i = 0; i < 7; i++)
        g_figuresBag[i] = i;

    // Fisher-Yates shuffle algorithm
    for (u16 i = 6; i > 0; i--)
    {
        u16 j = random() % (i + 1);
        u16 tmp = g_figuresBag[i];
        g_figuresBag[i] = g_figuresBag[j];
        g_figuresBag[j] = tmp;
    }
}

// Get next figure type from shuffled bag, reshuffle when bag is empty
u8 Figure_GetNextType(void)
{
    if (g_bagIndex >= 7)
        Figure_ShuffleBag();
    
    return g_figuresBag[g_bagIndex++];
}

// Check if figure at given position collides with borders or other figures
CollisionType Figure_IsCollided(const Figure* figPtr)
{
    u16 mask;
    s16 fy;

    for (u16 row = 0; row < 4; row++)
    {
        fy = figPtr->y + row;
        u8 line = g_figureLines[figPtr->type][figPtr->rot][row];
        if (!line)
            continue;

        // Check left border collision
        if (figPtr->x < 0)
        {
            u8 shift = (u8) (-figPtr->x);
            if (shift >= 4)
                return COL_BORDER;
            if (line & ((1 << shift) - 1))
                return COL_BORDER;
            mask = line >> shift;
        }
        else
        {
            mask = (u16) line << figPtr->x;
            // Check right border collision
            if (mask & ~((1 << FIELD_W) - 1))
                return COL_BORDER;
        }

        // Check bottom border collision
        if (fy >= FIELD_H)
            return COL_BORDER;

        // Check collision with already placed figures
        if (fy >= 0 && (g_rowMask[fy] & mask))
            return COL_FIGURE;
    }
    return COL_NONE;
}

// Detect collision direction by testing adjacent positions
FigCollision Figure_GetCollision(const Figure* figPtr)
{
    FigCollision col = {.type = COL_NONE, .dir = DIR_NONE};

    col.type = Figure_IsCollided(figPtr);

    if (col.type == COL_NONE)
        return (FigCollision) {.type = COL_NONE, .dir = DIR_NONE};

    Figure temp;

    temp = *figPtr;
    temp.x++;
    if (!Figure_IsCollided(&temp))
        return (FigCollision) {.type = col.type, .dir = DIR_LEFT}; // left side collision

    temp = *figPtr;
    temp.x--;
    if (!Figure_IsCollided(&temp))
        return (FigCollision) {.type = col.type, .dir = DIR_RIGHT};   // right side collision

    temp = *figPtr;
    temp.y--;
    if (!Figure_IsCollided(&temp))
        return (FigCollision) {.type = col.type, .dir = DIR_DOWN};   // below collision

    temp = *figPtr;
    temp.y++;
    if (!Figure_IsCollided(&temp))
        return (FigCollision) {.type = col.type, .dir = DIR_UP};   // above collision

    return col;
}

// Attempt to move figure horizontally, revert if collision occurs
void Figure_TryMoveTo(s16 horDir)
{
    g_activeFig.x += horDir;

    FigCollision figCol = Figure_GetCollision(&g_activeFig);
    g_input.colDir = figCol.dir;

    // Revert movement if horizontal collision detected
    if (g_input.colDir == DIR_LEFT || g_input.colDir == DIR_RIGHT)
        g_activeFig.x -= horDir;
}
