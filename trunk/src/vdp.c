#include "config.h"
#include "types.h"

#include "vdp.h"

#include "vdp_dma.h"
#include "vdp_tile.h"
#include "vdp_pal.h"
#include "vdp_spr.h"
#include "vdp_bg.h"

#include "font.h"

#include "tools.h"
#include "string.h"


static u8 regValues[0x13];
u16 textBasetile;


void VDP_init()
{
    vu16 *pw;
    u16 i;

    /* wait for DMA completion */
    VDP_waitDMACompletion();

    regValues[0x00] = 0x04;             /* reg. 0 - Disable HBL */
    regValues[0x01] = 0x74;             /* reg. 1 - Enable display, VBL, DMA + VCell size */
    regValues[0x02] = APLAN / 0x400;    /* reg. 2 - Plane A =$30*$400=$C000 */
    regValues[0x03] = WPLAN / 0x400;    /* reg. 3 - Window  =$2C*$400=$B000 */
    regValues[0x04] = BPLAN / 0x2000;   /* reg. 4 - Plane B =$7*$2000=$E000 */
    regValues[0x05] = SLIST / 0x200;    /* reg. 5 - sprite table begins at $BC00=$5E*$200 */
    regValues[0x06] = 0x00;             /* reg. 6 - not used */
    regValues[0x07] = 0x00;             /* reg. 7 - Background Color number*/
    regValues[0x08] = 0x00;             /* reg. 8 - not used */
    regValues[0x09] = 0x00;             /* reg. 9 - not used */
    regValues[0x0A] = 0x01;             /* reg 10 - HInterrupt timing */
    regValues[0x0B] = 0x00;             /* reg 11 - $0000abcd a=extr.int b=vscr cd=hscr */
    regValues[0x0C] = 0x81;             /* reg 12 - hcell mode + shadow/highight + interlaced mode (40 cell, no shadow, no interlace) */
    regValues[0x0D] = HSCRL / 0x400;    /* reg 13 - HScroll Table =$2E*$400=$B800 */
    regValues[0x0E] = 0x00;             /* reg 14 - not used */
    regValues[0x0F] = 0x02;             /* reg 15 - auto increment data */
    regValues[0x10] = 0x11;             /* reg 16 - scrl screen v&h size (64x64) */
    regValues[0x11] = 0x00;             /* reg 17 - window hpos */
    regValues[0x12] = 0x00;             /* reg 18 - window vpos */

    /* set registers */
    pw = (u16 *) GFX_CTRL_PORT;
    for (i = 0x00; i < 0x13; i++) *pw = 0x8000 | (i << 8) | regValues[i];

    /* reset video memory */
    VDP_doVRamDMAFill(0, 0xFFFF, 0);
    /* wait for DMA completion */
    VDP_waitDMACompletion();

    /* system tiles (16 "flat" tile) */
    i = 16;
    while(i--) VDP_fillTileData(i | (i << 4), TILE_SYSTEMINDEX + i, 1, 0);

    /* load default font (don't use DMA for > 4MB rom) */
    VDP_loadFont(font_base, 0);

    /* load defaults palettes */
    VDP_setPalette(PAL0, palette_grey);
    VDP_setPalette(PAL1, palette_red);
    VDP_setPalette(PAL2, palette_green);
    VDP_setPalette(PAL3, palette_blue);

    /* reset vertical scroll for plan A & B*/
    VDP_setVerticalScroll(APLAN,  0);
    VDP_setVerticalScroll(BPLAN,  0);

    /* reset sprite struct */
    VDP_resetSprites();

    /* default base tile attribut for draw text method */
    textBasetile = TILE_ATTR(0, 1, 0, 0);
}


u8 VDP_getReg(u16 reg)
{
    if (reg < 0x13) return regValues[reg];
    else return 0;
}

void VDP_setReg(u16 reg, u8 value)
{
    vu16 *pw;

    if (reg < 0x13) regValues[reg] = value;

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8000 | (reg << 8) | value;
}


u8 VDP_getEnable()
{
    return regValues[0x01] & 0x40;
}

void VDP_setEnable(u8 value)
{
    vu16 *pw;

    if (value) regValues[0x01] |= 0x40;
    else regValues[0x01] &= ~0x40;

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8100 | regValues[0x01];
}


u16 VDP_getScanlineNumber()
{
    if IS_PALSYSTEM return 312;
    else return 262;
}

u16 VDP_getScreenHeight()
{
    if (regValues[0x01] & 0x08) return 240;
    else return 224;
}

void VDP_setScreenHeight224()
{
    vu16 *pw;

    regValues[0x01] &= ~0x08;

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8100 | regValues[0x01];
}

void VDP_setScreenHeight240()
{
    vu16 *pw;

    if (IS_PALSYSTEM)
    {
        regValues[0x01] |= 0x08;

        pw = (u16 *) GFX_CTRL_PORT;
        *pw = 0x8100 | regValues[0x01];
    }
}

u16 VDP_getScreenWidth()
{
    if (regValues[0x0C] & 0x81) return 320;
    else return 256;
}

void VDP_setScreenWidth256()
{
    vu16 *pw;

    regValues[0x0C] &= ~0x81;

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8C00 | regValues[0x0C];
}

void VDP_setScreenWidth320()
{
    vu16 *pw;

    regValues[0x0C] |= 0x81;

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8C00 | regValues[0x0C];
}


u16 VDP_getPlanWidth()
{
    return ((regValues[0x10] & 0xF) + 1) << 5;
}

u16 VDP_getPlanHeight()
{
    return ((regValues[0x10] >> 4) + 1) << 5;
}

void VDP_setPlanSize(u16 w, u16 h)
{
    vu16 *pw;

    regValues[0x10] = (((h >> 5) - 1) << 4) | (((w >> 5) - 1) << 0);

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x9000 | regValues[0x10];
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

    regValues[0x0B] |= ((vscroll & 1) << 2) | (hscroll & 3);

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8B00 | regValues[0x0B];
}


u8 VDP_getAutoInc()
{
    return regValues[0x0F];
}

void VDP_setAutoInc(u8 value)
{
    vu16 *pw;

    regValues[0x0F] = value;

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8F00 | value;
}


void VDP_setHInterrupt(u8 value)
{
    vu16 *pw;

    if (value) regValues[0x00] |= 0x10;
    else regValues[0x00] &= ~0x10;

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8000 | regValues[0x00];
}

void VDP_setHilightShadow(u8 value)
{
    vu16 *pw;

    if (value) regValues[0x0C] |= 0x08;
    else regValues[0x0C] &= ~0x08;

    pw = (u16 *) GFX_CTRL_PORT;
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

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8A00 | regValues[0x0A];
}


u16 VDP_getAPlanAddress()
{
    return regValues[0x02] * 0x400;
}

u16 VDP_getBPlanAddress()
{
    return regValues[0x04] * 0x2000;
}

u16 VDP_getWindowPlanAddress()
{
    return regValues[0x03] * 0x400;
}

u16 VDP_getSpriteListAddress()
{
    return regValues[0x05] * 0x200;
}

u16 VDP_getHScrollTableAddress()
{
    return regValues[0x0D] * 0x400;
}


void VDP_setAPlanAddress(u16 value)
{
    vu16 *pw;

    regValues[0x02] = (value / 0x400) & 0x38;

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8200 | regValues[0x02];
}

void VDP_setWindowPlanAddress(u16 value)
{
    vu16 *pw;

    if (regValues[0x0C] & 0x81)
        // 40H mode
        regValues[0x03] = (value / 0x400) & 0x3C;
    else
        // 32H mode
        regValues[0x03] = (value / 0x400) & 0x3E;

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8300 | regValues[0x03];
}

void VDP_setBPlanAddress(u16 value)
{
    vu16 *pw;

    regValues[0x04] = (value / 0x2000) & 0x07;

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8400 | regValues[0x04];
}

void VDP_setSpriteListAddress(u16 value)
{
    vu16 *pw;

    if (regValues[0x0C] & 0x81)
        // 40H mode
        regValues[0x05] = (value / 0x200) & 0x7E;
    else
        // 32H mode
        regValues[0x05] = (value / 0x200) & 0x7F;

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8500 | regValues[0x05];
}

void VDP_setHScrollTableAddress(u16 value)
{
    vu16 *pw;

    regValues[0x0D] = (value / 0x400) & 0x3F;

    pw = (u16 *) GFX_CTRL_PORT;
    *pw = 0x8D00 | regValues[0x0D];
}


void VDP_waitDMACompletion()
{
    while(GET_VDPSTATUS(VDP_DMABUSY_FLAG));
}

void VDP_waitFIFOEmpty()
{
    while(!GET_VDPSTATUS(VDP_FIFOEMPTY_FLAG));
}


void VDP_waitVSync()
{
    vu16 *pw;

    pw = (u16 *) GFX_CTRL_PORT;

    while (*pw & VDP_VBLANK_FLAG);
    while (!(*pw & VDP_VBLANK_FLAG));
}


void VDP_resetScreen()
{
    VDP_clearPlan(APLAN, 1);
    VDP_waitDMACompletion();
    VDP_clearPlan(BPLAN, 1);
    VDP_waitDMACompletion();

    VDP_setPalette(PAL0, palette_grey);
    VDP_setPalette(PAL1, palette_red);
    VDP_setPalette(PAL2, palette_green);
    VDP_setPalette(PAL3, palette_blue);
}


void VDP_showFPS(u16 float_display)
{
    char str[16];

    if (float_display)
    {
        fix32ToStr(getFPS_f(), str, 1);
        VDP_clearText(2, 1, 5);
    }
    else
    {
        uintToStr(getFPS(), str, 1);
        VDP_clearText(2, 1, 2);
    }

    // display FPS
    VDP_drawText(str, 1, 1);
}
