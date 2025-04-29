#include <genesis.h>

#include "inc/main.h"
#include "inc/level.h"
#include "inc/camera.h"

#include "res/gfx.h"


// forward
static u16 execute(u16 hscrollSpeed, u16 vscrollSpeed, bool lowLevel);


u16 executeMapTest(u16 *scores)
{
    u16 *score;
    u16 globalScore;

    score = scores;
    globalScore = 0;

    VDP_resetScreen();
    VDP_drawText("Slow H scrolling", 2, 0);
    *score = execute(3, 0, FALSE);
    globalScore += *score++;

    VDP_resetScreen();
    VDP_drawText("Slow V scrolling", 2, 0);
    *score = execute(0, 3, FALSE);
    globalScore += *score++;

    VDP_resetScreen();
    VDP_drawText("Slow HV scrolling", 2, 0);
    *score = execute(3, 3, FALSE);
    globalScore += *score++;

    VDP_resetScreen();
    VDP_drawText("Fast H scrolling", 2, 0);
    *score = execute(24, 0, FALSE);
    globalScore += *score++;

    VDP_resetScreen();
    VDP_drawText("Fast V scrolling", 2, 0);
    *score = execute(0, 24, FALSE);
    globalScore += *score++;

    VDP_resetScreen();
    VDP_drawText("Fast HV scrolling", 2, 0);
    *score = execute(24, 24, FALSE);
    globalScore += *score++;
    VDP_resetScreen();

    VDP_resetScreen();
    VDP_drawText("Slow H scrolling (alternate)", 2, 0);
    *score = execute(3, 0, TRUE);
    globalScore += *score++;

    VDP_resetScreen();
    VDP_drawText("Slow V scrolling (alternate)", 2, 0);
    *score = execute(0, 3, TRUE);
    globalScore += *score++;

    VDP_resetScreen();
    VDP_drawText("Slow HV scrolling (alternate)", 2, 0);
    *score = execute(3, 3, TRUE);
    globalScore += *score++;

    VDP_resetScreen();
    VDP_drawText("Fast H scrolling (alternate)", 2, 0);
    *score = execute(24, 0, TRUE);
    globalScore += *score++;

    VDP_resetScreen();
    VDP_drawText("Fast V scrolling (alternate)", 2, 0);
    *score = execute(0, 24, TRUE);
    globalScore += *score++;

    VDP_resetScreen();
    VDP_drawText("Fast HV scrolling (alternate)", 2, 0);
    *score = execute(24, 24, TRUE);
    globalScore += *score++;

    VDP_resetScreen();

    return globalScore;
}

static u16 execute(u16 hscrollSpeed, u16 vscrollSpeed, bool lowLevel)
{
    waitMs(2000);
    PAL_fadeOutAll(20, FALSE);

    s16 x = MIN_POSX;
    s16 y = MAX_POSY;
    s16 sx = hscrollSpeed;
    s16 sy = vscrollSpeed;

    LEVEL_init(TILE_USER_INDEX);
    CAMERA_init(lowLevel);

    // increase DMA capacity
    DMA_setMaxQueueSize(200);
    DMA_setBufferSize(10000);
    DMA_setMaxTransferSize(10000);

    CAMERA_centerOn(x, y);
    SYS_doVBlankProcess();
    LEVEL_onVBlank();

    // can restore default DMA capacity
    DMA_setMaxQueueSizeToDefault();
    DMA_setBufferSizeToDefault();
    DMA_setMaxTransferSizeToDefault();

    SYS_showFrameLoad(TRUE);
    PAL_fadeInAll(bg_palette.data, 20, FALSE);

    u32 freeCpuTime = 0;
    u32 startTime = getTime(TRUE);
    u32 endTime = startTime + (6 << 8);

    while(getTime(TRUE) < endTime)
    {
        CAMERA_centerOn(x, y);

        x += sx;
        y += sy;

        if ((x < MIN_POSX) || (x > MAX_POSX))
        {
            sx = -sx;
            x += sx;
        }
        if ((y < MIN_POSY) || (y > MAX_POSY))
        {
            sy = -sy;
            y += sy;
        }

        SYS_doVBlankProcess();
        LEVEL_onVBlank();
        s16 cpuLoad = SYS_getCPULoad();
        cpuLoad -= (hscrollSpeed + vscrollSpeed) >> 1;

        if (cpuLoad < 50) freeCpuTime += (50 - cpuLoad);
    }

    PAL_fadeOutAll(20, FALSE);

    SYS_hideFrameLoad();
    LEVEL_end();
    MEM_pack();

    return freeCpuTime >> 7;
}
