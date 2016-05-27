#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>


const char* version = "1.2";

double *readSample(FILE* file, int chunkSize, int sampleSize, int numChan);


int main(int argc, char *argv[ ])
{
    FILE *infile, *outfile;
    char prefix[4];
    char fileFormat[4];
    char ckID[4];
    uint32_t nChunkSize;
    short wFormatTag;
    short nChannels;
    uint32_t nSamplesPerSecond;
    uint32_t nBytesPerSecond;
    uint32_t nOutputSamplesPerSecond;
    short nBlockAlign;
    short nBitsPerSample;
    int i, j;

    if (argc < 3)
    {
        printf("WavToRaw %s - Stephane Dallongeville - copyright 2016\n", version);
        printf("\n");
        printf("Usage: wav2raw sourceFile destFile <outRate>\n");
        printf("Output rate is given in Hz.\n");
        printf("Success returns errorlevel 0. Error return greater than zero.\n");
        printf("\n");
        printf("Ex: wav2raw input.wav output.raw 11025\n");
        exit(1);
    }

    /* Open source for binary read (will fail if file does not exist) */
    if ((infile = fopen( argv[1], "rb" )) == NULL)
    {
        printf("The source file %s was not opened\n", argv[1]);
        exit(2);
    }

    if (argc > 3)
        nOutputSamplesPerSecond = atoi(argv[3]);
    else
        nOutputSamplesPerSecond = 0;

    /* Read the header bytes. */
    #define saferead(x, y, z) if (fscanf(x, y, z) != 1) puts("Failed to read " #z)

    saferead( infile, "%4s", prefix );
    saferead( infile, "%4c", &nChunkSize );
    saferead( infile, "%4c", fileFormat );
    saferead( infile, "%4c", ckID );
    saferead( infile, "%4c", &nChunkSize );
    saferead( infile, "%2c", &wFormatTag );
    saferead( infile, "%2c", &nChannels );
    saferead( infile, "%4c", &nSamplesPerSecond );
    saferead( infile, "%4c", &nBytesPerSecond );
    saferead( infile, "%2c", &nBlockAlign );
    saferead( infile, "%2c", &nBitsPerSample );

    // pass extra bytes in bloc
    for(i = 0; i < nChunkSize - 0x10; i++)
        saferead( infile, "%1c", &j);

    saferead( infile, "%4c", ckID );
    saferead( infile, "%4c", &nChunkSize );

    #undef saferead

    if (nOutputSamplesPerSecond == 0)
        nOutputSamplesPerSecond = nSamplesPerSecond;

    /* Open output for write */
    if( (outfile = fopen( argv[2], "wb" )) == NULL )
    {
        printf("The output file %s was not opened\n", argv[2]);
        exit(3);
    }

    int nBytesPerSample = nBitsPerSample / 8;
    int size = nChunkSize / (nChannels * nBytesPerSample);
    const double *data = readSample(infile, nChunkSize, nBytesPerSample, nChannels);
    int iOffset;
    double offset;
    double step;
    double value;
    double lastSample;

    step = nSamplesPerSecond;
    step /= nOutputSamplesPerSecond;
    value = 0;
    lastSample = 0;
    iOffset = 0;

    for(offset = 0; offset < size; offset += step)
    {
        char byte;
        double sample = 0;

        // extrapolation
        if (step > 1.0)
        {
            if (value < 0) sample += lastSample * -value;

            value += step;

            while(value > 0)
            {
                lastSample = data[iOffset++];

                if (value >= 1)
                    sample += lastSample;
                else
                    sample += lastSample * value;

                value--;
            }

            sample /= step;
        }
        // interpolation
        else
        {
//            if (floor(offset) == offset)
                sample = data[(int) offset];
//            else
//            {
//                double sample0 = data[(int) floor(offset)];
//                double sample1 = data[(int) ceil(offset)];
//
//                sample += sample0 * (ceil(offset) - offset);
//                sample += sample1 * (offset - floor(offset));
//            }
        }

        byte = round(sample);
        fwrite(&byte, 1, 1, outfile);
    }

    fclose(infile);
    fclose(outfile);

    return 0;
}

double nextSample(FILE* file, int sampleSize, int numChan)
{
    unsigned char b;
    short w;
    long l;
    int i;

    double res = 0;

    for(i = 0; i < numChan; i++)
    {
        switch(sampleSize)
        {
            case 1:
                fscanf( file, "%1c", &b );
                res += ((int)b) - 0x80;
                break;

            case 2:
                fscanf( file, "%2c", &w );
                res += w >> 8;
                break;

            case 3:
                fscanf( file, "%3c", &l );
                res += w >> 16;
                break;

            case 4:
                fscanf( file, "%4c", &l );
                res += w >> 24;
                break;
        }
    }

    return res / numChan;
}

double *readSample(FILE* file, int chunkSize, int sampleSize, int numChan)
{
    double *result;
    double *dst;
    int i, size;

    size = chunkSize / (numChan * sampleSize);
    result = malloc(size * sizeof(double));

    dst = result;
    for(i = 0; i < size; i++)
        *dst++ = nextSample(file, sampleSize, numChan);

    return result;
}
