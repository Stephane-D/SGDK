#ifndef MINIMUS_H_
#define MINIMUS_H_

// SGDK replacement for stdint.h
#include "types.h"

#define MINIMUSIC_MEMORY_BARRIER() asm volatile ("" : : : "memory")

static inline void MINIMUSIC_Z80_GUARD_BEGIN() {
   volatile uint16_t *port = (uint16_t*)(0xA11100);
   MINIMUSIC_MEMORY_BARRIER();
   *port = 0x100;
   MINIMUSIC_MEMORY_BARRIER();
}

static inline void MINIMUSIC_Z80_GUARD_END() {
   volatile uint16_t *port = (uint16_t*)(0xA11100);
   MINIMUSIC_MEMORY_BARRIER();
   *port = 0x000;
   MINIMUSIC_MEMORY_BARRIER();
}

#define MINIMUSIC_STATUS_BGM        0x01

void minimusic_init(const void *, uint16_t);
void minimusic_sendcmd(uint8_t);
uint8_t minimusic_get_status(void);

#endif
