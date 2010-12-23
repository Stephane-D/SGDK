#ifndef _BMP_FF_H_
#define _BMP_FF_H_

#include "bmp_cmn.h"

#ifdef ENABLE_BMP


extern u16 *bmp_tilemap_read;
extern u16 *bmp_tilemap_write;


void BMP_FF_init(u16 flags);
void BMP_FF_end();
void BMP_FF_reset();

void BMP_FF_setFlags(u16 value);

void BMP_FF_flip();

void BMP_FF_clear();

void BMP_FF_drawLine(Line *l);


#endif // ENABLE_BMP

#endif // _BMP_FF_H_

