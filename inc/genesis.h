#ifndef _GENESIS_H_
#define _GENESIS_H_


#define SGDK_VERSION    1.62

#include "config.h"
#include "asm.h"
#include "types.h"

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

// preserve compatibility with old resources name
#define logo_lib sgdk_logo
#define font_lib font_default
#define font_pal_lib font_pal_default

// exist through rom_head.c
typedef struct
{
    char console[16];               /* Console Name (16) */
    char copyright[16];             /* Copyright Information (16) */
    char title_local[48];           /* Domestic Name (48) */
    char title_int[48];             /* Overseas Name (48) */
    char serial[14];                /* Serial Number (2, 12) */
    u16 checksum;                   /* Checksum (2) */
    char IOSupport[16];             /* I/O Support (16) */
    u32 rom_start;                  /* ROM Start Address (4) */
    u32 rom_end;                    /* ROM End Address (4) */
    u32 ram_start;                  /* Start of Backup RAM (4) */
    u32 ram_end;                    /* End of Backup RAM (4) */
    char sram_sig[2];               /* "RA" for save ram (2) */
    u16 sram_type;                  /* 0xF820 for save ram on odd bytes (2) */
    u32 sram_start;                 /* SRAM start address - normally 0x200001 (4) */
    u32 sram_end;                   /* SRAM end address - start + 2*sram_size (4) */
    char modem_support[12];         /* Modem Support (24) */
    char notes[40];                 /* Memo (40) */
    char region[16];                /* Country Support (16) */
} ROMHeader;

extern const ROMHeader rom_header;

#endif // _GENESIS_H_
