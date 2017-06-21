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
#include "timer.h"
#include "string.h"
#include "vdp.h"
#include "vdp_bg.h"
#include "vdp_pal.h"
#include "tile_cache.h"
#include "sound.h"
#include "xgm.h"
#include "dma.h"

#include "tools.h"
#include "kdebug.h"

#if (ENABLE_LOGO != 0)
#include "libres.h"
#endif


#define IN_VINT         1
#define IN_HINT         2
#define IN_EXTINT       4


// we don't want to share them
extern u16 randbase;
extern TileSet** uploads;
extern s16 currentDriver;

// extern library callback function (we don't want to share them)
extern u16 BMP_doHBlankProcess();
extern void BMP_doVBlankProcess();
extern void TC_doVBlankProcess();
extern u16 SPR_doVBlankProcess();
extern void XGM_doVBlankProcess();

// main function
extern int main(u16 hard);

static void internal_reset();

// exception callbacks
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

// user V-Int, H-Int and Ext-Int callbacks
static _voidCallback *VIntCBPre;
static _voidCallback *VIntCB;
static _voidCallback *HIntCB;
static _voidCallback *ExtIntCB;


// exception state consumes 78 bytes of memory
__attribute__((externally_visible)) u32 registerState[8+8];
__attribute__((externally_visible)) u32 pcState;
__attribute__((externally_visible)) u32 addrState;
__attribute__((externally_visible)) u16 ext1State;
__attribute__((externally_visible)) u16 ext2State;
__attribute__((externally_visible)) u16 srState;

__attribute__((externally_visible)) vu32 VIntProcess;
__attribute__((externally_visible)) vu32 HIntProcess;
__attribute__((externally_visible)) vu32 ExtIntProcess;
__attribute__((externally_visible)) vu16 intTrace;

static u16 intLevelSave;
static s16 disableIntStack;


static void addValueU8(char *dst, char *str, u8 value)
{
    char v[16];

    strcat(dst, str);
    intToHex(value, v, 2);
    strcat(dst, v);
}

static void addValueU16(char *dst, char *str, u16 value)
{
    char v[16];

    strcat(dst, str);
    intToHex(value, v, 4);
    strcat(dst, v);
}

static void addValueU32(char *dst, char *str, u32 value)
{
    char v[16];

    strcat(dst, str);
    intToHex(value, v, 8);
    strcat(dst, v);
}

static u16 showValueU32U16(char *str1, u32 value1, char *str2, u16 value2, u16 pos)
{
    char s[64];

    strclr(s);
    addValueU32(s, str1, value1);
    addValueU16(s, str2, value2);

    VDP_drawText(s, 0, pos);

    return pos + 1;
}

static u16 showValueU16U32U16(char *str1, u16 value1, char *str2, u32 value2, char *str3, u16 value3, u16 pos)
{
    char s[64];

    strclr(s);
    addValueU16(s, str1, value1);
    addValueU32(s, str2, value2);
    addValueU16(s, str3, value3);

    VDP_drawText(s, 0, pos);

    return pos + 1;
}

static u16 showValueU32U16U16(char *str1, u32 value1, char *str2, u16 value2, char *str3, u16 value3, u16 pos)
{
    char s[64];

    strclr(s);
    addValueU32(s, str1, value1);
    addValueU16(s, str2, value2);
    addValueU16(s, str3, value3);

    VDP_drawText(s, 0, pos);

    return pos + 1;
}

static u16 showValueU32U32(char *str1, u32 value1, char *str2, u32 value2, u16 pos)
{
    char s[64];

    strclr(s);
    addValueU32(s, str1, value1);
    addValueU32(s, str2, value2);

    VDP_drawText(s, 0, pos);

    return pos + 1;
}

static u16 showValueU32U32U32(char *str1, u32 value1, char *str2, u32 value2, char *str3, u32 value3, u16 pos)
{
    char s[64];

    strclr(s);
    addValueU32(s, str1, value1);
    addValueU32(s, str2, value2);
    addValueU32(s, str3, value3);

    VDP_drawText(s, 0, pos);

    return pos + 1;
}

//static u16 showValueU32U32U32U32(char *str1, u32 value1, char *str2, u32 value2, char *str3, u32 value3, char *str4, u32 value4, u16 pos)
//{
//    char s[64];
//
//    strclr(s);
//    addValueU32(s, str1, value1);
//    addValueU32(s, str2, value2);
//    addValueU32(s, str3, value3);
//    addValueU32(s, str4, value4);
//
//    VDP_drawText(s, 0, pos);
//
//    return pos + 1;
//}

static u16 showRegisterState(u16 pos)
{
    u16 y = pos;

    y = showValueU32U32U32("D0=", registerState[0], " D1=", registerState[1], " D2=", registerState[2], y);
    y = showValueU32U32U32("D3=", registerState[3], " D4=", registerState[4], " D5=", registerState[5], y);
    y = showValueU32U32("D6=", registerState[6], " D7=", registerState[7], y);
    y = showValueU32U32U32("A0=", registerState[8], " A1=", registerState[9], " A2=", registerState[10], y);
    y = showValueU32U32U32("A3=", registerState[11], " A4=", registerState[12], " A5=", registerState[13], y);
    y = showValueU32U32("A6=", registerState[14], " A7=", registerState[15], y);

    return y;
}

static u16 showStackState(u16 pos)
{
    char s[64];
    u16 y = pos;
    u32 *sp = (u32*) registerState[15];

    u16 i = 0;
    while(i < 24)
    {
        strclr(s);
        addValueU8(s, "SP+", i * 4);
        strcat(s, " ");
        y = showValueU32U32(s, *(sp + (i + 0)), " ", *(sp + (i + 1)), y);
        i += 2;
    }

    return y;
}

static u16 showExceptionDump(u16 pos)
{
    u16 y = pos;

    y = showValueU32U16("PC=", pcState, " SR=", srState, y) + 1;
    y = showRegisterState(y) + 1;
    y = showStackState(y);

    return y;
}

static u16 showException4WDump(u16 pos)
{
    u16 y = pos;

    y = showValueU32U16U16("PC=", pcState, " SR=", srState, " VO=", ext1State, y) + 1;
    y = showRegisterState(y) + 1;
    y = showStackState(y);

    return y;
}

static u16 showBusAddressErrorDump(u16 pos)
{
    u16 y = pos;

    y = showValueU16U32U16("FUNC=", ext1State, " ADDR=", addrState, " INST=", ext2State, y);
    y = showValueU32U16("PC=", pcState, " SR=", srState, y) + 1;
    y = showRegisterState(y) + 1;
    y = showStackState(y);

    return y;
}


// bus error default callback
void _buserror_callback()
{
    VDP_init();
    VDP_drawText("BUS ERROR !", 10, 3);

    showBusAddressErrorDump(5);

    while(1);
}

// address error default callback
void _addresserror_callback()
{
    VDP_init();
    VDP_drawText("ADDRESS ERROR !", 10, 3);

    showBusAddressErrorDump(5);

    while(1);
}

// illegal instruction exception default callback
void _illegalinst_callback()
{
    VDP_init();
    VDP_drawText("ILLEGAL INSTRUCTION !", 7, 3);

    showException4WDump(5);

    while(1);
}

// division by zero exception default callback
void _zerodivide_callback()
{
    VDP_init();
    VDP_drawText("DIVIDE BY ZERO !", 10, 3);

    showExceptionDump(5);

    while(1);
}

// CHK instruction default callback
void _chkinst_callback()
{
    VDP_init();
    VDP_drawText("CHK INSTRUCTION EXCEPTION !", 5, 10);

    showException4WDump(12);

    while(1);
}

// TRAPV instruction default callback
void _trapvinst_callback()
{
    VDP_init();
    VDP_drawText("TRAPV INSTRUCTION EXCEPTION !", 5, 3);

    showException4WDump(5);

    while(1);
}

// privilege violation exception default callback
void _privilegeviolation_callback()
{
    VDP_init();
    VDP_drawText("PRIVILEGE VIOLATION !", 5, 3);

    showExceptionDump(5);

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
    VDP_drawText("EXCEPTION ERROR !", 5, 3);

    showExceptionDump(5);

    while(1);
}

// level interrupt default callback
void _int_callback()
{

}


// V-Int Callback
void _vint_callback()
{
    u16 vintp;

    intTrace |= IN_VINT;

    vtimer++;

    // call user callback (pre V-Int)
    if (VIntCBPre) VIntCBPre();

    vintp = VIntProcess;
    // may worth it
    if (vintp)
    {
        // xgm processing (have to be done first !)
        if (vintp & PROCESS_XGM_TASK)
            XGM_doVBlankProcess();

        // dma processing
        if (vintp & PROCESS_DMA_TASK)
        {
            // DMA protection for XGM driver
            if (currentDriver == Z80_DRIVER_XGM)
            {
                XGM_set68KBUSProtection(TRUE);

                // delay enabled ? --> wait a bit to improve PCM playback (test on SOR2)
                if (SND_getForceDelayDMA_XGM()) waitSubTick(10);

                DMA_flushQueue();

                XGM_set68KBUSProtection(FALSE);
            }
            else
                DMA_flushQueue();

            // always clear process
            vintp &= ~PROCESS_DMA_TASK;
        }

        // tile cache processing
        if (vintp & PROCESS_TILECACHE_TASK)
            TC_doVBlankProcess();
        // bitmap processing
        if (vintp & PROCESS_BITMAP_TASK)
            BMP_doVBlankProcess();
        // palette fading processing
        if (vintp & PROCESS_PALETTE_FADING)
        {
            if (!VDP_doStepFading(FALSE)) vintp &= ~PROCESS_PALETTE_FADING;
        }

        VIntProcess = vintp;
    }

    // then call user callback
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
        if (!BMP_doHBlankProcess()) HIntProcess &= ~PROCESS_BITMAP_TASK;
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
    setRandomSeed(0xC427);
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
        Bitmap *logo = unpackBitmap(&logo_lib, NULL);

        // correctly unpacked
        if (logo)
        {
            const Palette *logo_pal = logo->palette;

            // display logo (use BMP mode for that)
            BMP_init(TRUE, PAL0, FALSE);

    #if (ZOOMING_LOGO != 0)
            // init fade in to 30 step
            u16 step_fade = 30;

            if (VDP_initFading(logo_pal->index, logo_pal->index + (logo_pal->length - 1), palette_black, logo_pal->data, step_fade))
            {
                // prepare zoom
                u16 size = 256;

                // while zoom not completed
                while(size > 0)
                {
                    // sort of log decrease
                    if (size > 20) size = size - (size / 6);
                    else if (size > 5) size -= 5;
                    else size = 0;

                    // get new size
                    const u32 w = 256 - size;

                    // adjust palette for fade
                    if (step_fade-- > 0) VDP_doStepFading(FALSE);

                    // zoom logo
                    BMP_loadAndScaleBitmap(logo, 64 + ((256 - w) >> 2), (256 - w) >> 1, w >> 1, w >> 1, FALSE);
                    // flip to screen
                    BMP_flip(0);
                }

                // while fade not completed
                while(step_fade--) VDP_doStepFading(TRUE);
            }

            // wait 1 second
            waitTick(TICKPERSECOND * 1);
    #else
            // set palette 0 to black
            VDP_setPalette(PAL0, palette_black);

            // don't load the palette immediatly
            BMP_loadBitmap(logo, 64, 0, FALSE);
            // flip
            BMP_flip(0);

            // fade in logo
            VDP_fade((PAL0 << 4) + logo_pal->index, (PAL0 << 4) + (logo_pal->index + (logo_pal->length - 1)), palette_black, logo_pal->data, 30, FALSE);

            // wait 1.5 second
            waitTick(TICKPERSECOND * 1.5);
    #endif

            // fade out logo
            VDP_fadePalOut(PAL0, 20, 0);
            // wait 0.5 second
            waitTick(TICKPERSECOND * 0.5);

            // shut down bmp mode
            BMP_end();
            // release bitmap memory
            MEM_free(logo);
            // reinit vdp before program execution
            VDP_init();
        }
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
    VIntCBPre = NULL;
    VIntCB = NULL;
    HIntCB = NULL;
    VIntProcess = 0;
    HIntProcess = 0;
    ExtIntProcess = 0;
    intTrace = 0;
    intLevelSave = 0;
    disableIntStack = 0;

    // reset variables which own engine initialization state
    uploads = NULL;

    // init part
    MEM_init();
    VDP_init();
    DMA_init(0, 0);
    PSG_init();
    JOY_init();
    // reseting z80 also reset the ym2612
    Z80_init();

    // enable interrupts
    SYS_setInterruptMaskLevel(3);
}

void SYS_disableInts()
{
    // in interrupt --> return
    if (intTrace != 0)
    {
#if (LIB_DEBUG != 0)
        KDebug_Alert("SYS_disableInts() fails: call during interrupt");
#endif

        return;
    }

    // disable interrupts
    if (disableIntStack++ == 0)
        intLevelSave = SYS_getAndSetInterruptMaskLevel(7);
#if (LIB_DEBUG != 0)
    else
        KDebug_Alert("SYS_disableInts() info: inner call");
#endif
}

void SYS_enableInts()
{
    // in interrupt --> return
    if (intTrace != 0)
    {
#if (LIB_DEBUG != 0)
        KDebug_Alert("SYS_enableInts() fails: call during interrupt");
#endif

        return;
    }

    // reenable interrupts
    if (--disableIntStack == 0)
        SYS_setInterruptMaskLevel(intLevelSave);
#if (LIB_DEBUG != 0)
    else
    {
        if (disableIntStack < 0)
            KDebug_Alert("SYS_enableInts() fails: already enabled");
        else
            KDebug_Alert("SYS_enableInts() info: inner call");
    }
#endif
}

void SYS_setVIntPreCallback(_voidCallback *CB)
{
    VIntCBPre = CB;
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

u16 SYS_isInInterrupt()
{
    return intTrace;
}

u16 SYS_isNTSC()
{
    return !IS_PALSYSTEM;
}

u16 SYS_isPAL()
{
    return IS_PALSYSTEM;
}

void SYS_die(char *err)
{
    VDP_init();
    VDP_drawText("A fatal error occured !", 2, 2);
    VDP_drawText("cannot continue...", 4, 3);
    if (err) VDP_drawText(err, 0, 5);

    while(1);
}
