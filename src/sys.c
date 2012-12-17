#include "config.h"
#include "types.h"

#include "sys.h"

#include "memory.h"
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


#define IN_VINT         1
#define IN_HINT         2
#define IN_EXTINT       4


// we don't want to share them
extern u16 randbase;

// extern library callback function (we don't want to share them)
extern u16 VDP_doStepFading();
extern u16 BMP_doBlankProcess();


// main function
extern int main(u16 hard);

static void internal_reset();

// interrrupt callback
_voidCallback *busErrorCB;
_voidCallback *addressErrorCB;
_voidCallback *illegalInstCB;
_voidCallback *zeroDivideCB;
_voidCallback *chkInstCB;
_voidCallback *trapvInstCB;
_voidCallback *privilegeViolationCB;
_voidCallback *traceCB;
_voidCallback *line1x1xCB;
_voidCallback *errorExceptionCB;
_voidCallback *intCB;
_voidCallback *internalVIntCB;
_voidCallback *internalHIntCB;
_voidCallback *internalExtIntCB;

// user V-Int, H-Int and Ext-Int callback
static _voidCallback *VIntCB;
static _voidCallback *HIntCB;
static _voidCallback *ExtIntCB;

u16 intTrace;
u32 VIntProcess;
u32 HIntProcess;
u32 ExtIntProcess;


// bus error default callback
void _buserror_callback()
{
    VDP_init();
    VDP_drawText("BUS ERROR !", 10, 10);

    while(1);
}

// address error default callback
void _addresserror_callback()
{
    VDP_init();
    VDP_drawText("ADDRESS ERROR !", 10, 10);

    while(1);
}

// illegal instruction exception default callback
void _illegalinst_callback()
{
    VDP_init();
    VDP_drawText("ILLEGAL INSTRUCTION !", 5, 10);

    while(1);
}

// division by zero exception default callback
void _zerodivide_callback()
{
    VDP_init();
    VDP_drawText("DIVIDE BY ZERO !", 10, 10);

    while(1);
}

// CHK instruction default callback
void _chkinst_callback()
{

}

// TRAPV instruction default callback
void _trapvinst_callback()
{

}

// privilege violation exception default callback
void _privilegeviolation_callback()
{
    VDP_init();
    VDP_drawText("PRIVILEGE VIOLATION !", 5, 10);

    while(1);
}

// trace default callback
void _trace_callback()
{

}

// line 1x1x exception default callback
void _line1x1x_callback()
{

}

// error exception default callback
void _errorexception_callback()
{
    VDP_init();
    VDP_drawText("EXCEPTION ERROR !", 5, 10);

    while(1);
}

// level interrupt default callback
void _int_callback()
{

}


// V-Int Callback
void _vint_callback()
{
    intTrace |= IN_VINT;

    vtimer++;

    // palette fading processing
    if (VIntProcess & PROCESS_PALETTE_FADING)
    {
        if (!VDP_doStepFading()) VIntProcess &= ~PROCESS_PALETTE_FADING;
    }

    // bitmap process
//        if (VIntProcess & PROCESS_BITMAP_TASK)
//        {
//            if (!BMP_doBlankProcess()) VIntProcess &= ~PROCESS_BITMAP_TASK;
//        }

    // ...

    // then call user's callback
    if (VIntCB) VIntCB();

    // joy state refresh (better to do it after user's callback as it can eat some time)
    JOY_update();

    intTrace &= ~IN_VINT;
}

// H-Int Callback
void _hint_callback()
{
    intTrace |= IN_HINT;

    // bitmap processing
    if (HIntProcess & PROCESS_BITMAP_TASK)
    {
        if (!BMP_doBlankProcess()) HIntProcess &= ~PROCESS_BITMAP_TASK;
    }

    // ...

    // then call user's callback
    if (HIntCB) HIntCB();

    intTrace &= ~IN_HINT;
}

// Ext-Int Callback
void _extint_callback()
{
    intTrace |= IN_EXTINT;

    // processing
//    if (ExtIntProcess & ...)
//    {
//      ...
//    }

    // then call user's callback
    if (ExtIntCB) ExtIntCB();

    intTrace &= ~IN_EXTINT;
}


void _start_entry()
{
    // initiate random number generator
    randbase = 0xD94B;
    vtimer = 0;

    // default interrupt callback
    busErrorCB = _buserror_callback;
    addressErrorCB = _addresserror_callback;
    illegalInstCB = _illegalinst_callback;
    zeroDivideCB = _zerodivide_callback;
    chkInstCB = _chkinst_callback;
    trapvInstCB = _trapvinst_callback;
    privilegeViolationCB = _privilegeviolation_callback;
    traceCB = _trace_callback;
    line1x1xCB = _line1x1x_callback;
    errorExceptionCB = _errorexception_callback;
    intCB = _int_callback;
    internalVIntCB = _vint_callback;
    internalHIntCB = _hint_callback;
    internalExtIntCB = _extint_callback;

    internal_reset();

#if (ENABLE_LOGO != 0)
    {
        const Bitmap* logo = (Bitmap*) logo_lib;

        // display logo (BMP mode use 128x160 resolution with doubled X pixel)
        BMP_init(BMP_ENABLE_WAITVSYNC | BMP_ENABLE_EXTENDEDBLANK);

#if (ZOOMING_LOGO != 0)
        // init fade in to 30 step
        u16 step_fade = 30;

        if (VDP_initFading(0, 15, palette_black, logo->palette, step_fade))
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

                // get new size
                const u32 w = 128 - size;

                // adjust palette for fade
                if (step_fade-- > 0) VDP_doStepFading();

                // zoom logo
                BMP_loadAndScaleGenBitmap(logo, 32 + ((128 - w) >> 2), (128 - w) >> 1, w >> 1, w, 0xFF);
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

        // don't load the palette immediatly
        BMP_loadGenBitmap(logo_lib, 32, 0, 0xFF);
        // flip
        BMP_flip();

        // fade in logo
        VDP_fadePalIn(0, logo->palette, 30, 0);

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
    VIntCB = NULL;
    HIntCB = NULL;
    VIntProcess = 0;
    HIntProcess = 0;
    ExtIntProcess = 0;
    intTrace = 0;

    // init part
    MEM_init();
    VDP_init();
    PSG_init();
    JOY_init();
    // reseting z80 also reset the ym2612
    Z80_init();

    // enable interrupts
    SYS_setInterruptMaskLevel(3);
}


void SYS_setVIntCallback(_voidCallback *CB)
{
    VIntCB = CB;
}

void SYS_setHIntCallback(_voidCallback *CB)
{
    HIntCB = CB;
}

void SYS_setExtIntCallback(_voidCallback *CB)
{
    ExtIntCB = CB;
}

u16 SYS_isInVIntCallback()
{
    return intTrace & IN_VINT;
}

u16 SYS_isInHIntCallback()
{
    return intTrace & IN_HINT;
}

u16 SYS_isInExtIntCallback()
{
    return intTrace & IN_EXTINT;
}
