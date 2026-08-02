#include <genesis.h>
#include "defs.h"
#include "global.h"
#include "glass.h"


// Draw game field border (stats panel and glass borders with corners)
void Glass_DrawBorder(void)
{
    // right hor borders
    u16 attrBG = TILE_ATTR_FULL(PAL1, false, false, false, BORDER_THINK_TOP);

    // top & bottom
    VDP_fillTileMapRect(FG, attrBG, PREVIEW_X - 1, GLASS_Y, 7, 1);
    VDP_fillTileMapRect(FG, attrBG, PREVIEW_X - 1, GLASS_Y + 22, 7, 1);

    // middle
    VDP_fillTileMapRect(FG, attrBG, PREVIEW_X - 1, GLASS_Y + 7, 7, 1);
    VDP_fillTileMapRect(FG, attrBG, PREVIEW_X - 1, GLASS_Y + 12, 7, 1);
    VDP_fillTileMapRect(FG, attrBG, PREVIEW_X - 1, GLASS_Y + 17, 7, 1);

    // right vert borders
    attrBG = TILE_ATTR_FULL(PAL1, 0, false, false, TILE_USER + 10);
    VDP_fillTileMapRect(FG, attrBG, PREVIEW_X - 2, GLASS_Y, 1, 22);
    VDP_fillTileMapRect(FG, attrBG, PREVIEW_X - 2 + 8, GLASS_Y, 1, 22);

    // right corners
    attrBG = TILE_ATTR_FULL(PAL1, 0, false, false, TILE_USER + 11);
    VDP_setTileMapXY(FG, attrBG, PREVIEW_X - 2, GLASS_Y);
    attrBG = TILE_ATTR_FULL(PAL1, 0, false, false, TILE_USER + 12);
    VDP_setTileMapXY(FG, attrBG, PREVIEW_X - 2 + 8, GLASS_Y);

    attrBG = TILE_ATTR_FULL(PAL1, 0, false, false, TILE_USER + 13);
    VDP_setTileMapXY(FG, attrBG, PREVIEW_X - 2, GLASS_Y + 22);
    attrBG = TILE_ATTR_FULL(PAL1, 0, false, false, TILE_USER + 14);
    VDP_setTileMapXY(FG, attrBG, PREVIEW_X - 2 + 8, GLASS_Y + 22);

    // left hor borders
    attrBG = TILE_ATTR_FULL(PAL1, 0, false, false, BORDER_THINK_TOP);
    for (u16 x = 0; x < BORDER_STAT_W; x++)
    {
        VDP_setTileMapXY(FG, attrBG, 2 + x, GLASS_Y);
        VDP_setTileMapXY(FG, attrBG, 2 + x, GLASS_Y + FIELD_H);

        VDP_setTileMapXY(FG, attrBG, 2 + x, GLASS_Y + 14);
    }

    // left vert borders
    attrBG = TILE_ATTR_FULL(PAL1, 0, false, false, TILE_USER + 10);
    for (u16 y = 0; y < 22; y++)
    {
        VDP_setTileMapXY(FG, attrBG, 2, GLASS_Y + y);
        VDP_setTileMapXY(FG, attrBG, 2 + BORDER_STAT_W, GLASS_Y + y);
    }

    // left corners
    attrBG = TILE_ATTR_FULL(PAL1, 0, false, false, TILE_USER + 11);
    VDP_setTileMapXY(FG, attrBG, 2, GLASS_Y);
    attrBG = TILE_ATTR_FULL(PAL1, 0, false, false, TILE_USER + 12);
    VDP_setTileMapXY(FG, attrBG, 2 + BORDER_STAT_W, GLASS_Y);

    attrBG = TILE_ATTR_FULL(PAL1, 0, false, false, TILE_USER + 13);
    VDP_setTileMapXY(FG, attrBG, 2, GLASS_Y + FIELD_H);
    attrBG = TILE_ATTR_FULL(PAL1, 0, false, false, TILE_USER + 14);
    VDP_setTileMapXY(FG, attrBG, 2 + BORDER_STAT_W, GLASS_Y + FIELD_H);

    // draw glass
    u16 attr = TILE_ATTR_FULL(PAL0, 0, false, false, TILE_GLASS_LEFT);
    for (u16 y = 0; y < GLASS_H; y++)
        VDP_setTileMapXY(FG, attr, FIELD_X - 1, GLASS_Y + y);

    attr = TILE_ATTR_FULL(PAL0, 0, false, false, TILE_GLASS_RIGHT);
    for (u16 y = 0; y < GLASS_H; y++)
        VDP_setTileMapXY(FG, attr, FIELD_X + FIELD_W, GLASS_Y + y);

    attr = TILE_ATTR_FULL(PAL0, 0, false, false, TILE_GLASS_BOTTOM);
    for (u16 x = 0; x < FIELD_W + 2; x++)
        VDP_setTileMapXY(FG, attr, FIELD_X + x - 1, FIELD_Y + FIELD_H);

    attr = TILE_ATTR_FULL(PAL0, 0, false, false, TILE_GLASS_LEFT_COR);
    VDP_setTileMapXY(FG, attr, FIELD_X - 1, FIELD_Y + FIELD_H);

    attr = TILE_ATTR_FULL(PAL0, 0, false, false, TILE_GLASS_RIGHT_COR);
    VDP_setTileMapXY(FG, attr, FIELD_X + FIELD_W, FIELD_Y + FIELD_H);

    attr = TILE_ATTR_FULL(PAL0, 0, false, false, TILE_GLASS_LEFT_TOP);
    VDP_setTileMapXY(FG, attr, FIELD_X - 1, FIELD_Y-2);

    attr = TILE_ATTR_FULL(PAL0, 0, false, false, TILE_GLASS_RIGHT_TOP);
    VDP_setTileMapXY(FG, attr, FIELD_X + FIELD_W, FIELD_Y-2);
}

// Render all fixed figures in the game field, highlighting rows being removed
void Glass_RedrawDirty(void)
{
    for (u16 y = 0; y < FIELD_H; y++)
    {
        if (!g_dirtyRows[y])
            continue;
        g_dirtyRows[y] = 0;
//        kprintf("y=%d, g_dirtyRows: %d", y, g_dirtyRows[y]);
        
        u16 pal = PAL0;

        for (u16 i = 0; i < 4; i++)
        {
            if (y == g_rowsBlockToRemove.lines[i])
            {
                pal = g_palRowRemoving;
                g_dirtyRows[y] = 1;
            }
        }

        for (u16 x = 0; x < FIELD_W; x++)
        {
            u16 block = g_blockField[y][x];
            u16 tile = block ? (TILE_BLOCK + block - 1) : TILE_EMPTY;
            VDP_setTileMapXY(FG, TILE_ATTR_FULL(pal, true, false, false, tile), FIELD_X + x, FIELD_Y + y);
        }
    }
}

// GameplayState_OnUpdate screen shake effect when figure is fixed or rows are removed
void Glass_UpdateShake(void)
{
    if (g_glassShakeInd && g_glassShakeInd != GLASS_SHAKE_FRAMES_Y)
    {
        if (g_glassShakeEnabled)
        {
            VDP_setVerticalScrollTile(FG, GLASS_X / 2 - 1,
                                      (s16*) g_glassShakeTable[g_glassShakeType][g_glassShakeInd],
                                      GLASS_W / 2 + 2, DMA_QUEUE);
        }

        if (g_game.isSoundOn)
        {
            if (g_glassShakeInd == SOUND_LENGTH_SHAKE / 4 * 3)
            {
                if (!g_rowsRemoveDownCounter)
                    PSG_setEnvelope(3, PSG_ENVELOPE_MIN);
                PSG_setTone(0, 1023);
            }
            if (g_glassShakeInd == SOUND_LENGTH_SHAKE)
            {
                if (!g_rowsRemoveDownCounter)
                    PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
            }
        }

        g_glassShakeInd++;
    }
}
