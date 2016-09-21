#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../inc/xgmtool.h"
#include "../inc/util.h"
#include "../inc/vgm.h"
#include "../inc/xgm.h"
#include "../inc/xgc.h"

#define SYSTEM_AUTO     -1
#define SYSTEM_NTSC     0
#define SYSTEM_PAL      1


const char* version = "1.71";
int sys;
bool silent;
bool verbose;
bool sampleRateFix;
bool sampleIgnore;
bool delayKeyOff;


int main(int argc, char *argv[ ])
{
    int i;
    FILE *infile, *outfile;

    if (argc < 3)
    {
        printf("XGMTool %s - Stephane Dallongeville - copyright 2016\n", version);
        printf("\n");
        printf("Usage: xgmtool inputFile outputFile <options>\n");
        printf("XGMTool can do the following operations:\n");
        printf(" - Optimize and reduce size of Sega Megadrive VGM file\n");
        printf("   Note that it won't work correctly on VGM file which require sub frame accurate timing.\n");
        printf(" - Convert a Sega Megadrive VGM file to XGM file\n");
        printf(" - Convert a XGM file to Sega Megadrive VGM file\n");
        printf(" - Compile a XGM file into a binary file (XGC) ready to played by the Z80 XGM driver\n");
        printf(" - Convert a XGC binary file to XGM file (experimental)\n");
        printf(" - Convert a XGC binary file to Sega Megadrive VGM file (experimental)\n");
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
        printf("Convert XGC to XGM (experimental):\n");
        printf("  xgmtool input.xgc output.xgm\n");
        printf("\n");
        printf("Compile XGC to VGM (experimental):\n");
        printf("  xgmtool input.xgc output.vgm\n");
        printf("\n");
        printf("The action xmgtool performs is dependant from the input and output file extension.\n");
        printf("Supported options:\n");
        printf("-s\tenable silent mode (no message except error and warning).\n");
        printf("-v\tenable verbose mode (give more info about conversion).\n");
        printf("-n\tforce NTSC timing (only meaningful for VGM to XGM conversion).\n");
        printf("-p\tforce PAL timing (only meaningful for VGM to XGM conversion).\n");
        printf("-di\tdisable PCM sample auto ignore (it can help when PCM are not properly extracted).\n");
        printf("-dr\tdisable PCM sample rate auto fix (it can help when PCM are not properly extracted).\n");
        printf("-dd\tdisable delayed KEY OFF event when we have KEY ON/OFF in a single frame (it can fix incorrect instrument sound).\n");

        exit(1);
    }

    sys = SYSTEM_AUTO;
    silent = false;
    verbose = false;
    sampleIgnore = true;
    sampleRateFix = true;
    delayKeyOff = true;

    // Open source for binary read (will fail if file does not exist)
    if ((infile = fopen(argv[1], "rb")) == NULL)
    {
        printf("Error: the source file %s could not be opened\n", argv[1]);
        exit(2);
    }

    // test open output for write
    if ((outfile = fopen(argv[2], "wb")) == NULL)
    {
        printf("Error: the output file %s could not be opened\n", argv[2]);
        exit(3);
    }
    // can close
    fclose(outfile);

    // options
    for(i = 3; i < argc; i++)
    {
        if (!strcasecmp(argv[i], "-s"))
        {
            silent = true;
            verbose = false;
        }
        else if (!strcasecmp(argv[i], "-v"))
        {
            verbose = true;
            silent = false;
        }
        else if (!strcasecmp(argv[i], "-di"))
            sampleIgnore = false;
        else if (!strcasecmp(argv[i], "-dr"))
            sampleRateFix = false;
        else if (!strcasecmp(argv[i], "-dd"))
            delayKeyOff = false;
        else if (!strcasecmp(argv[i], "-n"))
            sys = SYSTEM_NTSC;
        else if (!strcasecmp(argv[i], "-p"))
            sys = SYSTEM_PAL;
        else
            printf("Warning: option %s not recognized (ignored)\n", argv[i]);
    }

    // silent mode has priority
    if (silent)
        verbose = false;

    char* inExt = getFileExtension(argv[1]);
    char* outExt = getFileExtension(argv[2]);
    int errCode = 0;

    // VGM or empty (assumed as VGM)
    if (!strcasecmp(inExt, "VGM") || !strlen(inExt))
    {
        if ((!strcasecmp(outExt, "VGM")) || (!strcasecmp(outExt, "XGM")) || (!strcasecmp(outExt, "BIN")) || (!strcasecmp(outExt, "XGC")))
        {
            // VGM optimization
            int inDataSize;
            unsigned char* inData;
            int outDataSize;
            unsigned char* outData;
            VGM* vgm;
//            VGM* optVgm;

            // load file
            inData = readBinaryFile(argv[1], &inDataSize);
            if (inData == NULL) exit(1);
            // load VGM
            if (sys == SYSTEM_NTSC)
                inData[0x24] = 60;
            else if (sys == SYSTEM_PAL)
                inData[0x24] = 50;
            // create with conversion
            vgm = VGM_create(inData, inDataSize, 0, true);
            if (vgm == NULL) exit(1);
//            // optimize
//            optVgm = VGM_createFromVGM(vgm, true);
//            if (optVgm == NULL) exit(1);

            VGM_convertWaits(vgm);
            VGM_cleanCommands(vgm);
            VGM_cleanSamples(vgm);
            VGM_fixKeyCommands(vgm);

            // VGM output
            if (!strcasecmp(outExt, "VGM"))
            {
                // get byte array
                outData = VGM_asByteArray(vgm, &outDataSize);
                if (outData == NULL) exit(1);
                // write to file
                writeBinaryFile(outData, outDataSize, argv[2]);
            }
            else
            {
                XGM* xgm;

                // convert to XGM
                xgm = XGM_createFromVGM(vgm);
                if (xgm == NULL) exit(1);

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
                    if (xgc == NULL) exit(1);
                    // get byte array
                    outData = XGC_asByteArray(xgc, &outDataSize);
                }

                if (outData == NULL) exit(1);
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
            if (inData == NULL) exit(1);
            // load XGM
            xgm = XGM_createFromData(inData, inDataSize);
            if (xgm == NULL) exit(1);

            // VGM conversion
            if (!strcasecmp(outExt, "VGM"))
            {
                VGM* vgm;

                // convert to VGM
                vgm = VGM_createFromXGM(xgm);
                if (vgm == NULL) exit(1);
                // get byte array
                outData = VGM_asByteArray(vgm, &outDataSize);
            }
            else
            {
                XGM* xgc;

                // convert to XGC (compiled XGM)
                xgc = XGC_create(xgm);
                if (xgc == NULL) exit(1);
                // get byte array
                outData = XGC_asByteArray(xgc, &outDataSize);
            }

            if (outData == NULL) exit(1);
            // write to file
            writeBinaryFile(outData, outDataSize, argv[2]);
        }
        else
        {
            printf("Error: the output file %s is incorrect (should be a VGM or BIN/XGC file)\n", argv[2]);
            errCode = 4;
        }
    }
    else if (!strcasecmp(inExt, "XGC"))
    {
        if ((!strcasecmp(outExt, "VGM")) || (!strcasecmp(outExt, "XGM")))
        {
            // XGC to XGM
            int inDataSize;
            unsigned char* inData;
            int outDataSize;
            unsigned char* outData;
            XGM* xgm;

            // load file
            inData = readBinaryFile(argv[1], &inDataSize);
            if (inData == NULL) exit(1);
            // load XGM
            xgm = XGM_createFromXGCData(inData, inDataSize);
            if (xgm == NULL) exit(1);

            // VGM conversion
            if (!strcasecmp(outExt, "VGM"))
            {
                VGM* vgm;

                // convert to VGM
                vgm = VGM_createFromXGM(xgm);
                if (vgm == NULL) exit(1);
                // get byte array
                outData = VGM_asByteArray(vgm, &outDataSize);
            }
            else
            {
                // get byte array
                outData = XGM_asByteArray(xgm, &outDataSize);
            }

            if (outData == NULL) exit(1);
            // write to file
            writeBinaryFile(outData, outDataSize, argv[2]);
        }
        else
        {
            printf("Error: the output file %s is incorrect (should be a XGM or VGM file)\n", argv[2]);
            errCode = 4;
        }
    }
    else
    {
        printf("Error: the input file %s is incorrect (should be a VGM, XGM or XGC file)\n", argv[1]);
        errCode = 4;
    }

    fclose(infile);

    remove("tmp.bin");

    return errCode;
}
