#include "config.h"
#include "types.h"

#include "vdp.h"
#include "bmp_cmn.h"
#include "bmp_intr.h"

#include "vdp_tile.h"
#include "vdp_pal.h"
#include "vdp_bg.h"

#include "tab_vram.h"

#ifdef ENABLE_BMP


u8 bmp_buffer_0[BMP_WIDTH * BMP_HEIGHT];
u8 bmp_buffer_1[BMP_WIDTH * BMP_HEIGHT];

u16 (*doBlit)();


// not shared
extern void drawLineFF(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col);
extern void drawLine(u16 offset, s16 dx, s16 dy, s16 step_x, s16 step_y, u8 col);


void _bmp_init()
{
    // need 64x64 cells sized plan
    VDP_setPlanSize(BMP_PLANWIDTH, BMP_PLANHEIGHT);

    // clear plan (complete tilemap)
    VDP_clearPlan(BMP_PLAN, 1);
    VDP_waitDMACompletion();

    // set to -1 to force flags update on new BMP_setFlags(...)
    bmp_flags = -1;
    bmp_state = 0;

    // default
    bmp_buffer_read = bmp_buffer_0;
    bmp_buffer_write = bmp_buffer_1;
}

void _bmp_setFlags(u16 value)
{
    bmp_flags = value;

    // flag dependancies
    if (bmp_flags & BMP_ENABLE_EXTENDEDBLANK) bmp_flags |= BMP_ENABLE_BLITONBLANK;
    if (bmp_flags & BMP_ENABLE_BLITONBLANK) bmp_flags |= BMP_ENABLE_ASYNCFLIP;
    if (bmp_flags & BMP_ENABLE_ASYNCFLIP) bmp_flags |= BMP_ENABLE_WAITVSYNC;

    // clear pending task
    bmp_state &= ~(BMP_STAT_FLIPWAITING | BMP_STAT_BLITTING);
}


void _bmp_doFlip()
{
    // wait for DMA completion if used otherwise VDP writes can be corrupted
    VDP_waitDMACompletion();

    // copy tile buffer to VRAM
    if ((*doBlit)())
    {
        u16 vscr;

        // display FPS
        if HAS_FLAG(BMP_ENABLE_FPSDISPLAY) BMP_showFPS();

        // switch displayed buffer
        if READ_IS_FB0 vscr = ((BMP_PLANHEIGHT * BMP_YPIXPERTILE) * 0) / 2;
        else vscr = ((BMP_PLANHEIGHT * BMP_YPIXPERTILE) * 1) / 2;
        VDP_setVerticalScroll(BMP_PLAN, 0, vscr);

        // flip done
        bmp_state &= ~BMP_STAT_FLIPWAITING;
    }
}


#endif // ENABLE_BMP
