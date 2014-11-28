#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "../inc/util.h"


void initList(List* list)
{
    list->elements = NULL;
    list->allocated = 0;
    list->size = 0;
}

List* createList()
{
    List* result = malloc(sizeof(List));

    initList(result);

    return result;
}

void deleteList(List* list)
{
    if (list != NULL)
    {
        if (list->elements != NULL)
            free(list->elements);

        free(list);
    }
}

void clearList(List* list)
{
    if (list->elements != NULL)
    {
        free(list->elements);
        list->elements = NULL;
    }

    list->allocated = 0;
    list->size = 0;
}

void addToList(List* list, void* element)
{
    if (list->size >= list->allocated)
    {

        if (list->elements == NULL)
        {
            list->allocated = 256;
            list->elements = malloc(sizeof(void*) * list->allocated);
        }
        else
        {
            list->allocated *= 2;
            list->elements = realloc(list->elements, sizeof(void*) * list->allocated);
        }
    }

    list->elements[list->size++] = element;
}

void addAllToList(List* list, List* elements)
{
    int i;

    for(i = 0; i < elements->size; i++)
        addToList(list, elements->elements[i]);
}

void* getFromList(List* list, int index)
{
    if (index < list->size)
        return list->elements[index];

    return NULL;
}

void addToListEx(List* list, int index, void* element)
{
    int i;

    // add 1 slot
    addToList(list, NULL);

    for(i = list->size - 2; i >= index; i--)
        list->elements[i + 1] = list->elements[i + 0];

    setToList(list, index, element);
}

void addAllToListEx(List* list, int index, List* elements)
{
    int i;

    for(i = 0; i < elements->size; i++)
        addToListEx(list, index + i, elements->elements[i]);
}

void setToList(List* list, int index, void* element)
{
    list->elements[index] = element;
}

void* removeFromList(List* list, int index)
{
    if (index < list->size)
    {
        int i;
        void* result = getFromList(list, index);

        // remove 1 element
        list->size--;
        for(i = index; i < list->size; i++)
            list->elements[i + 0] = list->elements[i + 1];

        return result;
    }

    return NULL;
}

void** listToArray(List* list)
{
    void** result;

    if (list->size == 0)
    {
        result = malloc(sizeof(void*));
        *result = NULL;
    }
    else
    {
        int i;

        result = malloc(sizeof(void*) * list->size);

        for(i = 0; i < list->size; i++)
            result[i] = list->elements[i];
    }

    return result;
}


bool arrayEquals(unsigned char* array1, unsigned char* array2, int size)
{
    int i;

    for (i = 0; i < size; i++)
        if (array1[i] != array2[i])
            return false;

    return true;
}

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

unsigned short getShort(unsigned char* data, int offset)
{
    unsigned short res;

    res = data[offset + 0] << 0;
    res += data[offset + 1] << 8;

    return res;
}

unsigned int getInt16(unsigned char* data, int offset)
{
    unsigned int res;

    res = data[offset + 0] << 0;
    res += data[offset + 1]  << 8;

    return (int) res;
}

unsigned int getInt24(unsigned char* data, int offset)
{
    unsigned int res;

    res = data[offset + 0] << 0;
    res += data[offset + 1] << 8;
    res += data[offset + 2] << 16;

    return res;
}

unsigned int getInt(unsigned char* data, int offset)
{
    unsigned int res;

    res = data[offset + 0] << 0;
    res += data[offset + 1] << 8;
    res += data[offset + 2] << 16;
    res += data[offset + 3] << 24;

    return res;
}

void setInt(unsigned char* array, int offset, unsigned int value)
{
    array[offset + 0] = value >> 0;
    array[offset + 1] = value >> 8;
    array[offset + 2] = value >> 16;
    array[offset + 3] = value >> 24;
}

void setInt24(unsigned char* array, int offset, unsigned int value)
{
    array[offset + 0] = value >> 0;
    array[offset + 1] = value >> 8;
    array[offset + 2] = value >> 16;
}

void setInt16(unsigned char* array, int offset, unsigned int value)
{
    array[offset + 0] = value >> 0;
    array[offset + 1] = value >> 8;
}

char* getFileExtension(char* path)
{
    char* fext = strrchr(path, '.');

    if (fext) return fext + 1;

    // equivalent to ""
    return path + strlen(path);
}

unsigned int getFileSizeEx(FILE* f)
{
    unsigned int len;

    fseek(f, 0, SEEK_END);
    len = ftell(f);

    return len;
}

unsigned int getFileSize(char* file)
{
    unsigned int len;
    FILE * f;

    f = fopen(file, "rb");
    len = getFileSizeEx(f);
    fclose(f);

    return len;
}

unsigned char* readBinaryFile(char* fileName, int* size)
{
    FILE *f;
    unsigned char *data;

    f = fopen(fileName, "rb");

    if (!f)
    {
        printf("Error: couldn't open input file %s\n", fileName);
        // error
        return NULL;
    }

    *size = getFileSize(fileName);

    if (*size == 0)
    {
        printf("Error: empty file %s\n", fileName);
        // error
        return NULL;
    }

    data = malloc(*size);
    fread(data, 1, *size, f);
    fclose(f);

    return data;
}

bool writeBinaryFile(unsigned char* data, int size, char* fileName)
{
    return out(data, 0, size, 1, false, fileName);
}

unsigned char* inEx(FILE* fin, int inOffset, int size, int *outSize)
{
    unsigned char* result;

    // calculate size of output buffer
    *outSize = getFileSizeEx(fin) - inOffset;

    // nothing to read
    if (*outSize <= 0) return NULL;

    // alloc out buffer
    result = malloc(size);
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

bool out(unsigned char* data, int inOffset, int size, int intSize, bool swap, char* out)
{
    int result;
    FILE *fout;

    fout = fopen(out, "wb");

    if (!fout)
    {
        printf("Error: couldn't create output file %s\n", out);

        // error
        return false;
    }

    result = outEx(data, inOffset, size, intSize, swap, fout, 0);
    fclose(fout);

    return result;
}

bool outEx(unsigned char* data, int inOffset, int size, int intSize, bool swap, FILE* fout, int outOffset)
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

    if (remain != 0) return false;
    else return true;
}


unsigned char* resample(unsigned char* data, int offset, int len, int inputRate, int outputRate, int align, int* outSize)
{
    FILE* f = fopen("tmp.bin", "wb+");

    if (f == NULL)
    {
        printf("Error: cannot open file tmp.bin\n");
        return NULL;
    }

    const double step = (double) inputRate / (double) outputRate;

    double value;
    double lastSample;
    double sample = 0;
    unsigned char byte;
    double dOff;
    int off;
    int outOff;

    value = 0;
    lastSample = 0;
    off = 0;
    outOff = 0;
    for (dOff = 0; dOff < len; dOff += step)
    {
        sample = 0;

        // extrapolation
        if (step >= 1)
        {
            if (value < 0)
                sample += lastSample * -value;

            value += step;

            while (value > 0)
            {
                lastSample = (data[off + offset] & 0xFF) - 0x80;
                off++;

                if (value >= 1)
                    sample += lastSample;
                else
                    sample += lastSample * value;

                value--;
            }

            sample /= step;
        }
        else
        {
//            if (floor(dOff) == dOff)
                sample = (data[(int) dOff + offset] & 0xFF) - 0x80;
//            else
//            {
//                double sample0 = (data[(int) floor(dOff) + offset] & 0xFF) - 0x80;
//                double sample1 = (data[(int) ceil(dOff) + offset] & 0xFF) - 0x80;
//
//                sample += sample0 * (ceil(dOff) - dOff);
//                sample += sample1 * (dOff - floor(dOff));
//            }
        }

        byte = round(sample);
        fwrite(&byte, 1, 1, f);
        outOff++;
    }

    // do alignment
    if (align > 1)
    {
        const int mask = align - 1;
        const int size = align - (outOff & mask);

        if (size != align)
        {
            double reduce = sample / size;
            int i;

            for (i = 0; i < size; i++)
            {
                sample -= reduce;
                byte = round(sample);
                fwrite(&byte, 1, 1, f);
                outOff++;
            }
        }
    }

    unsigned char* result = inEx(f, 0, outOff, outSize);

    fclose(f);

    return result;
}
