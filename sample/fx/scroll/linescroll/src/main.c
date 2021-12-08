#include <genesis.h>

#include "resources.h"

#define SCROLL_BASE_SPEED       2.0

#define SCROLL_END_SPEED        (SCROLL_BASE_SPEED * 2)
#define SCROLL_OFFSET_SPEED     (SCROLL_END_SPEED - SCROLL_BASE_SPEED)

#define SCROLL_WIDTH          96
#define SCROLL_HEIGHT       80


// forward
static void initScrollTables();


static f16 scrollLoop[224];
static f16 scrollSpeed[224];
static f16 scroll[224];
static s16 scrollI[224];

int main()
{
    // Setup VDP
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);
    VDP_setPaletteColors(0, palette_black, 64);

    // because we like to have it =)
    SYS_showFrameLoad(TRUE);

    // Setup graphics
    VDP_drawImageEx(BG_A, &bg_img, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USERINDEX), 0, 8, FALSE, TRUE);

    // Fade in graphics
    PAL_fadeIn(0, 15, bg_img.palette->data, 30, TRUE);

    // init scroll tables
    initScrollTables();

    // wait fade done
    PAL_waitFadeCompletion();

    s16 scrollX = 0;

    while (TRUE)
    {
        u16 joy = JOY_readJoypad(JOY_1);

        if (joy & BUTTON_RIGHT)
        {
            scrollX--;

            // loop ?
            if (scrollI[64 + SCROLL_HEIGHT] <= -SCROLL_WIDTH)
            {
                scrollX += SCROLL_WIDTH;

                // set scroll to loop point
                for(u16 i = 0; i < 224; i++)
                    scroll[i] = 0;//scrollLoop[i];
            }
            else
            {
                // just update scroll
                for(u16 i = 0; i < 224; i++)
                    scroll[i] -= scrollSpeed[i];
            }
        }
        else if (joy & BUTTON_LEFT)
        {
            scrollX++;

            // loop ?
            if (scrollI[64 + SCROLL_HEIGHT] >= 0)
            {
                scrollX -= SCROLL_WIDTH;

                // set scroll to loop point
                for(u16 i = 0; i < 224; i++)
                    scroll[i] = -scrollLoop[i];
            }
            else
            {
                // just update scroll
                for(u16 i = 0; i < 224; i++)
                    scroll[i] += scrollSpeed[i];
            }
        }

        // update scroll from fix16 scroll table
        for(u16 i = 0; i < 224; i++)
            scrollI[i] = fix16ToInt(scroll[i]);

        // update line scroll using DMA queue
        VDP_setHorizontalScrollLine(BG_A, 0, scrollI, 224, DMA_QUEUE);

        SYS_doVBlankProcess();
    }
}

static void initScrollTables()
{
    s16 i;

    // init scroll offset
    memsetU16((u16*) scroll, FIX16(0), 224);

    // init scroll speed
    for(i = 0; i < 64; i++)
        scrollSpeed[i] = FIX16(SCROLL_BASE_SPEED);
    for(i = 64; i < 64 + SCROLL_HEIGHT; i++)
    {
        f16 f;

        f = intToFix16(i - 64);
        f = fix16Mul(FIX16(SCROLL_OFFSET_SPEED), f);
        f = fix16Div(f, FIX16(SCROLL_HEIGHT));
        f += FIX16(SCROLL_BASE_SPEED);

        scrollSpeed[i] = f;
    }
    for(i = 64 + SCROLL_HEIGHT; i < 224; i++)
        scrollSpeed[i] = FIX16(SCROLL_END_SPEED);

    // init scroll loop
    for(i = 0; i < 64; i++)
        scrollLoop[i] = FIX16(SCROLL_WIDTH / SCROLL_END_SPEED);
    for(i = 64; i < 64 + SCROLL_HEIGHT; i++)
    {
        f32 f;

        f = intToFix32(i - 64);
        f = fix32Mul(FIX32(SCROLL_OFFSET_SPEED), f);
        f += FIX32(SCROLL_BASE_SPEED * SCROLL_HEIGHT);
        f = fix32Mul(FIX32(SCROLL_WIDTH / SCROLL_END_SPEED), f);
        f = fix32Div(f, FIX32(SCROLL_HEIGHT));

        scrollLoop[i] = fix32ToFix16(f);
    }
    for(i = 64 + SCROLL_HEIGHT; i < 224; i++)
        scrollLoop[i] = FIX16(SCROLL_WIDTH);
}
