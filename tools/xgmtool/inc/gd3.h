#ifndef GD3_H
#define GD3_H


//"Sega Mega Drive / Genesis"

typedef struct
{
    int version;

    short *trackName_EN;
    short *trackName_JP;
    short *gameName_EN;
    short *gameName_JP;
    short *systemName_EN;
    short *systemName_JP;
    short *authorName_EN;
    short *authorName_JP;
    short *date;
    short *vgmConversionAuthor;
    short *notes;
} GD3;

typedef struct
{
    char *trackName;
    char *gameName;
    char *authorName;
    char *date;
    char *conversionAuthor;
    char *notes;
    int duration;           // duration in frame
    int loopDuration;       // duration in frame (0 if no loop)
} XD3;


GD3* GD3_create();
XD3* XD3_create();
GD3* GD3_createFromData(unsigned char* data);
XD3* XD3_createFromGD3(GD3 *gd3, int duration, int loopDuration);

int GD3_computeDataSize(GD3* gd3);
int XD3_computeDataSize(XD3* xd3);

unsigned char* GD3_asByteArray(GD3* gd3, int *outSize);
unsigned char* XD3_asByteArray(XD3* xd3, int *outSize);


#endif // GD3_H

