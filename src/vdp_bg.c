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


static VDPPlan text_plan;
static u16 text_basetile;

// current VRAM upload tile position
u16 curTileInd;


void VDP_setHorizontalScroll(VDPPlan plan, s16 value)
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = VDP_HSCROLL_TABLE;
    if (plan.value == CONST_PLAN_B) addr += 2;

    *pl = GFX_WRITE_VRAM_ADDR(addr);
    *pw = value;
}

void VDP_setHorizontalScrollTile(VDPPlan plan, u16 tile, s16* values, u16 len, u16 use_dma)
{
    u16 addr;

    addr = VDP_HSCROLL_TABLE + ((tile & 0x1F) * (4 * 8));
    if (plan.value == CONST_PLAN_B) addr += 2;

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

        src = (u16*) values;

        i = len;
        while(i--) *pw = *src++;
    }
}

void VDP_setHorizontalScrollLine(VDPPlan plan, u16 line, s16* values, u16 len, u16 use_dma)
{
    u16 addr;

    addr = VDP_HSCROLL_TABLE + ((line & 0xFF) * 4);
    if (plan.value == CONST_PLAN_B) addr += 2;

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

        src = (u16*) values;

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
    if (plan.value == CONST_PLAN_B) addr += 2;

    *pl = GFX_WRITE_VSRAM_ADDR(addr);
    *pw = value;
}

void VDP_setVerticalScrollTile(VDPPlan plan, u16 tile, s16* values, u16 len, u16 use_dma)
{
    u16 addr;

    addr = (tile & 0x1F) * 4;
    if (plan.value == CONST_PLAN_B) addr += 2;

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

        src = (u16*) values;

        i = len;
        while(i--) *pw = *src++;
    }
}


void VDP_clearPlan(VDPPlan plan, u16 wait)
{
    switch(plan.value)
    {
        case CONST_PLAN_A:
            VDP_clearTileMap(VDP_PLAN_A, 0, 1 << (planWidthSft + planHeightSft), wait);
            break;

        case CONST_PLAN_B:
            VDP_clearTileMap(VDP_PLAN_B, 0, 1 << (planWidthSft + planHeightSft), wait);
            break;

        case CONST_PLAN_WINDOW:
            VDP_clearTileMap(VDP_PLAN_WINDOW, 0, 1 << (windowWidthSft + 5), wait);
            break;
    }
}

VDPPlan VDP_getTextPlan()
{
    return text_plan;
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
    text_plan = plan;
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

void VDP_drawTextBG(VDPPlan plan, const char *str, u16 x, u16 y)
{
    u32 len;
    u16 data[128];
    char *s;
    u16 *d;
    u16 i;

    // get the horizontal plan size (in cell)
    i = (plan.value == CONST_PLAN_WINDOW)?windowWidth:planWidth;
    len = strlen(str);

    // if string don't fit in plan, we cut it
    if (len > (i - x))
        len = i - x;

    s = (char*) str;
    d = data;
    while(i--)
        *d++ = TILE_FONTINDEX + (*s++ - 32);

    VDP_setTileMapDataRectEx(plan, data, text_basetile, x, y, len, 1, len);
}

void VDP_clearTextBG(VDPPlan plan, u16 x, u16 y, u16 w)
{
    VDP_fillTileMapRect(plan, 0, x, y, w, 1);
}

void VDP_clearTextAreaBG(VDPPlan plan, u16 x, u16 y, u16 w, u16 h)
{
    VDP_fillTileMapRect(plan, 0, x, y, w, h);
}

void VDP_clearTextLineBG(VDPPlan plan, u16 y)
{
    VDP_fillTileMapRect(plan, 0, 0, y, (plan.value == CONST_PLAN_WINDOW)?windowWidth:planWidth, 1);
}

void VDP_drawText(const char *str, u16 x, u16 y)
{
    VDP_drawTextBG(text_plan, str, x, y);
}

void VDP_clearText(u16 x, u16 y, u16 w)
{
    VDP_clearTextBG(text_plan, x, y, w);
}

void VDP_clearTextArea(u16 x, u16 y, u16 w, u16 h)
{
    VDP_clearTextAreaBG(text_plan, x, y, w, h);
}

void VDP_clearTextLine(u16 y)
{
    VDP_clearTextLineBG(text_plan, y);
}


u16 VDP_drawBitmap(VDPPlan plan, const Bitmap *bitmap, u16 x, u16 y)
{
    u16 numTile;
    u16 result;

    numTile = ((bitmap->h + 7) >> 3) * ((bitmap->w + 7) >> 3);
    // not enough tiles to display the image, get back to first user index
    if ((curTileInd + numTile) > TILE_USERMAXINDEX)
        curTileInd = TILE_USERINDEX;

    result = VDP_drawBitmapEx(plan, bitmap, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, curTileInd), x, y, TRUE);

    curTileInd += numTile;

    return result;
}

u16 VDP_drawBitmapEx(VDPPlan plan, const Bitmap *bitmap, u16 basetile, u16 x, u16 y, u16 loadpal)
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

u16 VDP_drawImage(VDPPlan plan, const Image *image, u16 x, u16 y)
{
    u16 numTile;
    u16 result;

    numTile = image->tileset->numTile;
    // not enough tiles to display the image, get back to first user index
    if ((curTileInd + numTile) > TILE_USERMAXINDEX)
        curTileInd = TILE_USERINDEX;

    result = VDP_drawImageEx(plan, image, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, curTileInd), x, y, TRUE, TRUE);

    curTileInd += numTile;

    return result;
}

u16 VDP_drawImageEx(VDPPlan plan, const Image *image, u16 basetile, u16 x, u16 y, u16 loadpal, u16 use_dma)
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
