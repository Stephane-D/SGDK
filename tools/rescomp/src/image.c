#include <stdio.h>
#include <string.h>

#include "../inc/rescomp.h"
#include "../inc/plugin.h"
#include "../inc/tools.h"
#include "../inc/img_tools.h"
#include "../inc/tile_tools.h"

#include "../inc/image.h"
#include "../inc/palette.h"
#include "../inc/map.h"
#include "../inc/tileset.h"


// forward
static int isSupported(char *type);
static int execute(char *info, FILE *fs, FILE *fh);

// IMAGE resource support
Plugin image = { isSupported, execute };


static int isSupported(char *type)
{
    if (!stricmp(type, "IMAGE")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    int w, h, bpp;
    int wt, ht;
    int size, psize;
    int packed;
    int maxIndex, mapBase;
    int nbElem;
    unsigned char *data;
    unsigned short *palette;
    tileimg_ *result;

    packed = 0;
    mapBase = 0;

    nbElem = sscanf(info, "%s %s \"%[^\"]\" %d %d", temp, id, temp, &packed, &mapBase);

    if (nbElem < 3)
    {
        printf("Wrong IMAGE definition\n");
        printf("IMAGE name \"file\" [packed [mapbase]]\n");
        printf("  name\t\tImage variable name\n");
        printf("  file\tthe image to convert to Image structure (should be a 8bpp .bmp or .png)\n");
        printf("  packed\tset to 1 to pack the Image data (Image), by default 0 is assumed.\n");
        printf("  mapbase\tdefine the base tilemap value, useful to set the priority, default palette and base tile index.\n\n");

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
    if (maxIndex >= 64)
    {
        printf("Error: Image %s use color index >= 64\n", fileIn);
        printf("IMAGE resource require image with a maximum of 64 colors.\n");
        return FALSE;
    }

    // convert to tiled image
    result = getTiledImage(data, wt, ht, TRUE, mapBase);
    if (!result) return FALSE;

    // pack data
    if (packed != PACK_NONE)
    {
        if (!packTileSet(result->tileset, &packed)) return FALSE;
        if (!packMap(result->map, &packed)) return FALSE;
    }

    // get palette
    palette = Img_getPalette(fileIn, &psize);
    if (!palette) return FALSE;

    // optimize palette size
    if (maxIndex < 16) psize = 16;
    else if (maxIndex < 32) psize = 32;
    else if (maxIndex < 48) psize = 48;
    else psize = 64;

    // EXPORT PALETTE
    strcpy(temp, id);
    strcat(temp, "_palette");
    outPalette(palette, 0, psize, fs, fh, temp, FALSE);

    // EXPORT TILEMAP
    strcpy(temp, id);
    strcat(temp, "_tilemap");
    outMap(result->map, fs, fh, temp, FALSE);

    // EXPORT TILESET
    strcpy(temp, id);
    strcat(temp, "_tileset");
    outTileset(result->tileset, fs, fh, temp, FALSE);

    // EXPORT IMAGE
    outImage(fs, fh, id, TRUE);

    return TRUE;
}

void outImage(FILE* fs, FILE* fh, char* id, int global)
{
    // output Image structure
    decl(fs, fh, "Image", id, 2, global);
    // Palette pointer
    fprintf(fs, "    dc.l    %s_palette\n", id);
    // TileSet pointer
    fprintf(fs, "    dc.l    %s_tileset\n", id);
    // Map pointer
    fprintf(fs, "    dc.l    %s_tilemap\n", id);
    fprintf(fs, "\n");
}
