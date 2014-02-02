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
    if (!stricmp(type, "WAV")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char temp2[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    int size;
    int nbElem;
    int driver;
    int outRate;
    int unsign;
    unsigned char *data;

    driver = 0;
    outRate = 0;
    unsign = 0;
    nbElem = sscanf(info, "%s %s \"%[^\"]\" %d %d", temp, id, temp, &driver, &outRate);

    if (nbElem < 3)
    {
        printf("Wrong WAV definition\n");
        printf("WAV name file [driver [out_rate]]\n");
        printf("  name      variable name\n");
        printf("  file      path of the .wav file (will be converted to 8 bits signed PCM)\n");
        printf("  driver    specify the Z80 driver we will use to play the WAV file:\n");
        printf("            0 (default) = Z80_DRIVER_PCM\n");
        printf("              Single channel 8 bits signed sample driver.\n");
        printf("              It can play a sample (8 bit signed) from 8 Khz up to 32 Khz rate.\n");
        printf("              <b>Input:</b> 8 bits signed PCM at 8000 / 11025 / 13400 / 16000 / 22050 / 32000 Hz\n");
        printf("            1 = Z80_DRIVER_2ADPCM\n");
        printf("              2 channels 4 bits ADPCM sample driver.\n");
        printf("              It can mix up to 2 ADCPM samples at a fixed 22050 Hz Khz rate.\n");
        printf("              <b>Input:</b> 8 bits signed PCM at 22050 Hz\n");
        printf("            2 = Z80_DRIVER_4PCM\n");
        printf("              4 channels 8 bits signed sample driver.\n");
        printf("              It can mix up to 4 samples (8 bit signed) at a fixed 16 Khz rate.\n");
        printf("              <b>Input:</b> 8 bits signed PCM at 16000 Hz\n");
        printf("            3 = Z80_DRIVER_4PCM_ENV\n");
        printf("              4 channels 8 bits signed sample driver with volume support.\n");
        printf("              It can mix up to 4 samples (8 bit signed) at a fixed 16 Khz rate.\n");
        printf("              with volume support (16 levels du to memory limitation).\n");
        printf("              <b>Input:</b> 8 bits signed PCM at 16000 Hz\n");
        printf("            4 = Z80_DRIVER_VGM\n");
        printf("              VGM music driver.\n");
        printf("              It supports PCM SFX (8 bit unsigned) at a fixed 8 Khz rate.\n");
        printf("              <b>Input:</b> 8 bits signed PCM at 8000 Hz\n");
        printf("  out_rate  output PCM rate (only used for Z80_DRIVER_PCM driver)\n");
        printf("            By default the default WAV output rate is used.\n");

        return FALSE;
    }

    // adjust input file path
    adjustPath(resDir, temp, fileIn);

    // determine output rate
    switch(driver)
    {
        case 1:
            outRate = 22050;
            break;

        case 2:
        case 3:
            outRate = 16000;

        case 4:
            outRate = 8000;
            unsign = 1;
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

        case 1:
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
    outWAV(data, size, (driver==1)?128:256, fs, fh, id, TRUE);

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
