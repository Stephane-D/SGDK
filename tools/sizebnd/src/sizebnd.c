#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
    int ii;
    int needed;
    int sizealign;
    int nullfill;
    char *FileName;
    FILE *FileInput;

    // default
    FileName = "";
    sizealign = 256;
    nullfill = 0;

    // parse parmeters
    for (ii=1; ii<argc; ii++)
    {
        if (!strcmp(argv[ii], "-sizealign"))
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
    }

    FileInput = fopen(FileName, "rb");
    if (!FileInput)
    {
        printf("Couldn't open input file %s\n", FileName);
        return 1;
    }
    else
    {
        // file exists, close and reopen for appending
        fclose(FileInput);
        FileInput = fopen(FileName, "ab");
    }

    // go to end of file
    fseek(FileInput, 0, SEEK_END);
    // calculate how many extra byte are needed
    needed = ftell(FileInput) & (sizealign - 1);

    if (needed)
    {
        unsigned char *mem;
        // complete missing bytes
        needed = sizealign - needed;
        mem = malloc(needed);
        if (mem)
        {
            memset(mem, nullfill, needed);
            fwrite(mem, needed, 1, FileInput);
            free(mem);
        }
        else
            for (ii=0; ii<needed; ii++) fputc(nullfill, FileInput);
    }

    fclose(FileInput);

    return 0;
}
