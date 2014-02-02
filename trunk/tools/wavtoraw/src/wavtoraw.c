#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


double nextSample(FILE* file, int sampleSize, int numChan);


int main(int argc, char *argv[ ])
{
    FILE *infile, *outfile;
    char prefix[4];
    char fileFormat[4];
    char ckID[4];
    unsigned long nChunkSize;
    short wFormatTag;
    short nChannels;
    unsigned long nSamplesPerSecond;
    unsigned long nBytesPerSecond;
    unsigned long nOutputSamplesPerSecond;
    short nBlockAlign;
    short nBitsPerSample;
    int i, j;

    if (argc < 3)
    {
        printf("Usage: wav2raw sourceWavFile destRawFile <outRate>\n");
        printf("Output rate is given in Hand should be <= to the input sample sample.\n");
        printf("Success returns errorlevel 0. Error return greater than zero.\n");
        printf("\n");
        printf("Ex: wav2raw input.wav output.raw 11025\n");
        exit(1);
    }

    /* Open source for binary read (will fail if file does not exist) */
    if ((infile = fopen( argv[1], "rb" )) == NULL)
//    if ((infile = fopen( "test.wav", "rb" )) == NULL)
    {
        printf("The source file %s was not opened\n", argv[1]);
        exit(2);
    }

    if (argc > 3)
        nOutputSamplesPerSecond = atoi(argv[3]);
    else
        nOutputSamplesPerSecond = 0;

    /* Read the header bytes. */
    fscanf( infile, "%4s", prefix );
    fscanf( infile, "%4c", &nChunkSize );
    fscanf( infile, "%4c", fileFormat );
    fscanf( infile, "%4c", ckID );
    fscanf( infile, "%4c", &nChunkSize );
    fscanf( infile, "%2c", &wFormatTag );
    fscanf( infile, "%2c", &nChannels );
    fscanf( infile, "%4c", &nSamplesPerSecond );
    fscanf( infile, "%4c", &nBytesPerSecond );
    fscanf( infile, "%2c", &nBlockAlign );
    fscanf( infile, "%2c", &nBitsPerSample );

    // pass extra bytes in bloc
    for(i = 0; i < nChunkSize - 0x10; i++)
        fscanf( infile, "%1c", &j);

    fscanf( infile, "%4c", ckID );
    fscanf( infile, "%4c", &nChunkSize );

    if (nOutputSamplesPerSecond == 0)
        nOutputSamplesPerSecond = nSamplesPerSecond;

    if(nSamplesPerSecond < nOutputSamplesPerSecond)
    {
        printf("Output rate (%ld) cannot be above input rate (%ld)\n", nOutputSamplesPerSecond, nSamplesPerSecond);
        printf("Use lower output rate value or higher input rate file\n");
        exit(4);
    }

        /* Open output for write */
    if( (outfile = fopen( argv[2], "w" )) == NULL )
//    if( (outfile = fopen( "out.raw", "w" )) == NULL )
    {
        printf("The output file %s was not opened\n", argv[2]);
        exit(3);
    }

    int nBytesPerSample = nBitsPerSample / 8;
    int size = nChunkSize / (nChannels * nBytesPerSample);
    double offset;
    double step;
    double value;
    double lastSample;

    step = nSamplesPerSecond;
    step /= nOutputSamplesPerSecond;
    value = 0;
    lastSample = 0;

    for(offset = 0; offset < size; offset += step)
    {
        char byte;
        double sample = 0;

        if (value < 0) sample += lastSample * -value;

        value += step;

        while(value > 0)
        {
            lastSample = nextSample(infile, nBytesPerSample, nChannels);

            if (value >= 1)
                sample += lastSample;
            else
                sample += lastSample * value;

            value--;
        }

        sample /= step;
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
