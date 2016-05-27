#include <genesis.h>

#include "main.h"
#include "gfx.h"


typedef struct
{
    Vect2D_s16 *pts;
    u16 numPts;
    u8 col;
} Polygone;


// forward
static void initPixels(Pixel *pixels, u16 num);
static void initLines(Line *lines, u16 num);
static void initPolys(Vect2D_s16 *pts, Polygone *polys, u16 numPts, u16 num);

static u16 displayResult(u32 op, fix32 time, u16 y);
static u16 displayResult2(u32 op, fix32 time, u16 y);
static u16 displayResult3(u32 op, fix32 time, u16 y);


u16 executeBMPTest(u16 *scores)
{
    fix32 start;
    fix32 end;
    fix32 time;
    u16 i;
    u16* score;
    u16 globalScore;
    Pixel *pixels;
    Line *lines;
    Vect2D_s16 *pts;
    Polygone *polys;
    Bitmap *bmp;
    u16 palette[16];

    pixels = MEM_alloc(1000 * sizeof(Pixel));
    lines = MEM_alloc(150 * sizeof(Line));
    pts = MEM_alloc(50 * 10 * sizeof(Vect2D_s16));
    polys = MEM_alloc(50 * sizeof(Polygone));

    initPixels(pixels, 1000);

    // init palette
    palette[0] = RGB24_TO_VDPCOLOR(0x000000);
    palette[1] = RGB24_TO_VDPCOLOR(0x0000FF);
    palette[2] = RGB24_TO_VDPCOLOR(0x00FF00);
    palette[3] = RGB24_TO_VDPCOLOR(0xFF0000);
    palette[4] = RGB24_TO_VDPCOLOR(0x00FFFF);
    palette[5] = RGB24_TO_VDPCOLOR(0xFFFF00);
    palette[6] = RGB24_TO_VDPCOLOR(0xFF00FF);
    palette[7] = RGB24_TO_VDPCOLOR(0xFFFFFF);
    palette[8] = RGB24_TO_VDPCOLOR(0x404040);
    palette[9] = RGB24_TO_VDPCOLOR(0x000080);
    palette[10] = RGB24_TO_VDPCOLOR(0x008000);
    palette[11] = RGB24_TO_VDPCOLOR(0x800000);
    palette[12] = RGB24_TO_VDPCOLOR(0x008080);
    palette[13] = RGB24_TO_VDPCOLOR(0x808000);
    palette[14] = RGB24_TO_VDPCOLOR(0x800080);
    palette[15] = RGB24_TO_VDPCOLOR(0x808080);

    VDP_setPalette(PAL1, palette);

    score = scores;
    globalScore = 0;

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("Bitmap buffer clear", 2, 1);
    waitMs(3000);
    VDP_clearPlan(PLAN_A, TRUE);

    BMP_init(TRUE, PLAN_A, PAL1, FALSE);
    i = 1000;
    start = getTimeAsFix32(FALSE);
    while(i--) BMP_clear();
    end = getTimeAsFix32(FALSE);
    BMP_end();

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("Bitmap buffer clear", 2, 1);
    *score = displayResult(1000, end - start, 3);
    globalScore += *score++;

    VDP_drawText("Pixel draw simple", 2, 8);
    waitMs(4000);
    VDP_clearPlan(PLAN_A, TRUE);

    BMP_init(TRUE, PLAN_A, PAL1, FALSE);
    BMP_setBufferCopy(TRUE);
    time = FIX32(0);
    i = 150;
    while(i--)
    {
        Pixel *pix = pixels;
        u16 j = 600;

        initPixels(pixels, 600);

        start = getTimeAsFix32(FALSE);
        while(j)
        {
            BMP_setPixel(pix->pt.x, pix->pt.y, pix->col);
            pix++;
            BMP_setPixel(pix->pt.x, pix->pt.y, pix->col);
            pix++;
            BMP_setPixel(pix->pt.x, pix->pt.y, pix->col);
            pix++;
            BMP_setPixel(pix->pt.x, pix->pt.y, pix->col);
            pix++;
            BMP_setPixel(pix->pt.x, pix->pt.y, pix->col);
            pix++;
            BMP_setPixel(pix->pt.x, pix->pt.y, pix->col);
            pix++;
            BMP_setPixel(pix->pt.x, pix->pt.y, pix->col);
            pix++;
            BMP_setPixel(pix->pt.x, pix->pt.y, pix->col);
            pix++;
            BMP_setPixel(pix->pt.x, pix->pt.y, pix->col);
            pix++;
            BMP_setPixel(pix->pt.x, pix->pt.y, pix->col);
            pix++;

            j -= 10;
        }
        end = getTimeAsFix32(FALSE);

        BMP_flip(TRUE);

        time += end - start;
    }
    BMP_end();

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("Pixel draw simple", 2, 1);
    *score = displayResult2(150 * 600, time, 3);
    globalScore += *score++;

    VDP_drawText("Pixel draw array", 2, 8);
    waitMs(4000);
    VDP_clearPlan(PLAN_A, TRUE);

    time = FIX32(0);
    i = 150;
    BMP_init(TRUE, PLAN_A, PAL1, FALSE);
    BMP_setBufferCopy(TRUE);
    start = getTimeAsFix32(FALSE);
    while(i--)
    {
        initPixels(pixels, 750);

        start = getTimeAsFix32(FALSE);
        BMP_setPixels(pixels, 750);
        end = getTimeAsFix32(FALSE);

        BMP_flip(TRUE);

        time += end - start;
    }
    BMP_end();

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("Pixel draw array", 2, 1);
    *score = displayResult2(150 * 750, time, 3);
    globalScore += *score++;

    VDP_drawText("Line draw", 2, 8);
    waitMs(4000);
    VDP_clearPlan(PLAN_A, TRUE);

    BMP_init(TRUE, PLAN_A, PAL1, FALSE);
    BMP_setBufferCopy(TRUE);
    time = FIX32(0);
    i = 100;
    while(i--)
    {
        Line *lin = lines;
        u16 j = 130;

        initLines(lines, 130);

        start = getTimeAsFix32(FALSE);
        while(j)
        {
            BMP_drawLine(lin++);
            BMP_drawLine(lin++);
            BMP_drawLine(lin++);
            BMP_drawLine(lin++);
            BMP_drawLine(lin++);
            BMP_drawLine(lin++);
            BMP_drawLine(lin++);
            BMP_drawLine(lin++);
            BMP_drawLine(lin++);
            BMP_drawLine(lin++);

            j -= 10;
        }
        end = getTimeAsFix32(FALSE);

        time += end - start;

        BMP_flip(TRUE);
    }
    BMP_end();

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("Line draw", 2, 1);
    *score = displayResult3(100 * 130, time, 3);
    globalScore += *score++;

    VDP_drawText("Polygon draw", 2, 8);
    waitMs(4000);
    VDP_clearPlan(PLAN_A, TRUE);

    BMP_init(TRUE, PLAN_A, PAL1, FALSE);
    BMP_setBufferCopy(TRUE);
    time = FIX32(0);
    i = 100;
    while(i--)
    {
        Polygone *pol = polys;
        u16 j = 25;

        initPolys(pts, pol, 0, 25);

        start = getTimeAsFix32(FALSE);
        while(j--)
        {
            BMP_drawPolygon(pol->pts, pol->numPts, pol->col);
            pol++;
        }
        end = getTimeAsFix32(FALSE);

        time += end - start;

        BMP_flip(TRUE);
    }
    BMP_end();

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("Polygon draw", 2, 1);
    *score = displayResult(100 * 25, time, 3);
    globalScore += *score++;

    VDP_drawText("128x64 image draw (packed slow)", 2, 8);
    waitMs(4000);
    VDP_clearPlan(PLAN_A, TRUE);

    VDP_setPalette(PAL1, logo_med_bmp.palette->data);
    BMP_init(TRUE, PLAN_A, PAL1, FALSE);
    BMP_setBufferCopy(TRUE);
    time = FIX32(0);
    i = 25;
    while(i--)
    {
        Vect2D_s16 *pos;
        u16 j;

        // init image positions
        for(j = 0; j < 5; j++)
        {
            pts[j].x = random() % (256 - 128);
            pts[j].y = random() % (160 - 64);
        }

        pos = pts;
        j = 5;
        start = getTimeAsFix32(FALSE);
        while(j--)
        {
            BMP_drawBitmap(&logo_med_bmp, pos->x, pos->y, FALSE);
            pos++;
        }
        end = getTimeAsFix32(FALSE);

        time += end - start;

        BMP_flip(TRUE);
    }
    BMP_end();

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("128x64 image draw (packed slow)", 2, 1);
    *score = displayResult(25 * 5, time, 3);
    globalScore += *score++;

    VDP_drawText("128x64 image draw (packed fast)", 2, 8);
    waitMs(4000);
    VDP_clearPlan(PLAN_A, TRUE);

    VDP_setPalette(PAL1, logo_med_bmp.palette->data);
    BMP_init(TRUE, PLAN_A, PAL1, FALSE);
    BMP_setBufferCopy(TRUE);
    time = FIX32(0);
    i = 35;
    while(i--)
    {
        Vect2D_s16 *pos;
        u16 j;

        // init image positions
        for(j = 0; j < 8; j++)
        {
            pts[j].x = random() % (256 - 128);
            pts[j].y = random() % (160 - 64);
        }

        pos = pts;
        j = 8;
        start = getTimeAsFix32(FALSE);
        while(j--)
        {
            BMP_drawBitmap(&logo_med_bmp_f, pos->x, pos->y, FALSE);
            pos++;
        }
        end = getTimeAsFix32(FALSE);

        time += end - start;

        BMP_flip(TRUE);
    }
    BMP_end();

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("128x64 image draw (packed fast)", 2, 1);
    *score = displayResult(35 * 8, time, 3);
    globalScore += *score++;

    VDP_drawText("128x64 image draw (unpacked)", 2, 8);
    waitMs(4000);
    VDP_clearPlan(PLAN_A, TRUE);

    // unpack bitmap
    bmp = unpackBitmap(&logo_med_bmp, NULL);
    VDP_setPalette(PAL1, logo_med_bmp.palette->data);
    BMP_init(TRUE, PLAN_A, PAL1, FALSE);
    BMP_setBufferCopy(TRUE);
    time = FIX32(0);
    i = 50;
    while(i--)
    {
        Vect2D_s16 *pos;
        u16 j;

        // init image positions
        for(j = 0; j < 10; j++)
        {
            pts[j].x = random() % (256 - 128);
            pts[j].y = random() % (160 - 64);
        }

        pos = pts;
        j = 10;
        start = getTimeAsFix32(FALSE);
        while(j--)
        {
            BMP_drawBitmap(bmp, pos->x, pos->y, FALSE);
            pos++;
        }
        end = getTimeAsFix32(FALSE);

        time += end - start;

        BMP_flip(TRUE);
    }
    BMP_end();
    MEM_free(bmp);

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("128x64 image draw (unpacked)", 2, 1);
    *score = displayResult(50 * 10, time, 3);
    globalScore += *score++;

    VDP_drawText("64x32 image draw (packed slow)", 2, 8);
    waitMs(4000);
    VDP_clearPlan(PLAN_A, TRUE);

    VDP_setPalette(PAL1, logo_sm_bmp.palette->data);
    BMP_init(TRUE, PLAN_A, PAL1, FALSE);
    BMP_setBufferCopy(TRUE);
    time = FIX32(0);
    i = 30;
    while(i--)
    {
        Vect2D_s16 *pos;
        u16 j;

        // init image positions
        for(j = 0; j < 10; j++)
        {
            pts[j].x = random() % (256 - 64);
            pts[j].y = random() % (160 - 32);
        }

        pos = pts;
        j = 10;
        start = getTimeAsFix32(FALSE);
        while(j--)
        {
            BMP_drawBitmap(&logo_sm_bmp, pos->x, pos->y, FALSE);
            pos++;
        }
        end = getTimeAsFix32(FALSE);

        time += end - start;

        BMP_flip(TRUE);
    }
    BMP_end();

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("64x32 image draw (packed slow)", 2, 1);
    *score = displayResult(30 * 10, time, 3);
    globalScore += *score++;

    VDP_drawText("64x32 image draw (packed fast)", 2, 8);
    waitMs(4000);
    VDP_clearPlan(PLAN_A, TRUE);

    VDP_setPalette(PAL1, logo_sm_bmp.palette->data);
    BMP_init(TRUE, PLAN_A, PAL1, FALSE);
    BMP_setBufferCopy(TRUE);
    time = FIX32(0);
    i = 50;
    while(i--)
    {
        Vect2D_s16 *pos;
        u16 j;

        // init image positions
        for(j = 0; j < 25; j++)
        {
            pts[j].x = random() % (256 - 64);
            pts[j].y = random() % (160 - 32);
        }

        pos = pts;
        j = 25;
        start = getTimeAsFix32(FALSE);
        while(j--)
        {
            BMP_drawBitmap(&logo_sm_bmp_f, pos->x, pos->y, FALSE);
            pos++;
        }
        end = getTimeAsFix32(FALSE);

        time += end - start;

        BMP_flip(TRUE);
    }
    BMP_end();

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("64x32 image draw (packed fast)", 2, 1);
    *score = displayResult(50 * 25, time, 3);
    globalScore += *score++;

    VDP_drawText("64x32 image draw (unpacked)", 2, 8);
    waitMs(4000);
    VDP_clearPlan(PLAN_A, TRUE);

    // unpack bitmap
    bmp = unpackBitmap(&logo_sm_bmp, NULL);
    VDP_setPalette(PAL1, logo_sm_bmp.palette->data);
    BMP_init(TRUE, PLAN_A, PAL1, FALSE);
    BMP_setBufferCopy(TRUE);
    time = FIX32(0);
    i = 50;
    while(i--)
    {
        Vect2D_s16 *pos;
        u16 j;

        // init image positions
        for(j = 0; j < 30; j++)
        {
            pts[j].x = random() % (256 - 64);
            pts[j].y = random() % (160 - 32);
        }

        pos = pts;
        j = 30;
        start = getTimeAsFix32(FALSE);
        while(j--)
        {
            BMP_drawBitmap(bmp, pos->x, pos->y, FALSE);
            pos++;
        }
        end = getTimeAsFix32(FALSE);

        time += end - start;

        BMP_flip(TRUE);
    }
    BMP_end();
    MEM_free(bmp);

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("64x32 image draw (unpacked)", 2, 1);
    *score = displayResult(50 * 30, time, 3);
    globalScore += *score++;

    VDP_drawText("128x64 image scaling", 2, 8);
    waitMs(4000);
    VDP_clearPlan(PLAN_A, TRUE);

    // unpack bitmap
    bmp = unpackBitmap(&logo_med_x2_bmp, NULL);
    VDP_setPalette(PAL1, logo_med_x2_bmp.palette->data);
    BMP_init(TRUE, PLAN_A, PAL1, FALSE);
    time = FIX32(0);
    i = 4;
    while(i--)
    {
        u16 j;

        for(j = 32; j < 128; j++)
        {
            start = getTimeAsFix32(FALSE);
            BMP_drawBitmapScaled(bmp, 128 - j, 80 - (j/2), j*2, j, FALSE);
            end = getTimeAsFix32(FALSE);
            time += end - start;
            j += j >> 5;
            BMP_showFPS(FALSE);
            BMP_flip(TRUE);
        }
        for(j = 127; j >= 32; j--)
        {
            start = getTimeAsFix32(FALSE);
            BMP_drawBitmapScaled(bmp, 128 - j, 80 - (j/2), j*2, j, FALSE);
            end = getTimeAsFix32(FALSE);
            time += end - start;
            j -= j >> 5;
            BMP_showFPS(FALSE);
            BMP_flip(TRUE);
        }
    }
    BMP_end();
    MEM_free(bmp);

    VDP_clearPlan(PLAN_A, TRUE);
    VDP_drawText("64x32 image scaling", 2, 1);
    *score = displayResult(2 * 2 * (128-32), time, 3) * 10;
    globalScore += *score++;

    waitMs(5000);
    VDP_clearPlan(PLAN_A, TRUE);

    MEM_free(polys);
    MEM_free(pts);
    MEM_free(pixels);
    MEM_free(lines);

    return globalScore;
}


static void initPixels(Pixel *pixels, u16 num)
{
    u16 i = num;
    Pixel *p = pixels;

    while(i--)
    {
        const u16 r = random();

        p->pt.x = r >> 8;
        // allow to handle clip
        p->pt.y = (r & 0xFF) - 64;
        p->col = random() & 0xF;
        p->col |= p->col << 4;
        p++;
    }
}

static void initLines(Line *lines, u16 num)
{
    u16 i = num;
    Line *l = lines;

    while(i--)
    {
        const u16 r1 = random();
        const u16 r2 = random();

        l->pt1.x = r1 >> 8;
        // allow to handle clip
        l->pt1.y = (r1 & 0xFF) - 64;
        l->pt2.x = r2 >> 8;
        // allow to handle clip
        l->pt2.y = (r2 & 0xFF) - 64;
        l->col = random() & 0xF;
        l->col |= l->col << 4;
        l++;
    }
}

static void initPolys(Vect2D_s16 *pts, Polygone *polys, u16 numPts, u16 num)
{
    u16 i = num;
    Polygone *p = polys;
    Vect2D_s16 *curPts = pts;

    if (numPts)
    {
        while(i--)
        {
            do
            {
                u16 j = numPts;
                Vect2D_s16 *c = curPts;

                while(j--)
                {
                    const u16 r = random();

                    c->x = r >> 8;
                    // allow to handle clip
                    c->y = (r & 0xFF) - 64;
                    c++;
                }
            }
            while (BMP_isPolygonCulled(curPts, numPts));

            p->pts = curPts;
            p->numPts = numPts;
            p->col = random() & 0xF;
            p->col |= p->col << 4;

            // next
            curPts += numPts;
            p++;
        }
    }
    else
    {
        while(i--)
        {
            // dynamic number fo points
            numPts = (random() & 3) + 3;

            do
            {
                u16 j = numPts;
                Vect2D_s16 *c = curPts;

                while(j--)
                {
                    const u16 r = random();

                    c->x = r >> 8;
                    // allow to handle clip
                    c->y = (r & 0xFF) - 64;
                    c++;
                }
            }
            while (BMP_isPolygonCulled(curPts, numPts));

            p->pts = curPts;
            p->numPts = numPts;
            p->col = random() & 0xF;
            p->col |= p->col << 4;

            // next
            curPts += numPts;
            p++;
        }
    }
}


static u16 displayResult(u32 op, fix32 time, u16 y)
{
    char timeStr[32];
    char speedStr[32];
    char str[64];
    fix32 speed;

    fix32ToStr(time, timeStr, 2);
    speed = intToFix32(op);
    // get speed in op/s
    speed = fix32Div(speed, time);
    // put it in speedStr
    intToStr(fix32ToRoundedInt(speed), speedStr, 1);

    strcpy(str, "Elapsed time = ");
    strcat(str, timeStr);
    strcat(str, "s (");
    strcat(str, speedStr);
    strcat(str, " op/s)");

    // display test string
    VDP_drawText(str, 3, y);

    return fix32ToInt(speed);
}

static u16 displayResult2(u32 op, fix32 time, u16 y)
{
    char timeStr[32];
    char speedStr[32];
    char str[64];
    fix32 speed;

    fix32ToStr(time, timeStr, 2);
    speed = intToFix32(op / 100);
    // get speed in op/s
    speed = fix32Div(speed, time);
    // put it in speedStr
    intToStr(fix32ToRoundedInt(speed) * 100, speedStr, 1);

    strcpy(str, "Elapsed time = ");
    strcat(str, timeStr);
    strcat(str, "s (");
    strcat(str, speedStr);
    strcat(str, " op/s)");

    // display test string
    VDP_drawText(str, 3, y);

    return fix32ToInt(speed);
}

static u16 displayResult3(u32 op, fix32 time, u16 y)
{
    char timeStr[32];
    char speedStr[32];
    char str[64];
    fix32 speed;

    fix32ToStr(time, timeStr, 2);
    speed = intToFix32(op / 10);
    // get speed in op/s
    speed = fix32Div(speed, time);
    // put it in speedStr
    intToStr(fix32ToRoundedInt(speed) * 10, speedStr, 1);

    strcpy(str, "Elapsed time = ");
    strcat(str, timeStr);
    strcat(str, "s (");
    strcat(str, speedStr);
    strcat(str, " op/s)");

    // display test string
    VDP_drawText(str, 3, y);

    return fix32ToInt(speed);
}
