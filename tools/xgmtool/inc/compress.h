
typedef struct
{
    uint8_t nextb;
    uint8_t *out;
    uint8_t *outptr;
    size_t out_l;
} lz77_compress_t;

size_t lz77c_compress_buf(unsigned char *in, size_t inlen, void **pout);

