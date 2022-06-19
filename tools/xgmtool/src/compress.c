#include <stdlib.h>
#include <stdio.h>
#include "../inc/lz77.h"
#include "../inc/compress.h"

void lz77c_init(lz77_compress_t* s, uint8_t*out)
{
    memset(s, 0, sizeof(*s));
    s->nextb = 0;
    s->out = out;
    s->outptr = out;
    s->out_l = 1;
}

bool lz77c_check_flush(lz77_compress_t* s)
{
    s->nextb = (s->nextb + 1) & 7;
    if (!s->nextb) {
        s->outptr += s->out_l;
        s->outptr[0] = 0;
        s->out_l = 1;
    }
    return true;
}

bool lz77c_lit_callback(uint8_t lit, void* aux)
{
    lz77_compress_t* s = aux;

    s->outptr[0] = (s->outptr[0] >> 1);
    s->outptr[s->out_l++] = lit;

    return lz77c_check_flush(s);
}

bool lz77c_backref_callback(size_t dist, size_t len, void* aux)
{
    lz77_compress_t* s = aux;

    s->outptr[0] = (s->outptr[0] >> 1) | 0x80;
    s->outptr[s->out_l++] = ((dist-1) >> 8) & 0xff;
    s->outptr[s->out_l++] = ((dist-1) >> 0) & 0xff;
    s->outptr[s->out_l++] = (len-1) & 0xff;

    return lz77c_check_flush(s);
}

void lz77c_finish(lz77_compress_t* s)
{
    s->outptr[0] = ((s->outptr[0] >> 1) | 0x80) >> (7 - s->nextb);
    s->outptr[s->out_l++] = 0;
    s->outptr[s->out_l++] = 0;
    s->outptr[s->out_l++] = 0;
    s->outptr += s->out_l;
}

size_t lz77c_compress_buf(unsigned char *in, size_t inlen, void **pout)
{
    bool res;
    lz77_compress_t s;
    unsigned char *out;

    out = (unsigned char*)malloc((inlen * 10) / 8 + 4);

    lz77c_init(&s, out);
    res = lz77_compress(in, inlen, 32 * 1024, 256, true, lz77c_lit_callback, lz77c_backref_callback, &s);
    lz77c_finish(&s);

    if (!res) {
        free(out);
        return 0;
    }

    *pout = (void *)s.out;
    return s.outptr - s.out;
}
