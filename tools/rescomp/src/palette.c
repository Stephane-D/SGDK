#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../inc/rescomp.h"
#include "../inc/plugin.h"
#include "../inc/tools.h"
#include "../inc/img_tools.h"

#include "../inc/palette.h"


// forward
static int isSupported(char *type);
static int execute(char *info, FILE *fs, FILE *fh);

// PALETTE resource support
Plugin palette = { isSupported, execute };


static int isSupported(char *type)
{
    if (!stricmp(type, "PALETTE")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    int ind, size;
    int w, h, bpp;
    int nbElem;
    short *palette;

    ind = 0;
    nbElem = sscanf(info, "%s %s \"%[^\"]\" %d", temp, id, temp, &ind);

    if (nbElem < 3)
    {
        printf("Wrong PALETTE definition\n");
        printf("PALETTE name file [index]\n");
        printf("  name\t\tPalette variable name\n");
        printf("  file\tpath of the .pal or image file to convert to Palette structure (should be a 8bpp .bmp or .png)\n");
        printf("  index\tstart index where to copy the palette in CRAM\n");

        return FALSE;
    }

    // adjust input file path
    adjustPath(resDir, temp, fileIn);

    // PAL file ?
    if (!stricmp(getFileExtension(fileIn), "pal"))
    {
        // get palette data
        palette = (unsigned short*) readFile(fileIn, &size);
        size /= 2;
    }
    else
    {
        // retrieve basic infos about the image
        if (!Img_getInfos(fileIn, &w, &h, &bpp)) return FALSE;

        // retrieve palette data
        palette = Img_getPalette(fileIn, &size);

        // special case where we use the image as palette
        if (h == 1)
        {
            int i, imgSize;
            unsigned short *tmpPalette;
            unsigned char *data;

            tmpPalette = malloc(size * 2);
            data = Img_getData(fileIn, &imgSize, 1, 1);

            // set temp palette from pixel value
            for(i = 0; i < w; i++)
                tmpPalette[i] = palette[data[i]];

            // then store in palette
            memcpy(palette, tmpPalette, size * 2);
        }
    }

    // EXPORT PALETTE
    outPalette(palette, 0, size, fs, fh, id, TRUE);

    return TRUE;
}


void outPalette(unsigned short* palette, int startInd, int palSize, FILE* fs, FILE* fh, char* id, int global)
{
    char temp[MAX_PATH_LEN];
    int size;

    // we can't have more than 64 colors max
    size = MIN(64 - startInd, palSize);

    // palette data
    strcpy(temp, id);
    strcat(temp, "_pal");
    // declare
    decl(fs, fh, NULL, temp, 2, FALSE);
    // output data
    outS((unsigned char*) palette, 0, size * 2, fs, 2);
    fprintf(fs, "\n");

    // palette structure
    decl(fs, fh, "Palette", id, 2, global);
    // first color index and palette size
    fprintf(fs, "    dc.w    %d, %d\n", startInd, size);
    // set palette pointer
    fprintf(fs, "    dc.l    %s\n", temp);
    fprintf(fs, "\n");
}

