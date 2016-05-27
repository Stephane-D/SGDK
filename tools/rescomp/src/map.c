#include <stdio.h>
#include <string.h>

#include "../inc/rescomp.h"
#include "../inc/plugin.h"
#include "../inc/tools.h"
#include "../inc/img_tools.h"
#include "../inc/tile_tools.h"


// forward
static int isSupported(char *type);
static int execute(char *info, FILE *fs, FILE *fh);

// MAP resource support
Plugin map = { isSupported, execute };


static int isSupported(char *type)
{
    if (!strcasecmp(type, "MAP")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    char packedStr[256];
    int w, h;
    int packed;
    int transInd;
    int nbElem;
    tilemap_ *result;

    packed = 0;
    transInd = 0;

    nbElem = sscanf(info, "%s %s \"%[^\"]\" %d %d %s", temp, id, temp, &w, &h, packedStr);

    if (nbElem < 5)
    {
        printf("Wrong MAP definition\n");
        printf("MAP name \"file\" width height [packed]\n");
        printf("  name      Map variable name\n");
        printf("  file      the map file to convert to Map structure (.map Mappy file)\n");
        printf("  width     the map width\n");
        printf("  height    the map height\n");
        printf("  packed    compression type, accepted values:\n");
        printf("              -1 / BEST / AUTO = use best compression\n");
        printf("               0 / NONE        = no compression\n");
        printf("               1 / APLIB       = aplib library (good compression ratio but slow)\n");
        printf("               2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)\n");

        return FALSE;
    }

    // adjust input file path
    adjustPath(resDir, temp, fileIn);
    // get packed value
    packed = getCompression(packedStr);

    printf("MAP resource not yet supported !\n");



    return FALSE;
}


void outMap(tilemap_* map, FILE* fs, FILE* fh, char* id, int global)
{
    int size;
    char temp[MAX_PATH_LEN];

    if (map->packed != PACK_NONE) size = map->packedSize;
    else size = map->w * map->h * 2;

    // map data
    strcpy(temp, id);
    strcat(temp, "_map");
    // declare
    decl(fs, fh, NULL, temp, 2, FALSE);
    // output data
    if (size == (map->w * map->h * 2)) outS((unsigned char*) map->data, 0, size, fs, 2);
    else outS((unsigned char*) map->data, 0, size, fs, 1);
    fprintf(fs, "\n");

    // map structure
    decl(fs, fh, "Map", id, 2, global);
    // compression
    fprintf(fs, "    dc.w    %d\n", map->packed);
    // size
    fprintf(fs, "    dc.w    %d, %d\n", map->w, map->h);
    // map data pointer
    fprintf(fs, "    dc.l    %s\n", temp);
    fprintf(fs, "\n");
}
