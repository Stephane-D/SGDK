#include <stdio.h>
#include <string.h>

#include "../inc/rescomp.h"
#include "../inc/plugin.h"
#include "../inc/tools.h"
#include "../inc/img_tools.h"
#include "../inc/tile_tools.h"

#include "../inc/tileset.h"
#include "../inc/palette.h"

// forward
static int isSupported(char *type);
static int execute(char *info, FILE *fs, FILE *fh);

// TILESET resource support
Plugin tileset = { isSupported, execute };


static int isSupported(char *type)
{
    if (!stricmp(type, "TILESET")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    int w, h, bpp;
    int wt, ht;
    int size;
    int packed;
    int transInd;
    int nbElem;
    unsigned char* data;
    unsigned char maxIndex;
    tileset_* result;

    packed = 0;
    transInd = 0;

    nbElem = sscanf(info, "%s %s \"%[^\"]\" %d %d", temp, id, temp, &packed, &transInd);

    if (nbElem < 3)
    {
        printf("Wrong TILESET definition\n");
        printf("TILESET name \"file\" [packed]\n");
        printf("  name\t\tTileset variable name\n");
        printf("  file\tthe image to convert to TileSet structure (should be a 8bpp .bmp or .png)\n");
        printf("  packed\tset to 1 to pack the TileSet data (TileSet), by default 0 is assumed.\n\n");

        return FALSE;
    }

    // adjust input file path
    adjustPath(resDir, temp, fileIn);

    // retrieve basic infos about the image
    if (!Img_getInfos(fileIn, &w, &h, &bpp)) return FALSE;

    // get size in tile
    wt = (w + 7) / 8;
    ht = (h + 7) / 8;

    // inform about incorrect size
    if ((w & 7) != 0)
    {
        printf("Warning: Image %s width is not a multiple of 8 (%d)\n", fileIn, w);
        printf("Width changed to %d\n", wt * 8);
    }
    if ((h & 7) != 0)
    {
        printf("Warning: Image %s height is not a multiple of 8 (%d)\n", fileIn, h);
        printf("Height changed to %d\n", ht * 8);
    }

    // get image data (always 8bpp)
    data = Img_getData(fileIn, &size, 8, 8);
    if (!data) return FALSE;

    // find max color index
    maxIndex = getMaxIndex(data, size);
    // not allowed here
    if (maxIndex >= 16)
    {
        printf("Error: Image %s use color index >= 16\n", fileIn);
        printf("TILESET resource require image with a maximum of 16 colors.\n");
        printf("Use 4bpp image instead if you are unsure.\n");
        return FALSE;
    }

    // convert to 4BPP
    data = to4bppAndFree(data, size);
    if (!data) return FALSE;

    // convert to tile
    data = bmpToTile(data, 0, wt, ht);
    if (!data) return FALSE;

    // create tileset structure
    result = createTileSet((unsigned int*) data, wt * ht);

    // pack if needed
    if (packed != PACK_NONE)
    {
        if (!packTileSet(result, &packed)) return FALSE;
    }

    // EXPORT TILESET
    outTileset(result, fs, fh, id, TRUE);

    return TRUE;
}


void outTileset(tileset_* tileset, FILE* fs, FILE* fh, char* id, int global)
{
    int size;
    char temp[MAX_PATH_LEN];

    if (tileset->packed != PACK_NONE) size = tileset->packedSize;
    else size = tileset->num * 32;

    // tileset data
    strcpy(temp, id);
    strcat(temp, "_tiles");
    // declare
    decl(fs, fh, NULL, temp, 2, FALSE);
    // output data
    outS((unsigned char*) tileset->tiles, 0, size, fs, 1);
    fprintf(fs, "\n");

    // tileset structure
    decl(fs, fh, "TileSet", id, 2, global);
    // set compression info
    fprintf(fs, "    dc.w    %d\n", tileset->packed);
    // set number of tile
    fprintf(fs, "    dc.w    %d\n", tileset->num);
    // set tileset pointer
    fprintf(fs, "    dc.l    %s\n", temp);
    fprintf(fs, "\n");
}
