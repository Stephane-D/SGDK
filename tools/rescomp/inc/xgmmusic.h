#ifndef XGMMUSIC_H_
#define XGMMUSIC_H_


extern Plugin xgm;

void outXGM(unsigned char* data, int size, int align, FILE* fs, FILE* fh, char* id, int global);


#endif // XGM_H_
