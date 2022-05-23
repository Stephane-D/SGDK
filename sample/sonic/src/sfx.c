#include <genesis.h>

#include "sfx.h"

#include "res_sound.h"


void SFX_init(void)
{
    XGM_setPCM(SFX_JUMP, sonic_jump_sfx, sizeof(sonic_jump_sfx));
    XGM_setPCM(SFX_ROLL, sonic_roll_sfx, sizeof(sonic_roll_sfx));
    XGM_setPCM(SFX_STOP, sonic_stop_sfx, sizeof(sonic_stop_sfx));
}