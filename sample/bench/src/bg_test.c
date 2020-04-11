#include <genesis.h>

#include "main.h"
#include "gfx.h"


// forward
static u32 displayResult(u32 op, fix32 time, u16 y);


u16 executeBGTest(u16 *scores)
{
    fix32 start;
    fix32 end;
    u16 i;
    u16 *score;
    u16 globalScore;
    Vect2D_s16 *xy;
    Vect2D_s16 *pos;
    Image *img;
    char str[41];

    xy = MEM_alloc(1000 * sizeof(Vect2D_s16));

    pos = xy;
    for(i = 0; i < 1000; i++)
    {
        pos->x = random() % (40-13);
        pos->y = random() % (28-2);
        pos->y += 2;
        pos++;
    }
    strcpy(str, "Hello world !");

    score = scores;
    globalScore = 0;

    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("Draw text...", 2, 0);
    i = 10;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        u16 j = 1000;
        pos = xy;

        while(j)
        {
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            j -= 10;
        }
    }
    end = getTimeAsFix32(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    *score = displayResult(10000, end - start, 2);
    globalScore += *score++;

    // wait 5 seconds
    waitMs(5000);

    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("Draw/clear text...", 2, 0);
    i = 10;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        u16 j = 1000;
        pos = xy;

        while(j)
        {
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            VDP_drawText(str, pos->x, pos->y);
            pos++;
            pos -= 10;
            VDP_clearText(pos->x, pos->y, 13);
            pos++;
            VDP_clearText(pos->x, pos->y, 13);
            pos++;
            VDP_clearText(pos->x, pos->y, 13);
            pos++;
            VDP_clearText(pos->x, pos->y, 13);
            pos++;
            VDP_clearText(pos->x, pos->y, 13);
            pos++;
            VDP_clearText(pos->x, pos->y, 13);
            pos++;
            VDP_clearText(pos->x, pos->y, 13);
            pos++;
            VDP_clearText(pos->x, pos->y, 13);
            pos++;
            VDP_clearText(pos->x, pos->y, 13);
            pos++;
            VDP_clearText(pos->x, pos->y, 13);
            pos++;

            j -= 10;
        }
    }
    end = getTimeAsFix32(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    *score = displayResult(10000, end - start, 2);
    globalScore += *score++;

    // wait 5 seconds
    waitMs(5000);

    // Image draw test
    pos = xy;
    for(i = 0; i < 1000; i++)
    {
        pos->x = random() % (40-16);
        pos->y = random() % (28-(8+2));
        pos->y += 2;
        pos++;
    }

    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("128x64 image draw (packed slow)", 2, 0);
    i = 1;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        u16 j = 300;
        pos = xy;

        while(j)
        {
            VDP_drawImageEx(BG_A, &logo_med, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            j -= 10;
        }
    }
    end = getTimeAsFix32(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    *score = displayResult(300, end - start, 2);
    globalScore += *score++;

    // wait 5 seconds
    waitMs(5000);

    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("128x64 image draw (packed fast)", 2, 0);
    i = 1;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        u16 j = 800;
        pos = xy;

        while(j)
        {
            VDP_drawImageEx(BG_A, &logo_med_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_med_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            j -= 10;
        }
    }
    end = getTimeAsFix32(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    *score = displayResult(800, end - start, 2);
    globalScore += *score++;

    // wait 5 seconds
    waitMs(5000);

    // unpack image
    img = unpackImage(&logo_med, NULL);
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("128x64 image draw (unpacked)", 2, 0);
    i = 1;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        u16 j = 1000;
        pos = xy;

        while(j)
        {
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            j -= 10;
        }
    }
    end = getTimeAsFix32(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    *score = displayResult(1000, end - start, 2);
    globalScore += *score++;
    MEM_free(img);

    // wait 5 seconds
    waitMs(5000);

    // unpack image
    img = unpackImage(&logo_med, NULL);
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("128x64 image draw (preloaded)", 2, 0);
    VDP_loadTileSet(img->tileset, TILE_USER, TRUE);
    VDP_setPalette(PAL1, img->palette->data);
    i = 5;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        u16 j = 1000;
        pos = xy;

        while(j)
        {
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            j -= 10;
        }
    }
    end = getTimeAsFix32(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    *score = displayResult(5000, end - start, 2) / 5;
    globalScore += *score++;
    MEM_free(img);

    // wait 5 seconds
    waitMs(5000);

    pos = xy;
    for(i = 0; i < 1000; i++)
    {
        pos->x = random() % (40-8);
        pos->y = random() % (28-(4+2));
        pos->y += 2;
        pos++;
    }

    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("64x32 image draw (packed slow)", 2, 0);
    i = 1;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        u16 j = 1000;
        pos = xy;

        while(j)
        {
            VDP_drawImageEx(BG_A, &logo_sm, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            j -= 10;
        }
    }
    end = getTimeAsFix32(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    VDP_setPalette(PAL0, palette_grey);
    *score = displayResult(1000, end - start, 2);
    globalScore += *score++;

    // wait 5 seconds
    waitMs(5000);

    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("64x32 image draw (packed fast)", 2, 0);
    i = 2;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        u16 j = 1000;
        pos = xy;

        while(j)
        {
            VDP_drawImageEx(BG_A, &logo_sm_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, &logo_sm_f, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            j -= 10;
        }
    }
    end = getTimeAsFix32(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    VDP_setPalette(PAL0, palette_grey);
    *score = displayResult(2000, end - start, 2);
    globalScore += *score++;

    // wait 5 seconds
    waitMs(5000);

    // unpack image
    img = unpackImage(&logo_sm, NULL);
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("64x32 image draw (unpacked)", 2, 0);
    i = 3;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        u16 j = 1000;
        pos = xy;

        while(j)
        {
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            VDP_drawImageEx(BG_A, img, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, TRUE, TRUE);
            pos++;
            j -= 10;
        }
    }
    end = getTimeAsFix32(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    VDP_setPalette(PAL0, palette_grey);
    *score = displayResult(3000, end - start, 2);
    globalScore += *score++;
    MEM_free(img);

    // wait 5 seconds
    waitMs(5000);

    // unpack image
    img = unpackImage(&logo_sm, NULL);
    VDP_clearPlane(BG_A, TRUE);
    VDP_drawText("64x32 image draw (preloaded)", 2, 0);
    VDP_loadTileSet(img->tileset, TILE_USER, TRUE);
    VDP_setPalette(PAL1, img->palette->data);
    i = 10;
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        u16 j = 1000;
        pos = xy;

        while(j)
        {
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            VDP_setTileMapEx(BG_A, img->tilemap, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, TILE_USERINDEX), pos->x, pos->y, 0, 0, img->tilemap->w, img->tilemap->h, CPU);
            pos++;
            j -= 10;
        }
    }
    end = getTimeAsFix32(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    VDP_setPalette(PAL0, palette_grey);
    *score = displayResult(10000, end - start, 2) / 5;
    globalScore += *score++;
    MEM_free(img);

    waitMs(5000);
    VDP_clearPlane(BG_A, TRUE);

    MEM_free(xy);

    return globalScore;
}


static u32 displayResult(u32 op, fix32 time, u16 y)
{
    char timeStr[32];
    char speedStr[32];
    char str[64];
    fix32 speedOp;
    u32 speed;

    fix32ToStr(time, timeStr, 2);
    speedOp = intToFix32(op >> 4);
    // get number of points computed per second
    speedOp = fix32Div(speedOp, time);
    speed = fix32ToRoundedInt(speedOp << 4);
    // put it in speedStr
    intToStr(speed, speedStr, 1);

    strcpy(str, "Elapsed time = ");
    strcat(str, timeStr);
    strcat(str, "s (");
    strcat(str, speedStr);
    strcat(str, " op/s)");

    // display test string
    VDP_drawText(str, 3, y);

    return speed;
}
