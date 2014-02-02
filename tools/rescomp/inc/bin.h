#ifndef _BIN_H_
#define _BIN_H_


extern Plugin bin;

void outBIN(unsigned char* data, int size, int align, FILE* fs, FILE* fh, char* id, int global);


#endif // _BIN_H_
