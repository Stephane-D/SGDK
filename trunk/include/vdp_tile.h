#ifndef _VDP_TILE_H_
#define _VDP_TILE_H_


#define TILE_ATTR(pal, pri, flipV, flipH)               (((flipH) << 11) + ((flipV) << 12) + ((pal) << 13) + ((pri) << 15))
#define TILE_ATTR_FULL(pal, pri, flipV, flipH, index)   (((flipH) << 11) + ((flipV) << 12) + ((pal) << 13) + ((pri) << 15) + (index))


// GenRes support
typedef struct
{
    u16 *pal;               // pointer to pal data
    u32 *tiles;             // pointer to tiles data
    u16 width;              // width in tiles
    u16 height;             // height in tiles
    u16 compressedSize;     // 0
} GenResTiles;


void VDP_loadTileData(const u32 *data, u16 index, u16 num, u8 use_dma);
void VDP_loadFont(const u32 *font, u8 use_dma);
void VDP_loadBMPTileData(const u32 *data, u16 index, u16 w, u16 h, u16 bmp_w);
void VDP_loadBMPTileDataEx(const u32 *data, u16 index, u16 x, u16 y, u16 w, u16 h, u16 bmp_w);
void VDP_fillTileData(u8 value, u16 index, u16 num, u8 use_dma);

void VDP_setTileMap(u16 plan, u16 tile, u16 x, u16 y);
void VDP_fillTileMapRect(u16 plan, u16 tile, u16 x, u16 y, u16 w, u16 h);
void VDP_clearTileMapRect(u16 plan, u16 x, u16 y, u16 w, u16 h);
void VDP_fillTileMapRectInc(u16 plan, u16 basetile, u16 x, u16 y, u16 w, u16 h);
void VDP_setTileMapRect(u16 plan, const u16 *data, u16 basetile, u16 x, u16 y, u16 w, u16 h);


#endif // _VDP_TILE_H_
