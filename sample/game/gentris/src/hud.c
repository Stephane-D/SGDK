#include <genesis.h>
#include "defs.h"
#include "typedefs.h"
#include "global.h"
#include "hud.h"


// Draw preview of next figure in the next-piece box
void HUD_DrawNextFigure(void)
{
    VDP_clearTileMapRect(FG, PREVIEW_X - 1, PREVIEW_Y + 1, FIG_LINES + 2, FIG_LINES);

    VDP_drawText("Next", PREVIEW_X, PREVIEW_Y);

    // Draw the shape
    const u8* shape = g_figureLines[g_activeFig.nextFigType][0]; // rotation 0

    for (u16 row = 0; row < 4; row++)
    {
        u8 line = shape[row];
        for (u16 col = 0; col < 4; col++)
        {
            if (!(line & (1 << col)))
                continue;

            u16 tile = TILE_BLOCK + g_activeFig.nextFigType;
            VDP_setTileMapXY(FG, TILE_ATTR_FULL(PAL0, true, false, false, tile), PREVIEW_X + col, PREVIEW_Y + 2 + row);
        }
    }
}

// GameplayState_OnUpdate on-screen display of current score, lines cleared, and level
void HUD_ScoreRedraw(void)
{
    char bufScore[8];
    char bufLines[8];
    char bufLevel[8];

    VDP_setTextPlane(FG);
    VDP_setTextPalette(PAL0);

    sprintf(bufScore, "%05d", g_score.score);
    sprintf(bufLines, "%03d", g_score.lines);
    sprintf(bufLevel, "%02d", g_stats.level);

    VDP_drawText("Score", SCORE_X, SCORE_Y + 1);
    VDP_drawText(bufScore, SCORE_X, SCORE_Y + 2);

    VDP_drawText("Lines", SCORE_X, SCORE_Y + 6);
    VDP_drawText(bufLines, SCORE_X + 1, SCORE_Y + 7);

    VDP_drawText("Level", SCORE_X, SCORE_Y + 11);
    VDP_drawText(bufLevel, SCORE_X + 1, SCORE_Y + 12);

    HUD_StatisticsRedraw();
}

// GameplayState_OnUpdate on-screen display of game statistics (Tetris, combo counts, etc.)
void HUD_StatisticsRedraw(void)
{
    char bufTetris[4];
    char bufTriple[4];
    char bufDouble[4];
    char bufSingle[4];

    char bufCombo[3];
    char bufMaxCombo[3];

    sprintf(bufTetris, "%03d", g_stats.tetris[4]);
    sprintf(bufTriple, "%03d", g_stats.tetris[3]);
    sprintf(bufDouble, "%03d", g_stats.tetris[2]);
    sprintf(bufSingle, "%03d", g_stats.tetris[1]);

    sprintf(bufCombo, "%02d", g_stats.combo);
    sprintf(bufMaxCombo, "%02d", g_stats.maxCombo);

    VDP_drawText("Tetris", BORDER_STAT_X + 2, BORDER_STAT_Y + 2);
    VDP_drawText(bufTetris, BORDER_STAT_X + 3, BORDER_STAT_Y + 3);

    VDP_drawText("Triples", BORDER_STAT_X + 2, BORDER_STAT_Y + 5);
    VDP_drawText(bufTriple, BORDER_STAT_X + 3, BORDER_STAT_Y + 6);

    VDP_drawText("Doubles", BORDER_STAT_X + 2, BORDER_STAT_Y + 8);
    VDP_drawText(bufDouble, BORDER_STAT_X + 3, BORDER_STAT_Y + 9);

    VDP_drawText("Singles", BORDER_STAT_X + 2, BORDER_STAT_Y + 11);
    VDP_drawText(bufSingle, BORDER_STAT_X + 3, BORDER_STAT_Y + 12);


    if (g_game.gameMode == MODE_CLASSIC)
    {
        VDP_drawText("Combo", BORDER_STAT_X + 2, BORDER_STAT_Y + 16);
        VDP_drawText(bufCombo, BORDER_STAT_X + 3, BORDER_STAT_Y + 17);
    }
    else
    {
        HUD_TimeDraw();
    }
    
    VDP_drawText("MaxComb", BORDER_STAT_X + 2, BORDER_STAT_Y + 19);
    VDP_drawText(bufMaxCombo, BORDER_STAT_X + 3, BORDER_STAT_Y + 20);
}

// GameplayState_OnUpdate elapsed time display for 40-line mode
void HUD_TimeDraw(void)
{
    g_stats.seconds = (getTime(0) - g_stats.secondsStart) >> 8;
    sprintf(g_timeStrBuf, "%ld sec", g_stats.seconds);

    VDP_drawText("Time ", BORDER_STAT_X + 2, BORDER_STAT_Y + 16);
    VDP_drawText(g_timeStrBuf, BORDER_STAT_X + 3, BORDER_STAT_Y + 17);
}

// Draw game field border (stats panel and glass borders with corners)
void Hud_DrawGlassAndBorders(void)
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