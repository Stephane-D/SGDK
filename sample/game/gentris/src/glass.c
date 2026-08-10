#include <genesis.h>
#include "defs.h"
#include "global.h"
#include "glass.h"


// Render all fixed figures in the game field, highlighting rows being removed
void Glass_RedrawDirty(void)
{
    for (u16 y = 0; y < FIELD_H; y++)
    {
        if (!g_dirtyRows[y])
            continue;
        
        g_dirtyRows[y] = 0;

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
        if (g_game.g_glassShakeEnabled)
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
