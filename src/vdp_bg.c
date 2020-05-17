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


static VDPPlane text_plan;
static u16 text_basetile;

// current VRAM upload tile position
u16 curTileInd;


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

void VDP_setVerticalScrollTile(VDPPlane plane, u16 tile, s16* values, u16 len, TransferMethod tm)
{
    u16 addr;

    addr = (tile & 0x1F) * 4;
    if (plane == BG_B) addr += 2;

    DMA_transfer(tm, DMA_VSRAM, values, addr, len, 4);
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

void VDP_drawTextBG(VDPPlane plane, const char *str, u16 x, u16 y)
{
    u16 data[128];
    const u8 *s;
    u16 *d;
    u16 i, w, len;

    // get the horizontal plane size (in cell)
    w = (plane == WINDOW)?windowWidth:planeWidth;
    len = strlen(str);

    // if string don't fit in plane, we cut it
    if (len > (w - x))
        len = w - x;

    s = (const u8*) str;
    d = data;
    i = len;
    while(i--)
        *d++ = TILE_FONTINDEX + (*s++ - 32);

    VDP_setTileMapDataRowEx(plane, data, text_basetile, y, x, len, CPU);
}

void VDP_clearTextBG(VDPPlane plane, u16 x, u16 y, u16 w)
{
    VDP_fillTileMapRect(plane, 0, x, y, w, 1);
}

void VDP_clearTextAreaBG(VDPPlane plane, u16 x, u16 y, u16 w, u16 h)
{
    VDP_fillTileMapRect(plane, 0, x, y, w, h);
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


u16 VDP_drawBitmap(VDPPlane plane, const Bitmap *bitmap, u16 x, u16 y)
{
    u16 numTile;
    u16 result;

    numTile = ((bitmap->h + 7) >> 3) * ((bitmap->w + 7) >> 3);
    // not enough tiles to display the image, get back to first user index
    if ((curTileInd + numTile) > TILE_USERMAXINDEX)
        curTileInd = TILE_USERINDEX;

    result = VDP_drawBitmapEx(plane, bitmap, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, curTileInd), x, y, TRUE);

    curTileInd += numTile;

    return result;
}

u16 VDP_drawBitmapEx(VDPPlane plane, const Bitmap *bitmap, u16 basetile, u16 x, u16 y, u16 loadpal)
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
        VDP_loadBMPTileData((u32*) FAR(bitmap->image), basetile & TILE_INDEX_MASK, wt, ht, wt);

    // tilemap
    VDP_fillTileMapRectInc(plane, basetile, x, y, wt, ht);
    // palette
    if (loadpal) PAL_setPaletteColors((basetile >> 9) & 0x30, palette);

    return TRUE;
}

u16 VDP_drawImage(VDPPlane plane, const Image *image, u16 x, u16 y)
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

u16 VDP_drawImageEx(VDPPlane plane, const Image *image, u16 basetile, u16 x, u16 y, u16 loadpal, bool dma)
{
    if (!VDP_loadTileSet(image->tileset, basetile & TILE_INDEX_MASK, dma?DMA:CPU))
        return FALSE;

    TileMap* tilemap = image->tilemap;

    // no interest in using VDP_setTileMapEx with DMA (DMA_QUEUE is ok)
    if (!VDP_setTileMapEx(plane, tilemap, basetile, x, y, 0, 0, tilemap->w, tilemap->h, CPU))
        return FALSE;

    Palette* palette = image->palette;

    // palette
    if (loadpal) PAL_setPaletteColors((basetile >> 9) & 0x30, palette);

    return TRUE;
}
