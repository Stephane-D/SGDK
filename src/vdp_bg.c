#include "config.h"
#include "types.h"

#include "vdp.h"
#include "vdp_bg.h"

#include "tools.h"
#include "string.h"
#include "dma.h"
#include "vdp_pal.h"
#include "vdp_tile.h"

#include "font.h"
#include "memory.h"
#include "mapper.h"
#include "sys.h"


// we don't want to share it
extern vu16 VBlankProcess;


static VDPPlane text_plan;
static u16 text_basetile;

// current VRAM upload tile position
u16 curTileInd;

static s16 hscroll[2];
static s16 vscroll[2];
static u8 hscroll_update = 0;
static u8 vscroll_update = 0;


void VDP_setHorizontalScroll(VDPPlane plane, s16 value)
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = VDP_HSCROLL_TABLE;
    if (plane == BG_B) addr += 2;

    *pl = GFX_WRITE_VRAM_ADDR((u32) addr);
    *pw = value;
}

void VDP_setHorizontalScrollVSync(VDPPlane plane, s16 value)
{
    if (plane == BG_B)
    {
        hscroll[1] = value;
        hscroll_update |= 1 << 1;
    }
    else
    {
        hscroll[0] = value;
        hscroll_update |= 1 << 0;
    }

    // add task for vblank process
    VBlankProcess |= PROCESS_VDP_SCROLL_TASK;
}

void VDP_setHorizontalScrollTile(VDPPlane plane, u16 tile, s16* values, u16 len, TransferMethod tm)
{
    u16 addr;

    addr = VDP_HSCROLL_TABLE + ((tile & 0x1F) * (4 * 8));
    if (plane == BG_B) addr += 2;

    DMA_transfer(tm, DMA_VRAM, values, addr, len, 4 * 8);
}

void VDP_setHorizontalScrollLine(VDPPlane plane, u16 line, s16* values, u16 len, TransferMethod tm)
{
    u16 addr;

    addr = VDP_HSCROLL_TABLE + ((line & 0xFF) * 4);
    if (plane == BG_B) addr += 2;

    DMA_transfer(tm, DMA_VRAM, values, addr, len, 4);
}

void VDP_setVerticalScroll(VDPPlane plane, s16 value)
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = 0;
    if (plane == BG_B) addr += 2;

    *pl = GFX_WRITE_VSRAM_ADDR((u32) addr);
    *pw = value;
}

void VDP_setVerticalScrollVSync(VDPPlane plane, s16 value)
{
    if (plane == BG_B)
    {
        vscroll[1] = value;
        vscroll_update |= 1 << 1;
    }
    else
    {
        vscroll[0] = value;
        vscroll_update |= 1 << 0;
    }

    // add task for vblank process
    VBlankProcess |= PROCESS_VDP_SCROLL_TASK;
}

void VDP_setVerticalScrollTile(VDPPlane plane, u16 tile, s16* values, u16 len, TransferMethod tm)
{
    u16 addr;

    addr = (tile & 0x1F) * 4;
    if (plane == BG_B) addr += 2;

    DMA_transfer(tm, DMA_VSRAM, values, addr, len, 4);
}

bool VDP_doVBlankScrollProcess()
{
    vu16 *pw;
    vu32 *pl;
    u16 addr;

    /* Point to vdp port */
    pw = (u16 *) GFX_DATA_PORT;
    pl = (u32 *) GFX_CTRL_PORT;

    addr = VDP_HSCROLL_TABLE;
    if (hscroll_update & 1)
    {
        *pl = GFX_WRITE_VRAM_ADDR((u32) addr);
        *pw = hscroll[0];
    }
    addr += 2;
    if (hscroll_update & 2)
    {
        *pl = GFX_WRITE_VRAM_ADDR((u32) addr);
        *pw = hscroll[1];
    }

    addr = 0;
    if (vscroll_update & 1)
    {
        *pl = GFX_WRITE_VSRAM_ADDR((u32) addr);
        *pw = vscroll[0];
    }
    addr += 2;
    if (vscroll_update & 2)
    {
        *pl = GFX_WRITE_VSRAM_ADDR((u32) addr);
        *pw = vscroll[1];
    }

    // done
    hscroll_update = 0;
    vscroll_update = 0;

    // no more task
    return FALSE;
}


void VDP_clearPlane(VDPPlane plane, bool wait)
{
    switch(plane)
    {
        case BG_A:
            VDP_clearTileMap(VDP_BG_A, 0, 1 << (planeWidthSft + planeHeightSft), wait);
            break;

        case BG_B:
            VDP_clearTileMap(VDP_BG_B, 0, 1 << (planeWidthSft + planeHeightSft), wait);
            break;

        case WINDOW:
            VDP_clearTileMap(VDP_WINDOW, 0, 1 << (windowWidthSft + 5), wait);
            break;
    }
}

VDPPlane VDP_getTextPlane()
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

void VDP_setTextPlane(VDPPlane plane)
{
    text_plan = plane;
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

void VDP_drawTextEx(VDPPlane plane, const char *str, u16 basetile, u16 x, u16 y, TransferMethod tm)
{
    u16 data[128];
    const u8 *s;
    u16 *d;
    u16 i, pw, ph, len;

    // get the horizontal plane size (in cell)
    pw = (plane == WINDOW)?windowWidth:planeWidth;
    ph = (plane == WINDOW)?32:planeHeight;

    // string outside plane --> exit
    if ((x >= pw) || (y >= ph))
        return;

    // get string len
    len = strlen(str);
    // if string don't fit in plane, we cut it
    if (len > (pw - x))
        len = pw - x;

    // prepare the data
    s = (const u8*) str;
    d = data;
    i = len;
    while(i--)
        *d++ = TILE_FONTINDEX + (*s++ - 32);

    // VDP_setTileMapDataRowEx(..) take care of using temporary buffer to build the data so we are ok here
    VDP_setTileMapDataRowEx(plane, data, basetile, y, x, len, tm);
}

void VDP_clearTextEx(VDPPlane plane, u16 basetile, u16 x, u16 y, u16 w, TransferMethod tm)
{
    u16 data[128];
    u16 pw, ph, len;

    // get the horizontal plane size (in cell)
    pw = (plane == WINDOW)?windowWidth:planeWidth;
    ph = (plane == WINDOW)?32:planeHeight;

    // string outside plane --> exit
    if ((x >= pw) || (y >= ph))
        return;

    // adjust width
    len = w;
    // if don't fit in plane, we cut it
    if (len > (pw - x))
        len = pw - x;

    // prepare the data
    memsetU16(data, 0, len);

    // VDP_setTileMapDataRowEx(..) take care of using temporary buffer to build the data so we are ok here
    VDP_setTileMapDataRowEx(plane, data, basetile, y, x, len, tm);
}

void VDP_clearTextAreaEx(VDPPlane plane, u16 basetile, u16 x, u16 y, u16 w, u16 h, TransferMethod tm)
{
    u16 data[128];
    u16 i, ya, len;
    u16 pw, ph;
    u16 wa, ha;

    // get the horizontal plane size (in cell)
    pw = (plane == WINDOW)?windowWidth:planeWidth;
    ph = (plane == WINDOW)?32:planeHeight;

    // string outside plane --> exit
    if ((x >= pw) || (y >= ph))
        return;

    // adjust width
    wa = w;
    // if don't fit in plane, we cut it
    if (wa > (pw - x))
        wa = pw - x;
    // adjust height
    ha = h;
    // if don't fit in plane, we cut it
    if (ha > (ph - y))
        ha = ph - y;

    // prepare the data
    memsetU16(data, 0, wa);

    ya = y;
    i = ha;
    while(i--)
        // VDP_setTileMapDataRowEx(..) take care of using temporary buffer to build the data so we are ok here
        VDP_setTileMapDataRowEx(plane, data, basetile, ya++, x, len, tm);
}

void VDP_drawTextBG(VDPPlane plane, const char *str, u16 x, u16 y)
{
    VDP_drawTextEx(plane, str, text_basetile, x, y, CPU);
}

void VDP_clearTextBG(VDPPlane plane, u16 x, u16 y, u16 w)
{
    u16 pw, ph;
    u16 wa;

    // get the horizontal plane size (in cell)
    pw = (plane == WINDOW)?windowWidth:planeWidth;
    ph = (plane == WINDOW)?32:planeHeight;

    // string outside plane --> exit
    if ((x >= pw) || (y >= ph))
        return;

    // adjust dim
    wa = w;

    // if don't fit in plane, we cut it
    if (wa > (pw - x))
        wa = pw - x;

    VDP_fillTileMapRect(plane, 0, x, y, wa, 1);
}

void VDP_clearTextAreaBG(VDPPlane plane, u16 x, u16 y, u16 w, u16 h)
{
    u16 pw, ph;
    u16 wa, ha;

    // get the horizontal plane size (in cell)
    pw = (plane == WINDOW)?windowWidth:planeWidth;
    ph = (plane == WINDOW)?32:planeHeight;

    // string outside plane --> exit
    if ((x >= pw) || (y >= ph))
        return;

    // adjust dim
    wa = w;
    ha = h;

    // if don't fit in plane, we cut it
    if (wa > (pw - x))
        wa = pw - x;
    if (ha > (ph - y))
        ha = ph - y;

    VDP_fillTileMapRect(plane, 0, x, y, wa, ha);
}

void VDP_clearTextLineBG(VDPPlane plane, u16 y)
{
    VDP_fillTileMapRect(plane, 0, 0, y, (plane == WINDOW)?windowWidth:planeWidth, 1);
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


bool VDP_drawBitmap(VDPPlane plane, const Bitmap *bitmap, u16 x, u16 y)
{
    u16 numTile;
    u16 result;

    numTile = mulu((bitmap->h + 7) >> 3, (bitmap->w + 7) >> 3);
    // not enough tiles to display the image, get back to first user index
    if ((curTileInd + numTile) > TILE_USERMAXINDEX)
        curTileInd = TILE_USERINDEX;

    result = VDP_drawBitmapEx(plane, bitmap, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, curTileInd), x, y, TRUE);

    curTileInd += numTile;

    return result;
}

bool VDP_drawBitmapEx(VDPPlane plane, const Bitmap *bitmap, u16 basetile, u16 x, u16 y, bool loadpal)
{
    const u16 wt = bitmap->w / 8;
    const u16 ht = bitmap->h / 8;
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
        VDP_loadBMPTileData((u32*) FAR_SAFE(bitmap->image, mulu(wt, ht) * 32), basetile & TILE_INDEX_MASK, wt, ht, wt);

    // tilemap
    VDP_fillTileMapRectInc(plane, basetile, x, y, wt, ht);
    // palette
    if (loadpal) PAL_setPaletteColors((basetile >> 9) & 0x30, palette, CPU);

    return TRUE;
}

bool VDP_drawImage(VDPPlane plane, const Image *image, u16 x, u16 y)
{
    u16 numTile;
    u16 result;

    numTile = image->tileset->numTile;
    // not enough tiles to display the image, get back to first user index
    if ((curTileInd + numTile) > TILE_USERMAXINDEX)
        curTileInd = TILE_USERINDEX;

    result = VDP_drawImageEx(plane, image, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, curTileInd), x, y, TRUE, DMA);

    curTileInd += numTile;

    return result;
}

bool VDP_drawImageEx(VDPPlane plane, const Image *image, u16 basetile, u16 x, u16 y, bool loadpal, bool dma)
{
    if (!VDP_loadTileSet(image->tileset, basetile & TILE_INDEX_MASK, dma?DMA:CPU))
        return FALSE;

    TileMap* tilemap = image->tilemap;

    // no interest in using VDP_setTileMapEx with DMA (DMA_QUEUE is ok)
    if (!VDP_setTileMapEx(plane, tilemap, basetile, x, y, 0, 0, tilemap->w, tilemap->h, CPU))
        return FALSE;

    Palette* palette = image->palette;

    // palette
    if (loadpal) PAL_setPaletteColors((basetile >> 9) & 0x30, palette, CPU);

    return TRUE;
}
