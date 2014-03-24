#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

#include "../inc/rescomp.h"
#include "../inc/tools.h"
#include "../inc/plugin.h"

// add your plugin include here
#include "../inc/palette.h"
#include "../inc/bitmap.h"
#include "../inc/tileset.h"
#include "../inc/map.h"
#include "../inc/image.h"
#include "../inc/sprite.h"
#include "../inc/tfmmusic.h"
#include "../inc/vgmmusic.h"
#include "../inc/pcm.h"
#include "../inc/wav.h"
#include "../inc/bin.h"


// forward
static int doConvert(char *dirName, char *fileNameOut);
static int doComp(char *fileName, char *fileNameOut, int header);
static int execute(char *info, FILE *fs, FILE *fh);


// shared directory informations
char *currentDirSystem;
char *currentDir;
char *resDirSystem;
char *resDir;

// add your plugin declaration here
Plugin *plugins[] =
{
    &palette,
    &bitmap,
    &tileset,
    &map,
    &image,
    &sprite,
    &tfm,
    &vgm,
    &pcm,
    &wav,
    &bin,
    NULL,
};


int main(int argc, char **argv)
{
    char fileName[MAX_PATH_LEN];
    char fileNameOut[MAX_PATH_LEN];
    int header;
    int convert;
    int ii;

    // default
    header = 1;
    convert = 0;
    fileName[0] = 0;
    fileNameOut[0] = 0;

    // save rescomp directory
    currentDirSystem = getDirectorySystem(argv[0]);
    currentDir = getDirectory(argv[0]);

    // parse parmeters
    for (ii = 1; ii < argc; ii++)
    {
        char *arg = argv[ii];

        if (!strcmp(arg, "-convert")) convert = 1;
        else if (!strcmp(arg, "-noheader")) header = 0;
        else if (!fileName[0]) strcpy(fileName, arg);
        else if (!fileNameOut[0]) strcpy(fileNameOut, arg);
    }

//    strcpy(fileName, "resources.res");

    printf("rescomp v1.12\n");

    if (!fileName[0])
    {
        printf("Error: missing the input file.\n");
        printf("\n");
        printf("Usage 1 - compile resource:\n");
        printf("  rescomp input [output] [-noheader]\n");
        printf("    input: the input resource file (.res)\n");
        printf("    output: the asm output filename (same name is used for the include file)\n");
        printf("    -noheader: specify that we don't want to generate the header file (.h)\n");
        printf("  Ex: rescomp resources.res outres.s\n");
        printf("\n");
        printf("Usage 2 - Scan specified folder and convert old resources to .res format:\n");
        printf("  rescomp -convert input [output]\n");
        printf("    input: input folder to scan for old resource files\n");
        printf("    output: filename of the generated .res file (default is converted.res)\n");
        printf("Ex: rescomp -convert res resources.res\n");

        return 1;
    }

    if (convert) return doConvert(fileName, fileNameOut);
    else return doComp(fileName, fileNameOut, header);
}

static int doConvert(char *dirName, char *fileNameOut)
{
    char *ext;
    DIR  *dirInput;
    struct dirent *dirEntry;
    FILE *fileOutput;

    if (!fileNameOut[0])
        strcpy(fileNameOut, "converted.res");
//        sprintf(fileNameOut, "%s/%s", dirName, "converted.res");

    dirInput = opendir(dirName);

    if (!dirInput)
    {
        printf("Couldn't open input directory %s\n", dirName);
        return 1;
    }

    fileOutput = fopen(fileNameOut, "w");

    if (!fileOutput)
    {
        closedir(dirInput);
        printf("Couldn't open output file %s\n", fileNameOut);
        return 1;
    }

    fprintf(fileOutput, "# Generated with rescomp\n");
    fprintf(fileOutput, "\n");

    while ((dirEntry = readdir(dirInput)))
    {
        char filename[MAX_PATH_LEN];
        char *name;
        char *id;

        name = dirEntry->d_name;

        // ignore this...
        if (!strcmp(name, "."))
            continue;
        if (!strcmp(name, ".."))
            continue;

        // set filename
        sprintf(filename, "\"%s\"", name);
        // get file extension
        ext = getFileExtension(name);
        // get id extension
        id = getFilename(name);
        removeExtension(id);

        if (!stricmp(ext, "bmp"))
        {
            // BMP resource
            fprintf(fileOutput, "BITMAP %s %s\n", id, filename);
        }
        else if (!stricmp(ext, "wav"))
        {
            // WAV resource
            fprintf(fileOutput, "WAV %s %s 0 16000\n", id, filename);
        }
        else if (!stricmp(ext, "wavpcm"))
        {
            // ADPCM resource
            fprintf(fileOutput, "WAV %s %s 1\n", id, name);
        }
        else if (!stricmp(ext, "pcm"))
        {
            // ADPCM resource
            fprintf(fileOutput, "PCM %s %s 1\n", id, filename);
        }
        else if (!stricmp(ext, "tfd"))
        {
            // TFD resource
            fprintf(fileOutput, "TFM %s %s 1\n", id, filename);
        }
        else if (!stricmp(ext, "tfc"))
        {
            // TFC resource
            fprintf(fileOutput, "TFM %s %s 1\n", id, filename);
        }
        else if (!stricmp(ext, "mvs"))
        {
            // MVS resource
            fprintf(fileOutput, "BIN %s %s 256\n", id, filename);
        }
        else if (!stricmp(ext, "vgm"))
        {
            // VGM resource
            fprintf(fileOutput, "VGM %s %s\n", id, filename);
        }
        else if (!stricmp(ext, "eif"))
        {
            // EIF resource
            fprintf(fileOutput, "BIN %s %s 256\n", id, filename);
        }
        else if (!stricmp(ext, "esf"))
        {
            // ESF resource
            fprintf(fileOutput, "BIN %s %s 32768\n", id, filename);
        }
        else if (!stricmp(ext, "dat"))
        {
            // DAT resource
            fprintf(fileOutput, "BIN %s %s 256 256\n", id, filename);
        }
        else if (!stricmp(ext, "raw"))
        {
            // RAW resource
            fprintf(fileOutput, "BIN %s %s 256 256\n", id, filename);
        }
    }

    fprintf(fileOutput, "\n");

    closedir(dirInput);
    fclose(fileOutput);

    return 0;
}

static int doComp(char *fileName, char *fileNameOut, int header)
{
    char tempName[MAX_PATH_LEN];
    char line[MAX_LINE_LEN];
    FILE *fileInput;
    FILE *fileOutputS;
    FILE *fileOutputH;

    tempName[0] = 0;

    // save input file directory
    resDirSystem = getDirectorySystem(fileName);
    resDir = getDirectory(fileName);

    if (!fileNameOut[0]) strcpy(fileNameOut, fileName);

    fileInput = fopen(fileName, "rb");

    if (!fileInput)
    {
        printf("Couldn't open input file %s\n", fileName);
        return 1;
    }

    // remove extension
    removeExtension(fileNameOut);

    // create output .s file
    strcpy(tempName, fileNameOut);
    strcat(tempName, ".s");
    fileOutputS = fopen(tempName, "w");

    if (!fileOutputS)
    {
        fclose(fileInput);
        printf("Couldn't open output file %s\n", tempName);
        return 1;
    }

    // create output .h file
    strcpy(tempName, fileNameOut);
    strcat(tempName, ".h");
    fileOutputH = fopen(tempName, "w");

    if (!fileOutputH)
    {
        fclose(fileInput);
        fclose(fileOutputS);
        printf("Couldn't open output file %s\n", tempName);
        return 1;
    }

    fprintf(fileOutputS, ".section .rodata\n\n");
//    fprintf(fileOutputS, ".text\n\n");

    // get file name in uppercase
    strcpy(tempName, getFilename(fileNameOut));
    strupr(tempName);

    fprintf(fileOutputH, "#ifndef _%s_H_\n", tempName);
    fprintf(fileOutputH, "#define _%s_H_\n\n", tempName);

    // process line by line
    while (fgets(line, sizeof(line), fileInput))
    {
        // error while executing --> return code 1
        if (!execute(line, fileOutputS, fileOutputH))
        {
            fclose(fileInput);
            fclose(fileOutputS);
            fclose(fileOutputH);

            return 1;
        }
    }

    fprintf(fileOutputH, "\n");
    fprintf(fileOutputH, "#endif // _%s_H_\n", tempName);

    fclose(fileInput);
    fclose(fileOutputS);
    fclose(fileOutputH);

    if (!header)
    {
        // remove unwanted header file
        strcpy(tempName, fileNameOut);
        strcat(tempName, ".h");
        remove(tempName);
    }

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    Plugin **plugin;

    char type[MAX_NAME_LEN];

    // pass empty line
    if (sscanf(info, "%s", type) < 1)
        return TRUE;
    // pass comment
    if (!strnicmp(type, "//", 2))
        return TRUE;
    if (!strnicmp(type, "#", 1))
        return TRUE;

    plugin = plugins;

    while(*plugin != NULL)
    {
        if ((*plugin)->isSupported(type))
        {
            printf("\nResource: %s", info);
            printf("--> executing plugin %s...\n", type);
            return (*plugin)->execute(info, fs, fh);
        }

        // try next plugin
        plugin++;
    }

    printf("Error: Unknown resource '%s'", info);
    printf("Accepted resource types are:\n");
    printf("  BITMAP\n");
    printf("  PALETTE\n");
    printf("  TILESET\n");
    printf("  MAP\n");
    printf("  IMAGE\n");
    printf("  SPRITE\n");
    printf("  VGM\n");
    printf("  TFM\n");
    printf("  WAV\n");
    printf("  PCM\n");
    printf("  BIN\n");

    return FALSE;
}
