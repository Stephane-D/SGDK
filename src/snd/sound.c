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


// Z80_DRIVER_NULL
// dummy sound driver, Z80 waiting in an idle loop
//////////////////////////////////////////////////

void NO_INLINE SND_NULL_loadDriver()
{
    SYS_disableInts();
    Z80_requestBus(TRUE);

    // reset sound chips
    YM2612_reset();
    PSG_reset();

    // clear z80 memory
    Z80_clear();
    // upload Z80 driver and reset Z80
    Z80_upload(0, drv_null, sizeof(drv_null));

    Z80_startReset();
    Z80_releaseBus();
    // wait a bit so Z80 reset completed
    waitSubTick(50);
    Z80_endReset();

    SYS_enableInts();
}

void NO_INLINE SND_NULL_unloadDriver()
{
    // nothing to do here
}