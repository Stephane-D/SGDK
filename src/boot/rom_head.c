#include "types.h"


const struct
{
    char console[16];               /* Console Name (16) */
    char copyright[16];             /* Copyright Information (16) */
    char title_local[48];           /* Domestic Name (48) */
    char title_int[48];             /* Overseas Name (48) */
    char serial[16];                /* Serial Number (2, 14) */
    u16 checksum;                   /* Checksum (2) */
    char IOSupport[16];             /* I/O Support (16) */
    u32 rom_start;                  /* ROM Start Address (4) */
    u32 rom_end;                    /* ROM End Address (4) */
    u32 ram_start;                  /* Start of Backup RAM (4) */
    u32 ram_end;                    /* End of Backup RAM (4) */
    char modem_support[24];         /* Modem Support (24) */
    char notes[40];                 /* Memo (40) */
    char region[16];                /* Country Support (16) */
} rom_header = {
    "SEGA MEGA DRIVE ",
    "(C)FLEMTEAM 2011",
    "SAMPLE PROGRAM                                  ",
    "SAMPLE PROGRAM                                  ",
    "GM 00000000-00",
    0x0000,
    "JD              ",
    0x00000000,
    0x00020000,
    0x00FF0000,
    0x00FFFFFF,
    "                        ",
    "DEMONSTRATION PROGRAM                   ",
    "JUE             "
};
