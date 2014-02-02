#ifndef _VGMMUSIC_H_
#define _VGMMUSIC_H_


extern Plugin vgm;

void outVGM(unsigned char* data, int size, int align, FILE* fs, FILE* fh, char* id, int global);


#endif // _VGMMUSIC_H_
