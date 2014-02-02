#include <stdio.h>
#include <string.h>

#include "../inc/rescomp.h"
#include "../inc/plugin.h"
#include "../inc/tools.h"
#include "../inc/img_tools.h"

#include "../inc/bitmap.h"
#include "../inc/palette.h"


// forward
static int isSupported(char *type);
static int execute(char *info, FILE *fs, FILE *fh);

// BITMAP resource support
Plugin bitmap = { isSupported, execute };


static int isSupported(char *type)
{
    if (!stricmp(type, "BITMAP")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    int w, h, bpp;
    int isize, psize;
    int packed;
    int transInd;
    int nbElem;
    unsigned char *data;
    unsigned short *palette;
    unsigned char maxIndex;

    packed = 0;
    transInd = 0;

    nbElem = sscanf(info, "%s %s \"%[^\"]\" %d %d", temp, id, temp, &packed, &transInd);

    if (nbElem < 3)
    {
        printf("Wrong BITMAP definition\n");
        printf("BITMAP name \"file\" [packed]\n");
        printf("  name\t\tBitmap variable name\n");
        printf("  file\tthe image to convert to Bitmap structure (should be a 8bpp .bmp or .png)\n");
        printf("  packed\tset to 1 to pack the Bitmap data, by default 0 is assumed.\n\n");

        return FALSE;
    }

    // adjust input file path
    adjustPath(resDir, temp, fileIn);

    // retrieve basic infos about the image
    if (!Img_getInfos(fileIn, &w, &h, &bpp)) return FALSE;

    // adjust bitmap size on pixel pair
    w = ((w + 1) / 2) * 2;

    // get image data (always 8bpp)
    data = Img_getData(fileIn, &isize, 2, 1);
    if (!data) return FALSE;

    // find max color index
    maxIndex = getMaxIndex(data, isize);
    // not allowed here
    if (maxIndex >= 16)
    {
        printf("Error: Image %s use color index >= 16\n", fileIn);
        printf("BITMAP resource require image with a maximum of 16 colors.\n");
        printf("Use 4bpp image instead if you are unsure.\n");
        return FALSE;
    }

    // convert to 4BPP
    data = to4bppAndFree(data, isize);
    isize /= 2;
    if (!data) return FALSE;

    // pack data
    if (packed)
    {
        data = pack(data, 0, isize, &isize, &packed);
        if (!data) return FALSE;
    }

    // get palette
    palette = Img_getPalette(fileIn, &psize);
    if (!palette) return FALSE;

    // we keep 16 colors max
    psize = MIN(16, psize);

    // EXPORT PALETTE
    strcpy(temp, id);
    strcat(temp, "_palette");
    outPalette(palette, 0, psize, fs, fh, temp, FALSE);

    // EXPORT BITMAP
    outBitmap(data, packed, w, h, isize, fs, fh, id, TRUE);

    return TRUE;
}


void outBitmap(unsigned char* bitmap, int packed, int w, int h, int size, FILE* fs, FILE* fh, char* id, int global)
{
    char temp[MAX_PATH_LEN];

    // bitmap data
    strcpy(temp, id);
    strcat(temp, "_image");
    // declare
    decl(fs, fh, NULL, temp, 2, FALSE);
    // output data
    outS(bitmap, 0, size, fs, 1);
    fprintf(fs, "\n");

    // output Bitmap structure
    decl(fs, fh, "Bitmap", id, 2, global);
    // set compression info
    fprintf(fs, "    dc.w    %d\n", packed);
    // set size in pixel
    fprintf(fs, "    dc.w    %d, %d\n", w, h);
    // set palette pointer
    fprintf(fs, "    dc.l    %s_palette\n", id);
    // set image pointer
    fprintf(fs, "    dc.l    %s\n", temp);
    fprintf(fs, "\n");
}
