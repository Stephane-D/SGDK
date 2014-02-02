#ifndef _IMG_TOOLS_H_
#define _IMG_TOOLS_H_


int Img_getInfos(char* fileName, int *w, int *h, int *bpp);
unsigned short *Img_getPalette(char* fileName, int *size);
unsigned char *Img_getData(char* fileName, int *size, int wAlign, int hAlign);

unsigned char *to4bppAndFree(unsigned char* buf8bpp, int size);
unsigned char *to8bppAndFree(unsigned char* buf4bpp, int size);
unsigned char *to4bpp(unsigned char* buf8bpp, int size);
unsigned char *to8bpp(unsigned char* buf4bpp, int size);

unsigned char getMaxIndex(unsigned char* buf8bpp, int size);

#endif // _IMG_TOOLS_H_


