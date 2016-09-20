#include <genesis.h>

#include "gfx.h"


#define SGDK_BENCHMARK      "SGDK benchmark v1.2"

#define MAX_TEST            9
#define MAX_SUBTEST         16


u16 detailledScores[MAX_TEST][MAX_SUBTEST];
u16 scores[MAX_TEST];


// extern
u16 executeMemsetTest(u16 *scores);
u16 executeMemcpyTest(u16 *scores);
u16 executeMemAllocTest(u16 *scores);
u16 executeVRamAllocTest(u16 *scores);
u16 executeMathsBasicTest(u16 *scores);
u16 executeMathsAdvTest(u16 *scores);
u16 executeBGTest(u16 *scores);
u16 executeBMPTest(u16 *scores);
u16 executeSpritesTest(u16 *scores);


// forward
static void showIntroScreen();
static void preTest(char *title, s16 num);
static void postTest(char *title, u16 score, s16 num);
static void postResume(u32 score);


int main()
{
    s16 testNum;
    u16 score;
    u32 globalScore;

    showIntroScreen();

    while(TRUE)
    {
        // clear scores table
        memset(scores, 0, sizeof(scores));

        testNum = 0;
        globalScore = 0;

        preTest("Memory set test", testNum);
        score = executeMemsetTest(detailledScores[testNum]);
        scores[testNum] = score;
        postTest("Memory set test", score, testNum);
        globalScore += score;
        testNum++;

        preTest("Memory copy test", testNum);
        score = executeMemcpyTest(detailledScores[testNum]);
        scores[testNum] = score;
        postTest("Memory copy test", score, testNum);
        globalScore += score;
        testNum++;

        preTest("Memory alloc/release test", testNum);
        score = executeMemAllocTest(detailledScores[testNum]);
        scores[testNum] = score;
        postTest("Memory alloc/release test", score, testNum);
        globalScore += score;
        testNum++;

        preTest("VRAM alloc/release test", testNum);
        score = executeVRamAllocTest(detailledScores[testNum]);
        scores[testNum] = score;
        postTest("VRAM alloc/release test", score, testNum);
        globalScore += score;
        testNum++;

        preTest("Maths basic test", testNum);
        score = executeMathsBasicTest(detailledScores[testNum]);
        scores[testNum] = score;
        postTest("Maths basic test", score, testNum);
        globalScore += score;
        testNum++;

        preTest("Maths advanced test", testNum);
        score = executeMathsAdvTest(detailledScores[testNum]);
        scores[testNum] = score;
        postTest("Maths advanced test", score, testNum);
        globalScore += score;
        testNum++;

        preTest("VDP background test", testNum);
        score = executeBGTest(detailledScores[testNum]);
        scores[testNum] = score;
        postTest("VDP background test", score, testNum);
        globalScore += score;
        testNum++;

        preTest("Bitmap mode test", testNum);
        score = executeBMPTest(detailledScores[testNum]);
        scores[testNum] = score;
        postTest("Bitmap mode test", score, testNum);
        globalScore += score;
        testNum++;

        preTest("Sprite test", testNum);
        score = executeSpritesTest(detailledScores[testNum]);
        scores[testNum] = score;
        postTest("Sprite test", score, testNum);
        globalScore += score;
        testNum++;

        postResume(globalScore);

        JOY_waitPress(JOY_1, BUTTON_START);
        // fade text
        VDP_fadeOut(15, 15, 30, FALSE);
        // clear text
        VDP_clearPlan(PLAN_A, TRUE);

        // reset text color to white
        VDP_setPaletteColor(15, 0xEEE);
    }
}


static void showIntroScreen()
{
    u16 col = 0xEEE;

    // set text color to black
    VDP_setPaletteColor(15, 0x000);

    // display test string
    VDP_drawText(SGDK_BENCHMARK, 10, 10);
    VDP_drawText("Press START to begin", 10, 14);

    // fade text color to white
    VDP_fadeIn(15, 15, &col, 30, FALSE);
    // wait for Start button pressed
    JOY_waitPress(JOY_1, BUTTON_START);
    // fade text
    VDP_fadeOut(15, 15, 30, FALSE);
    // clear text
    VDP_clearPlan(PLAN_A, TRUE);

    // reset text color to white
    VDP_setPaletteColor(15, 0xEEE);
}

static void preTest(char *title, s16 num)
{
    char str[64];
    u16 col = 0xEEE;

    // set text color to black
    VDP_setPaletteColor(15, 0x000);

    // display test string
    if (num == -1) strcpy(str, title);
    else sprintf(str, "%s (%d/%d)", title, num + 1, MAX_TEST);
    VDP_drawText(str, 1, 1);

    // fade text color to white
    VDP_fadeIn(15, 15, &col, 30, FALSE);
    // wait 3s
    waitMs(3000);
    // fade text
    VDP_fadeOut(15, 15, 30, FALSE);
    // clear text
    VDP_clearPlan(PLAN_A, TRUE);

    // reset text color to white
    VDP_setPaletteColor(15, 0xEEE);
}

static void postTest(char *title, u16 score, s16 num)
{
    char str[64];
    u16 col = 0xEEE;

    // set text color to black
    VDP_setPaletteColor(15, 0x000);

    // display test string
    if (num == -1) strcpy(str, title);
    else sprintf(str, "%s (%d/%d)", title, num + 1, MAX_TEST);
    VDP_drawText(str, 1, 1);
    sprintf(str, "Score = %d", score);
    VDP_drawText(str, 2, 3);

    // fade text color to white
    VDP_fadeIn(15, 15, &col, 30, FALSE);
    // wait 5s
    waitMs(5000);
    // fade text
    VDP_fadeOut(15, 15, 30, FALSE);
    // clear text
    VDP_clearPlan(PLAN_A, TRUE);

    // reset text color to white
    VDP_setPaletteColor(15, 0xEEE);
}

static void postResume(u32 score)
{
    char str[64];
    u16 testNum;
    u16 y;
    u16 col = 0xEEE;

    // set text color to black
    VDP_setPaletteColor(15, 0x000);

    // display test string
    sprintf(str, "%s score = %d", SGDK_BENCHMARK, score);
    VDP_drawText(str, 1, 1);

    testNum = 0;
    y = 4;
    sprintf(str, "Memory set score = %d", scores[testNum++]);
    VDP_drawText(str, 4, y);
    y += 2;
    sprintf(str, "Memory copy score = %d", scores[testNum++]);
    VDP_drawText(str, 4, y);
    y += 2;
    sprintf(str, "Memory alloc/release score = %d", scores[testNum++]);
    VDP_drawText(str, 4, y);
    y += 2;
    sprintf(str, "VRAM alloc/release score = %d", scores[testNum++]);
    VDP_drawText(str, 4, y);
    y += 2;
    sprintf(str, "Maths basic score = %d", scores[testNum++]);
    VDP_drawText(str, 4, y);
    y += 2;
    sprintf(str, "Maths advanced score = %d", scores[testNum++]);
    VDP_drawText(str, 4, y);
    y += 2;
    sprintf(str, "VDP background score = %d", scores[testNum++]);
    VDP_drawText(str, 4, y);
    y += 2;
    sprintf(str, "Bitmap mode score = %d", scores[testNum++]);
    VDP_drawText(str, 4, y);
    y += 2;
    sprintf(str, "Sprite mode score = %d", scores[testNum++]);
    VDP_drawText(str, 4, y);

    VDP_drawText(" PRESS START TO RESTART ALL TESTS", 1, 24);

    // fade text color to white
    VDP_fadeIn(15, 15, &col, 30, FALSE);
}
