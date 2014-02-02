#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../inc/rescomp.h"
#include "../inc/plugin.h"

#include "../inc/tfmmusic.h"

#include "../inc/tools.h"


// forward
static int isSupported(char *type);
static int execute(char *info, FILE *fs, FILE *fh);

// TFM resource support
Plugin tfm = { isSupported, execute };


static int isSupported(char *type)
{
    if (!stricmp(type, "TFM")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    int size;
    int nbElem;
    int z80;
    int converted;
    unsigned char *data;

    converted = 0;
    nbElem = sscanf(info, "%s %s \"%[^\"]\" %d", temp, id, temp, &z80);

    if (nbElem < 3)
    {
        printf("Wrong TFM definition\n");
        printf("TFM name file z80driver\n");
        printf("  name\t\tTFM music variable name\n");
        printf("  file\tpath of the .tfd or .tfc music file to convert to binary data array\n");
        printf("  z80driver\twhen set to 1 the music is aligned in memory so you can use the Z80 driver to play it (but it consumes more ROM).\n");

        return FALSE;
    }

    // adjust input file path
    adjustPath(resDir, temp, fileIn);
    // get input file name in temp
    strcpy(temp, fileIn);

    // need conversion to TFC ?
    if (stricmp(getFileExtension(fileIn), "tfc"))
    {
        // change extension of output file name to .tfc
        removeExtension(temp);
        strcat(temp, ".tfc");

        // do TFM conversion
        if (!tfmcom(fileIn, temp))
        {
            printf("Error while doing TFM conversion on '%s'\n", fileIn);

            return FALSE;
        }

        converted = 1;
    }

    // read data from TFC file
    data = in(temp, &size);

    // error while reading data
    if (!data) return FALSE;

    // clean
    if (converted) remove(temp);

    // EXPORT TFM
    outTFM(data, size, z80?32768:2, fs, fh, id, TRUE);

    return TRUE;
}


void outTFM(unsigned char* data, int size, int align, FILE* fs, FILE* fh, char* id, int global)
{
    // declare
    declArray(fs, fh, "u8", id, size, align, TRUE);
    // output data
    outS(data, 0, size, fs, 1);
    fprintf(fs, "\n");
}
