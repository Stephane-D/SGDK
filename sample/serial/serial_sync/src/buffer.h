#include <types.h>

u8 sync_buffer_read(void);
void sync_buffer_write(u8 data);
u8 sync_buffer_canRead(void);
u16 sync_buffer_available(void);
