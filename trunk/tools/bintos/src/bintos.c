#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>




//static int min(int x, int y)
//{
//    return (x < y) ? x : y;
//}
//
//static int vdpcolor(char b, char g, char r)
//{
//    // genesis only define on 3 bits (but shifted to 1 left)
//    r = (r >> 4) & 0xE;
//    g = (g >> 4) & 0xE;
//    b = (b >> 4) & 0xE;
//
//    return (r << 0) | (g << 4) | (b << 8);
//}

static char *getFilename(char *pathname)
{
    char* fname = strrchr(pathname, '/');

    if (fname) return fname + 1;

    return pathname;
}

static char *getFileExtension(char *pathname)
{
    char* fname = strrchr(pathname, '.');

    if (fname) return fname + 1;

    return "";
}

static void removeExtension(char *pathname)
{
    char* fname = strrchr(pathname, '.');

    if (fname) *fname = 0;
}

//static unsigned int getFileSize(char *file)
//{
//    unsigned int len;
//    FILE * f;
//
//    f = fopen(file, "r");
//    fseek(f, 0, SEEK_END);
//    len = (unsigned long)ftell(f);
//    fclose(f);
//
//    return len;
//}


int main(int argc, char **argv)
{
    int ii, jj;
    int len;
    int total;
    int align;
    int sizealign;
    int formatint;
    int nullfill;
    char *format;
    char *formatasm;
    char *shortname;
    char *ext;
    char *FileName;
    char *FileNameOut;
    FILE *FileInput;
    FILE *FileOutput;
    char temp[32 * 1024];

    // default
    FileName = "";
    FileNameOut = "";
    format = "u8 ";
    formatasm = "b";
    formatint = 1;
    total = 0;
    align = 2;
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

    if (!FileNameOut[0]) FileNameOut = strdup(FileName);

    ext = getFileExtension(FileName);

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

    // start by setting buffer to nullfill
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
