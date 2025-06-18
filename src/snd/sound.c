#include "config.h"
#include "types.h"

#include "z80_ctrl.h"

#include "snd/sound.h"
// auto-generated
#include "src/snd/drv_null.h"

#include "ym2612.h"
#include "psg.h"
#include "timer.h"
#include "sys.h"


// we don't want to share them
extern s16 currentDriver;
extern void Z80_loadDriverInternal(const u8 *drv, u16 size);

// Z80_DRIVER_NULL
// dummy sound driver, Z80 waiting in an idle loop
//////////////////////////////////////////////////

NO_INLINE void SND_NULL_loadDriver()
{
    // already loaded
    if (currentDriver == Z80_DRIVER_NULL) return;

    Z80_loadDriverInternal(drv_null, sizeof(drv_null));

    // driver loaded
    currentDriver = Z80_DRIVER_NULL;
}
