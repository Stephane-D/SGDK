#ifndef _TAB_BMP_H_
#define _TAB_BMP_H_


#if (BMP_TABLES != 0)

extern const u16 offset2tile[BMP_WIDTH * BMP_HEIGHT];
extern const u16 tile2offset[BMP_CELLWIDTH * BMP_CELLHEIGHT];

#else

// used to fake asm sources
extern const u16 offset2tile[1];
extern const u16 tile2offset[1];

#endif


#endif // _TAB_BMP_H_

