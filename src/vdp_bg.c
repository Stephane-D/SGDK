#include "config.h"
#include "types.h"

#include "vdp.h"
#include "vdp_bg.h"

#include "tools.h"
#include "string.h"
#include "vdp_dma.h"
#include "vdp_pal.h"
#include "vdp_tile.h"

#include "font.h"
#include "memory.h"


u16 *text_plan;
u16 text_basetile;


void VDP_setHorizontalScroll(VDPPlan plan, s16 value)
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = HSCRL;
    if (plan.v == PLAN_B.v) addr += 2;

    *pl = GFX_WRITE_VRAM_ADDR(addr);
    *pw = value;
}

void VDP_setHorizontalScrollTile(VDPPlan plan, u16 tile, s16* values, u16 len, u16 use_dma)
{
    u16 addr;

    addr = HSCRL + ((tile & 0x1F) * (4 * 8));
    if (plan.v == PLAN_B.v) addr += 2;

    VDP_setAutoInc(4 * 8);

    if (use_dma) VDP_doDMAEx(VDP_DMA_VRAM, (u32) values, addr, len, -1);
    else
    {
        vu16 *pw;
        vu32 *pl;
        u16 *src;
        u16 i;

        /* Point to vdp port */
        pw = (u16 *) GFX_DATA_PORT;
        pl = (u32 *) GFX_CTRL_PORT;

        *pl = GFX_WRITE_VRAM_ADDR(addr);

        src = values;

        i = len;
        while(i--) *pw = *src++;
    }
}

void VDP_setHorizontalScrollLine(VDPPlan plan, u16 line, s16* values, u16 len, u16 use_dma)
{
    u16 addr;

    addr = HSCRL + ((line & 0xFF) * 4);
    if (plan.v == PLAN_B.v) addr += 2;

    VDP_setAutoInc(4);

    if (use_dma) VDP_doDMAEx(VDP_DMA_VRAM, (u32) values, addr, len, -1);
    else
    {
        vu16 *pw;
        vu32 *pl;
        u16 *src;
        u16 i;

        /* Point to vdp port */
        pw = (u16 *) GFX_DATA_PORT;
        pl = (u32 *) GFX_CTRL_PORT;

        *pl = GFX_WRITE_VRAM_ADDR(addr);

        src = values;

        i = len;
        while(i--) *pw = *src++;
    }
}

void VDP_setVerticalScroll(VDPPlan plan, s16 value)
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = 0;
    if (plan.v == PLAN_B.v) addr += 2;

    *pl = GFX_WRITE_VSRAM_ADDR(addr);
    *pw = value;
}

void VDP_setVerticalScrollTile(VDPPlan plan, u16 tile, s16* values, u16 len, u16 use_dma)
{
    u16 addr;

    addr = (tile & 0x1F) * 4;
    if (plan.v == PLAN_B.v) addr += 2;

    VDP_setAutoInc(4);

    if (use_dma) VDP_doDMAEx(VDP_DMA_VSRAM, (u32) values, addr, len, -1);
    else
    {
        vu16 *pw;
        vu32 *pl;
        u16 *src;
        u16 i;

        /* Point to vdp port */
        pw = (u16 *) GFX_DATA_PORT;
        pl = (u32 *) GFX_CTRL_PORT;

        *pl = GFX_WRITE_VSRAM_ADDR(addr);

        src = values;

        i = len;
        while(i--) *pw = *src++;
    }
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

VDPPlan VDP_getTextPlan()
{
    if (text_plan == &aplan_adr) return PLAN_A;
    else return PLAN_B;
}

u16 VDP_getTextPalette()
{
    return (text_basetile >> 13) & 3;
}

u16 VDP_getTextPriority()
{
    return (text_basetile >> 15) & 1;
}

void VDP_setTextPlan(VDPPlan plan)
{
    if (plan.v == PLAN_B.v) text_plan = &bplan_adr;
    else text_plan = &aplan_adr;
}

void VDP_setTextPalette(u16 pal)
{
    text_basetile &= ~(3 << 13);
    text_basetile |= (pal & 3) << 13;
}

void VDP_setTextPriority(u16 prio)
{
    text_basetile &= ~(1 << 15);
    text_basetile |= (prio & 1) << 15;
}

void VDP_drawTextBG(u16 plan, const char *str, u16 flags, u16 x, u16 y)
{
    u32 len;
    u16 data[128];
    u16 i;

    // get the horizontal plan size (in cell)
    i = VDP_getPlanWidth();
    len = strlen(str);

    // if string don't fit in plan, we cut it
    if (len > (i - x))
        len = i - x;

    for (i = 0; i < len; i++)
        data[i] = TILE_FONTINDEX + (str[i] - 32);

    VDP_setTileMapDataRectEx(plan, data, flags, x, y, len, 1, len);
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
    VDP_drawTextBG(*text_plan, str, text_basetile, x, y);
}

void VDP_clearText(u16 x, u16 y, u16 w)
{
    VDP_clearTextBG(*text_plan, x, y, w);
}

void VDP_clearTextLine(u16 y)
{
    VDP_clearTextLineBG(*text_plan, y);
}


u16 VDP_drawBitmap(u16 plan, const Bitmap *bitmap, u16 x, u16 y)
{
    return VDP_drawBitmapEx(plan, bitmap, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USERINDEX), x, y, TRUE);
}

u16 VDP_drawBitmapEx(u16 plan, const Bitmap *bitmap, u16 basetile, u16 x, u16 y, u16 loadpal)
{
    const int wt = bitmap->w / 8;
    const int ht = bitmap->h / 8;
    const Palette *palette = bitmap->palette;

    // compressed bitmap ?
    if (bitmap->compression != COMPRESSION_NONE)
    {
        Bitmap *b = unpackBitmap(bitmap, NULL);

        if (b == NULL) return FALSE;

        // tiles
        VDP_loadBMPTileData((u32*) b->image, basetile & TILE_INDEX_MASK, wt, ht, wt);
        MEM_free(b);
    }
    else
        // tiles
        VDP_loadBMPTileData((u32*) bitmap->image, basetile & TILE_INDEX_MASK, wt, ht, wt);

    // tilemap
    VDP_fillTileMapRectInc(plan, basetile, x, y, wt, ht);
    // palette
    if (loadpal) VDP_setPaletteColors(((basetile >> 9) & 0x30) + (palette->index & 0xF), palette->data, palette->length);

    return TRUE;
}

u16 VDP_drawImage(u16 plan, const Image *image, u16 x, u16 y)
{
    return VDP_drawImageEx(plan, image, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USERINDEX), x, y, TRUE, TRUE);
}

u16 VDP_drawImageEx(u16 plan, const Image *image, u16 basetile, u16 x, u16 y, u16 loadpal, u16 use_dma)
{
    Palette *palette;

    if (!VDP_loadTileSet(image->tileset, basetile & TILE_INDEX_MASK, use_dma))
        return FALSE;

    if (!VDP_setMap(plan, image->map, basetile, x, y))
        return FALSE;

    palette = image->palette;

    // palette
    if (loadpal) VDP_setPaletteColors(((basetile >> 9) & 0x30) + (palette->index & 0xF), palette->data, palette->length);

    return TRUE;
}
