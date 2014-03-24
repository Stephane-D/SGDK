#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "../inc/libpng.h"
#include "../inc/tools.h"
#include "../inc/img_tools.h"


#define BMP_HEADER_CORE         0
#define BMP_HEADER_INFO         1

#define BMP_HEADER_SIZE         0x0E

#define BMP_WIDTH_CORE          0x12
#define BMP_HEIGHT_CORE         0x14
#define BMP_BPP_CORE            0x16

#define BMP_WIDTH_INFO          0x12
#define BMP_HEIGHT_INFO         0x16
#define BMP_BPP_INFO            0x1C
#define BMP_PALETTE_SIZE_INFO   0x2E

#define PNG_WIDTH               0x12
#define PNG_HEIGHT              0x16
#define PNG_BPP                 0x18


// forward
static void Bmp_getHeaderInfos(unsigned char *header, int *size, int *type);
static void Bmp_getImageInfos(unsigned char *header, int *w, int *h, int *bpp);
static void Bmp_getPaletteInfos(unsigned char *header, int *offset, int *size);
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


static void Bmp_getHeaderInfos(unsigned char *header, int *size, int *type)
{
    // get header size (only need two bytes, but is really four bytes LE)
    *size = header[BMP_HEADER_SIZE + 0] | (header[BMP_HEADER_SIZE + 1] << 8);
    // get header type
    if ((*size == 12) || (*size == 60)) *type = BMP_HEADER_CORE;
    else *type = BMP_HEADER_INFO;
}

static void Bmp_getImageInfos(unsigned char *header, int *w, int *h, int *bpp)
{
    int hsize, type;

    // get header infos
    Bmp_getHeaderInfos(header, &hsize, &type);

    // core header
    if (type == BMP_HEADER_CORE)
    {
        *w = header[BMP_WIDTH_CORE + 0] | (header[BMP_WIDTH_CORE + 1] << 8);
        *h = header[BMP_HEIGHT_CORE + 0] | (header[BMP_HEIGHT_CORE + 1] << 8);
        // only need one byte, but is really two bytes LE
        *bpp = header[BMP_BPP_CORE];
    }
    // info header
    else
    {
        // only need two bytes, but is really four bytes LE
        *w = header[BMP_WIDTH_INFO + 0] | (header[BMP_WIDTH_INFO + 1] << 8);
        // only need two bytes, but is really four bytes LE
        *h = header[BMP_HEIGHT_INFO + 0] | (header[BMP_HEIGHT_INFO + 1] << 8);
        // only need one byte, but is really two bytes LE
        *bpp = header[BMP_BPP_INFO];
    }
}

static void Bmp_getPaletteInfos(unsigned char *header, int *offset, int *size)
{
    int hsize, type, bpp;

    // get header infos
    Bmp_getHeaderInfos(header, &hsize, &type);
    // palette offset
    *offset = hsize + 14;

    // core header
    if (type == BMP_HEADER_CORE)
    {
        Bmp_getImageInfos(header, &hsize, &hsize, &bpp);
        // get palette size
        *size = 1 << bpp;
    }
    // info header
    else
    {
        // only need two bytes, but is really four bytes LE
        *size = header[BMP_PALETTE_SIZE_INFO + 0] | (header[BMP_PALETTE_SIZE_INFO + 1] << 8);
    }
}

static int Bmp_getInfos(char* fileName, int *w, int *h, int *bpp)
{
    FILE *f;
    unsigned char data[0x100];

    f = fopen(fileName, "rb");

    if (!f)
    {
        printf("Couldn't open input file %s\n", fileName);
        return 0;
    }

    // read header
    fread(data, 1, 0x100, f);
    // get header infos
    Bmp_getImageInfos(data, w, h, bpp);

    fclose(f);

    return 1;
}

static unsigned short *Bmp_getPalette(char* fileName, int *size)
{
    FILE *f;
    unsigned char data[0x100 + (256 * 4)];
    unsigned short *result;
    int i, poff;

    f = fopen(fileName, "rb");

    if (!f)
    {
        printf("Couldn't open input file %s\n", fileName);
        // error
        return NULL;
    }

    // read header and palette data
    fread(data, 1, sizeof(data), f);
    // get palette infos
    Bmp_getPaletteInfos(data, &poff, size);

    // allocate palette buffer
    result = malloc(*size * sizeof(short));

    // convert to sega palette
    for(i = 0; i < *size; i++)
    {
        result[i] = toVDPColor(data[poff + (i * 4) + 0],
                               data[poff + (i * 4) + 1],
                               data[poff + (i * 4) + 2]);
    }

    fclose(f);

    return result;
}

static unsigned char *Bmp_getData(char* fileName, int *size, int wAlign, int hAlign)
{
    FILE *f;
    int w, h, bpp;
    int wAligned, hAligned;
    int palOffset, palSize;
    int i, bmpPitch;
    unsigned char data[0x100];
    unsigned char *result;

    f = fopen(fileName, "rb");

    if (!f)
    {
        printf("Couldn't open input file %s\n", fileName);
        // error
        return NULL;
    }

    // read header
    fread(data, 1, 0x100, f);
    // get header infos
    Bmp_getImageInfos(data, &w, &h, &bpp);
    // get palette infos
    Bmp_getPaletteInfos(data, &palOffset, &palSize);

    // set data position in file
    fseek(f, palOffset + (palSize * 4), SEEK_SET);

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
                fread(&result[((h - i) - 1) * bmpPitch], 1, MIN(bmpPitch, wAligned / 2), f);
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
                fread(&result[((h - i) - 1) * bmpPitch], 1, MIN(bmpPitch, wAligned), f);
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
        if (*src > result)
            result = *src;
        src++;
    }

    return result;
}
