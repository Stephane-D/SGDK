#ifndef _BMP_FF_H_
#define _BMP_FF_H_

#include "bmp_cmn.h"


extern u16 *bmp_tilemap_read;
extern u16 *bmp_tilemap_write;


void BMP_FF_init(u16 flags);
void BMP_FF_end();
void BMP_FF_reset();

void BMP_FF_setFlags(u16 value);

void BMP_FF_enableWaitVSync();
void BMP_FF_disableWaitVSync();
void BMP_FF_enableASyncFlip();
void BMP_FF_disableASyncFlip();
void BMP_FF_enableFPSDisplay();
void BMP_FF_disableFPSDisplay();
void BMP_FF_enableBlitOnBlank();
void BMP_FF_disableBlitOnBlank();
void BMP_FF_enableExtendedBlank();
void BMP_FF_disableExtendedBlank();

void BMP_FF_flip();
void BMP_FF_internalBufferFlip();

void BMP_FF_clear();

u8   BMP_FF_getPixel(u16 x, u16 y);
void BMP_FF_setPixel(u16 x, u16 y, u8 col);
void BMP_FF_setPixels_V2D(const Vect2D_u16 *crd, u8 col, u16 num);
void BMP_FF_setPixels(const Pixel *pixels, u16 num);

void BMP_FF_drawLine(Line *l);


#endif // _BMP_FF_H_

