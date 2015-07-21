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
    char opt[256];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    int size;
    int nbElem;
    int timing;
    unsigned char *data;

    timing = -1;
    strcpy(opt, "");
    nbElem = sscanf(info, "%s %s \"%[^\"]\" %d \"%[^\"]\"", temp, id, temp, &timing, opt);

    if (nbElem < 3)
    {
        printf("Wrong XGM definition\n");
        printf("XGM name file [timing [options]]\n");
        printf("  name\t\tXGM music variable name\n");
        printf("  file\tpath of the .vgm or .xgm music file to convert to binary data array\n");
        printf("  timing\tdefine the XGM base timing\n");
        printf("      \t -1 (default) = AUTO (NTSC or PAL depending the information in source VGM file)\n");
        printf("      \t  0 = NTSC (XGM is generated for NTSC system)\n");
        printf("      \t  1 = PAL (XGM is generated for PAL system)\n");
        printf("  options\toptionals parameters for xgmtool\n");
        printf("      \t ex: \"-dr -di\" to disable some sample auto process (see xgmtool to get more info)\n");

        return FALSE;
    }

    // adjust input file path
    adjustPath(resDir, temp, fileIn);

    strcpy(temp, fileIn);
    removeExtension(temp);
    strcat(temp, ".bin");

    // convert VGM/XGM to bin
    if (!xgmtool(fileIn, temp, timing, opt))
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
