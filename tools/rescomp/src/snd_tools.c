#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../inc/rescomp.h"
#include "../inc/tools.h"
#include "../inc/snd_tools.h"


int dpcmPack(char* fin, char* fout)
{
    char cmd[MAX_PATH_LEN * 2];
    int size;
    FILE *f;
    unsigned char *data;

    // better to remove output file for pcmtoraw
    remove(fout);

    // command
    adjustPath(currentDir, "pcmtoraw", cmd);

    // arguments
    strcat(cmd, " \"");
    strcat(cmd, fin);
    strcat(cmd, "\" \"");
    strcat(cmd, fout);
    strcat(cmd, "\"");

    printf("Executing %s\n", cmd);

    system(cmd);

    // we test for the out file existence
    f = fopen(fout, "rb");
    fclose(f);

    // file don't exist --> error
    if (f == NULL) return FALSE;

    printf("Align %s to 128 bytes\n", fout);

    // open output file
    data = readFile(fout, &size);
    // align data to 128 bytes and fill empty with 136 (silent dpcm data)
    data = sizeAlign(data, size, 128, 136, &size);

    // write out file
    return out(data, 0, size, 1, FALSE, fout);
}

int wavToRaw(char* fin, char* fout)
{
    return wavToRawEx(fin, fout, 0);
}

int wavToRawEx(char* fin, char* fout, int outRate)
{
    char tmp[64];
    char cmd[MAX_PATH_LEN * 2];
    FILE *f;

    // better to remove output file for wavtoraw
    remove(fout);

    // command
    adjustPath(currentDir, "wavtoraw", cmd);

    // arguments
    strcat(cmd, " \"");
    strcat(cmd, fin);
    strcat(cmd, "\" \"");
    strcat(cmd, fout);
    strcat(cmd, "\"");
    if (outRate)
    {
        sprintf(tmp, " %d", outRate);
        strcat(cmd, tmp);
    }

    printf("Executing %s\n", cmd);

    system(cmd);

    // we test for the out file existence
    f = fopen(fout, "rb");
    fclose(f);

    // file don't exist --> error
    if (f == NULL) return FALSE;

    return TRUE;
}
