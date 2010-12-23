#ifndef _BMP_H_
#define _BMP_H_

#include "bmp_cmn.h"

#ifdef ENABLE_BMP


void BMP_init(u16 flags);
void BMP_end();
void BMP_reset();

void BMP_setFlags(u16 value);

void BMP_flip();

void BMP_clear();

u8*  BMP_getWritePointer(u32 x, u32 y);
u8*  BMP_getReadPointer(u32 x, u32 y);

u8   BMP_getPixel(u32 x, u32 y);
void BMP_setPixel(u32 x, u32 y, u8 col);
void BMP_setPixels_V2D(const Vect2D_u16 *crd, u8 col, u16 num);
void BMP_setPixels(const Pixel *pixels, u16 num);

void BMP_drawLine(Line *l);
void BMP_drawPolygone(const Vect2D_s16 *pts, u16 num, u8 col);

void BMP_loadBitmap(const u8 *data, u32 x, u32 y, u16 w, u16 h, u32 pitch);
void BMP_loadGenBmp16(const u16 *genbmp16, u32 x, u32 y, u16 numpal);
void BMP_loadAndScaleGenBmp16(const u16 *genbmp16, u32 x, u32 y, u16 w, u16 h, u16 numpal);
void BMP_getGenBmp16Palette(const u16 *genbmp16, u16 *pal);

void BMP_scale(const u8 *src_buf, u16 src_w, u16 src_h, u16 src_pitch, u8 *dst_buf, u16 dst_w, u16 dst_h, u16 dst_pitch);


#endif // ENABLE_BMP

#endif // _BMP_H_

