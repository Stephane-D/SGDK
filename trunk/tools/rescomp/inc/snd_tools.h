#ifndef _SND_TOOLS_H_
#define _SND_TOOLS_H_


int dpcmPack(char* fin, char* fout);
int wavToRaw(char* fin, char* fout);
int wavToRawEx(char* fin, char* fout, int outRate);


#endif // _SND_TOOLS_H_



