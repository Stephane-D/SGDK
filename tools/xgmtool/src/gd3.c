#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/gd3.h"
#include "../inc/xgmtool.h"
#include "../inc/util.h"


const char* XGMTOOL_PRINT = "Optimized with XGMTool";


static int getWideStringSize(void* in);
static short* getWideStringFromString(char* str);
static short* getWideString(unsigned char* in, int *outSize);
static char* getString(short* wstr);


GD3* GD3_create()
{
    GD3* result;

    result = malloc(sizeof(GD3));
    memset(result, 0, sizeof(GD3));

    result->version = 0x100;

    return result;
}

XD3* XD3_create()
{
    XD3* result;

    result = malloc(sizeof(XD3));
    memset(result, 0, sizeof(XD3));

    return result;
}

GD3* GD3_createFromData(unsigned char* data)
{
    GD3* result = GD3_create();
    int offset = 0;
    int size;

    if (!silent)
        printf("Parsing GD3...\n");

    if (strncasecmp(data + offset, "Gd3 ", 4))
    {
        printf("Error: GD3 header not recognized !\n");
        return NULL;
    }
    offset += 4;

    // get version info
    result->version = getInt(data, 4);
    offset += 4;

    // get total size for GD3 data (we can ignore this field)
    size = getInt(data, 8);
    offset += 4;

    result->trackName_EN = getWideString(data + offset, &size);
    offset += size * 2;
    result->trackName_JP = getWideString(data + offset, &size);
    offset += size * 2;
    result->gameName_EN = getWideString(data + offset, &size);
    offset += size * 2;
    result->gameName_JP = getWideString(data + offset, &size);
    offset += size * 2;
    result->systemName_EN = getWideString(data + offset, &size);
    offset += size * 2;
    result->systemName_JP = getWideString(data + offset, &size);
    offset += size * 2;
    result->authorName_EN = getWideString(data + offset, &size);
    offset += size * 2;
    result->authorName_JP = getWideString(data + offset, &size);
    offset += size * 2;
    result->date = getWideString(data + offset, &size);
    offset += size * 2;
    result->vgmConversionAuthor = getWideString(data + offset, &size);
    offset += size * 2;
    result->notes = getWideString(data + offset, &size);

    // convert to classic string
    char* str = getString(result->vgmConversionAuthor);
    // not ending with our signature ?
    if ((strlen(str) < strlen(XGMTOOL_PRINT)) || strncmp(str + strlen(str) - strlen(XGMTOOL_PRINT), XGMTOOL_PRINT, strlen(XGMTOOL_PRINT)))
    {
        // realloc string
        str = realloc(str, strlen(str) + 4 + strlen(XGMTOOL_PRINT));
        // print XGMTool signature
        strcat(str, " - ");
        strcat(str, XGMTOOL_PRINT);
    }
    // convert back to wide string
    result->vgmConversionAuthor = getWideStringFromString(str);

    return result;
}

XD3* XD3_createFromGD3(GD3 *gd3, int duration, int loopDuration)
{
    XD3* result = XD3_create();

    if (!silent)
        printf("Converting GD3 to XD3...\n");

    result->trackName = getString(gd3->trackName_EN);
    result->gameName = getString(gd3->gameName_EN);
    result->authorName = getString(gd3->authorName_EN);
    result->date = getString(gd3->date);
    result->conversionAuthor = getString(gd3->vgmConversionAuthor);
    result->notes = getString(gd3->notes);
    result->duration = duration;
    result->loopDuration = loopDuration;

    return result;
}

int GD3_computeDataSize(GD3* gd3)
{
    return (getWideStringSize(gd3->trackName_EN) * 2) +
        (getWideStringSize(gd3->trackName_JP) * 2) +
        (getWideStringSize(gd3->gameName_EN) * 2) +
        (getWideStringSize(gd3->gameName_JP) * 2) +
        (getWideStringSize(gd3->systemName_EN) * 2) +
        (getWideStringSize(gd3->systemName_JP) * 2) +
        (getWideStringSize(gd3->authorName_EN) * 2) +
        (getWideStringSize(gd3->authorName_JP) * 2) +
        (getWideStringSize(gd3->date) * 2) +
        (getWideStringSize(gd3->vgmConversionAuthor) * 2) +
        (getWideStringSize(gd3->notes) * 2) + (11 * 2);
}

int XD3_computeDataSize(XD3* xd3)
{
    return strlen(xd3->trackName) +
        strlen(xd3->gameName) +
        strlen(xd3->authorName) +
        strlen(xd3->date) +
        strlen(xd3->conversionAuthor) +
        strlen(xd3->notes) + (6 * 1) + 8;   // +8 for durations informations
}


unsigned char* GD3_asByteArray(GD3* gd3, int *outSize)
{
    int size = GD3_computeDataSize(gd3);
    unsigned char* result = malloc(size + 12);
    int offset = 0;

    // header
    strcpy(result, "Gd3 ");
    offset += 4;
    // version
    setInt(result, offset, gd3->version);
    offset += 4;
    // size
    setInt(result, offset, size);
    offset += 4;

    // set out size
    *outSize = size + 12;

    // fields
    size = (getWideStringSize(gd3->trackName_EN) * 2) + 2;
    memcpy(result + offset, gd3->trackName_EN, size);
    offset += size;
    size = (getWideStringSize(gd3->trackName_JP) * 2) + 2;
    memcpy(result + offset, gd3->trackName_JP, size);
    offset += size;
    size = (getWideStringSize(gd3->gameName_EN) * 2) + 2;
    memcpy(result + offset, gd3->gameName_EN, size);
    offset += size;
    size = (getWideStringSize(gd3->gameName_JP) * 2) + 2;
    memcpy(result + offset, gd3->gameName_JP, size);
    offset += size;
    size = (getWideStringSize(gd3->systemName_EN) * 2) + 2;
    memcpy(result + offset, gd3->systemName_EN, size);
    offset += size;
    size = (getWideStringSize(gd3->systemName_JP) * 2) + 2;
    memcpy(result + offset, gd3->systemName_JP, size);
    offset += size;
    size = (getWideStringSize(gd3->authorName_EN) * 2) + 2;
    memcpy(result + offset, gd3->authorName_EN, size);
    offset += size;
    size = (getWideStringSize(gd3->authorName_JP) * 2) + 2;
    memcpy(result + offset, gd3->authorName_JP, size);
    offset += size;
    size = (getWideStringSize(gd3->date) * 2) + 2;
    memcpy(result + offset, gd3->date, size);
    offset += size;
    size = (getWideStringSize(gd3->vgmConversionAuthor) * 2) + 2;
    memcpy(result + offset, gd3->vgmConversionAuthor, size);
    offset += size;
    size = (getWideStringSize(gd3->notes) * 2) + 2;
    memcpy(result + offset, gd3->notes, size);

    return result;
}

unsigned char* XD3_asByteArray(XD3* xd3, int *outSize)
{
    // align size on 2 bytes
    int size = (XD3_computeDataSize(xd3) + 1) & 0xFFFFFFFE;
    unsigned char* result = malloc(size + 4);
    int offset = 0;

    // size of XD3 infos
    setInt(result, offset, size);
    offset += 4;

    // set out size
    *outSize = size + 4;

    // fields
    size = strlen(xd3->trackName) + 1;
    memcpy(result + offset, xd3->trackName, size);
    offset += size;
    size = strlen(xd3->gameName) + 1;
    memcpy(result + offset, xd3->gameName, size);
    offset += size;
    size = strlen(xd3->authorName) + 1;
    memcpy(result + offset, xd3->authorName, size);
    offset += size;
    size = strlen(xd3->date) + 1;
    memcpy(result + offset, xd3->date, size);
    offset += size;
    size = strlen(xd3->conversionAuthor) + 1;
    memcpy(result + offset, xd3->conversionAuthor, size);
    offset += size;
    size = strlen(xd3->notes) + 1;
    memcpy(result + offset, xd3->notes, size);
    offset += size;
    setInt(result, offset + 0, xd3->duration);
    setInt(result, offset + 4, xd3->loopDuration);

    return result;
}


static int getWideStringSize(void* in)
{
    short* src = (short*) in;
    int result = 0;

    while(*src++) result++;

    return result;
}


static short* getWideStringFromString(char* str)
{
    int size = strlen(str) + 1;
    short* result = malloc(size * sizeof(short));
    int i;

    // set to 0 first
    memset(result, 0, size * sizeof(short));
    // then convert
    for(i = 0; i < size; i++)
        result[i] = str[i];

    return result;
}

static short* getWideString(unsigned char* in, int *outSize)
{
    int size = getWideStringSize(in) + 1;
    short* result = malloc(size * sizeof(short));

    memcpy(result, in, size * sizeof(short));

    *outSize = size;

    return result;
}

static char* getString(short* wstr)
{
    int size = getWideStringSize(wstr) + 1;
    char* result = malloc(size * sizeof(char));
    int i;

    // convert to byte string
    for(i = 0; i < size; i++)
        result[i] = wstr[i];

    return result;
}
