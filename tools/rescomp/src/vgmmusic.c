#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../inc/rescomp.h"
#include "../inc/plugin.h"

#include "../inc/vgmmusic.h"

#include "../inc/tools.h"


// forward
static int isSupported(char *type);
static int execute(char *info, FILE *fs, FILE *fh);

// VGM resource support
Plugin vgm = { isSupported, execute };


static int isSupported(char *type)
{
    if (!stricmp(type, "VGM")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    int size;
    int nbElem;
    unsigned char *data;

    nbElem = sscanf(info, "%s %s \"%[^\"]\"", temp, id, temp);

    if (nbElem < 3)
    {
        printf("Wrong VGM definition\n");
        printf("VGM name file\n");
        printf("  name\t\tVGM music variable name\n");
        printf("  file\tpath of the .vgm music file to convert to binary data array\n");

        return FALSE;
    }

    // adjust input file path
    adjustPath(resDir, temp, fileIn);

    strcpy(temp, fileIn);
//    removeExtension(temp);
//    strcat(temp, ".tfc");
//
//    // do TFM conversion
//    if (!tfmcom(fileIn, temp))
//    {
//        printf("Error while doing VGM conversion on '%s'\n", fileIn);
//
//        return FALSE;
//    }

    // read data from VGM file
    data = in(temp, &size);

    // error while reading data
    if (!data) return FALSE;

    // EXPORT VGM
    outVGM(data, size, 2, fs, fh, id, TRUE);

    return TRUE;
}


void outVGM(unsigned char* data, int size, int align, FILE* fs, FILE* fh, char* id, int global)
{
    // declare
    declArray(fs, fh, "u8", id, size, align, TRUE);
    // output data
    outS(data, 0, size, fs, 1);
    fprintf(fs, "\n");
}
