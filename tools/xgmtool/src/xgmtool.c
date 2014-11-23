#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../inc/xgmtool.h"
#include "../inc/util.h"
#include "../inc/vgm.h"
#include "../inc/xgm.h"
#include "../inc/xgc.h"


const char* version = "1.0";
bool silent;
bool verbose;


int main(int argc, char *argv[ ])
{
    int i;
    FILE *infile, *outfile;

    if (argc < 3)
    {
        printf("XGMTool %s - Stephane Dallongeville - copyright 2014\n", version);
        printf("\n");
        printf("Usage: xgmtool inputFile outputFile <options>\n");
        printf("XGMTool can do the following operations:\n");
        printf(" - Optimize and reduce size of Sega Megadrive VGM file\n");
        printf("   Note that it won't work correctly on VGM file which require sub frame accurate timing.\n");
        printf(" - Convert a Sega Megadrive VGM file to XGM file\n");
        printf(" - Convert a XGM file to Sega Megadrive VGM file\n");
        printf(" - Compile a XGM file into a binary file ready to played by the Z80 XGM driver\n");
        printf("\n");
        printf("Optimize VGM:\n");
        printf("  xgmtool input.vgm output.vgm\n");
        printf("\n");
        printf("Convert VGM to XGM:\n");
        printf("  xgmtool input.vgm output.xgm\n");
        printf("\n");
        printf("Convert and compile VGM to binary/XGC:\n");
        printf("  xgmtool input.vgm output.bin\n");
        printf("  xgmtool input.vgm output.xgc\n");
        printf("\n");
        printf("Convert XGM to VGM:\n");
        printf("  xgmtool input.xgm output.vgm\n");
        printf("\n");
        printf("Compile XGM to binary/XGC:\n");
        printf("  xgmtool input.xgm output.bin\n");
        printf("  xgmtool input.xgm output.xgc\n");
        printf("\n");
        printf("The action xmgtool perform is dependant from the input and output file extension.\n");
        printf("\n");
        printf("Supported options:\n");
        printf("-s\tenable silent mode (no message except error and warning).\n");
        printf("-v\tenable verbose mode.\n");

        exit(1);
    }

    silent = false;
    verbose = false;

    // Open source for binary read (will fail if file does not exist)
    if ((infile = fopen(argv[1], "rb")) == NULL)
    {
        printf("Error: the source file %s could not be opened\n", argv[1]);
        exit(2);
    }

    // Open output for write
    if ((outfile = fopen(argv[2], "wb")) == NULL)
    {
        printf("Error: the output file %s could not be opened\n", argv[2]);
        exit(3);
    }

    // options
    for(i = 3; i < argc; i++)
    {
        if (!strcasecmp(argv[i], "-s"))
            silent = true;
        else if (!strcasecmp(argv[i], "-v"))
            verbose = true;
        else
            printf("Warning: option %s not recognized (ignored)\n", argv[3]);
    }

    // silent mode has priority
    if (silent)
        verbose = false;

    char* inExt = getFileExtension(argv[1]);
    char* outExt = getFileExtension(argv[2]);
    int errCode = 0;

    if (!strcasecmp(inExt, "VGM"))
    {
        if ((!strcasecmp(outExt, "VGM")) || (!strcasecmp(outExt, "XGM")) || (!strcasecmp(outExt, "BIN")) || (!strcasecmp(outExt, "XGC")))
        {
            // VGM optimization
            int inDataSize;
            unsigned char* inData;
            int outDataSize;
            unsigned char* outData;
            VGM* vgm;
            VGM* optVgm;

            // load file
            inData = readBinaryFile(argv[1], &inDataSize);
            // load VGM
            vgm = VGM_create1(inData, inDataSize, 0);
            // optimize
            optVgm = VGM_createFromVGM(vgm, true);
            VGM_convertWaits(optVgm);
            VGM_cleanCommands(optVgm);
            VGM_cleanSamples(optVgm);

            // VGM output
            if (!strcasecmp(outExt, "VGM"))
            {
                // get byte array
                outData = VGM_asByteArray(optVgm, &outDataSize);
                // write to file
                writeBinaryFile(outData, outDataSize, argv[2]);
            }
            else
            {
                XGM* xgm;

                // convert to XGM
                xgm = XGM_createFromVGM(optVgm);

                // XGM output
                if (!strcasecmp(outExt, "XGM"))
                {
                    // get byte array
                    outData = XGM_asByteArray(xgm, &outDataSize);
                }
                else
                {
                    XGM* xgc;

                    // convert to XGC (compiled XGM)
                    xgc = XGC_create(xgm);
                    // get byte array
                    outData = XGC_asByteArray(xgc, &outDataSize);
                }

                // write to file
                writeBinaryFile(outData, outDataSize, argv[2]);
            }
        }
        else
        {
            printf("Error: the output file %s is incorrect (should be a VGM, XGM or BIN/XGC file)\n", argv[2]);
            errCode = 4;
        }
    }
    else if (!strcasecmp(inExt, "XGM"))
    {
        if ((!strcasecmp(outExt, "VGM")) || (!strcasecmp(outExt, "BIN")) || (!strcasecmp(outExt, "XGC")))
        {
            // XGM to VGM
            int inDataSize;
            unsigned char* inData;
            int outDataSize;
            unsigned char* outData;
            XGM* xgm;

            // load file
            inData = readBinaryFile(argv[1], &inDataSize);
            // load XGM
            xgm = XGM_createFromData(inData, inDataSize);

            // VGM conversion
            if (!strcasecmp(outExt, "VGM"))
            {
                VGM* vgm;

                // convert to VGM
                vgm = VGM_createFromXGM(xgm);
                // get byte array
                outData = VGM_asByteArray(vgm, &outDataSize);
            }
            else
            {
                XGM* xgc;

                // convert to XGC (compiled XGM)
                xgc = XGC_create(xgm);
                // get byte array
                outData = XGC_asByteArray(xgc, &outDataSize);
            }

            // write to file
            writeBinaryFile(outData, outDataSize, argv[2]);
        }
        else
        {
            printf("Error: the output file %s is incorrect (should be a VGM or BIN/XGC file)\n", argv[2]);
            errCode = 4;
        }
    }
    else
    {
        printf("Error: the input file %s is incorrect (should be a VGM or XGM file)\n", argv[1]);
        errCode = 4;
    }

    fclose(infile);
    fclose(outfile);

    remove("tmp.bin");

    return errCode;
}

