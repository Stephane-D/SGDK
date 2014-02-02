#ifndef _PALETTE_H_
#define _PALETTE_H_


extern Plugin palette;

void outPalette(unsigned short* palette, int startInd, int palSize, FILE* fs, FILE* fh, char* id, int global);


#endif // _PALETTE_H_


