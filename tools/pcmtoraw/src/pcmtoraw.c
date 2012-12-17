#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

const static int delta_tab[16] =
    { -34,-21,-13,-8,-5,-3,-2,-1,0,1,2,3,5,8,13,21 };

static int GetBestDeltaIndex(int pWantedLevel, int pCurLevel)
{
    int i;
    int ind;
    int diff;
    int mindiff;
    int wdelta;

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

    int newLevel = delta_tab[ind] + pCurLevel;

    // check for overflow (8 bits signed)
    if (newLevel > 127)
        return ind - 1;
    if (newLevel < -128)
        return ind + 1;

    return ind;
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
    int ind;
    int curlevel;
    int in;
    int out;
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

    FileOutput = fopen(FileNameOut, "wb");

    if (!FileOutput)
    {
        fclose(FileInput);
        printf("Couldn't open output file %s\n", FileNameOut);
        return 1;
    }

    FileOutputLoss = fopen("loss", "wb");

    ii = 0;
    curlevel = 0;
    fseek(FileInput, 0, SEEK_END);
    size = ftell(FileInput);
    fseek(FileInput, 0, SEEK_SET);


    while (ii < size)
    {
        // input is 8 bits signed
        in = (char) fgetc(FileInput);
        ind = GetBestDeltaIndex(in, curlevel);
        curlevel += delta_tab[ind];
        fputc(curlevel, FileOutputLoss);

        out = ind;

        // input is 8 bits signed
        in = (char) fgetc(FileInput);
        ind = GetBestDeltaIndex(in, curlevel);
        curlevel += delta_tab[ind];
        fputc(curlevel, FileOutputLoss);

        out |= (ind << 4);

        fputc(out, FileOutput);

        ii += 2;
    }


    fclose(FileOutputLoss);
    fclose(FileOutput);
    fclose(FileInput);

    return 0;
}
