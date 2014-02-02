#ifndef _TFMMUSIC_H_
#define _TFMMUSIC_H_


extern Plugin tfm;

void outTFM(unsigned char* data, int size, int align, FILE* fs, FILE* fh, char* id, int global);


#endif // _TFMMUSIC_H_


