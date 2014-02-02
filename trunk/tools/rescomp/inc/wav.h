#ifndef _WAV_H_
#define _WAV_H_


extern Plugin wav;

void outWAV(unsigned char* data, int size, int align, FILE* fs, FILE* fh, char* id, int global);


#endif // _WAV_H_


