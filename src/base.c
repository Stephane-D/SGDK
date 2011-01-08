#include "config.h"
#include "types.h"

#include "base.h"

#include "vdp.h"
#include "vdp_pal.h"
#include "psg.h"
#include "ym2612.h"
#include "joy.h"
#include "z80_ctrl.h"
#include "maths.h"
#include "bmp.h"
#include "logo_lib.h"
#include "timer.h"
#include "vdp.h"
#include "vdp_bg.h"


// we don't want to share them
extern u16 randbase;

// extern library callback function (we don't want to share them)
extern u16 VDP_doStepFading();
extern u16 BMP_doBlankProcess();

// main function
extern int main(u16 hard);

static void internal_reset();


static _voidCallback *VBlankCB;
static _voidCallback *HBlankCB;

u32 VBlankProcess;
u32 HBlankProcess;


void _buserror_callback()
{
    VDP_init();
    VDP_drawText("BUS ERROR !", 10, 10);
    while(1);
}

void _addresserror_callback()
{
    VDP_init();
    VDP_drawText("ADDRESS ERROR !", 10, 10);
    while(1);
}

void _illegalinst_callback()
{
    VDP_init();
    VDP_drawText("ILLEGAL INSTRUCTION !", 5, 10);
    while(1);
}

void _zerodivide_callback()
{
    VDP_init();
    VDP_drawText("DIVIDE BY ZERO !", 10, 10);
    while(1);
}

void _chkinst_callback()
{

}

void _trapvinst_callback()
{

}

void _privilegeviolation_callback()
{
    VDP_init();
    VDP_drawText("PRIVILEGE VIOLATION !", 5, 10);
    while(1);
}

void _trace_callback()
{

}

void _line1x1x_callback()
{

}

void _errorexception_callback()
{
    VDP_init();
    VDP_drawText("EXCEPTION ERROR !", 5, 10);
    while(1);
}

void _int_callback()
{

}


// VBlank Callback
void _vblank_callback()
{
    vtimer++;

    // joy state refresh
    JOY_update();

    // specials VBlank processing
    if (VBlankProcess)
    {
        // palette fading
        if (VBlankProcess & PROCESS_PALETTE_FADING)
        {
            if (!VDP_doStepFading()) VBlankProcess &= ~PROCESS_PALETTE_FADING;
        }
        // bitmap process
        if (VBlankProcess & PROCESS_BITMAP_TASK)
        {
            if (!BMP_doBlankProcess()) VBlankProcess &= ~PROCESS_BITMAP_TASK;
        }

        // ...
    }

    // then call user's callback
    if (VBlankCB) VBlankCB();
}

// HBlank Callback
void _hblank_callback()
{
    // special HBlank processing
    if (HBlankProcess)
    {
        // bitmap process
        if (HBlankProcess & PROCESS_BITMAP_TASK)
        {
            if (!BMP_doBlankProcess()) HBlankProcess &= ~PROCESS_BITMAP_TASK;
        }
        // ...
    }

    // then call user's callback
    if (HBlankCB) HBlankCB();
}


void _start_entry()
{
    // initiate random number generator
    randbase = 0xD94B;
    vtimer = 0;

    internal_reset();

#ifdef ENABLE_LOGO
    {
        u16 tmp_pal[16];

        // display logo (BMP mode use 128x160 resolution with doubled X pixel)
        BMP_init(BMP_ENABLE_WAITVSYNC | BMP_ENABLE_EXTENDEDBLANK);

#ifdef ZOOMING_LOGO
        // get the bitmap palette
        BMP_getGenBmp16Palette(logo_lib, tmp_pal);

        // init fade in to 30 step
        u16 step_fade = 30;
        if (VDP_initFading(0, 15, palette_black, tmp_pal, step_fade))
        {
            // prepare zoom
            u16 size = 128;
            // while zoom not completed
            while(size > 0)
            {
                // sort of log decrease
                if (size > 20) size = size - (size / 6);
                else if (size > 5) size -= 5;
                else size = 0;

                // get new lo
                const u32 w = 128 - size;

                // adjust palette for fade
                if (step_fade-- > 0) VDP_doStepFading();
                // zoom logo
                BMP_loadAndScaleGenBmp16(logo_lib, 32 + ((128 - w) >> 2), (128 - w) >> 1, w >> 1, w, 0xFF);
                // flip to screen
                BMP_flip();
            }

            // while fade not completed
            while(step_fade--)
            {
                VDP_waitVSync();
                VDP_doStepFading();
            }
        }

        // wait 1 second
        waitTick(TICKPERSECOND * 1);

#else
        // set palette 0 to black
        VDP_setPalette(0, palette_black);
        // get the bitmap palette
        BMP_getGenBmp16Palette(logo_lib, tmp_pal);
        // don't load the palette immediatly
        BMP_loadGenBmp16(logo_lib, 32, 0, 0xFF);
        // flip
        BMP_flip();
        // fade in logo
        VDP_fadePalIn(0, tmp_pal, 30, 0);

        // wait 1.5 second
        waitTick(TICKPERSECOND * 1.5);
#endif

        // fade out logo
        VDP_fadePalOut(0, 20, 0);
        // wait 0.5 second
        waitTick(TICKPERSECOND * 0.5);

        // shut down bmp mode
        BMP_end();
        // reinit vdp before program execution
        VDP_init();
    }
#endif

    // let's the fun go on !
    main(1);
}

void _reset_entry()
{
    internal_reset();
    main(0);
}


static void internal_reset()
{
    VBlankCB = NULL;
    HBlankCB = NULL;
    VBlankProcess = 0;
    HBlankProcess = 0;

    // init part
    VDP_init();
    PSG_init();
    JOY_init();
    // reseting z80 also reset the ym2612
    Z80_init();
}

// assert reset
void assert_reset()
{
    asm("reset\n\t");
}

// soft reset
void reset()
{
    asm("move   #0x2700,%sr\n\t"
        "move.l (0),%a7\n\t"
        "move.l (4),%a0\n\t"
        "jmp    (%a0)");
}

void setVBlankCallback(_voidCallback *CB)
{
    VBlankCB = CB;
}

void setHBlankCallback(_voidCallback *CB)
{
    HBlankCB = CB;
}

