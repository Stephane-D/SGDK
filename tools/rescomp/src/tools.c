#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "../inc/rescomp.h"
#include "../inc/tools.h"
#include "../inc/img_tools.h"


//#ifdef _WIN32
//  #define FILE_SEPARATOR              "\\"
//  #define FILE_SEPARATOR_CHAR         '\\'
//#else
//    #ifdef _WIN64
//      #define FILE_SEPARATOR          "\\"
//      #define FILE_SEPARATOR_CHAR     '\\'
//    #else
//      #define FILE_SEPARATOR          "/"
//      #define FILE_SEPARATOR_CHAR     '/'
//    #endif
//#endif
//

// forward
static int appack(char* fin, char* fout);
static int lz4wpack(char* fin, char* fout);

unsigned int swapNibble32(unsigned int value)
{
    return swapNibble16(value >> 16) | (swapNibble16(value) << 16);
}

unsigned short swapNibble16(unsigned short value)
{
    return swapNibble8(value >> 8) | (swapNibble8(value) << 8);
}

unsigned char swapNibble8(unsigned char value)
{
    return (value >> 4) | (value << 4);
}

unsigned short toVDPColor(unsigned char b, unsigned char g, unsigned char r)
{
    // genesis only define on 3 bits (but shifted to 1 left)
    r = (r >> 4) & 0xE;
    g = (g >> 4) & 0xE;
    b = (b >> 4) & 0xE;

    return (r << 0) | (g << 4) | (b << 8);
}

char* strupper(char* text)
{
    char* c;

    c = text;
    while(*c)
    {
        *c = toupper(*c);
        c++;
    }

    return text;
}

void strreplace(char* text, char chr, char repl_chr)
{
    char* src;

    src = text;
    while(*src)
    {
        if (*src == chr)
            *src = repl_chr;
        src++;
    }
}

//int isAbsolutePathSystem(char* path)
//{
//    int len = strlen(path);
//
//    if (len > 0)
//    {
//        if (path[0] == FILE_SEPARATOR_CHAR) return TRUE;
//
//        if (len > 2)
//        {
//            // windows
//            if ((path[1] == ':') && (path[2] == FILE_SEPARATOR_CHAR))  return TRUE;
//        }
//    }
//
//    return FALSE;
//}

int isAbsolutePath(char* path)
{
    int len = strlen(path);

    if (len > 0)
    {
        // unix
        if (path[0] == '/') return TRUE;
        // windows
        if (path[0] == '\\') return TRUE;

        if (len > 2)
        {
            // windows
            if (path[1] == ':')
            {
                if (path[2] == '\\')  return TRUE;

                if (path[2] == '/')  return TRUE;
            }
        }
    }

    return FALSE;
}

//char* getDirectorySystem(char* path)
//{
//    char* result = strdup(path);
//    char* fdir = strrchr(result, FILE_SEPARATOR_CHAR);
//
//    if (fdir) *fdir = 0;
//    else
//    {
//        fdir = strrchr(result, ':');
//
//        if (fdir) fdir[1] = 0;
//        // no directory
//        else return strdup("");
//    }
//
//    return result;
//}

char* getDirectory(char* path)
{
    char* result = strdup(path);
    char* fdir = strrchr(result, '/');

    if (fdir) *fdir = 0;
    else
    {
        fdir = strrchr(result, '\\');

        if (fdir) *fdir = 0;
        else
        {
            fdir = strrchr(result, ':');

            if (fdir) fdir[1] = 0;
            // no directory
            else return strdup("");
        }
    }

    return result;
}

//char* getFilenameSystem(char* path)
//{
//    char* fname = strrchr(path, FILE_SEPARATOR_CHAR);
//
//    if (fname) return fname + 1;
//    else
//    {
//        fname = strrchr(path, ':');
//
//        if (fname) return fname + 1;
//    }
//
//    return path;
//}

char* getFilename(char* path)
{
    char* fname = strrchr(path, '/');

    if (fname) return fname + 1;
    else
    {
        fname = strrchr(path, '\\');

        if (fname) return fname + 1;
        else
        {
            fname = strrchr(path, ':');

            if (fname) return fname + 1;
        }
    }

    return path;
}

char* getFileExtension(char* path)
{
    char* fext = strrchr(path, '.');

    if (fext) return fext + 1;

    // equivalent to ""
    return path + strlen(path);
}

void removeExtension(char* path)
{
    char* fext = strrchr(path, '.');

    if (fext) *fext = 0;
}

//void adjustPathSystem(char* dir, char* path, char* dst)
//{
//    if (isAbsolutePathSystem(path)) strcpy(dst, path);
//    else
//    {
//        if (strlen(dir) > 0)
//        {
//            strcpy(dst, dir);
//            strcat(dst, FILE_SEPARATOR);
//            strcat(dst, path);
//        }
//        else strcpy(dst, path);
//    }
//}

void adjustPath(char* dir, char* path, char* dst)
{
    if (isAbsolutePath(path)) strcpy(dst, path);
    else
    {
        if (strlen(dir) > 0)
        {
            strcpy(dst, dir);
            strcat(dst, "/");
            strcat(dst, path);
        }
        else strcpy(dst, path);
    }
}

unsigned int getFileSize(char* file)
{
    unsigned int len;
    FILE * f;

    f = fopen(file, "rb");
    fseek(f, 0, SEEK_END);
    len = (unsigned long)ftell(f);
    fclose(f);

    return len;
}

unsigned char *readFile(char *fileName, int *size)
{
    FILE *f;
    unsigned char *data;

    f = fopen(fileName, "rb");

    if (!f)
    {
        printf("Error: Couldn't open input file %s\n", fileName);
        // error
        return NULL;
    }

    *size = getFileSize(fileName);

    if (*size == 0)
    {
        printf("Empty file %s\n", fileName);
        // error
        return NULL;
    }

    data = malloc(*size);
    fread(data, 1, *size, f);
    fclose(f);

    return data;
}

int writeFile(char* filename, unsigned char* data, int size)
{
    return out(data, 0, size, 1, FALSE, filename);
}

void unsign8b(unsigned char* data, int size)
{
    int i;

    for(i = 0; i < size; i++)
        data[i] += 0x80;
}

unsigned char* sizeAlign(unsigned char* data, int size, int align, unsigned char fill, int *outSize)
{
    int newSize;
    unsigned char* result;

    newSize = (size + (align - 1));
    newSize /= align;
    newSize *= align;
    *outSize = newSize;

    result = malloc(newSize);

    memcpy(result, data, size);
    memset(result + size, fill, newSize - size);

    free(data);

    return result;
}


unsigned char* in(char *in, int *outSize)
{
    FILE *fin;
    unsigned char *result;

    fin = fopen(in, "rb");

    if (!fin)
    {
        printf("Error: Couldn't open input file %s\n", in);

        // error
        *outSize = 0;
        return NULL;
    }

    result = inEx(fin, 0, getFileSize(in), outSize);
    fclose(fin);

    return result;
}

unsigned char* inEx(FILE* fin, int inOffset, int size, int *outSize)
{
    unsigned char* result;

    // get end of file position
    fseek(fin, 0, SEEK_END);
    // calculate size of output buffer
    *outSize = ftell(fin) - inOffset;

    // nothing to read
    if (*outSize <= 0) return NULL;

    // alloc out buffer
    result = malloc(*outSize);
    // and read
    *outSize = inEx2(fin, inOffset, size, result, 0);

    return result;
}

int inEx2(FILE* fin, int inOffset, int size, unsigned char* dest, int outOffset)
{
    unsigned char* d;
    int remain, l;

    fseek(fin, inOffset, SEEK_SET);
    d = dest + outOffset;

    remain = size;

    while (remain > 0)
    {
        l = fread(d, 1, size, fin);
        d += l;
        remain -= l;
    }

    return size - remain;
}

int out(unsigned char* data, int inOffset, int size, int intSize, int swap, char* out)
{
    int result;
    FILE *fout;

    fout = fopen(out, "wb");

    if (!fout)
    {
        printf("Error: Couldn't create output file %s\n", out);

        // error
        return FALSE;
    }

    result = outEx(data, inOffset, size, intSize, swap, fout, 0);
    fclose(fout);

    return result;
}

int outEx(unsigned char* data, int inOffset, int size, int intSize, int swap, FILE* fout, int outOffset)
{
    unsigned char* s;
    int remain, l;
    unsigned int v;

    fseek(fout, outOffset, SEEK_SET);
    s = data + inOffset;

    remain = size;

    while (remain > 0)
    {
        switch(intSize)
        {
            default:
                v = s[0];
                break;

            case 2:
                if (swap) v = (s[0] << 8) | (s[1] << 0);
                else v = (s[0] << 0) | (s[1] << 8);
                break;

            case 4:
                if (swap) v = (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | (s[3] << 0);
                else v = (s[0] << 0) | (s[1] << 8) | (s[2] << 16) | (s[3] << 24);
                break;
        }

        l = fwrite(&v, 1, intSize, fout);
        s += l;
        remain -= l;
    }

    if (remain != 0) return FALSE;
    else return TRUE;
}


void decl(FILE* fs, FILE* fh, char* type, char* name, int align, int global)
{
    // asm declaration
    fprintf(fs, "    .align %d\n", (align < 2)?2:align);

    if (global)
        fprintf(fs, "    .global %s\n", name);

    fprintf(fs, "%s:\n", name);

    // include declaration
    if (global)
        fprintf(fh, "extern const %s %s;\n", type, name);
}

void declArray(FILE* fs, FILE* fh, char* type, char* name, int size, int align, int global)
{
    // asm declaration
    fprintf(fs, "    .align  %d\n", (align < 2)?2:align);

    if (global)
        fprintf(fs, "    .global %s\n", name);

    fprintf(fs, "%s:\n", name);

    // include declaration
    if (global)
        fprintf(fh, "extern const %s %s[%d];\n", type, name, size);
}


void outS(unsigned char* data, int inOffset, int size, FILE* fout, int intSize)
{
    char* const formatAsm[] = {"b", "b", "w", "w", "d"};

    int ii, jj;
    int offset = inOffset;
    // align remain on word
    int remain = ((size + 1) / 2) * 2;
    int adjIntSize = (intSize < 2)?2:intSize;

    while (remain > 0)
    {
        fprintf(fout, "    dc.%s    ", formatAsm[adjIntSize]);

        for (ii = 0; ii < MIN(16, remain) / adjIntSize; ii++)
        {
            if (ii) fprintf(fout, ", ");

            fprintf(fout, "0x");

            if (intSize == 1)
            {
                // we cannot use byte data because of GCC bugs with -G parameter
                fprintf(fout, "%02X", data[offset + 0]);

                if ((offset - inOffset) > (size + 1))
                    fprintf(fout, "00");
                else
                    fprintf(fout, "%02X", data[offset + 1]);

                offset += adjIntSize;
            }
            else
            {
                offset += adjIntSize;

                for (jj = 0; jj < adjIntSize; jj++)
                    fprintf(fout, "%02X", data[offset - (jj + 1)]);
            }
        }

        fprintf(fout, "\n");
        remain -= 16;
    }
}

int getDriver(char *str)
{
    char *upstr = strupper(str);

    if (!strcmp(upstr, "PCM") || !strcmp(upstr, "0")) return DRIVER_PCM;
    if (!strcmp(upstr, "2ADPCM") || !strcmp(upstr, "1")) return DRIVER_2ADPCM;
    if (!strcmp(upstr, "4PCM") || !strcmp(upstr, "2") || !strcmp(upstr, "3")) return DRIVER_4PCM;
    if (!strcmp(upstr, "VGM") || !strcmp(upstr, "4")) return DRIVER_VGM;
    if (!strcmp(upstr, "XGM") || !strcmp(upstr, "5")) return DRIVER_XGM;

    return 0;
}

int getCompression(char *str)
{
    char *upstr = strupper(str);

    if (!strcmp(upstr, "AUTO") || !strcmp(upstr, "BEST") || !strcmp(upstr, "-1")) return PACK_AUTO;
    if (!strcmp(upstr, "NONE") || !strcmp(upstr, "0")) return PACK_NONE;
    if (!strcmp(upstr, "APLIB") || !strcmp(upstr, "1")) return PACK_APLIB;
    if (!strcmp(upstr, "LZ4W") || !strcmp(upstr, "2") || !strcmp(upstr, "FAST")) return PACK_LZ4W;

    // not recognized --> use AUTO
    if (strlen(upstr) > 0) return PACK_AUTO;

    return PACK_NONE;
}

unsigned char *pack(unsigned char* data, int inOffset, int size, int *outSize, int *method)
{
    return packEx(data, inOffset, size, 1, outSize, method);
}

unsigned char *packEx(unsigned char* data, int inOffset, int size, int intSize, int *outSize, int *method)
{
    unsigned char* src;
    unsigned char* result;
    unsigned char* results[PACK_MAX_IND + 1];
    int sizes[PACK_MAX_IND + 1];
    int minSize;
    int autoSelect;
    int i;

    // init no compression infos
    results[0] = data;
    sizes[0] = size;

    // create out file from data input
    if (!out(data, inOffset, size, intSize, (intSize > 1)?TRUE:FALSE, "pack.in"))
    {
        // error
        *outSize = 0;
        return NULL;
    }

    // get source data arranged if needed
    src = in("pack.in", outSize);
    if (src == NULL)
    {
        *outSize = 0;
        return NULL;
    }

    // init results (use 'data' as 'src' can have different endianess)
    for(i = 1; i <= PACK_MAX_IND; i++)
        results[i] = NULL;

    // select best compression scheme
    autoSelect = (*method == PACK_AUTO);

    for(i = 1; i <= PACK_MAX_IND; i++)
    {
        if (autoSelect || (*method == i))
        {
            switch(i)
            {
                case PACK_APLIB:
                    if (appack("pack.in", "pack.out"))
                        results[PACK_APLIB] = in("pack.out", &sizes[PACK_APLIB]);
                    break;

                case PACK_LZ4W:
                    if (lz4wpack("pack.in", "pack.out"))
                        results[PACK_LZ4W] = in("pack.out", &sizes[PACK_LZ4W]);
                    break;
            }
        }
    }

    // find best compression
    minSize = sizes[0];
    result = results[0];
    *method = PACK_NONE;
    for(i = 1; i <= PACK_MAX_IND; i++)
    {
        if (results[i] != NULL)
        {
            if (sizes[i] < minSize)
            {
                minSize = sizes[i];
                result = results[i];
                *method = i;
            }
        }
    }

    switch(*method)
    {
        case PACK_APLIB:
            printf("Packed with APLIB, ");
            break;

        case PACK_LZ4W:
            printf("Packed with LZ4W, ");
            break;

        default:
            printf("No compression, ");
    }

    printf("original size = %d compressed to %d (%g %%)\n", size, minSize, (minSize * 100.0) / (float) size);

    // update out size
    *outSize = minSize;

    // release unused buffers
    for(i = 0; i <= PACK_MAX_IND; i++)
    {
        if ((results[i] != NULL) && (results[i] != result))
            free(results[i]);
    }

    // clean
    remove("pack.in");
    remove("pack.out");

    return result;
}

int maccer(char* fin, char* fout)
{
    char cmd[MAX_PATH_LEN * 2];
    FILE *f;

    // better to remove output file for appack
    remove(fout);

    // command
//    adjustPathSystem(currentDirSystem, "mac68k", cmd);
    adjustPath(currentDir, "mac68k", cmd);

    // arguments
    strcat(cmd, " -o \"");
    strcat(cmd, fout);
    strcat(cmd, "\" \"");
    strcat(cmd, fin);
    strcat(cmd, "\"");

    printf("Executing %s\n", cmd);

    system(cmd);

    // we test for the out file existence
    f = fopen(fout, "rb");
    fclose(f);

    // file exist --> ok
    if (f != NULL) return TRUE;

    return FALSE;
}

int tfmcom(char* fin, char* fout)
{
    char cmd[MAX_PATH_LEN * 2];
    FILE *f;

    // better to remove output file for tfmcom
    remove(fout);

    // command
//    adjustPathSystem(currentDirSystem, "tfmcom", cmd);
    adjustPath(currentDir, "tfmcom", cmd);

    // arguments
    strcat(cmd, " \"");
    strcat(cmd, fin);
    strcat(cmd, "\"");

    printf("Executing %s\n", cmd);

    system(cmd);

    // clean
    remove("tempm0");
    remove("tempm1");
    remove("tempm2");
    remove("tempm3");
    remove("tempm4");
    remove("tempm5");
    remove("tempm6");
    remove("tempm7");
    remove("tempm8");
    remove("tempm9");
    remove("tempma");
    remove("tempmb");
    remove("tempmc");
    remove("tempmd");
    remove("tempme");
    remove("tempmf");

    remove("tempt0");
    remove("tempt1");
    remove("tempt2");
    remove("tempt3");
    remove("tempt4");
    remove("tempt5");
    remove("tempt6");
    remove("tempt7");
    remove("tempt8");
    remove("tempt9");
    remove("tempta");
    remove("temptb");
    remove("temptc");
    remove("temptd");
    remove("tempte");
    remove("temptf");

    remove("temptfm0");
    remove("temptfm1");
    remove("temptfm2");
    remove("temptfm3");
    remove("temptfm4");
    remove("temptfm5");
    remove("temptfm6");
    remove("temptfm7");
    remove("temptfm8");
    remove("temptfm9");
    remove("temptfma");
    remove("temptfmb");
    remove("temptfmc");
    remove("temptfmd");
    remove("temptfme");
    remove("temptfmf");

    f = fopen(fout, "rb");
    fclose(f);

    // file exist --> ok
    if (f != NULL) return TRUE;

    return FALSE;
}


static int appack(char* fin, char* fout)
{
    char cmd[MAX_PATH_LEN * 2];
    FILE *f;

    // better to remove output file for appack
    remove(fout);

    // command
//    adjustPathSystem(currentDirSystem, "appack", cmd);
    adjustPath(currentDir, "appack", cmd);

    // arguments
    strcat(cmd, " c \"");
    strcat(cmd, fin);
    strcat(cmd, "\" \"");
    strcat(cmd, fout);
    strcat(cmd, "\"");
    strcat(cmd, " -s");

    printf("Executing %s\n", cmd);

    system(cmd);

    // we test for the out file existence
    f = fopen(fout, "rb");
    fclose(f);

    // file exist --> ok
    if (f != NULL) return TRUE;

    return FALSE;
}

static int lz4wpack(char* fin, char* fout)
{
    char tmp[MAX_PATH_LEN * 2];
    char cmd[MAX_PATH_LEN * 2];
    FILE *f;

    // better to remove output file for uftcpack
    remove(fout);

    // command
//    adjustPathSystem(currentDirSystem, "java -jar lz4w.jar", cmd);

    adjustPath(currentDir, "lz4w.jar", tmp);
    strcpy(cmd, "java -jar ");
    strcat(cmd, tmp);

    // arguments
    strcat(cmd, " p \"");
    strcat(cmd, fin);
    strcat(cmd, "\" \"");
    strcat(cmd, fout);
    strcat(cmd, "\" -s");

    printf("Executing %s\n", cmd);

    system(cmd);

    // we test for the out file existence
    f = fopen(fout, "rb");
    fclose(f);

    // file exist --> ok
    if (f != NULL) return TRUE;

    return FALSE;
}
