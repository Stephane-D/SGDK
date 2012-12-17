#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>


static int min(int x, int y)
{
    return (x < y) ? x : y;
}

static int vdpcolor(char b, char g, char r)
{
    // genesis only define on 3 bits (but shifted to 1 left)
    r = (r >> 4) & 0xE;
    g = (g >> 4) & 0xE;
    b = (b >> 4) & 0xE;

    return (r << 0) | (g << 4) | (b << 8);
}

static char *getFilename(char *pathname)
{
    char* fname = strrchr(pathname, '/');

    if (fname) return fname + 1;

    return pathname;
}

static void removeExtension(char *pathname)
{
    char* fname = strrchr(pathname, '.');

    if (fname) *fname = 0;
}


int main(int argc, char **argv)
{
    int bitmap;
    int ii, jj, kk, ll;
    int w, h;
    int len;
    int total;
    int align;
    int sizealign;
    int formatint;
    int nullfill;
    char *format;
    char *formatasm;
    char *shortname;
    char *FileName;
    char *FileNameOut;
    FILE *FileInput;
    FILE *FileOutput;
    char temp[512];

    // default
    bitmap = 0;
    FileName = "";
    FileNameOut = "";
    format = "u8 ";
    formatasm = "b";
    formatint = 1;
    total = 0;
    align = 4;
    sizealign = 1;
    nullfill = 0;

    // parse parmeters
    for (ii = 1; ii < argc; ii++)
    {
        if (!strcmp(argv[ii], "-u8"))
        {
            format = "u8 ";
            formatasm = "b";
            formatint = 1;
        }
        else if (!strcmp(argv[ii], "-s8"))
        {
            format = "s8 ";
            formatasm = "b";
            formatint = 1;
        }
        else if (!strcmp(argv[ii], "-u16"))
        {
            format = "u16 ";
            formatasm = "w";
            formatint = 2;
        }
        else if (!strcmp(argv[ii], "-s16"))
        {
            format = "s16 ";
            formatasm = "w";
            formatint = 2;
        }
        else if (!strcmp(argv[ii], "-u32"))
        {
            format = "u32 ";
            formatasm = "l";
            formatint = 4;
        }
        else if (!strcmp(argv[ii], "-s32"))
        {
            format = "s32 ";
            formatasm = "l";
            formatint = 4;
        }
        else if (!strcmp(argv[ii], "-bmp"))
        {
            bitmap = 1;
            format = "u16 ";
            formatasm = "w";
            formatint = 2;
        }
        else if (!strcmp(argv[ii], "-align"))
        {
            ii++;
            align = strtoimax(argv[ii], NULL, 0);

            if (!align) align = 4;
        }
        else if (!strcmp(argv[ii], "-sizealign"))
        {
            ii++;
            sizealign = strtoimax(argv[ii], NULL, 0);

            if (!sizealign) sizealign = 1;
        }
        else if (!strcmp(argv[ii], "-nullfill"))
        {
            ii++;
            nullfill = strtoimax(argv[ii], NULL, 0);
        }
        else if (!FileName[0]) FileName = argv[ii];
        else if (!FileNameOut[0]) FileNameOut = argv[ii];
    }

    if (!FileNameOut[0]) FileNameOut = FileName;

    if (!strcasecmp(&FileName[strlen(FileName)-4], ".bmp"))
    {
        bitmap = 1;
        format = "u16 ";
        formatasm = "w";
        formatint = 2;
        align = 16;
    }

    FileInput = fopen(FileName, "rb");

    if (!FileInput)
    {
        printf("Couldn't open input file %s\n", FileName);
        return 1;
    }

    // remove extension
    removeExtension(FileNameOut);
    // keep only file name
    shortname = getFilename(FileNameOut);

    // build output .s file
    strcpy(temp, FileNameOut);
    strcat(temp, ".s");
    FileOutput = fopen(temp, "w");

    if (!FileOutput)
    {
        fclose(FileInput);
        printf("Couldn't open output file %s\n", temp);
        return 1;
    }

    fprintf(FileOutput, ".text\n\n");
    fprintf(FileOutput, "    .align  %d\n\n", align);
    fprintf(FileOutput, "    .global %s\n", shortname);
    fprintf(FileOutput, "%s:\n", shortname);

    if (bitmap)
    {
        char *mem;

        // process header
        fread(temp, 1, 0x36, FileInput); // read header
        w = (unsigned char)temp[18] | (temp[19] << 8); // only need two bytes, but is really four byte LE
        h = (unsigned char)temp[22] | (temp[23] << 8); // only need two bytes, but is really four byte LE

        fprintf(FileOutput, "    dc.w    %d, %d\n\n", w, h);
        total += 4;

        // process palette (assumes 16 color image)
        fread(temp, 4, 16, FileInput); // read palette

        fprintf(FileOutput, "    dc.w    ");
        for (ii = 0; ii < 7; ii++)
            fprintf(FileOutput, "0x%04X, ", vdpcolor(temp[ii*4+0], temp[ii*4+1], temp[ii*4+2]));
        fprintf(FileOutput, "0x%04X", vdpcolor(temp[7*4+0], temp[7*4+1], temp[7*4+2]));
        fprintf(FileOutput, "\n");

        fprintf(FileOutput, "    dc.w    ");
        for (ii = 8; ii < 15; ii++)
            fprintf(FileOutput, "0x%04X, ", vdpcolor(temp[ii*4+0], temp[ii*4+1], temp[ii*4+2]));
        fprintf(FileOutput, "0x%04X", vdpcolor(temp[15*4+0], temp[15*4+1], temp[15*4+2]));
        fprintf(FileOutput, "\n");

        fprintf(FileOutput, "\n");
        total += 32;

        // process bitmap data (assumes 16 color image)
        mem = malloc(w * h / 2);

        for (ii = 0; ii < h; ii++)
            fread(&mem[(h-ii-1)*w/2], 1, w / 2, FileInput); // fill from bitmap backwards (BMP is bottom up)

        total += w * h / 2;

        for (jj = 0; jj < h; jj++)
        {
            len = w / 2 / formatint;
            ll = 0;

            while (len > 0)
            {
                fprintf(FileOutput, "    dc.%s    ", formatasm);

                for (ii = 0; ii < min(16 / formatint, len); ii++)
                {
                    if (ii)
                        fprintf(FileOutput, ", ");

                    fprintf(FileOutput, "0x");

                    for (kk = 0; kk < formatint; kk++)
                        fprintf(FileOutput, "%02X", (unsigned char)mem[jj*w/2 + (ii + ll)*formatint + kk]);
                }

                fprintf(FileOutput, "\n");
                ll += ii;
                len -= ii;
            }
        }

        free(mem);

        fprintf(FileOutput, "\n");
        fclose(FileOutput);
        fclose(FileInput);
    }
    else
    {
        // non-BMP is dumped as straight data

        memset(temp, nullfill, sizeof(temp));

        while (1)
        {
            len = fread(temp, 1, 16, FileInput);
            len = (len + (formatint - 1)) & ~(formatint - 1); // align length for size of units

            if (len)
            {
                fprintf(FileOutput, "    dc.%s    ", formatasm);

                for (ii = 0; ii < (len / formatint); ii++)
                {
                    if (ii)
                        fprintf(FileOutput, ",");

                    fprintf(FileOutput, "0x");

                    for (jj = 0; jj < formatint; jj++)
                        fprintf(FileOutput, "%02X", (unsigned char)temp[ii*formatint + jj]);
                }

                fprintf(FileOutput, "\n");
            }

            total += len;

            if (len < 16)
                break;

            memset(temp, nullfill, sizeof(temp));
        }

        fprintf(FileOutput, "\n");
        fclose(FileOutput);
        fclose(FileInput);
    }

    // now make .h file
    strcpy(temp, FileNameOut);
    strcat(temp, ".h");
    FileOutput = fopen(temp, "w");

    if (!FileOutput)
    {
        printf("Couldn't open output file %s\n", temp);
        return 1;
    }

    for (ii = 0; ii < strlen(shortname); ii++)
        temp[ii] = toupper(shortname[ii]);

    temp[ii] = 0;
    fprintf(FileOutput, "#ifndef _%s_H_\n", temp);
    fprintf(FileOutput, "#define _%s_H_\n\n", temp);
    fprintf(FileOutput, "extern const %s%s[0x%X];\n\n", format, shortname, total / formatint);
    fprintf(FileOutput, "#endif // _%s_H_\n", temp);

    fclose(FileOutput);
    return 0;
}
