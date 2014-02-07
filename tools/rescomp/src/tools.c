#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../inc/rescomp.h"
#include "../inc/tools.h"
#include "../inc/img_tools.h"


#ifdef _WIN32
#define FILE_SEPARATOR              "\\"
#define FILE_SEPARATOR_CHAR         '\\'
#else
    #ifdef _WIN64
    #define FILE_SEPARATOR          "\\"
    #define FILE_SEPARATOR_CHAR     '\\'
    #else
    #define FILE_SEPARATOR          "/"
    #define FILE_SEPARATOR_CHAR     '/'
    #endif
#endif


// forward
static int appack(char* fin, char* fout);
//static int lzknpack(char* fin, char* fout);
static unsigned char *rlepack(unsigned char* src, int inSize, int *outSize);
static unsigned char *rlemappack(unsigned char* src, int inSize, int *outSize);


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


int isAbsolutePathSystem(char* path)
{
    int len = strlen(path);

    if (len > 0)
    {
        if (path[0] == FILE_SEPARATOR_CHAR) return TRUE;

        if (len > 2)
        {
            // windows
            if ((path[1] == ':') && (path[2] == FILE_SEPARATOR_CHAR))  return TRUE;
        }
    }

    return FALSE;
}

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

char* getDirectorySystem(char* path)
{
    char* result = strdup(path);
    char* fdir = strrchr(result, FILE_SEPARATOR_CHAR);

    if (fdir) *fdir = 0;
    else
    {
        fdir = strrchr(result, ':');

        if (fdir) fdir[1] = 0;
        // no directory
        else return strdup("");
    }

    return result;
}

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

char* getFilenameSystem(char* path)
{
    char* fname = strrchr(path, FILE_SEPARATOR_CHAR);

    if (fname) return fname + 1;
    else
    {
        fname = strrchr(path, ':');

        if (fname) return fname + 1;
    }

    return path;
}

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

void adjustPathSystem(char* dir, char* path, char* dst)
{
    if (isAbsolutePathSystem(path)) strcpy(dst, path);
    else
    {
        if (strlen(dir) > 0)
        {
            strcpy(dst, dir);
            strcat(dst, FILE_SEPARATOR);
            strcat(dst, path);
        }
        else strcpy(dst, path);
    }
}

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

    f = fopen(file, "r");
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
    fprintf(fs, "    .align  %d\n", align);

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
    fprintf(fs, "    .align  %d\n", align);

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
    int remain = size;

    while (remain > 0)
    {
        fprintf(fout, "    dc.%s    ", formatAsm[intSize]);

        for (ii = 0; ii < MIN(16, remain) / intSize; ii++)
        {
            if (ii) fprintf(fout, ", ");

            fprintf(fout, "0x");

            offset += intSize;

            for (jj = 0; jj < intSize; jj++)
                fprintf(fout, "%02X", data[offset - (jj + 1)]);
        }

        fprintf(fout, "\n");
        remain -= 16;
    }
}

//void outSNibble(unsigned char* data, int inOffset, int size, FILE* fout, int intSize, int swapNibble)
//{
//    char* const formatAsm[] = {"b", "b", "w", "w", "d"};
//
//    int ii, jj;
//    int offset = 0;
//    int remain = size;
//
//    while (remain > 0)
//    {
//        fprintf(fout, "    dc.%s    ", formatAsm[intSize]);
//
//        for (ii = 0; ii < MIN(16, remain) / intSize; ii++)
//        {
//            if (ii) fprintf(fout, ", ");
//
//            fprintf(fout, "0x");
//
//            offset += intSize;
//
//            for (jj = 0; jj < intSize; jj++)
//            {
//                int value;
//                int invOff = offset - (jj + 1);
//
//                if (swapNibble)
//                {
//                    value = (data[inOffset + (invOff * 2) + 0] & 0x0F) << 4;
//                    value |= (data[inOffset + (invOff * 2) + 1] & 0x0F) << 0;
//                }
//                else
//                {
//                    value = (data[inOffset + (invOff * 2) + 0] & 0x0F) << 0;
//                    value |= (data[inOffset + (invOff * 2) + 1] & 0x0F) << 4;
//                }
//
//                fprintf(fout, "%02X", value);
//            }
//        }
//
//        fprintf(fout, "\n");
//        remain -= 16;
//    }
//}

void outSValue(unsigned char value, int size, FILE* fout)
{
    int ii;
    int remain = size;

    while (remain > 0)
    {
        fprintf(fout, "    dc.b    ");

        for (ii = 0; ii < MIN(16, remain); ii++)
        {
            if (ii) fprintf(fout, ", ");

            fprintf(fout, "0x%02X", value);
        }

        fprintf(fout, "\n");
        remain -= 16;
    }
}



unsigned char *pack(unsigned char* data, int inOffset, int size, int *outSize, int *method)
{
    return packEx(data, inOffset, size, 1, FALSE, outSize, method);
}

unsigned char *packEx(unsigned char* data, int inOffset, int size, int intSize, int swap, int *outSize, int *method)
{
    unsigned char* src;
    unsigned char* result1;
    unsigned char* result2;
    unsigned char* result3;
    unsigned char* result4;
    unsigned char* result;
    int size1, size2, size3, size4;
    int minSize;
    int autoSelect;

    // create out file from data input
    if (!out(data, inOffset, size, intSize, swap, "pack.in"))
    {
        // error
        *outSize = 0;
        return NULL;
    }

    // get source data arranger if needed
    src = in("pack.in", &size1);
    if (src == NULL)
    {
        *outSize = 0;
        return NULL;
    }

    // select best compression scheme
    autoSelect = (*method == PACK_AUTO);
    // initialize min size with original size
    minSize = size;
    size1 = size + 1;
    size2 = size + 1;
    size3 = size + 1;
    size4 = size + 1;
    result1 = NULL;
    result2 = NULL;
    result3 = NULL;
    result4 = NULL;

    if (autoSelect || (*method == PACK_APLIB))
    {
        if (appack("pack.in", "pack1.out"))
        {
            result1 = in("pack1.out", &size1);
            minSize = MIN(minSize, size1);
        }
    }

    if (autoSelect || (*method == PACK_RLE))
    {
        result2 = rlepack(src, size, &size2);
        if (result2 != NULL)
            minSize = MIN(minSize, size2);
    }

    if (autoSelect || (*method == PACK_MAP_RLE))
    {
        result3 = rlemappack(src, size, &size3);
        if (result3 != NULL)
            minSize = MIN(minSize, size3);
    }

//    if (autoSelect || (*method == PACK_LZKN))
//    {
//        if (lzknpack("pack.in", "pack4.out"))
//        {
//            result4 = in("pack4.out", &size4);
//            minSize = MIN(minSize, size4);
//        }
//    }

    if (minSize == size1)
    {
        *method = PACK_APLIB;
        result = result1;
        free(src);
        if (result2) free(result2);
        if (result3) free(result3);
        if (result4) free(result4);
        printf("Packed with APLIB, original size = %d compressed to %d (%g %%)\n", size, minSize, (minSize * 100.0) / (float) size);
    }
    else if (minSize == size2)
    {
        *method = PACK_RLE;
        result = result2;
        free(src);
        if (result1) free(result1);
        if (result3) free(result3);
        if (result4) free(result4);
        printf("Packed with RLE, original size = %d compressed to %d (%g %%)\n", size, minSize, (minSize * 100.0) / (float) size);
    }
    else if (minSize == size3)
    {
        *method = PACK_MAP_RLE;
        result = result3;
        free(src);
        if (result1) free(result1);
        if (result2) free(result2);
        if (result4) free(result4);
        printf("Packed with MAP RLE, original size = %d compressed to %d (%g %%)\n", size, minSize, (minSize * 100.0) / (float) size);
    }
//    else if (minSize == size4)
//    {
//        *method = PACK_LZKN;
//        result = result4;
//        free(src);
//        if (result1) free(result1);
//        if (result2) free(result2);
//        if (result3) free(result3);
//        printf("Packed with LZKN, original size = %d compressed to %d (%g %%)\n", size, minSize, (minSize * 100.0) / (float) size);
//    }
    else
    {
        *method = PACK_NONE;
        result = src;
        if (result1) free(result1);
        if (result2) free(result2);
        if (result3) free(result3);
        if (result4) free(result4);
        printf("Could not pack, original data remain (size = %d)\n", size);
    }

    // update out size
    *outSize = minSize;

    // clean
    remove("pack.in");
    remove("pack1.out");
    remove("pack4.out");

    return result;
}

int maccer(char* fin, char* fout)
{
    char cmd[MAX_PATH_LEN * 2];
    FILE *f;

    // better to remove output file for appack
    remove(fout);

    // command
    adjustPathSystem(currentDirSystem, "mac68k", cmd);

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

    // better to remove output file for appack
    remove(fout);

    // command
    adjustPathSystem(currentDirSystem, "tfmcom", cmd);

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
    adjustPathSystem(currentDirSystem, "appack", cmd);

    // arguments
    strcat(cmd, " c \"");
    strcat(cmd, fin);
    strcat(cmd, "\" \"");
    strcat(cmd, fout);
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

//static int lzknpack(char* fin, char* fout)
//{
//    char cmd[MAX_PATH_LEN * 2];
//    FILE *f;
//
//    // better to remove output file for appack
//    remove(fout);
//
//    // command
//    adjustPathSystem(currentDirSystem, "lzknpack", cmd);
//
//    // arguments
//    strcat(cmd, " \"");
//    strcat(cmd, fin);
//    strcat(cmd, "\" \"");
//    strcat(cmd, fout);
//    strcat(cmd, "\"");
//
//    printf("Executing %s\n", cmd);
//
//    system(cmd);
//
//    // we test for the out file existence
//    f = fopen(fout, "rb");
//    fclose(f);
//
//    // file exist --> ok
//    if (f != NULL) return TRUE;
//
//    return FALSE;
//}

/**
 * RLE compressor, original code by Charles MacDonald
 * WWW: http://cgfm2.emuviews.com
 **/
static unsigned char *rlepack(unsigned char *src, int inSize, int *outSize)
{
    unsigned char *nib;
    unsigned char *result;
    int offset;
    int niblen;
    int in_run;
    int bloc_num;
    int i;
    unsigned char data;
    unsigned char rle;

    // source size have to be aligned on 4 bytes (tile data)
    if (inSize & 3) return NULL;

    niblen = inSize * 2;

    // Output data buffer (worst case is 1 byte for each nibble, 2x size)
    result = (unsigned char *) malloc(niblen);
    if (result == NULL)
    {
        printf("rlepack failed: could not allocate memory !\n");
        return NULL;
    }

    // convert nibble to byte for easier manipulation
    nib = to8bpp(src, inSize);

    bloc_num = 0;
    data = 0;
    rle = 0;
    in_run = 0;
    // reserve 2 bytes for size
    offset = 2;
    for(i = 0; i < niblen; i++)
    {
        // Are we encoding a run?
        if (in_run)
        {
            // Data has changed, write run and start new one
            if (data != nib[i])
            {
                result[offset++] = ((rle << 4) | data);
                bloc_num++;
                data = nib[i];
                rle = 0;
            }
            else
            {
                // Max run length reached, write run and stop
                if (++rle == 0x0F)
                {
                    result[offset++] = (rle << 4 | data);
                    bloc_num++;
                    in_run = 0;
                }
            }
        }
        else
        {
            // Start new run
            in_run = 1;
            data = nib[i];
            rle = 0;
        }
    }

    // If still in a run, write it result
    if (in_run)
    {
        result[offset++] = (rle << 4 | data);
        bloc_num++;
    }

    // bloc number > 64KB
    if (bloc_num >= 0x10000)
    {
        printf("rlemappack failed: more than 2^16 bloc (%d)\n", bloc_num);
        free(result);
        return NULL;
    }

    // Assign result size and return result
    *outSize = offset;

    // set bloc number
    result[0] = bloc_num >> 8;
    result[1] = bloc_num >> 0;

    free(nib);

    return (result);
}

static unsigned char *rlemappack(unsigned char *src, int inSize, int *outSize)
{
    unsigned char *result;
    int outOffset, inOffset;
    int rle, rle_type, in_run;
    int bloc_num;
    unsigned short data;

    // source size have to be aligned on 2 bytes (map data)
    if (inSize & 1) return NULL;

    // Output data buffer (allocate double of input buffer)
    result = (unsigned char *) malloc(inSize * 2);
    if (result == NULL)
    {
        printf("rlemappack failed: could not allocate memory !\n");
        return NULL;
    }

    bloc_num = 0;
    rle = 0;
    data = 0;
    in_run = 0;
    rle_type = 0;
    // reserve 2 bytes for size
    outOffset = 2;
    for(inOffset = 0; inOffset < inSize; inOffset += 2)
    {
        unsigned short value = (src[inOffset + 1] << 0) | (src[inOffset + 0] << 8);

        // Are we encoding a run ?
        if (in_run)
        {
            // first pack method
            int pack1 = (data == value) && !rle_type;
            // second pack method
            int pack2 = ((rle == 0) || rle_type) && ((data + rle + 1) == value);

            // data can be compressed
            if (pack1 || pack2)
            {
                // second method of compression
                if (pack2) rle_type = 0x80;

                // Max run length reached, write run and stop
                if (++rle == 0x7F)
                {
                    result[outOffset++] = rle_type | rle;
                    result[outOffset++] = data >> 8;
                    result[outOffset++] = data >> 0;
                    bloc_num++;
                    in_run = 0;
                }
            }
            else
            {
                // data has changed, write run & data and start new one
                result[outOffset++] = rle_type | rle;
                result[outOffset++] = data >> 8;
                result[outOffset++] = data >> 0;
                bloc_num++;
                data = value;
                rle = 0;
                rle_type = 0;
            }
        }
        else
        {
            // Start new run
            in_run = 1;
            data = value;
            rle = 0;
            rle_type = 0;
        }
    }

    // If still in a run, write it result
    if (in_run)
    {
        result[outOffset++] = rle_type | rle;
        result[outOffset++] = data >> 8;
        result[outOffset++] = data >> 0;
        bloc_num++;
    }

    // bloc number > 64KB
    if (bloc_num >= 0x10000)
    {
        printf("rlemappack failed: more than 2^16 bloc (%d)\n", bloc_num);
        free(result);
        return NULL;
    }

    // Assign result size and return result
    *outSize = outOffset;

    // set bloc number
    result[0] = bloc_num >> 8;
    result[1] = bloc_num >> 0;

    return (result);
}
