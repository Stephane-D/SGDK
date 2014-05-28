#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>


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


int main(int argc, char **argv)
{
    int ii, jj;
    int len;
    int total;
    int align;
    int formatint;
    int formatintasm;
    int nullfill;
    char *format;
    char *formatasm;
    char *shortname;
    char *ext;
    char *FileName;
    char *FileNameOut;
    FILE *FileInput;
    FILE *FileOutput;
    char path[4096];
    unsigned char temp[16];

    // default
    FileName = "";
    FileNameOut = "";
    format = "u8";
    formatint = 1;
    formatasm = "w";
    formatintasm = 2;
    total = 0;
    align = 2;
    nullfill = 0;

    // parse parmeters
    for (ii = 1; ii < argc; ii++)
    {
        if (!strcmp(argv[ii], "-u8"))
        {
            format = "u8";
            formatint = 1;
            formatasm = "w";
            formatintasm = 2;
        }
        else if (!strcmp(argv[ii], "-s8"))
        {
            format = "s8";
            formatint = 1;
            formatasm = "w";
            formatintasm = 2;
        }
        else if (!strcmp(argv[ii], "-u16"))
        {
            format = "u16";
            formatint = 2;
            formatasm = "w";
            formatintasm = 2;
        }
        else if (!strcmp(argv[ii], "-s16"))
        {
            format = "s16";
            formatint = 2;
            formatasm = "w";
            formatintasm = 2;
        }
        else if (!strcmp(argv[ii], "-u32"))
        {
            format = "u32";
            formatint = 4;
            formatasm = "l";
            formatintasm = 4;
        }
        else if (!strcmp(argv[ii], "-s32"))
        {
            format = "s32";
            formatint = 4;
            formatasm = "l";
            formatintasm = 4;
        }
        else if (!strcmp(argv[ii], "-align"))
        {
            ii++;
            align = strtoimax(argv[ii], NULL, 0);

            if (!align) align = 2;
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
    strcpy(path, FileNameOut);
    strcat(path, ".s");
    FileOutput = fopen(path, "w");

    if (!FileOutput)
    {
        fclose(FileInput);
        printf("Couldn't open output file %s\n", path);
        return 1;
    }

    // force align on 2
    if (align < 2) align = 2;

    fprintf(FileOutput, ".text\n\n");

    fprintf(FileOutput, "    .align  %d\n\n", align);
    fprintf(FileOutput, "    .global %s\n", shortname);
    fprintf(FileOutput, "%s:\n", shortname);

    while (1)
    {
        // start by setting buffer to nullfill
        memset(temp, nullfill, sizeof(temp));

        len = fread(temp, 1, sizeof(temp), FileInput);
        total += len;

         // align length for size of units
        len = (len + (formatintasm - 1)) & ~(formatintasm - 1);

        if (len)
        {
            fprintf(FileOutput, "    dc.%s    ", formatasm);

            for (ii = 0; ii < (len / formatintasm); ii++)
            {
                if (ii)
                    fprintf(FileOutput, ",");

                fprintf(FileOutput, "0x");

                for (jj = 0; jj < formatintasm; jj++)
                    fprintf(FileOutput, "%02X", temp[(ii * formatintasm) + jj]);
            }

            fprintf(FileOutput, "\n");
        }

        if (len < 16)
            break;
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
    fprintf(FileOutput, "extern const %s %s[0x%X];\n\n", format, shortname, total / formatint);
    fprintf(FileOutput, "#endif // _%s_H_\n", temp);

    fclose(FileOutput);
    return 0;
}
