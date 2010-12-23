#ifndef _BMP_INTR_H_
#define _BMP_INTR_H_

#ifdef ENABLE_BMP


#define BMP_BASETILEINDEX       TILE_USERINDEX

#define BMP_FB0TILEINDEX        BMP_BASETILEINDEX
#define BMP_FB1TILEINDEX        (BMP_BASETILEINDEX + (BMP_CELLWIDTH * BMP_CELLHEIGHT))

#define BMP_SYSTEMTILE          (BMP_SYSTEMTILEINDEX * 32)
#define BMP_BASETILE            (BMP_BASETILEINDEX * 32)
#define BMP_FB0TILE             (BMP_FB0TILEINDEX * 32)
#define BMP_FB1TILE             (BMP_FB1TILEINDEX * 32)

#define BMP_FB0TILEMAP          BMP_PLAN
#define BMP_FB1TILEMAP          (BMP_PLAN + ((BMP_PLANWIDTH * (BMP_PLANHEIGHT / 2)) * 2))
#define BMP_FBTILEMAPOFFSET     (((BMP_PLANWIDTH * BMP_CELLYOFFSET) + BMP_CELLXOFFSET) * 2)
#define BMP_FB0TILEMAP_ADJ      (BMP_FB0TILEMAP + BMP_FBTILEMAPOFFSET)
#define BMP_FB1TILEMAP_ADJ      (BMP_FB1TILEMAP + BMP_FBTILEMAPOFFSET)

#define BMP_STAT_FLIPWAITING    (1 << 0)
#define BMP_STAT_BLITTING       (1 << 1)

#define HAS_FLAG(f)             (bmp_flags & (f))
#define HAS_FLAGS(f)            ((bmp_flags & (f)) == (f))

#define READ_IS_FB0             (bmp_buffer_read == bmp_buffer_0)
#define READ_IS_FB1             (bmp_buffer_read == bmp_buffer_1)
#define WRITE_IS_FB0            (bmp_buffer_write == bmp_buffer_0)
#define WRITE_IS_FB1            (bmp_buffer_write == bmp_buffer_1)


extern u8 bmp_buffer_0[BMP_WIDTH * BMP_HEIGHT];
extern u8 bmp_buffer_1[BMP_WIDTH * BMP_HEIGHT];

extern u16 (*doBlit)();


void _bmp_init();
void _bmp_setFlags(u16 value);
void _bmp_doFlip();


#endif // ENABLE_BMP

#endif // _BMP_INTR_H_
