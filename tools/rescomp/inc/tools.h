#ifndef _TOOLS_H_
#define _TOOLS_H_


#define FALSE           0
#define TRUE            1

#define DRIVER_PCM      0
#define DRIVER_2ADPCM   1
#define DRIVER_4PCM     2
#define DRIVER_VGM      3
#define DRIVER_XGM      4

#define PACK_AUTO       -1
#define PACK_NONE       0
#define PACK_APLIB      1
#define PACK_LZ4W       2

#define PACK_MAX_IND    PACK_LZ4W


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


typedef struct
{
	int x;
	int y;
	int w;
	int h;
} box_;

typedef struct
{
	int x;
	int y;
	int ray;
} circle_;


unsigned int swapNibble32(unsigned int value);
unsigned short swapNibble16(unsigned short value);
unsigned char swapNibble8(unsigned char value);

unsigned short toVDPColor(unsigned char b, unsigned char g, unsigned char r);

char* strupper(char* text);
void strreplace(char* text, char chr, char repl_chr);

int isAbsolutePathSystem(char *path);
int isAbsolutePath(char *path);
char* getDirectorySystem(char* path);
char* getDirectory(char* path);
char* getFilenameSystem(char* path);
char* getFilename(char* path);
char* getFileExtension(char* path);
void removeExtension(char* path);
void adjustPathSystem(char *dir, char* path, char* dst);
void adjustPath(char *dir, char* path, char* dst);
unsigned int getFileSize(char* file);
unsigned char* readFile(char *fileName, int *size);
int writeFile(char* filename, unsigned char* data, int size);

void unsign8b(unsigned char* data, int size);
unsigned char* sizeAlign(unsigned char* data, int size, int align, unsigned char fill, int *outSize);

unsigned char* in(char* in, int* outSize);
unsigned char* inEx(FILE* fin, int inOffset, int size, int* outSize);
int inEx2(FILE* fin, int inOffset, int size, unsigned char* dest, int outOffset);

int out(unsigned char* data, int inOffset, int size, int intSize, int swap, char* out);
int outEx(unsigned char* data, int inOffset, int size, int intSize, int swap, FILE* fout, int outOffset);

void decl(FILE* fs, FILE* fh, char* type, char* name, int align, int global);
void declArray(FILE* fs, FILE* fh, char* type, char* name, int size, int align, int global);
void outS(unsigned char* data, int inOffset, int size, FILE* fout, int intSize);

int getDriver(char *str);
int getCompression(char *str);

unsigned char *pack(unsigned char* data, int inOffset, int size, int *outSize, int *method);
unsigned char *packEx(unsigned char* data, int inOffset, int size, int intSize, int *outSize, int *method);

int maccer(char* fin, char* fout);
int tfmcom(char* fin, char* fout);


#endif // _TOOLS_H_
