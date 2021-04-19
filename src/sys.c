#include "config.h"
#include "types.h"

#include "sys.h"

#include "memory.h"
#include "mapper.h"
#include "vdp.h"
#include "vdp_pal.h"
#include "vdp_spr.h"
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
//#define IN_HINT                     2
//#define IN_EXTINT                   4

#define SHOW_FRAME_LOAD             (1 << 0)
#define SHOW_FRAME_LOAD_MEAN        (2 << 0)

#define VINT_ALLOWED_LINE_DELAY     4

#define LOAD_MEAN_FRAME_NUM         8


// we don't want to share them
extern u16 randbase;
extern u16 currentDriver;
// size of text segment --> start of initialized data (RO)
extern u32 _stext;
// size of initialized data segment
extern u32 _sdata;
// last V-Counter on VDP_waitVSync() / VDP_waitVInt() call
extern u16 lastVCnt;

// extern library callback function (we don't want to share them)
extern void BMP_doVBlankProcess();
extern void XGM_doVBlankProcess();
extern bool MAP_doVBlankProcess();
extern bool VDP_doVBlankScrollProcess();

// we don't want to share that method
extern void MEM_init();

// main function
extern int main(bool hardReset);

// forward
static void internal_reset();
// this one can't be static (used by vdp.c)
bool addFrameLoad(u16 frameLoad, u32 vtime);

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

// user V-Int, H-Int and Ext-Int callbacks
VoidCallback *vintCB;
VoidCallback *hintCB;
VoidCallback *eintCB;


// exception state consumes 78 bytes of memory
__attribute__((externally_visible)) u32 registerState[8+8];
__attribute__((externally_visible)) u32 pcState;
__attribute__((externally_visible)) u32 addrState;
__attribute__((externally_visible)) u16 ext1State;
__attribute__((externally_visible)) u16 ext2State;
__attribute__((externally_visible)) u16 srState;

__attribute__((externally_visible)) vu16 VBlankProcess;
__attribute__((externally_visible)) vu16 intTrace;

// need to be accessed from external
u16 intLevelSave;
static s16 disableIntStack;
static u16 flags;

// store last frames CPU load (in [0..255] range), need to shared as it can be updated by vdp.c unit
static u16 frameLoads[LOAD_MEAN_FRAME_NUM];
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
    //
}


// Dummy V-Int Callback
void _vint_dummy_callback()
{
    //
}

// Dummy H-Int Callback
void _hint_dummy_callback()
{
    //
}

// Dummy Ext-Int Callback
void _extint_dummy_callback()
{
    //
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

    // reset banks as we messed them during init
    len = 8;
    while(--len) SYS_setBank(len, len);

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
    main(TRUE);

    // for safety
    while(TRUE) SYS_doVBlankProcess();
}

void _reset_entry()
{
    internal_reset();

    main(FALSE);

    // for safety
    while(TRUE) SYS_doVBlankProcess();
}

static void internal_reset()
{
    vintCB = _vint_dummy_callback;
    hintCB = _hint_dummy_callback;
    eintCB = _extint_dummy_callback;
    VBlankProcess = 0;
    intTrace = 0;
    intLevelSave = 0;
    disableIntStack = 0;

    // default
    flags = 0;

    // reset frame load monitor
    memsetU16(frameLoads, 0, LOAD_MEAN_FRAME_NUM);
    frameLoadIndex = 0;
    cpuFrameLoad = 0;
    frameCnt = 0;
    lastSubTick = 0;

    // safe to check for DMA completion before dealing with VDP (this also clear internal VDP latch)
    // WARNING: it's important to not access the VDP too soon or you can lock the system (it's why we do it just here) !
    while(GET_VDPSTATUS(VDP_DMABUSY_FLAG));

    // init part (always do MEM_init() first)
    MEM_init();
    DMA_init();
    DMA_setMaxTransferSizeToDefault();
    VDP_init();
    PSG_init();
    JOY_init();
    // reseting z80 also reset the ym2612
    Z80_init();

    // enable interrupts
    SYS_setInterruptMaskLevel(3);
}

bool SYS_doVBlankProcess()
{
    return SYS_doVBlankProcessEx(ON_VBLANK_START);
}

bool SYS_doVBlankProcessEx(VBlankProcessTime processTime)
{
    if (processTime != IMMEDIATELY)
    {
        // wait for VBlank
        if (VDP_waitVBlank(processTime == ON_VBLANK_START))
        {
            // frame late/miss detection and VBlank process forced on VBlank start ?
            if (processTime == ON_VBLANK_START)
            {
                // SYS_doVBlankProcess() was called from V-Int callback ?
                if (SYS_isInVInt())
                    // we need to return from interrupt as we don't have anyway to clear the new pending interrupt
                    // so we will take it again immediately but should be in time for this one :)
                    return FALSE;
            }
        }
    }

    u16 vbp = VBlankProcess;
#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
    u16 vcnt = 0;
#endif

    // dma processing
    if (vbp & PROCESS_DMA_TASK)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
        u16 dmaSize = DMA_getQueueTransferSize();
#endif

        // DMA protection for XGM driver
        if (currentDriver == Z80_DRIVER_XGM)
        {
            XGM_set68KBUSProtection(TRUE);

            // delay enabled ? --> wait a bit to improve PCM playback (test on SOR2)
            if (XGM_getForceDelayDMA()) waitSubTick(10);
            DMA_flushQueue();

            XGM_set68KBUSProtection(FALSE);
        }
        else
            DMA_flushQueue();

#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
        vcnt = GET_VCOUNTER;

        // above scanline 2 ? better to warn about DMA overrun..
        if ((vcnt < 224) && (vcnt > 2))
            KLog_U3("Warning: DMA task (", dmaSize, " bytes) completed outside VBlank area. Scanline after completion = ", vcnt, " on frame #", vtimer);
#endif
    }

    // VDP scroll process (async scroll update)
    if (vbp & PROCESS_VDP_SCROLL_TASK)
    {
        if (!VDP_doVBlankScrollProcess()) vbp &= ~PROCESS_VDP_SCROLL_TASK;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
        // previous v-counter was ok ?
        if ((vcnt >= 224) || (vcnt < 3))
        {
            vcnt = GET_VCOUNTER;

            // above scanline 2 ? better to warn about frame overrun..
            if ((vcnt < 224) && (vcnt > 2))
                KLog_U2("Warning: Scroll task completed outside VBlank area. Scanline after completion = ", vcnt, " on frame #", vtimer);
        }
#endif
    }

    // palette fading process
    if (vbp & PROCESS_PALETTE_FADING)
    {
        if (!PAL_doFadeStep()) vbp &= ~PROCESS_PALETTE_FADING;

#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
        // previous v-counter was ok ?
        if ((vcnt >= 224) || (vcnt < 3))
        {
            vcnt = GET_VCOUNTER;

            // above scanline 2 ? better to warn about frame overrun..
            if ((vcnt < 224) && (vcnt > 2))
                KLog_U2("Warning: Palette fade task completed outside VBlank area. Scanline after completion = ", vcnt, " on frame #", vtimer);
        }
#endif
    }

    // store back
    VBlankProcess = vbp;

    // frame load display enabled ?
    if (flags & SHOW_FRAME_LOAD)
    {
        // use internal sprite 0 to show cursor
        VDPSprite* vdpSprite = &vdpSpriteCache[0];

        // use CPU load display instead (mean)
        if (flags & SHOW_FRAME_LOAD_MEAN)
        {
            // get CPU load (0-255)
            u16 load = cpuFrameLoad / LOAD_MEAN_FRAME_NUM;

            if (load > 224) vdpSprite->y = 220 + 0x80;
            else vdpSprite->y = load + (0x80 - 4);
        }
        // directly use VCounter
        else
        {
            // update position relative to last stored VCounter
            if ((lastVCnt > 224) || (lastVCnt < 4)) vdpSprite->y = 0x80;
            else if (lastVCnt > 220) vdpSprite->y = 220 + 0x80;
            else vdpSprite->y = lastVCnt + (0x80 - 4);
        }

        // write immediately in VRAM the sprite position change
        vu16* pw = (u16 *) GFX_DATA_PORT;
        vu32* pl = (u32 *) GFX_CTRL_PORT;

        *pl = GFX_WRITE_VRAM_ADDR(VDP_SPRITE_TABLE);
        *pw = vdpSprite->y;
    }

    // joy state refresh
    JOY_update();

    return TRUE;
}

void SYS_disableInts()
{
    // in interrupt --> return
    if (intTrace != 0)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
        // KDebug_Alert("SYS_disableInts() warning: call during interrupt (ignored)");
#endif

        return;
    }

    // disable interrupts
    if (disableIntStack++ == 0)
        intLevelSave = SYS_getAndSetInterruptMaskLevel(7);
#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
    else
    {
        if (disableIntStack <= 0)
            KLog_S1_("SYS_disableInts() fails: need ", (-disableIntStack) + 1, " more");
    #if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
        else
            KLog_S1("SYS_disableInts() info: inner call = ", disableIntStack);
    #endif
    }
#endif
}

void SYS_enableInts()
{
    // in interrupt --> return
    if (intTrace != 0)
    {
#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
        // KDebug_Alert("SYS_enableInts() fails: call during interrupt");
#endif

        return;
    }

    // reenable interrupts
    if (--disableIntStack == 0)
        SYS_setInterruptMaskLevel(intLevelSave);
#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
    else
    {
        if (disableIntStack < 0)
            KLog_S1("SYS_enableInts() fails: already enabled = ", disableIntStack);
    #if (LIB_LOG_LEVEL >= LOG_LEVEL_INFO)
        else
            KLog_S1_("SYS_enableInts() info: inner call, need ", disableIntStack, " more");
    #endif
    }
#endif
}

void SYS_setVIntCallback(VoidCallback *CB)
{
    if (CB) vintCB = CB;
    else vintCB = _vint_dummy_callback;
}

void SYS_setHIntCallback(VoidCallback *CB)
{
    if (CB) hintCB = CB;
    else hintCB = _hint_dummy_callback;
}

void SYS_setExtIntCallback(VoidCallback *CB)
{
    if (CB) eintCB = CB;
    else eintCB = _extint_dummy_callback;
}

void SYS_setVIntAligned(bool value)
{
    // deprecated
}

bool SYS_isVIntAligned()
{
    return FALSE;
}

void SYS_showFrameLoad(bool mean)
{
    if (mean) flags |= (SHOW_FRAME_LOAD | SHOW_FRAME_LOAD_MEAN);
    else flags |= SHOW_FRAME_LOAD;

    // use internal sprite 0 to show cursor
    VDPSprite* vdpSprite = &vdpSpriteCache[0];
    vdpSprite->y = 0;
    vdpSprite->size = SPRITE_SIZE(1, 1);
    // point on left cursor tile in font
    vdpSprite->attribut = TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, TILE_FONTINDEX + 94);
    vdpSprite->x = 0x80;

    // apply changes immediately in VRAM
    vu16* pw = (u16 *) GFX_DATA_PORT;
    vu32* pl = (u32 *) GFX_CTRL_PORT;

    // prepare write to sprite #0
    *pl = GFX_WRITE_VRAM_ADDR(VDP_SPRITE_TABLE);

    // write fields in correct order
    *pw = vdpSprite->y;
    *pw = vdpSprite->size_link;
    *pw = vdpSprite->attribut;
    *pw = vdpSprite->x;
}

void SYS_hideFrameLoad()
{
    flags &= ~(SHOW_FRAME_LOAD | SHOW_FRAME_LOAD_MEAN);

    // use internal sprite 0 to show cursor
    VDPSprite* vdpSprite = &vdpSpriteCache[0];
    // hide it
    vdpSprite->y = 0;

    // apply changes immediately in VRAM
    vu16* pw = (u16 *) GFX_DATA_PORT;
    vu32* pl = (u32 *) GFX_CTRL_PORT;

    // prepare write to sprite #0
    *pl = GFX_WRITE_VRAM_ADDR(VDP_SPRITE_TABLE);
    // no need to write more
    *pw = vdpSprite->y;
}

u16 SYS_isInVIntCallback()
{
    return intTrace & IN_VINT;
}

u16 SYS_isInHIntCallback()
{
    return 0;
}

u16 SYS_isInExtIntCallback()
{
    return 0;
}

u16 SYS_isInInterrupt()
{
    return SYS_isInVInt();
}


bool SYS_isInVInt()
{
    return (intTrace & IN_VINT)?TRUE:FALSE;
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
bool addFrameLoad(u16 frameLoad, u32 vtime)
{
    static u16 lastVTimer = 0;
    u16 deltaFrame = vtime - lastVTimer;
    bool miss;
    u16 v;

    // frame miss ?
    if (deltaFrame > 1)
    {
        // force frame load to 255
        v = ((deltaFrame - 1) << 8) + frameLoad;
        miss = TRUE;
    }
    else
    {
        miss = FALSE;
        v = frameLoad;
    }

    cpuFrameLoad -= frameLoads[frameLoadIndex];
    frameLoads[frameLoadIndex] = v;
    cpuFrameLoad += v;
    frameLoadIndex = (frameLoadIndex + 1) & (LOAD_MEAN_FRAME_NUM - 1);
    lastVTimer = vtime;

    return miss;
}

u16 SYS_getCPULoad()
{
   return (cpuFrameLoad * ((u16) 100)) / (u16) (LOAD_MEAN_FRAME_NUM * 256);
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
