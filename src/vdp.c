#include "config.h"
#include "types.h"

#include "vdp.h"

#include "vdp_tile.h"
#include "vdp_spr.h"
#include "vdp_bg.h"

#include "tools.h"
#include "string.h"
#include "memory.h"
#include "dma.h"
#include "timer.h"
#include "sys.h"
#include "task.h"

#include "sprite_eng.h"
#include "sprite_eng_legacy.h"


#define WINDOW_DEFAULT          0xD000      // multiple of 0x1000 (0x0800 in H32)
#define HSCRL_DEFAULT           0xF000      // multiple of 0x0400
#define SLIST_DEFAULT           0xF400      // multiple of 0x0400 (0x0200 in H32)
#define APLAN_DEFAULT           0xE000      // multiple of 0x2000
#define BPLAN_DEFAULT           0xC000      // multiple of 0x2000


// we don't want to share it
extern u32 task_pc;
// we don't want to share it
extern bool addFrameLoad(u16 frameLoad, u32 vtime);

// forward
static void updateMapsAddress(bool initializing);
static bool computeFrameCPULoad(u16 blank, u16 vcnt, u32 vtime);
u16 getAdjustedVCounterInternal(u16 blank, u16 vcnt);
void updateUserTileMaxIndex();


static u8 regValues[0x13];

u16 window_addr;
u16 bga_addr;
u16 bgb_addr;
u16 hscrl_addr;
u16 slist_addr;
u16 maps_addr;

u16 userTileMaxIndex;

u16 screenWidth;
u16 screenHeight;
u16 planeWidth;
u16 planeHeight;
u16 windowWidth;
u16 planeWidthSft;
u16 planeHeightSft;
u16 windowWidthSft;

u16 lastVCnt;


NO_INLINE void VDP_init()
{
    vu16 *pw;
    u16 i;

    // wait for DMA completion
    VDP_waitDMACompletion();

    // default VRAM organization
    window_addr = WINDOW_DEFAULT;
    bga_addr = APLAN_DEFAULT;
    bgb_addr = BPLAN_DEFAULT;
    slist_addr = SLIST_DEFAULT;
    hscrl_addr = HSCRL_DEFAULT;

    // default resolution
    screenWidth = 320;
    screenHeight = 224;
    planeWidth = 64;
    planeHeight = 32;
    windowWidth = 64;
    planeWidthSft = 6;
    planeHeightSft = 5;
    windowWidthSft = 6;
    lastVCnt = 0;

    regValues[0x00] = 0x04;
    regValues[0x01] = 0x74;                     /* reg. 1 - Enable display, V-Int, DMA + VCell size */
    regValues[0x02] = bga_addr / 0x400;         /* reg. 2 - Plane A = $E000 */
    regValues[0x03] = window_addr / 0x400;      /* reg. 3 - Window  = $D000 */
    regValues[0x04] = bgb_addr / 0x2000;        /* reg. 4 - Plane B = $C000 */
    regValues[0x05] = slist_addr / 0x200;       /* reg. 5 - Sprite table = $F400 */
    regValues[0x06] = 0x00;                     /* reg. 6 - not used */
    regValues[0x07] = 0x00;                     /* reg. 7 - Background Color number*/
    regValues[0x08] = 0x00;                     /* reg. 8 - not used */
    regValues[0x09] = 0x00;                     /* reg. 9 - not used */
    regValues[0x0A] = 0x01;                     /* reg 10 - HInterrupt timing */
    regValues[0x0B] = 0x00;                     /* reg 11 - $0000abcd a=extr.int b=vscr cd=hscr */
    regValues[0x0C] = 0x81;                     /* reg 12 - hcell mode + shadow/highight + interlaced mode (40 cell, no shadow, no interlace) */
    regValues[0x0D] = hscrl_addr / 0x400;       /* reg 13 - HScroll Table = $F000 */
    regValues[0x0E] = 0x00;                     /* reg 14 - not used */
    regValues[0x0F] = 0x02;                     /* reg 15 - auto increment data */
    regValues[0x10] = 0x01;                     /* reg 16 - scrl screen v&h size (32x64) */
    regValues[0x11] = 0x00;                     /* reg 17 - window hpos */
    regValues[0x12] = 0x00;                     /* reg 18 - window vpos */

    // set registers
    pw = (u16 *) VDP_CTRL_PORT;
    for (i = 0x00; i < 0x13; i++) *pw = 0x8000 | (i << 8) | regValues[i];

    maps_addr = 0;
    // update minimum address of all tilemap/table (default is plane B)
    updateMapsAddress(TRUE);

    // clear VRAM, reset palettes / default tiles / font and scroll mode
    VDP_resetScreen();
    // reset sprite struct
    VDP_resetSprites();

    // default plane and base tile attribut for draw text method
    VDP_setTextPlane(BG_A);
    VDP_setTextPalette(PAL0);
    VDP_setTextPriority(TRUE);

    // internal
    curTileInd = TILE_USER_INDEX;
}


NO_INLINE void VDP_resetScreen()
{
    u16 i;
    bool enable = VDP_isEnable();

    // for faster operation
    VDP_setEnable(FALSE);

    // reset video memory (len = 0 is a special value to define 0x10000)
    DMA_doVRamFill(0, 0, 0, 1);
    // wait for DMA completion
    VDP_waitDMACompletion();

     // system tiles (16 plain tile)
    i = 16;
    while(i--) VDP_fillTileData(i | (i << 4), TILE_SYSTEM_INDEX + i, 1, TRUE);

    PAL_setPalette(PAL0, palette_grey, CPU);
    PAL_setPalette(PAL1, palette_red, CPU);
    PAL_setPalette(PAL2, palette_green, CPU);
    PAL_setPalette(PAL3, palette_blue, CPU);

    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_B, 0);

    {
        s16 values[20];

        // clear
        memset(values, 0, sizeof(values));

        // clear VSRAM
        VDP_setVerticalScrollTile(BG_A, 0, values, 20, CPU);
        VDP_setVerticalScrollTile(BG_B, 0, values, 20, CPU);
    }

    // load default font
    if (!VDP_loadDefaultFont(DMA))
    {
        VDP_setEnable(TRUE);

        KLog("A fatal error occured (not enough memory to reset VDP) !");

        // fatal error --> die here (the font did not get loaded so maybe not really useful to show this message...)
        VDP_drawText("Not enough memory to reset VDP !", 0, 2);

        VDP_drawText("A fatal error occured !", 2, 4);
        VDP_drawText("cannot continue...", 4, 5);

        // stop here
        while(TRUE);
    }

    // re-enable
    if (enable)
        VDP_setEnable(TRUE);
}


u8 VDP_getReg(u16 reg)
{
    if (reg < 0x13) return regValues[reg];
    else return 0;
}

void VDP_setReg(u16 reg, u8 value)
{
    vu16 *pw;
    u16 v;

    // update cached values
    switch (reg & 0x1F)
    {
        default:
            v = value;
            break;

        case 0x01:
            if (IS_PAL_SYSTEM)
            {
                v = value;
                if (v & 0x08) screenHeight = 240;
                else screenHeight = 224;
            }
            else
                v = value & 0xF7;
            break;

        case 0x02:
            v = value & 0x38;
            // update plane address
            bga_addr = v * 0x400;
            updateMapsAddress(FALSE);
            break;

        case 0x03:
            // 40H mode
            if (regValues[0x0C] & 0x81) v = value & 0x3C;
            // 32H mode
            else v = value & 0x3E;
            window_addr = v * 0x0400;
            updateMapsAddress(FALSE);
            break;

        case 0x04:
            v = value & 0x7;
            // update text plane address
            bgb_addr = v * 0x2000;
            updateMapsAddress(FALSE);
            break;

        case 0x05:
            // 40H mode
            if (regValues[0x0C] & 0x81) v = value & 0x7E;
            // 32H mode
            else v = value & 0x7F;
            slist_addr = v * 0x0200;
            updateMapsAddress(FALSE);
            break;

        case 0x0C:
            v = value;
            if (v & 0x81)
            {
                screenWidth = 320;
                windowWidth = 64;
                windowWidthSft = 6;
            }
            else
            {
                screenWidth = 256;
                windowWidth = 32;
                windowWidthSft = 5;
            }
            break;

        case 0x0D:
            v = value & 0x3F;
            hscrl_addr = v * 0x0400;
            updateMapsAddress(FALSE);
            break;

        case 0x10:
            v = value;
            if (v & 0x02)
            {
                planeWidth = 128;
                planeWidthSft = 7;
            }
            else if (v & 0x01)
            {
                planeWidth = 64;
                planeWidthSft = 6;
            }
            else
            {
                planeWidth = 32;
                planeWidthSft = 5;
            }
            if (v & 0x20)
            {
                planeHeight = 128;
                planeHeightSft = 7;
            }
            else if (v & 0x10)
            {
                planeHeight = 64;
                planeHeightSft = 6;
            }
            else
            {
                planeHeight = 32;
                planeHeightSft = 5;
            }
            break;
    }

    if (reg < 0x13) regValues[reg] = v;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8000 | (reg << 8) | v;
}

bool VDP_getEnable()
{
    return regValues[0x01] & 0x40;
}

bool VDP_isEnable()
{
    return VDP_getEnable();
}

void VDP_setEnable(bool value)
{
    vu16 *pw;

    if (value) regValues[0x01] |= 0x40;
    else regValues[0x01] &= ~0x40;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8100 | regValues[0x01];
}


u16 VDP_getScanlineNumber()
{
    if IS_PAL_SYSTEM return 312;
    else return 262;
}

u16 VDP_getScreenHeight()
{
    return screenHeight;
}

void VDP_setScreenHeight224()
{
    vu16 *pw;

    regValues[0x01] &= ~0x08;
    screenHeight = 224;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8100 | regValues[0x01];
}

void VDP_setScreenHeight240()
{
    vu16 *pw;

    if (IS_PAL_SYSTEM)
    {
        regValues[0x01] |= 0x08;
        screenHeight = 240;

        pw = (u16 *) VDP_CTRL_PORT;
        *pw = 0x8100 | regValues[0x01];
    }
}

u16 VDP_getScreenWidth()
{
    return screenWidth;
}

void VDP_setScreenWidth256()
{
    vu16 *pw;

    regValues[0x0C] &= ~0x81;
    screenWidth = 256;
    windowWidth = 32;
    windowWidthSft = 5;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8C00 | regValues[0x0C];
}

void VDP_setScreenWidth320()
{
    vu16 *pw;

    regValues[0x0C] |= 0x81;
    screenWidth = 320;
    windowWidth = 64;
    windowWidthSft = 6;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8C00 | regValues[0x0C];
}


u16 VDP_getPlaneWidth()
{
    return planeWidth;
}

u16 VDP_getPlaneHeight()
{
    return planeHeight;
}

NO_INLINE void VDP_setPlaneSize(u16 w, u16 h, bool setupVram)
{
    vu16 *pw;
    u16 v = 0;

    if (w & 0x80)
    {
        planeWidth = 128;
        planeWidthSft = 7;
        v |= 0x03;

        // plane height fixed to 32
        planeHeight = 32;
        planeHeightSft = 5;
    }
    else if (w & 0x40)
    {
        planeWidth = 64;
        planeWidthSft = 6;
        v |= 0x01;

        // only 64 or 32 accepted for plane height
        if (h & 0x40)
        {
            planeHeight = 64;
            planeHeightSft = 6;
            v |= 0x10;
        }
        else
        {
            planeHeight = 32;
            planeHeightSft = 5;
        }
    }
    else
    {
        planeWidth = 32;
        planeWidthSft = 5;

        // plane height can be 128, 64 or 32
        if (h & 0x80)
        {
            planeHeight = 128;
            planeHeightSft = 7;
            v |= 0x30;
        }
        else if (h & 0x40)
        {
            planeHeight = 64;
            planeHeightSft = 6;
            v |= 0x10;
        }
        else
        {
            planeHeight = 32;
            planeHeightSft = 5;
        }
    }

    regValues[0x10] = v;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x9000 | regValues[0x10];

    if (setupVram)
    {
        switch(planeWidthSft + planeHeightSft)
        {
            case 10:
                // 2KB tilemap VRAM setup: 0xC000 --> 0xEFFF
                // 0xD000-0xDFFF free
                VDP_setBGBAddress(0xC000);
                VDP_setWindowAddress(0xC800);
                VDP_setBGAAddress(0xE000);
                VDP_setSpriteListAddress(0xE800);
                VDP_setHScrollTableAddress(0xEC00);
                break;

            case 11:
                // 4KB tilemap VRAM setup: 0xC000 --> 0xFFFF
                // 0xF700-0xFFFF free
                VDP_setBGBAddress(0xC000);
                VDP_setWindowAddress(0xD000);
                VDP_setBGAAddress(0xE000);
                VDP_setHScrollTableAddress(0xF000);
                VDP_setSpriteListAddress(0xF400);
                break;

            default:
                // 8KB tilemap VRAM setup: 0xA800 --> 0xFFFF
                // 0xAF00-0xAFFF free
                VDP_setSpriteListAddress(0xAC00);
                VDP_setHScrollTableAddress(0xA800);
                VDP_setWindowAddress(0xB000);
                VDP_setBGBAddress(0xC000);
                VDP_setBGAAddress(0xE000);
                break;
        }

        updateMapsAddress(FALSE);
    }
}

u8 VDP_getVerticalScrollingMode()
{
    return (regValues[0x0B] >> 2) & 1;
}

u8 VDP_getHorizontalScrollingMode()
{
    return regValues[0x0B] & 3;
}

void VDP_setScrollingMode(u16 hscroll, u16 vscroll)
{
    vu16 *pw;

    regValues[0x0B] &= ~0x07;
    regValues[0x0B] |= ((vscroll & 1) << 2) | (hscroll & 3);

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8B00 | regValues[0x0B];
}


u8 VDP_getBackgroundColor()
{
    return regValues[0x07];
}

void VDP_setBackgroundColor(u8 value)
{
    vu16 *pw;

    regValues[0x07] = value & 0x3F;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8700 | regValues[0x07];
}


u8 VDP_getAutoInc()
{
    return regValues[0x0F];
}

void VDP_setAutoInc(u8 value)
{
    vu16 *pw;

    regValues[0x0F] = value;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8F00 | value;
}


u8 VDP_getDMAEnabled()
{
    return regValues[0x01] & 0x10;
}

void VDP_setDMAEnabled(bool value)
{
    vu16 *pw;

    if (value) regValues[0x01] |= 0x10;
    else regValues[0x01] &= ~0x10;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8100 | regValues[0x01];
}

u8 VDP_getHVLatching()
{
    return regValues[0x00] & 0x02;
}

void VDP_setHVLatching(bool value)
{
    vu16 *pw;

    if (value) regValues[0x00] |= 0x02;
    else regValues[0x00] &= ~0x02;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8000 | regValues[0x00];
}

void VDP_setVInterrupt(bool value)
{
    vu16 *pw;

    if (value) regValues[0x01] |= 0x20;
    else regValues[0x01] &= ~0x20;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8100 | regValues[0x01];
}

void VDP_setHInterrupt(bool value)
{
    vu16 *pw;

    if (value) regValues[0x00] |= 0x10;
    else regValues[0x00] &= ~0x10;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8000 | regValues[0x00];
}

void VDP_setExtInterrupt(bool value)
{
    vu16 *pw;

    if (value) regValues[0x0B] |= 0x08;
    else regValues[0x0B] &= ~0x08;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8B00 | regValues[0x0B];
}

void VDP_setHilightShadow(bool value)
{
    vu16 *pw;

    if (value) regValues[0x0C] |= 0x08;
    else regValues[0x0C] &= ~0x08;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8C00 | regValues[0x0C];
}


u8 VDP_getHIntCounter()
{
    return regValues[0x0A];
}

void VDP_setHIntCounter(u8 value)
{
    vu16 *pw;

    regValues[0x0A] = value;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8A00 | regValues[0x0A];
}


u16 VDP_getBGAAddress()
{
    return bga_addr;
}

u16 VDP_getBGBAddress()
{
    return bgb_addr;
}

u16 VDP_getWindowAddress()
{
    return window_addr;
}

u16 VDP_getSpriteListAddress()
{
    return slist_addr;
}

u16 VDP_getHScrollTableAddress()
{
    return hscrl_addr;
}


void VDP_setBGAAddress(u16 value)
{
    vu16 *pw;

    bga_addr = value & 0xE000;
    updateMapsAddress(FALSE);

    regValues[0x02] = bga_addr / 0x400;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8200 | regValues[0x02];
}

void VDP_setBGBAddress(u16 value)
{
    vu16 *pw;

    bgb_addr = value & 0xE000;
    updateMapsAddress(FALSE);

    regValues[0x04] = bgb_addr / 0x2000;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8400 | regValues[0x04];
}

void VDP_setWindowAddress(u16 value)
{
    vu16 *pw;

    // 40H mode
    if (regValues[0x0C] & 0x81) window_addr = value & 0xF000;
    // 32H mode
    else window_addr = value & 0xF800;
    updateMapsAddress(FALSE);

    regValues[0x03] = window_addr / 0x400;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8300 | regValues[0x03];
}

void VDP_setSpriteListAddress(u16 value)
{
    vu16 *pw;

    // 40H mode
    if (regValues[0x0C] & 0x81) slist_addr = value & 0xFC00;
    // 32H mode
    else slist_addr = value & 0xFE00;
    updateMapsAddress(FALSE);

    regValues[0x05] = slist_addr / 0x200;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8500 | regValues[0x05];
}

void VDP_setHScrollTableAddress(u16 value)
{
    vu16 *pw;

    hscrl_addr = value & 0xFC00;
    updateMapsAddress(FALSE);

    regValues[0x0D] = hscrl_addr / 0x400;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8D00 | regValues[0x0D];
}

void VDP_setScanMode(u16 value)
{
    vu16 *pw;

    if (value == 0)
        // non-interlaced
        regValues[0x0C] &= ~0x06;
    else if (value == 1)
        // interlace mode 1
        regValues[0x0C] = (regValues[0x0C] & ~0x04) | 0x02;
    else
        // interlace mode 2
        regValues[0x0C] |= 0x06;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x8C00 | regValues[0x0C];
}

void VDP_setWindowHPos(u16 right, u16 pos)
{
    vu16 *pw;
    u16 v;

    v = pos & 0x7F;
    if (right) v |= 0x80;

    regValues[0x11] = v;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x9100 | v;
}

void VDP_setWindowVPos(u16 down, u16 pos)
{
    vu16 *pw;
    u16 v;

    v = pos & 0x7F;
    if (down) v |= 0x80;

    regValues[0x12] = v;

    pw = (u16 *) VDP_CTRL_PORT;
    *pw = 0x9200 | v;
}

void VDP_setWindowOff() 
{
    VDP_setWindowVPos(false, 0);
    VDP_setWindowHPos(false, 0);
}

void VDP_setWindowOnTop(u16 rows) 
{
    VDP_setWindowVPos(false, rows);
}

void VDP_setWindowOnBottom(u16 rows) 
{
    VDP_setWindowVPos(true, (screenHeight / 8) - rows);
}

void VDP_setWindowOnLeft(u16 cols) 
{
    VDP_setWindowHPos(false, cols);
}

void VDP_setWindowOnRight(u16 cols) 
{
    VDP_setWindowHPos(true, (screenWidth / 16) - cols);
}

void VDP_setWindowFullScreen()
{
    VDP_setWindowVPos(false, screenHeight / 8);
}

void VDP_waitDMACompletion()
{
    while(GET_VDP_STATUS(VDP_DMABUSY_FLAG));
}

void VDP_waitFIFOEmpty()
{
    while(!GET_VDP_STATUS(VDP_FIFOEMPTY_FLAG));
}


NO_INLINE bool VDP_waitVInt()
{
    // in VInt --> return
    if (SYS_isInVInt()) return FALSE;

    // initial frame counter
    const u32 t = vtimer;
    // store V-Counter and initial blank state
    const u16 vcnt = GET_VCOUNTER;
    const u16 blank = GET_VDP_STATUS(VDP_VBLANK_FLAG);
    // save it (used to diplay frame load)
    lastVCnt = vcnt;

    // compute frame load now (return TRUE if frame miss / late detected)
    bool late = computeFrameCPULoad(blank, vcnt, t);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
    if (late)
    {
        // we cannot detect frame miss if HV latching is enabled..
        if (!VDP_getHVLatching())
            KLog_U2("Warning: frame missed detection on frame #", t, " - V-Counter = ", vcnt);
    }
#endif

    // background user task ? --> switch to it (automatically switch back to front task on v-int)
    if (task_pc != NULL) TSK_userYield();
    // otherwise we just wait for next VInt
    else while (vtimer == t);

    return late;
}


NO_INLINE bool VDP_waitVBlank(bool forceNext)
{
    vu16 *pw = (u16 *) VDP_CTRL_PORT;

    // initial frame counter
    const u32 t = vtimer;
    // store V-Counter and initial blank state
    const u16 vcnt = GET_VCOUNTER;
    const u16 blank = *pw & VDP_VBLANK_FLAG;
    // have an user task (multitasking) ?
    const bool yield_to_user = (task_pc != NULL);
    // VDP enable
    const bool enabled = VDP_isEnable();
    // save it (used to diplay frame load)
    lastVCnt = vcnt;

    // we want to wait for next start of VBlank ? (only if no multitasking)
    if (forceNext && blank && enabled && !yield_to_user)
    {
        // wait end of vblank if already in vblank
        while (*pw & VDP_VBLANK_FLAG);
    }

    // compute frame load now (return TRUE if frame miss / late detected)
    bool late = computeFrameCPULoad(blank, vcnt, t);

#if (LIB_LOG_LEVEL >= LOG_LEVEL_WARNING)
    if (late)
    {
        // we cannot detect late frame if VDP is disabled or HV latching is enabled
        if (enabled && !VDP_getHVLatching())
        {
            if (forceNext)
                KLog_U2("Warning: frame missed detection on frame #", vtimer, " - V-Counter = ", vcnt);
            else
                KLog_U2("Warning: frame late detection on frame #", vtimer, " - V-Counter = ", vcnt);
        }
    }
#endif

    // background user task ? --> switch to it (automatically switch back to front task on v-int)
    if (yield_to_user) TSK_userYield();
    // otherwise we just wait end of active period
    else while (!(*pw & VDP_VBLANK_FLAG));

    return late;
}

void VDP_waitVActive(bool forceNext)
{
    vu16 *pw = (u16 *) VDP_CTRL_PORT;

    // we want to wait for next start of VActive ?
    if (forceNext)
    {
        // wait end of vactive if already in vactive
        while (!(*pw & VDP_VBLANK_FLAG));
    }
    // we stay blank forever if VDP is disabled..
    if (VDP_isEnable())
    {
        // wait end of vblank
        while (*pw & VDP_VBLANK_FLAG);
    }
}

bool VDP_waitVSync()
{
    return VDP_waitVBlank(TRUE);
}


static bool computeFrameCPULoad(u16 blank, u16 vcnt, u32 vtime)
{
    // update CPU frame load
    return addFrameLoad(getAdjustedVCounterInternal(blank, vcnt), vtime);
}

u16 getAdjustedVCounterInternal(u16 blank, u16 vcnt)
{
    u16 result = vcnt;

    // adjust V-Counter to take care of blanking rollback
    if (IS_PAL_SYSTEM)
    {
        // blank adjustement
        if (blank && ((result >= 0xCA) || (result <= 0x0A))) result = 8;
        // sometime blank flag is not yet/anymore set on edge area so we double check
        else if (result >= VDP_getScreenHeight()) result = 8;
        else result += 16;
    }
    else
    {
        // blank adjustement
        if (blank && (result >= 0xDF)) result = 16;
        // sometime blank flag is not yet/anymore set on edge area
        else if (result >= 224) result = 16;
        else result += 32;
    }

    return result;
}

u16 VDP_getAdjustedVCounter()
{
    return getAdjustedVCounterInternal(GET_VDP_STATUS(VDP_VBLANK_FLAG), GET_VCOUNTER);
}


void VDP_showFPS(u16 asFloat, u16 x, u16 y)
{
    char str[16];

    if (asFloat)
    {
        fix32ToStr(SYS_getFPSAsFloat(), str, 1);
        // display FPS
        VDP_drawTextFill(str, x, y, 4);
    }
    else
    {
        uintToStr(SYS_getFPS(), str, 1);
        // display FPS
        VDP_drawTextFill(str, x, y, 2);
    }
}

void VDP_showCPULoad(u16 x, u16 y)
{
    char str[16];

    uintToStr(SYS_getCPULoad(), str, 1);
    strcat(str, "%");

    // display CPU load
    VDP_drawTextFill(str, x, y, 4);
}


void updateUserTileMaxIndex()
{
    // sprite engine always allocate VRAM just below FONT
    userTileMaxIndex = TILE_FONT_INDEX - spriteVramSize;
}

static void updateMapsAddress(bool initializing)
{
    u16 min_addr = window_addr;

    if (bgb_addr < min_addr) min_addr = bgb_addr;
    if (bga_addr < min_addr) min_addr = bga_addr;
    if (hscrl_addr < min_addr) min_addr = hscrl_addr;
    if (slist_addr < min_addr) min_addr = slist_addr;

    // need to reload font
    if (min_addr != maps_addr)
    {
        maps_addr = min_addr;
        // update user max tile index
        updateUserTileMaxIndex();

        // initialization will load font afterward
        if (!initializing)
        {
            // reload default font as its VRAM address has changed
            VDP_loadDefaultFont(CPU);
            // re-pack memory as VDP_lontFont allocate memory to unpack font
            MEM_pack();
        }

        // sprite engine in use ? --> defrag VRAM (this will basically re-allocate all dynamically managed VRAM)
        if (SPR_isInitialized())
            SPR_defragVRAM();
    }
}
