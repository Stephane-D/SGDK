#include "config.h"
#include "types.h"

#include "sys.h"

#include "memory.h"
#include "mapper.h"
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
#include "sound.h"
#include "xgm.h"
#include "dma.h"

#include "tools.h"
#include "kdebug.h"

#if (ENABLE_LOGO != 0)
#define LOGO_SIZE                   64

#include "libres.h"
#endif


#define IN_VINT                     1
#define IN_HINT                     2
#define IN_EXTINT                   4

#define FORCE_VINT_VBLANK_ALIGN     1
#define VINT_ALLOWED_LINE_DELAY     4

#define FRAME_LOAD_MEAN             8


// we don't want to share them
extern u16 randbase;
extern u16 currentDriver;
// size of text segment --> start of initialized data (RO)
extern u32 _stext;
// size of initialized data segment
extern u32 _sdata;

// extern library callback function (we don't want to share them)
extern u16 BMP_doHBlankProcess();
extern void BMP_doVBlankProcess();
extern u16 SPR_doVBlankProcess();
extern void XGM_doVBlankProcess();

// we don't want to share that method
extern void MEM_init();

// main function
extern int main(u16 hard);

// forward
static void internal_reset();
// this one can't be static (used by vdp.c)
void addFrameLoad(u16 frameLoad);

// exception callbacks
VoidCallback *busErrorCB;
VoidCallback *addressErrorCB;
VoidCallback *illegalInstCB;
VoidCallback *zeroDivideCB;
VoidCallback *chkInstCB;
VoidCallback *trapvInstCB;
VoidCallback *privilegeViolationCB;
VoidCallback *traceCB;
VoidCallback *line1x1xCB;
VoidCallback *errorExceptionCB;
VoidCallback *intCB;
VoidCallback *internalVIntCB;
VoidCallback *internalHIntCB;
VoidCallback *internalExtIntCB;

// user V-Int, H-Int and Ext-Int callbacks
static VoidCallback *VIntCBPre;
static VoidCallback *VIntCB;
static VoidCallback *HIntCB;
static VoidCallback *ExtIntCB;


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

// need to be accessed from external
u16 intLevelSave;
static s16 disableIntStack;
static u16 flag;
static u32 missedFrames;

// store last frames CPU load (in [0..255] range), need to shared as it can be updated by vdp.c unit
static u16 frameLoads[FRAME_LOAD_MEAN];
static u16 frameLoadIndex;
static u16 cpuFrameLoad;
static u32 frameCnt;
static u32 lastSubTick;


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
    SYS_setInterruptMaskLevel(7);
    VDP_init();
    VDP_drawText("BUS ERROR !", 10, 3);

    showBusAddressErrorDump(5);

    while(1);
}

// address error default callback
void _addresserror_callback()
{
    SYS_setInterruptMaskLevel(7);
    VDP_init();
    VDP_drawText("ADDRESS ERROR !", 10, 3);

    showBusAddressErrorDump(5);

    while(1);
}

// illegal instruction exception default callback
void _illegalinst_callback()
{
    SYS_setInterruptMaskLevel(7);
    VDP_init();
    VDP_drawText("ILLEGAL INSTRUCTION !", 7, 3);

    showException4WDump(5);

    while(1);
}

// division by zero exception default callback
void _zerodivide_callback()
{
    SYS_setInterruptMaskLevel(7);
    VDP_init();
    VDP_drawText("DIVIDE BY ZERO !", 10, 3);

    showExceptionDump(5);

    while(1);
}

// CHK instruction default callback
void _chkinst_callback()
{
    SYS_setInterruptMaskLevel(7);
    VDP_init();
    VDP_drawText("CHK INSTRUCTION EXCEPTION !", 5, 10);

    showException4WDump(12);

    while(1);
}

// TRAPV instruction default callback
void _trapvinst_callback()
{
    SYS_setInterruptMaskLevel(7);
    VDP_init();
    VDP_drawText("TRAPV INSTRUCTION EXCEPTION !", 5, 3);

    showException4WDump(5);

    while(1);
}

// privilege violation exception default callback
void _privilegeviolation_callback()
{
    SYS_setInterruptMaskLevel(7);
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
    SYS_setInterruptMaskLevel(7);
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
    const u16 vcnt = GET_VCOUNTER;

    intTrace |= IN_VINT;
    vtimer++;

    // detect if we are too late
    bool late = FALSE;

    // we cannot detect late frame if HV latching is enabled..
    if (!VDP_getHVLatching())
    {
        // V28 mode
        if (VDP_getScreenHeight() == 224)
        {
            // V Counter outside expected range ? (rollback in PAL mode can mess up the test here..)
            if ((vcnt < 224) || (vcnt > (224 + VINT_ALLOWED_LINE_DELAY))) late = TRUE;
        }
        // V30 mode
        else
        {
            // V Counter outside expected range ? (rollback in PAL mode can mess up the test here..)
            if ((vcnt < 240) || (vcnt > (240 + VINT_ALLOWED_LINE_DELAY))) late = TRUE;
        }
    }

    // interrupt happened too late ?
    if (late)
    {
        // we increase the number of missed frame
        missedFrames++;
        // assume 100% CPU usage (0-255 value) when V-Int happened too late
        addFrameLoad(255);

        // V-Interrupt VBlank alignment forced ? --> we force wait of next VBlank (and so V-Int)
        if (flag & FORCE_VINT_VBLANK_ALIGN)
        {
#if (LIB_DEBUG != 0)
            KLog_U1("Warning: forced V-Int delay for VBlank alignment (frame miss) on frame #", vtimer);
#endif

            VDP_waitVSync();

            // we need to return from interrupt as we don't have anyway to clear the new pending interrupt
            // so we will take it again immediately but in time this time :)
            intTrace &= ~IN_VINT;

            return;
        }

#if (LIB_DEBUG != 0)
        KLog_U2("Warning: V-Int happened too late (possible frame miss) for frame #", vtimer, " - VCounter = ", vcnt);
#endif
    }

    // call user callback (pre V-Int)
    if (VIntCBPre) VIntCBPre();

    u16 vintp = VIntProcess;
    // may worth it
    if (vintp)
    {
        // xgm processing (have to be done first, before DMA)
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

        // bitmap processing
        if (vintp & PROCESS_BITMAP_TASK)
            BMP_doVBlankProcess();
        // palette fading processing
        if (vintp & PROCESS_PALETTE_FADING)
        {
            if (!VDP_doFadingStep()) vintp &= ~PROCESS_PALETTE_FADING;
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
    u32 banklimit;
    u16* src;
    u16* dst;
    u16 len;

    // clear all RAM (DO NOT USE FUNCTION HERE as we clear all RAM so the stack as well)
    dst = (u16*) RAM;
    len = 0x8000;
    while(len--) *dst++ = 0;

    // then do variables initialization (those which have specific value)

    // point to start of RO initialized data
    src = (u16*) &_stext;
    // point to start initialized variable (always start at beginning of ram)
    dst = (u16*) RAM;
    // get number of byte to copy
    len = (u16)(u32)(&_sdata);
    // convert to word
    len = (len + 1) / 2;

    // get bank limit in word (bank size is 512KB)
    banklimit = (0x80000 - (((u32)src) & 0x7FFFF)) >> 1;
    // bank limit exceeded ?
    if (len > banklimit)
    {
        // we first do the second bank part
        memcpyU16(dst + banklimit, FAR(src + banklimit), len - banklimit);
        // adjust len
        len = banklimit;
    }
    // initialize "initialized variables"
    memcpyU16(dst, FAR(src), len);

    // initialize random number generator
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
        Bitmap *logo = unpackBitmap(&sgdk_logo, NULL);

        // correctly unpacked
        if (logo)
        {
            const Palette *logo_pal = logo->palette;

            // display logo (use BMP mode for that)
            BMP_init(TRUE, BG_A, PAL0, FALSE);

    #if (ZOOMING_LOGO != 0)
            // init fade in to 30 step
            if (VDP_initFading(0, logo_pal->length - 1, palette_black, logo_pal->data, 30))
            {
                // prepare zoom
                u16 size = LOGO_SIZE;

                // while zoom not completed
                while(size > 0)
                {
                    // sort of log decrease
                    if (size > 20) size = size - (size / 6);
                    else if (size > 5) size -= 5;
                    else size = 0;

                    // get new size
                    const u32 w = LOGO_SIZE - size;

                    // adjust palette for fade
                    VDP_doFadingStep();

                    // zoom logo
                    BMP_loadAndScaleBitmap(logo, 128 - (w >> 1), 80 - (w >> 1), w, w, FALSE);
                    // flip to screen
                    BMP_flip(FALSE);
                }

                // while fade not completed
                while(VDP_doFadingStep());
            }

            // wait 1 second
            waitTick(TICKPERSECOND * 1);
    #else
            // set palette 0 to black
            PAL_setPalette(PAL0, palette_black);

            // don't load the palette immediatly
            BMP_loadBitmap(logo, 128 - (LOGO_SIZE / 2), 80 - (LOGO_SIZE / 2), FALSE);
            // flip
            BMP_flip(0);

            // fade in logo
            PAL_fade((PAL0 << 4), (PAL0 << 4) + (logo_pal->length - 1), palette_black, logo_pal->data, 30, FALSE);

            // wait 1.5 second
            waitTick(TICKPERSECOND * 1.5);
    #endif
            // fade out logo
            PAL_fadeOutPalette(PAL0, 20, FALSE);

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

    // for safety
    while(TRUE);
}

void _reset_entry()
{
    internal_reset();

    main(0);

    // for safety
    while(TRUE);
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

    // default
    flag = FORCE_VINT_VBLANK_ALIGN;
    missedFrames = 0;

    // reset frame load monitor
    memsetU16(frameLoads, 0, FRAME_LOAD_MEAN);
    frameLoadIndex = 0;
    cpuFrameLoad = 0;
    frameCnt = 0;
    lastSubTick = 0;

    // safe to check for DMA completion before dealing with VDP (this also clear internal VDP latch)
    // WARNING: it's important to not access the VDP too soon or you can lock the system (it's why we do it just here) !
    while(GET_VDPSTATUS(VDP_DMABUSY_FLAG));

    // init part (always do MEM_init() first)
    MEM_init();
    VDP_init();
    DMA_init();
    DMA_setMaxTransferSizeToDefault();
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
        // KDebug_Alert("SYS_disableInts() fails: call during interrupt");
#endif

        return;
    }

    // disable interrupts
    if (disableIntStack++ == 0)
        intLevelSave = SYS_getAndSetInterruptMaskLevel(7);
#if (LIB_DEBUG != 0)
    else
        KLog_U1("SYS_disableInts() info: inner call = ", disableIntStack);
#endif
}

void SYS_enableInts()
{
    // in interrupt --> return
    if (intTrace != 0)
    {
#if (LIB_DEBUG != 0)
        // KDebug_Alert("SYS_enableInts() fails: call during interrupt");
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
            KLog_U1("SYS_enableInts() fails: already enabled = ", disableIntStack);
        else
            KLog_U1("SYS_enableInts() info: inner call = ", disableIntStack);
    }
#endif
}

void SYS_setVIntPreCallback(VoidCallback *CB)
{
    VIntCBPre = CB;
}

void SYS_setVIntCallback(VoidCallback *CB)
{
    VIntCB = CB;
}

void SYS_setHIntCallback(VoidCallback *CB)
{
    HIntCB = CB;
}

void SYS_setExtIntCallback(VoidCallback *CB)
{
    ExtIntCB = CB;
}

void SYS_setVIntAligned(bool value)
{
    if (value) flag |= FORCE_VINT_VBLANK_ALIGN;
    else flag &= ~FORCE_VINT_VBLANK_ALIGN;
}

u16 SYS_isVIntAligned()
{
    return (flag & FORCE_VINT_VBLANK_ALIGN)?TRUE:FALSE;
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


u32 SYS_getFPS()
{
    static s32 result;
    const u32 current = getSubTick();
    u32 delta = current - lastSubTick;

    if ((delta > 19200) && ((frameCnt > (76800 * 5)) || (delta > 76800)))
    {
        result = frameCnt / delta;
        if (result > 999) result = 999;
        lastSubTick = current;
        frameCnt = 76800;
    }
    else frameCnt += 76800;

    return result;
}

fix32 SYS_getFPSAsFloat()
{
    static fix32 result;
    const s32 current = getSubTick();
    u32 delta = current - lastSubTick;

    if ((delta > 19200) && ((frameCnt > (76800 * 5)) || (delta > 76800)))
    {
        if (frameCnt > (250 * 76800)) result = FIX32((u32) 999);
        else
        {
            result = (frameCnt << FIX16_FRAC_BITS) / delta;
            if (result > (999 << FIX16_FRAC_BITS)) result = FIX32((u32)999);
            else result <<= (FIX32_FRAC_BITS - FIX16_FRAC_BITS);
        }

        lastSubTick = current;
        frameCnt = 76800;
    }
    else frameCnt += 76800;

    return result;
}


// used to compute average frame load on 8 frames
void addFrameLoad(u16 frameLoad)
{
    static u16 lastMissedFrame = 0;
    static u16 lastVTimer = 0;

    u16 v = frameLoad;
    // force full load if we have frame miss
    if ((lastMissedFrame != missedFrames) || ((vtimer - lastVTimer) > 1))
        v = 255;

    cpuFrameLoad -= frameLoads[frameLoadIndex];
    frameLoads[frameLoadIndex] = v;
    cpuFrameLoad += v;
    frameLoadIndex = (frameLoadIndex + 1) & (FRAME_LOAD_MEAN - 1);
    lastMissedFrame = missedFrames;
    lastVTimer = vtimer;
}

u16 SYS_getCPULoad()
{
    return (cpuFrameLoad * ((u16) 100)) / ((u16) (FRAME_LOAD_MEAN * 255));
}

u32 SYS_getMissedFrames()
{
    return missedFrames;
}

void SYS_resetMissedFrames()
{
    missedFrames = 0;
}


void SYS_die(char *err)
{
    SYS_setInterruptMaskLevel(7);
    VDP_init();
    VDP_drawText("A fatal error occured !", 2, 2);
    VDP_drawText("cannot continue...", 4, 3);
    if (err) VDP_drawText(err, 0, 5);

    while(1);
}
