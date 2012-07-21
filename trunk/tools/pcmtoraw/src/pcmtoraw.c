#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <ctype.h>

const char delta_tab[16] =
    { -34,-21,-13,-8,-5,-3,-2,-1,0,1,2,3,5,8,13,21 };

int sample_over;
int sample_loss;

static int GetBestDelta(int pWantedLevel, int pCurLevel)
{
    int i;
    int ind;
    int diff;
    int mindiff;
    int wdelta;
    char result;

    wdelta = pWantedLevel - pCurLevel;
    ind = 0;
    mindiff = abs(wdelta - delta_tab[ind]);

    for (i=1; i<16; i++)
    {
        diff = abs(wdelta - delta_tab[i]);
        if (diff < mindiff)
        {
            mindiff = diff;
            ind = i;
        }
    }

    if (mindiff)
    {
        sample_over++;
        sample_loss += mindiff;
    }

    // check for overflow
    if ((delta_tab[ind] + pCurLevel) > 127)
        result = ind - 1;
    else if ((delta_tab[ind] + pCurLevel) < -128)
        result = ind + 1;
    else
        result = ind;

    return result;
}

//static char *getFilename(char *pathname)
//{
//    char* fname = strrchr(pathname, '/');
//
//    if (fname) return fname + 1;
//
//    return pathname;
//}

static void removeExtension(char *pathname)
{
    char* fname = strrchr(pathname, '.');

    if (fname) *fname = 0;
}


int main(int argc, char **argv)
{
    int ii;
    int size;
    int diff_ind;
    char curlevel;
    char data_sb;
    char diff_sb;
    char out_sb;
    char *FileName;
    char *FileNameOut;
    FILE *FileInput;
    FILE *FileOutput;
    FILE *FileOutputLoss;

    // default
    FileName = "";
    FileNameOut = "";

    // parse parmeters
    for (ii = 1; ii < argc; ii++)
    {
        if (!FileName[0]) FileName = argv[ii];
        else if (!FileNameOut[0]) FileNameOut = argv[ii];
    }

    FileInput = fopen(FileName, "rb");

    if (!FileInput)
    {
        printf("Couldn't open input file %s\n", FileName);
        return 1;
    }

    if (!FileNameOut[0])
    {
        FileNameOut = FileName;
        // remove extension
        removeExtension(FileNameOut);
        // set raw output
        strcat(FileNameOut, ".raw");
    }

    FileOutput = fopen(FileNameOut, "w");

    if (!FileOutput)
    {
        fclose(FileInput);
        printf("Couldn't open output file %s\n", FileNameOut);
        return 1;
    }

    FileOutputLoss = fopen("loss", "w");

    sample_over = 0;
    sample_loss = 0;

    ii = 0;
    curlevel = 0;
    fseek(FileInput, 0, SEEK_END);
    size = ftell(FileInput);
    fseek(FileInput, 0, SEEK_SET);

    while (ii < size)
    {
        data_sb = fgetc(FileInput);
        diff_ind = GetBestDelta(data_sb, curlevel);
        diff_sb = delta_tab[diff_ind];
        curlevel = curlevel + diff_sb;
        fputc(curlevel, FileOutputLoss);

        out_sb = diff_ind;

        data_sb = fgetc(FileInput);
        diff_ind = GetBestDelta(data_sb, curlevel);
        diff_sb = delta_tab[diff_ind];
        curlevel = curlevel + diff_sb;
        fputc(curlevel, FileOutputLoss);

        out_sb = out_sb | (diff_ind << 4);

        fputc(out_sb, FileOutput);

        ii += 2;
    }

    fclose(FileOutputLoss);
    fclose(FileOutput);
    fclose(FileInput);

    return 0;
}
