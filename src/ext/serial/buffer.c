#include "config.h"
#if (MODULE_SERIAL && SERIAL_ASYNC)
#include "ext/serial/buffer.h"
#define SERIAL_ASYNC_BUFFER_LEN 2048

static volatile u16 readHead = 0;
static volatile u16 writeHead = 0;

static volatile char buffer[SERIAL_ASYNC_BUFFER_LEN];

u8 buffer_read(void)
{
    u8 data = buffer[readHead++];
    if (readHead == SERIAL_ASYNC_BUFFER_LEN) {
        readHead = 0;
    }
    return data;
}

void buffer_write(u8 data)
{
    buffer[writeHead++] = data;
    if (writeHead == SERIAL_ASYNC_BUFFER_LEN) {
        writeHead = 0;
    }
}

u8 buffer_canRead(void)
{
    return writeHead != readHead;
}

u16 buffer_available(void)
{
    /*
    ----R--------W-----
    xxxxx        xxxxxx

    ----W--------R-----
        xxxxxxxxx
    */
    if (writeHead >= readHead) {
        return SERIAL_ASYNC_BUFFER_LEN - (writeHead - readHead);
    } else {
        return readHead - writeHead;
    }
}

#endif // (MODULE_SERIAL && SERIAL_ASYNC)