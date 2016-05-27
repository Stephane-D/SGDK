#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../inc/rescomp.h"
#include "../inc/plugin.h"

#include "../inc/wav.h"

#include "../inc/tools.h"
#include "../inc/snd_tools.h"


// forward
static int isSupported(char *type);
static int execute(char *info, FILE *fs, FILE *fh);

// WAV resource support
Plugin wav = { isSupported, execute };


static int isSupported(char *type)
{
    if (!strcasecmp(type, "WAV")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char temp2[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    char driverStr[256];
    int size;
    int nbElem;
    int driver;
    int outRate;
    int unsign;
    unsigned char *data;

    outRate = 0;
    unsign = 0;
    strcpy(driverStr, "");

    nbElem = sscanf(info, "%s %s \"%[^\"]\" %s %d", temp, id, temp, driverStr, &outRate);

    if (nbElem < 3)
    {
        printf("Wrong WAV definition\n");
        printf("WAV name \"file\" [driver [out_rate]]\n");
        printf("  name      variable name\n");
        printf("  file      path of the .wav file (will be converted to 8 bits signed PCM)\n");
        printf("  driver    specify the Z80 driver we will use to play the WAV file:\n");
        printf("              0 / PCM (default)\n");
        printf("                Single channel 8 bits sample driver.\n");
        printf("                It can play sample from 8 Khz up to 32 Khz rate.\n");
        printf("              1 / 2ADPCM\n");
        printf("                2 channels 4 bits ADPCM sample driver.\n");
        printf("                It can mix up to 2 ADCPM samples at a fixed 22050 Hz Khz rate.\n");
        printf("              2 / 3 / 4PCM\n");
        printf("                4 channels 8 bits sample driver with volume support.\n");
        printf("                It can mix up to 4 samples at a fixed 16 Khz rate\n");
        printf("                with volume support (16 levels du to memory limitation).\n");
        printf("              4 / VGM\n");
        printf("                VGM music driver with 8 bits PCM SFX support.\n");
        printf("                It supports single PCM SFX at a fixed ~9 Khz rate while playing VGM music.\n");
        printf("              5 / XGM\n");
        printf("                XGM music with 4 channels 8 bits samples driver.\n");
        printf("                It supports 4 PCM SFX at a fixed 14 Khz rate while playing XGM music.\n");
        printf("  out_rate  output PCM rate (only used for Z80_DRIVER_PCM driver)\n");
        printf("              By default the default WAV output rate is used.\n");

        return FALSE;
    }

    // adjust input file path
    adjustPath(resDir, temp, fileIn);
    // get driver value
    driver = getDriver(driverStr);

    // determine output rate
    switch(driver)
    {
        case DRIVER_2ADPCM:
            outRate = 22050;
            break;

        case DRIVER_4PCM:
            outRate = 16000;
            break;

        case DRIVER_VGM:
            outRate = 8000;
            unsign = 1;
            break;

        case DRIVER_XGM:
            outRate = 14000;
            break;
    }

    strcpy(temp, fileIn);
    removeExtension(temp);
    strcat(temp, ".tmp");

    // convert WAV to PCM
    if (!wavToRawEx(fileIn, temp, outRate))
    {
        printf("Error while converting WAV '%s' to PCM format\n", fileIn);
        return FALSE;
    }

    switch(driver)
    {
        default:
            // read data from PCM file
            data = in(temp, &size);
            // clean temp file
            remove(temp);
            // size align = 256
            if (data)
                data = sizeAlign(data, size, 256, 0, &size);
            break;

        case DRIVER_2ADPCM:
            strcpy(temp2, fileIn);
            removeExtension(temp2);
            strcat(temp2, ".t2");

            // do DPCM conversion and size data alignment
            if (!dpcmPack(temp, temp2))
            {
                printf("Error while compressing '%s' to DPCM format\n", fileIn);
                return FALSE;
            }

            // read data from DPCM file
            data = in(temp2, &size);
            // clean
            remove(temp);
            remove(temp2);
            break;
    }

    // error while reading data
    if (!data) return FALSE;

    // need to unsign data
    if (unsign)
        unsign8b(data, size);

    // EXPORT WAV
    outWAV(data, size, (driver==DRIVER_2ADPCM)?128:256, fs, fh, id, TRUE);

    return TRUE;
}


void outWAV(unsigned char* data, int size, int align, FILE* fs, FILE* fh, char* id, int global)
{
    // declare
    declArray(fs, fh, "u8", id, size, align, TRUE);
    // output data
    outS(data, 0, size, fs, 1);
    fprintf(fs, "\n");
}
