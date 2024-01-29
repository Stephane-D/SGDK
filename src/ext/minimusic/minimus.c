#include "config.h"
// SGDK replacement for stdint.h
#include "types.h"

#include "ext/minimusic/minimus.h"
#include "ext/minimusic/minimus_drv.h"

#define MEMORY_BARRIER() MINIMUSIC_MEMORY_BARRIER()
#define WASTE_TIME() for (int16_t i = 0x100; i >= 0; i--) { MEMORY_BARRIER(); }

void minimusic_init(const void *data, uint16_t size)
{
   const uint8_t *src;
   volatile uint8_t *dest;
   volatile uint16_t *busreq = (uint16_t*)(0xA11100);
   volatile uint16_t *reset = (uint16_t*)(0xA11200);

   // Reset the Z80 and request the bus
   // Don't wait for Z80 since it can't respond to our request!
   // (but we need to request in order to access Z80 RAM)
   MEMORY_BARRIER();
   *reset = 0x000;
   MEMORY_BARRIER();
   *busreq = 0x100;
   MEMORY_BARRIER();

   // Copy sound data into Z80 RAM
   src = (const uint8_t*)(data);
   dest = (volatile uint8_t*)(0xA00800);
   while (size-- > 0) {
      MEMORY_BARRIER();
      *dest++ = *src++;
      MEMORY_BARRIER();
   }

   // Copy Z80 program into Z80 RAM
   src = minimus_drv;
   dest = (volatile uint8_t*)(0xA00000);
   size = sizeof(minimus_drv);
   while (size-- > 0) {
      MEMORY_BARRIER();
      *dest++ = *src++;
      MEMORY_BARRIER();
   }

   // Reset the Z80 again
   MEMORY_BARRIER();
   *reset = 0x000;
   MEMORY_BARRIER();
   WASTE_TIME();

   // Now let the Z80 run
   MEMORY_BARRIER();
   *busreq = 0x000;
   MEMORY_BARRIER();
   *reset = 0x100;
   MEMORY_BARRIER();
}

void minimusic_send_cmd(uint8_t cmd)
{
   volatile uint16_t *busreq = (uint16_t*)(0xA11100);
   volatile uint8_t *z80ram = (uint8_t*)(0xA00000);
   volatile uint8_t *ptr;
   uint8_t temp;

retry:
   // Request the Z80 bus (we need to access Z80 RAM)
   MEMORY_BARRIER();
   *busreq = 0x100;
   MEMORY_BARRIER();
   while (*busreq & 0x100) {
      MEMORY_BARRIER();
   }

   // Try all slots to find an empty one
   ptr = z80ram;
   MEMORY_BARRIER(); temp = *ptr++; MEMORY_BARRIER(); if (temp) goto ok;
   MEMORY_BARRIER(); temp = *ptr++; MEMORY_BARRIER(); if (temp) goto ok;
   MEMORY_BARRIER(); temp = *ptr++; MEMORY_BARRIER(); if (temp) goto ok;

   // Resume Z80 execution and wait
   MEMORY_BARRIER();
   *busreq = 0x000;
   MEMORY_BARRIER();
   WASTE_TIME();

   // Retry and hope it freed up some space in the queue
   goto retry;

ok:
   // Write command in the queue
   MEMORY_BARRIER();
   ptr--; *ptr = cmd;
   MEMORY_BARRIER();

   // Resume Z80 execution
   MEMORY_BARRIER();
   *busreq = 0x000;
   MEMORY_BARRIER();
}

uint8_t minimusic_get_status(void)
{
   volatile uint16_t *busreq = (uint16_t*)(0xA11100);
   volatile uint8_t *status = (uint8_t*)(0xA00003);
   uint8_t temp;

   // Request the Z80 bus (we need to access Z80 RAM)
   MEMORY_BARRIER();
   *busreq = 0x100;
   MEMORY_BARRIER();
   while (*busreq & 0x100) {
      MEMORY_BARRIER();
   }

   // Read the sound driver status from Z80 RAM
   MEMORY_BARRIER();
   temp = *status;
   MEMORY_BARRIER();

   // Resume Z80 execution
   MEMORY_BARRIER();
   *busreq = 0x000;
   MEMORY_BARRIER();

   // Return the value read earlier
   return temp;
}
