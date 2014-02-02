#ifndef _PCM_H_
#define _PCM_H_


extern Plugin pcm;

void outPCM(unsigned char* data, int size, int align, FILE* fs, FILE* fh, char* id, int global);


#endif // _PCM_H_


