#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../inc/rescomp.h"
#include "../inc/plugin.h"

#include "../inc/bin.h"

#include "../inc/tools.h"


// forward
static int isSupported(char *type);
static int execute(char *info, FILE *fs, FILE *fh);

// BIN resource support
Plugin bin = { isSupported, execute };


static int isSupported(char *type)
{
    if (!stricmp(type, "BIN")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    int size;
    int nbElem;
    int align;
    int salign;
    int fill;
    unsigned char *data;

    align = 2;
    salign = 2;
    fill = 0;
    nbElem = sscanf(info, "%s %s \"%[^\"]\" %d %d %d", temp, id, temp, &align, &salign, &fill);

    if (nbElem < 3)
    {
        printf("Wrong BIN definition\n");
        printf("BIN name file [align [salign [fill]]]\n");
        printf("  name\t\tBIN data variable name\n");
        printf("  file\tpath of the data file to convert to binary data array\n");
        printf("  align\tmemory address alignment for generated data array (default is 2)\n");
        printf("  salign\tsize alignment for the generated data array (default is 2)\n");
        printf("  fill\tfill value for the size alignment (default is 0)\n");

        return FALSE;
    }

    // adjust input file path
    adjustPath(resDir, temp, fileIn);

    strcpy(temp, fileIn);

    // read data from BIN file
    data = in(temp, &size);

    // error while reading data
    if (!data) return FALSE;

    // do size alignement
    data = sizeAlign(data, size, salign, fill, &size);

    // EXPORT BIN
    outBIN(data, size, align, fs, fh, id, TRUE);

    return TRUE;
}


void outBIN(unsigned char* data, int size, int align, FILE* fs, FILE* fh, char* id, int global)
{
    // declare
    declArray(fs, fh, "u8", id, size, align, TRUE);
    // output data
    outS(data, 0, size, fs, 1);
    fprintf(fs, "\n");
}
