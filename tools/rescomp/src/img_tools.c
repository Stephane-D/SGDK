#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "../inc/libpng.h"
#include "../inc/tools.h"
#include "../inc/img_tools.h"


#define BMP_WIDTH               0x12
#define BMP_HEIGHT              0x16
#define BMP_BPP                 0x1C
#define BMP_PALETTE_SIZE        0x2E
#define BMP_PALETTE_OFFSET      0x36

#define PNG_WIDTH               0x12
#define PNG_HEIGHT              0x16
#define PNG_BPP                 0x18


// forward
static int Bmp_getInfos(char* fileName, int *w, int *h, int *bpp);
static unsigned short *Bmp_getPalette(char* fileName, int *size);
static unsigned char *Bmp_getData(char* fileName, int *size, int wAlign, int hAlign);
static int Png_getInfos(char* fileName, int *w, int *h, int *bpp);
static unsigned short *Png_getPalette(char* fileName, int *size);
static unsigned char *Png_getData(char* fileName, int *size, int wAlign, int hAlign);


int Img_getInfos(char* fileName, int *w, int *h, int *bpp)
{
    char* fileExt;

    fileExt = getFileExtension(fileName);

    if (!stricmp(fileExt, "PNG")) return Png_getInfos(fileName, w, h, bpp);
    if (!stricmp(fileExt, "BMP")) return Bmp_getInfos(fileName, w, h, bpp);

    printf("Unsupported image file %s\n", fileName);

    return 0;
}

unsigned short *Img_getPalette(char* fileName, int *size)
{
    char* fileExt;

    fileExt = getFileExtension(fileName);

    if (!stricmp(fileExt, "PNG")) return Png_getPalette(fileName, size);
    if (!stricmp(fileExt, "BMP")) return Bmp_getPalette(fileName, size);

    printf("Unsupported image file %s\n", fileName);

    return NULL;
}

unsigned char *Img_getData(char* fileName, int *size, int wAlign, int hAlign)
{
    unsigned char *result;
    char* fileExt;
    int w, h, bpp;

    fileExt = getFileExtension(fileName);

    if (!stricmp(fileExt, "PNG")) result = Png_getData(fileName, size, wAlign, hAlign);
    else if (!stricmp(fileExt, "BMP")) result = Bmp_getData(fileName, size, wAlign, hAlign);
    else
    {
        printf("Unsupported image file %s\n", fileName);
        result = NULL;
    }

    if (result)
    {
        // get image info
        if (!Img_getInfos(fileName, &w, &h, &bpp)) return NULL;

        // we want result in 8BPP (convert 4BPP --> 8BPP)
        if (bpp == 4)
        {
            result = to8bppAndFree(result, *size);
            *size *= 2;
        }
    }

    return result;
}


static int Bmp_getInfos(char* fileName, int *w, int *h, int *bpp)
{
    FILE *f;
    unsigned char data[0x20];

    f = fopen(fileName, "rb");

    if (!f)
    {
        printf("Couldn't open input file %s\n", fileName);
        return 0;
    }

    // process header
    fread(data, 1, 0x20, f);

    // only need two bytes, but is really four bytes LE
    *w = data[BMP_WIDTH + 0] | (data[BMP_WIDTH + 1] << 8);
    // only need two bytes, but is really four bytes LE
    *h = data[BMP_HEIGHT + 0] | (data[BMP_HEIGHT + 1] << 8);
    // only need one byte, but is really two bytes LE
    *bpp = data[BMP_BPP];

    fclose(f);

    return 1;
}

static unsigned short *Bmp_getPalette(char* fileName, int *size)
{
    FILE *f;
    unsigned char data[BMP_PALETTE_OFFSET + (256 * 4)];
    unsigned short *result;
    int i;

    f = fopen(fileName, "rb");

    if (!f)
    {
        printf("Couldn't open input file %s\n", fileName);
        // error
        return NULL;
    }

    // read header with palette data
    fread(data, 1, BMP_PALETTE_OFFSET + (256 * 4), f);

    // only need two bytes, but is really four bytes LE
    *size = data[BMP_PALETTE_SIZE + 0] | (data[BMP_PALETTE_SIZE + 1] << 8);

    // allocate palette buffer
    result = malloc(*size * sizeof(short));

    // convert to sega palette
    for(i = 0; i < *size; i++)
    {
        result[i] = toVDPColor(data[BMP_PALETTE_OFFSET + (i*4) + 0],
                               data[BMP_PALETTE_OFFSET + (i*4) + 1],
                               data[BMP_PALETTE_OFFSET + (i*4) + 2]);
    }

    fclose(f);

    return result;
}

static unsigned char *Bmp_getData(char* fileName, int *size, int wAlign, int hAlign)
{
    FILE *f;
    int w, h, bpp;
    int wAligned, hAligned;
    int i, bmpPitch;
    unsigned short *palette;
    unsigned char *result;

    f = fopen(fileName, "rb");

    if (!f)
    {
        printf("Couldn't open input file %s\n", fileName);
        // error
        return NULL;
    }

    // get infos
    Bmp_getInfos(fileName, &w, &h, &bpp);
    // get palette
    palette = Bmp_getPalette(fileName, size);

    // set data position in file
    fseek(f, BMP_PALETTE_OFFSET + (*size * 2), SEEK_SET);

    // compute bmp pitch
    bmpPitch = (((bpp * w) + 31) / 32) * 4;
    // fix incorrect W align
    if ((bpp == 4) && (wAlign < 2)) wAlign = 2;
    else if (wAlign < 1) wAlign = 1;
    // do size alignment
    wAligned = ((w + (wAlign - 1)) / wAlign) * wAlign;
    hAligned = ((h + (hAlign - 1)) / hAlign) * hAlign;

    switch(bpp)
    {
        case 4:
            // set size
            *size = (wAligned * hAligned) / 2;
            // allocate bitmap data (16 color image)
            result = malloc(*size);
            // clear
            memset(result, 0, *size);

            // fill from bitmap backwards (BMP is bottom up)
            for (i = 0; i < h; i++)
                fread(&result[(h - i - 1) * bmpPitch], 1, MIN(bmpPitch, wAligned / 2), f);
            break;

        case 8:
            // set size
            *size = wAligned * hAligned;
            // allocate bitmap data (256 color image)
            result = malloc(*size);
            // clear
            memset(result, 0, *size);

            // fill from bitmap backwards (BMP is bottom up)
            for (i = 0; i < h; i++)
                fread(&result[(h - i - 1) * bmpPitch], 1, MIN(bmpPitch, wAligned), f);
            break;

        default:
            // not supported
            printf("Image '%s':\n", fileName);
            printf("%d bpp bitmap not supported\n", bpp);
            *size = 0;
            return NULL;
    }

    fclose(f);

    return result;
}


static int Png_getInfos(char* fileName, int *w, int *h, int *bpp)
{
    FILE *f;
    unsigned char data[0x20];

    f = fopen(fileName, "rb");

    if (!f)
    {
        printf("Couldn't open input file %s\n", fileName);
        return 0;
    }

    // process header
    fread(data, 1, 0x20, f);

    // only need two bytes, but is really four bytes LE
    *w = data[PNG_WIDTH + 1] | (data[PNG_WIDTH + 0] << 8);
    // only need two bytes, but is really four bytes LE
    *h = data[PNG_HEIGHT + 1] | (data[PNG_HEIGHT + 0] << 8);
    *bpp = data[PNG_BPP];

    fclose(f);

    return 1;
}

static unsigned short *Png_getPalette(char* fileName, int *size)
{
    int i, w, h;
    unsigned int errcode;
    unsigned char *in;
    unsigned char *out;
    unsigned short *result;
    LodePNGState state;

    in = readFile(fileName, size);
    if (!in) return NULL;

    lodepng_state_init(&state);
    // no conversion
    state.decoder.color_convert = false;
    // get infos (have to use the decode methode to retrieve palette data)
    errcode = lodepng_decode(&out, &w, &h, &state, in, *size);

    // release memory
    free(in);
    free(out);

    // error ?
    if (errcode)
    {
        printf(lodepng_error_text(errcode));
        *size = 0;
        return NULL;
    }

    // get palette size
    *size = state.info_png.color.palettesize;

    // allocate palette buffer
    result = malloc(*size * sizeof(short));

    // convert to sega palette
    for(i = 0; i < *size; i++)
    {
        result[i] = toVDPColor(state.info_png.color.palette[(i*4) + 2],
                               state.info_png.color.palette[(i*4) + 1],
                               state.info_png.color.palette[(i*4) + 0]);
    }

    return result;
}

static unsigned char *Png_getData(char* fileName, int *size, int wAlign, int hAlign)
{
    int w, h, bpp;
    int wAligned, hAligned;
    int i, j;
    int srcPix;
    unsigned int errcode;
    unsigned char *in;
    unsigned char *out;
    unsigned char *result;
    LodePNGState state;

    in = readFile(fileName, size);
    if (!in) return NULL;

    lodepng_state_init(&state);
    // no conversion
    state.decoder.color_convert = false;
    // get data
    errcode = lodepng_decode(&out, &w, &h, &state, in, *size);

    // release memory
    free(in);

    // error ?
    if (errcode)
    {
        printf(lodepng_error_text(errcode));
        *size = 0;
        return NULL;
    }

    // only indexed images accepted
    if (state.info_raw.colortype != LCT_PALETTE)
    {
        printf("Image '%s':\n", fileName);
        printf("Error: RGB image not supported (only indexed color)\n");
        *size = 0;
        return NULL;
    }

    bpp = state.info_raw.bitdepth;

    // fix incorrect W align
    if ((bpp == 4) && (wAlign < 2)) wAlign = 2;
    else if (wAlign < 1) wAlign = 1;
    // do size alignment
    wAligned = ((w + (wAlign - 1)) / wAlign) * wAlign;
    hAligned = ((h + (hAlign - 1)) / hAlign) * hAlign;

    switch (bpp)
    {
        case 4:
            // get buffer size
            *size = (wAligned / 2) * hAligned;
            // and alloc
            result = malloc(*size);

            srcPix = 0;
            for (i = 0; i < h; i++)
            {
                unsigned char *dst = &result[i * (wAligned / 2)];

                memset(dst, 0, wAligned / 2);

                for (j = 0; j < w; j++)
                {
                    unsigned char v;

                    if (srcPix & 1) v = (out[srcPix / 2] >> 4) & 0xF;
                    else v =  (out[srcPix / 2] >> 0) & 0xF;
                    srcPix++;

                    if (j & 1) dst[j / 2] = (dst[j / 2] & 0x0F) | (v << 4);
                    else dst[j / 2] = (dst[j / 2] & 0xF0) | (v << 0);
                }
            }
            for(;i < hAligned; i++)
                memset(&result[i * (wAligned / 2)], 0, wAligned / 2);
            break;

        case 8:
            // get buffer size
            *size = wAligned * hAligned;
            // and alloc
            result = malloc(*size);

            for (i = 0; i < h; i++)
            {
                unsigned char *dst = &result[i * wAligned];

                memset(dst, 0, wAligned);
                memcpy(dst, &out[i * w], w);
            }
            for(;i < hAligned; i++)
                memset(&result[i * wAligned], 0, wAligned);
            break;

        default:
            // not supported
            printf("Image '%s':\n", fileName);
            printf("%d bpp PNG not supported\n", bpp);
            *size = 0;
            return NULL;
    }

    return result;
}


unsigned char *to4bppAndFree(unsigned char* buf8bpp, int size)
{
    unsigned char *result = to4bpp(buf8bpp, size);
    free(buf8bpp);
    return result;
}

unsigned char *to8bppAndFree(unsigned char* buf4bpp, int size)
{
    unsigned char *result = to8bpp(buf4bpp, size);
    free(buf4bpp);
    return result;
}

unsigned char *to4bpp(unsigned char* buf8bpp, int size)
{
    unsigned char *result = malloc(size / 2);
    unsigned char *src = buf8bpp;
    unsigned char *dst = result;
    int i;

    for(i = 0; i < size / 2; i++)
    {
        *dst++ = ((src[0] & 0x0F) << 4) | ((src[1] & 0x0F) << 0);
        src += 2;
    }

    return result;
}

unsigned char *to8bpp(unsigned char* buf4bpp, int size)
{
    unsigned char *result = malloc(size * 2);
    unsigned char *src = buf4bpp;
    unsigned char *dst = result;
    int i;

    for(i = 0; i < size; i++)
    {
        *dst++ = (*src & 0xF0) >> 4;
        *dst++ = (*src & 0x0F) >> 0;
        src++;
    }

    return result;
}


unsigned char getMaxIndex(unsigned char* buf8bpp, int size)
{
    unsigned char *src = buf8bpp;
    unsigned char result = 0;
    int i = size;

    while(i--)
    {
        if (*src > result) result = *src;
        src++;
    }

    return result;
}
