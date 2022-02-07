#ifndef _GENESIS_H_
#define _GENESIS_H_

#define SGDK_VERSION    1.70

#include "types.h"

#define SGDK            TRUE

#include "config.h"
#include "asm.h"

#include "sys.h"
#include "sram.h"
#include "mapper.h"
#include "memory.h"
#include "tools.h"

#include "font.h"
#include "string.h"

#include "tab_cnv.h"

#include "maths.h"
#include "maths3D.h"

#include "vdp.h"
#include "vdp_bg.h"
#include "vdp_dma.h"
#include "vdp_spr.h"
#include "vdp_tile.h"
#include "vdp_pal.h"

#include "pal.h"

#include "vram.h"
#include "dma.h"

#include "map.h"

#include "bmp.h"
#include "sprite_eng.h"

#include "sound.h"
#include "xgm.h"
#include "z80_ctrl.h"
#include "ym2612.h"
#include "psg.h"
#include "joy.h"
#include "timer.h"

#include "task.h"

// modules
#if (MODULE_EVERDRIVE != 0)
#include "ext/everdrive.h"
#endif

#if (MODULE_FAT16 != 0)
#include "ext/fat16.h"
#endif

#if (MODULE_MEGAWIFI != 0)
#include "ext/mw/megawifi.h"
#endif

// preserve compatibility with old resources name
#define logo_lib sgdk_logo
#define font_lib font_default
#define font_pal_lib font_pal_default

#endif // _GENESIS_H_
