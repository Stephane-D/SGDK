#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../inc/rescomp.h"
#include "../inc/plugin.h"

#include "../inc/xgmmusic.h"

#include "../inc/tools.h"
#include "../inc/snd_tools.h"


// forward
static int isSupported(char *type);
static int execute(char *info, FILE *fs, FILE *fh);

// XGM resource support
Plugin xgm = { isSupported, execute };


static int isSupported(char *type)
{
    if (!strcasecmp(type, "XGM")) return 1;

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
        printf("Wrong XGM definition\n");
        printf("XGM name file\n");
        printf("  name\t\tXGM music variable name\n");
        printf("  file\tpath of the .vgm or .xgm music file to convert to binary data array\n");

        return FALSE;
    }

    // adjust input file path
    adjustPath(resDir, temp, fileIn);

    strcpy(temp, fileIn);
    removeExtension(temp);
    strcat(temp, ".bin");

    // convert VGM/XGM to bin
    if (!xgmtool(fileIn, temp))
    {
        printf("Error while converting '%s' to BIN format\n", fileIn);
        return FALSE;
    }

    // read data from binary file
    data = in(temp, &size);
    // clean temp file
    remove(temp);

    // error while reading data
    if (!data) return FALSE;

    // EXPORT XGM
    outXGM(data, size, 256, fs, fh, id, TRUE);

    return TRUE;
}

void outXGM(unsigned char* data, int size, int align, FILE* fs, FILE* fh, char* id, int global)
{
    // declare
    declArray(fs, fh, "u8", id, size, align, TRUE);
    // output data
    outS(data, 0, size, fs, 1);
    fprintf(fs, "\n");
}
