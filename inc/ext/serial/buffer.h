#include "genesis.h"

u16 buffer_init(u16 size);
u8 buffer_read(void);
void buffer_write(u8 data);
u8 buffer_canRead(void);
u16 buffer_available(void);
void buffer_free(void);
