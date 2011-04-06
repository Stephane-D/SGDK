#include "config.h"
#include "types.h"

#include "font.h"
#include "vdp.h"
#include "vdp_bg.h"

#include "tools.h"
#include "vdp_dma.h"
#include "vdp_tile.h"

#include "tab_vram.h"


// don't want to share it
extern u16 textBasetile;


void VDP_setHorizontalScroll(u16 plan, u16 line, u16 value)
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = HSCRL + ((line & 0xFF) * 4);
    if (plan == BPLAN) addr += 2;

    *pl = GFX_WRITE_VRAM_ADDR(addr);
    *pw = value;
}

void VDP_setVerticalScroll(u16 plan, u16 cell, u16 value)
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = (cell & 0x1F) * 4;
    if (plan == BPLAN) addr += 2;

    *pl = GFX_WRITE_VSRAM_ADDR(addr);
    *pw = value;
}


void VDP_clearPlan(u16 plan, u8 use_dma)
{
    if (use_dma)
    {
        // wait for previous DMA completion
        VDP_waitDMACompletion();
        // then do DMA
        VDP_doVRamDMAFill(plan, VDP_getPlanWidth() * VDP_getPlanHeight() * 2, 0);
    }
    else
    {
        vu32 *plctrl;
        vu32 *pldata;
        u16 i;

        /* point to vdp port */
        plctrl = (u32 *) GFX_CTRL_PORT;
        pldata = (u32 *) GFX_DATA_PORT;

        *plctrl = GFX_WRITE_VRAM_ADDR(plan);

        // unroll a bit
        i = VDP_getPlanWidth() * VDP_getPlanHeight() / (2 * 8);
        while (i--)
        {
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
        }
    }
}

void VDP_setTextPalette(u16 pal)
{
    textBasetile &= ~(3 << 13);
    textBasetile |= (pal & 3) << 13;
}

void VDP_setTextPriority(u16 prio)
{
    textBasetile &= ~(1 << 15);
    textBasetile |= (prio & 1) << 15;
}

u16 VDP_getTextPalette()
{
    return (textBasetile >> 13) & 3;
}

u16 VDP_getTextPriority()
{
    return (textBasetile >> 15) & 1;
}

void VDP_drawTextBG(u16 plan, const char *str, u16 basetile, u16 x, u16 y)
{
    u32 len;
    u16 data[40];
    u16 i;

    // get the horizontal plan size (in cell)
    i = VDP_getPlanWidth();
    len = strlen(str);

    // if string don't fit in plan, we cut it
    if (len > (i - x)) len = i - x;

    for (i = 0; i < len; i++) data[i] = TILE_FONTINDEX + (str[i] - 32);
    VDP_setTileMapRect(plan, data, basetile, x, y, len, 1);
}

void VDP_clearTextBG(u16 plan, u16 x, u16 y, u16 w)
{
    VDP_fillTileMapRect(plan, 0, x, y, w, 1);
}

void VDP_clearTextLineBG(u16 plan, u16 y)
{
    VDP_fillTileMapRect(plan, 0, 0, y, VDP_getPlanWidth(), 1);
}

void VDP_drawText(const char *str, u16 x, u16 y)
{
    // use A plan & high priority by default
    VDP_drawTextBG(APLAN, str, textBasetile, x, y);
}

void VDP_clearText(u16 x, u16 y, u16 w)
{
    // use A plan by default
    VDP_clearTextBG(APLAN, x, y, w);
}

void VDP_clearTextLine(u16 y)
{
    // use A plan by default
    VDP_clearTextLineBG(APLAN, y);
}
