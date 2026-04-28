#include "config.h"
#if (MODULE_SERIAL)
#include "ext/serial/buffer.h"

static volatile u16 readHead = 0;
static volatile u16 writeHead = 0;

static volatile char* buffer = NULL;
static u16 bufferSize = 0;

u16 buffer_init(u16 size){
    buffer_free();
    if(size) buffer = (char*)MEM_alloc(size);
    if (buffer) {
        bufferSize = size;
        return size;
    }
    return 0;
}

void buffer_free(){
    if(buffer){
        MEM_free((void*)buffer);
        buffer = NULL;
        bufferSize = 0;
        readHead = 0;
        writeHead = 0;
    }
}

u8 buffer_read(void)
{
    u8 data = buffer[readHead++];
    if (readHead == bufferSize) {
        readHead = 0;
    }
    return data;
}

void buffer_write(u8 data)
{
    buffer[writeHead++] = data;
    if (writeHead == bufferSize) {
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
        return bufferSize - (writeHead - readHead);
    } else {
        return readHead - writeHead;
    }
}

#endif // (MODULE_SERIAL)