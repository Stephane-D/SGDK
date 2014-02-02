#ifndef _BITMAP_H_
#define _BITMAP_H_


extern Plugin bitmap;

void outBitmap(unsigned char* bitmap, int packed, int w, int h, int size, FILE* fs, FILE* fh, char* id, int global);


#endif // _BITMAP_H_
