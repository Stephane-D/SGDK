#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../inc/rescomp.h"
#include "../inc/plugin.h"

#include "../inc/pcm.h"

#include "../inc/tools.h"
#include "../inc/snd_tools.h"


// forward
static int isSupported(char *type);
static int execute(char *info, FILE *fs, FILE *fh);

// PCM resource support
Plugin pcm = { isSupported, execute };


static int isSupported(char *type)
{
    if (!stricmp(type, "PCM")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    int size;
    int nbElem;
    int driver;
    unsigned char *data;

    driver = 0;
    nbElem = sscanf(info, "%s %s \"%[^\"]\" %d", temp, id, temp, &driver);

    if (nbElem < 3)
    {
        printf("Wrong PCM definition\n");
        printf("PCM name file [driver]\n");
        printf("  name    variable name\n");
        printf("  file    path of the PCM data file\n");
        printf("          Should be 8 bits PCM signed data and size need to be aligned to 256 for correct loop operation for drive supporting it.\n");
        printf("  driver  specify the Z80 driver we will use to play the PCM (allow conversion if needed):\n");
        printf("          0 (default) = Z80_DRIVER_PCM\n");
        printf("            Single channel 8 bits signed sample driver.\n");
        printf("            It can play a sample (8 bit signed) from 8 Khz up to 32 Khz rate.\n");
        printf("            <b>Input:</b> 8 bits signed PCM at 8000 / 11025 / 13400 / 16000 / 22050 / 32000 Hz\n");
        printf("          1 = Z80_DRIVER_2ADPCM\n");
        printf("            2 channels 4 bits ADPCM sample driver.\n");
        printf("            It can mix up to 2 ADCPM samples at a fixed 22050 Hz Khz rate.\n");
        printf("            <b>Input:</b> 8 bits signed PCM at 22050 Hz\n");
        printf("          2 = Z80_DRIVER_4PCM\n");
        printf("            4 channels 8 bits signed sample driver.\n");
        printf("            It can mix up to 4 samples (8 bit signed) at a fixed 16 Khz rate.\n");
        printf("            <b>Input:</b> 8 bits signed PCM at 16000 Hz\n");
        printf("          3 = Z80_DRIVER_4PCM_ENV\n");
        printf("            4 channels 8 bits signed sample driver with volume support.\n");
        printf("            It can mix up to 4 samples (8 bit signed) at a fixed 16 Khz rate.\n");
        printf("            with volume support (16 levels du to memory limitation).\n");
        printf("            <b>Input:</b> 8 bits signed PCM at 16000 Hz\n");
        printf("          4 = Z80_DRIVER_VGM\n");
        printf("            VGM music driver.\n");
        printf("            It supports PCM SFX (8 bit unsigned) at a fixed 8 Khz rate.\n");
        printf("            <b>Input:</b> 8 bits signed PCM at 8000 Hz\n");

        return FALSE;
    }

    // adjust input file path
    adjustPath(resDir, temp, fileIn);

    switch(driver)
    {
        default:
            // read data from PCM file
            data = in(fileIn, &size);
            // size align = 256
            if (data)
                data = sizeAlign(data, size, 256, 0, &size);
            break;

        case 1:
            strcpy(temp, fileIn);
            removeExtension(temp);
            strcat(temp, ".tmp");

            // do DPCM conversion and size data alignment
            if (!dpcmPack(fileIn, temp))
            {
                printf("Error while compressing '%s' to DPCM format\n", fileIn);
                return FALSE;
            }

            // read data from DPCM file
            data = in(temp, &size);
            // clean
            remove(temp);
            break;
    }

    // error while reading data
    if (!data) return FALSE;

    // need to unsign data
    if (driver == 4)
        unsign8b(data, size);

    // EXPORT PCM
    outPCM(data, size, (driver==1)?128:256, fs, fh, id, TRUE);

    printf("done !");

    return TRUE;
}


void outPCM(unsigned char* data, int size, int align, FILE* fs, FILE* fh, char* id, int global)
{
    // declare
    declArray(fs, fh, "u8", id, size, align, TRUE);
    // output data
    outS(data, 0, size, fs, 1);
    fprintf(fs, "\n");
}
