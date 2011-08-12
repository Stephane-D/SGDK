/**
 * \file bmp_intr.c
 * \brief Software bitmap engine (interface)
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides common interfaces for bitmap engines.
 */

#include "config.h"
#include "types.h"

#include "memory.h"

#include "vdp.h"
#include "bmp_cmn.h"
#include "bmp_intr.h"

#include "vdp_tile.h"
#include "vdp_pal.h"
#include "vdp_bg.h"

#include "tab_vram.h"


u8 *bmp_buffer_0 = NULL;
u8 *bmp_buffer_1 = NULL;

// used for polygone drawing
s16 *LeftPoly = NULL;
s16 *RightPoly = NULL;

u16 (*doBlit)();


void _bmp_init()
{
    // release first if needed
    if (bmp_buffer_0) MEM_free(bmp_buffer_0);
    if (bmp_buffer_1) MEM_free(bmp_buffer_1);
    if (LeftPoly) MEM_free(LeftPoly);
    if (RightPoly) MEM_free(RightPoly);

    // tile map allocation
    bmp_buffer_0 = MEM_alloc(BMP_WIDTH * BMP_HEIGHT * sizeof(u8));
    bmp_buffer_1 = MEM_alloc(BMP_WIDTH * BMP_HEIGHT * sizeof(u8));
    // polygon edge buffer allocation
    LeftPoly = MEM_alloc(BMP_HEIGHT * sizeof(s16));
    RightPoly = MEM_alloc(BMP_HEIGHT * sizeof(s16));

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

void _bmp_end()
{
    // release memory
    if (bmp_buffer_0)
    {
        MEM_free(bmp_buffer_0);
        bmp_buffer_0 = NULL;
    }
    if (bmp_buffer_1)
    {
        MEM_free(bmp_buffer_1);
        bmp_buffer_1 = NULL;
    }
    if (LeftPoly)
    {
        MEM_free(LeftPoly);
        LeftPoly = NULL;
    }
    if (RightPoly)
    {
        MEM_free(RightPoly);
        RightPoly = NULL;
    }
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
        if (HAS_FLAG(BMP_ENABLE_FPSDISPLAY)) BMP_showFPS(0);

        // switch displayed buffer
        if (READ_IS_FB0) vscr = ((BMP_PLANHEIGHT * BMP_YPIXPERTILE) * 0) / 2;
        else vscr = ((BMP_PLANHEIGHT * BMP_YPIXPERTILE) * 1) / 2;

        VDP_setVerticalScroll(BMP_PLAN, 0, vscr);

        // flip done
        bmp_state &= ~BMP_STAT_FLIPWAITING;
    }
}
