#include "types.h"

#include "everdrive.h"

/** MMC/SD card SPI mode commands **/
#define CMD0  0x40    // software reset
#define CMD1  0x41    // brings card out of idle state
#define CMD17 0x51    // read single block
#define CMD24 0x58    // writes a single block


#define EPR_CMD(addr, data) *((volatile u16*) (addr << 1)) = data;


u8 evd_mmcCmd(u8 cmd, u32 arg);
u16 cfg;
u16 default_rom_bank;
u8 is_ram_app;


void evd_init(u16 def_rom_bank, u8 _is_ram_app) {

    cfg = 0;
    CFG_PORT = 0;
    default_rom_bank = def_rom_bank;
    is_ram_app = _is_ram_app;
    VBL_PORT = 0x0030;
}

u8 evd_mmcInit() {

    u16 i;

    SS_OFF;
    SPI_HI_SPEED_OFF;
    for (i = 0; i < 20; i++) {
        SPI_BUSY;
        SPI_PORT = 0xff;
    }

    if (evd_mmcCmd(CMD0, 0) != 1) {
        return 1;
    }

    i = 0;

    while (evd_mmcCmd(CMD1, 0) != 0) {
        if (i++ == 65535) {
            return 2;
        }
    }

    SS_ON;
    SPI_PORT = 0xff;
    SPI_BUSY;

    SPI_HI_SPEED_ON;
    SS_OFF;

    return 0;
}

u8 evd_mmcCmd(u8 cmd, u32 arg) {


    SPI_BUSY;
    SS_ON;
    SPI_PORT = 0xff;
    SPI_BUSY;
    SPI_PORT = cmd;
    SPI_BUSY;
    SPI_PORT = (arg >> 24);
    SPI_BUSY;
    SPI_PORT = (arg >> 16);
    SPI_BUSY;
    SPI_PORT = (arg >> 8);
    SPI_BUSY;
    SPI_PORT = 0;
    SPI_BUSY;
    SPI_PORT = 0x95;
    SPI_BUSY;
    SPI_PORT = 0xff;
    SPI_BUSY;
    SPI_PORT = 0xff;
    SPI_BUSY;
    SS_OFF;
    SPI_BUSY;
    return SPI_PORT & 0xff;
}

u8 evd_mmcWrBlock(u32 mmc_addr, u8 *data_ptr) {


    u16 i;
    if (evd_mmcCmd(CMD24, mmc_addr) != 0) {
        return 1;
    }


    SS_ON;
    SPI_BUSY;
    SPI_PORT = 0xff;
    SPI_BUSY;
    SPI_PORT = 0xff;
    SPI_BUSY;
    SPI_PORT = 0xfe;


    for (i = 0; i < 512; i++) {
        SPI_BUSY;
        SPI_PORT = *data_ptr++;
    }

    SPI_BUSY;
    SPI_PORT = 0xFF;
    SPI_BUSY;
    SPI_PORT = 0xFF;
    SPI_BUSY;
    SPI_PORT = 0xFF;
    SPI_BUSY;

    if ((SPI_PORT & 0x1f) != 0x05) {
        SS_OFF;
        return 2;
    }

    i = 0;
    SPI_BUSY;
    SPI_PORT = 0xFF;
    for (;;) {
        SPI_BUSY;
        SPI_PORT = 0xFF;
        SPI_BUSY;
        if ((SPI_PORT & 0xFF) == 0xff)break;
        if (i++ == 65535) {
            SS_OFF;
            return 3;
        }
    }
    SS_OFF;
    return 0;
}

//u32 stor_addr;

u8 evd_mmcRdBlock(u32 mmc_addr, u8 *stor) {

    u16 i = 0;
    u16 *stor16 = (u16 *) stor;
    //stor_addr = (u32) stor;


    if (evd_mmcCmd(CMD17, mmc_addr) != 0) {
        SS_ON;
        for (;;) {

            SPI_PORT = 0xff;
            SPI_BUSY;


            if ((SPI_PORT & 0xff) == 0) {
                break;
            }

            if (i++ == 65535) {
                SS_OFF;
                return 1;
            }
        }
    }

    SS_ON;


    i = 0;

    for (;;) {

        SPI_PORT = 0xff;
        SPI_BUSY;
        if ((SPI_PORT & 0xff) == 0xfe)break;

        if (i++ == 65535) {
            SS_OFF;
            return 2;
        }
    }

    CFGS(_SPI16);


    for (i = 0; i < 256; i++) {

        SPI_PORT = 0xffff;
        SPI_BUSY;
        *stor16++ = SPI_PORT;
    }

    CFGC(_SPI16);


    SPI_PORT = 0xff;
    SPI_BUSY;
    SPI_PORT = 0xff;
    SPI_BUSY;
    SS_OFF;
    return 0;
}

void evd_eprEraseBlock(u32 rom_addr) {

    u16 i;
    VBL_CATCH_ON;

    EPR_BUSY;

    if (VER_PORT >= 3) {
        ROM_MAP_PORT = 0;
        if (rom_addr >= 0x400000) {
            ROM_MAP_PORT = 1;
            rom_addr -= 0x400000;
        }
    }


    EPR_CMD(0x555, 0xaa);
    EPR_CMD(0x2aa, 0x55);
    EPR_CMD(0x555, 0x80);
    EPR_CMD(0x555, 0xaa);
    EPR_CMD(0x2aa, 0x55);

    for (i = 0; i < 8; i++) {
        *((volatile u16*) (rom_addr)) = 0x30;
        rom_addr += 8192;
    }
    EPR_BUSY;

    if (VER_PORT >= 3)ROM_MAP_PORT = default_rom_bank;

    VBL_CATCH_OFF;

}

void evd_eprProgBlock(u16 *data, u32 rom_addr, u32 len) {

    u32 i;

    volatile u16 *addr;
    VBL_CATCH_ON;

    if (VER_PORT >= 3) {

        if (rom_addr >= 0x400000) {
            ROM_MAP_PORT = 1;
            rom_addr -= 0x400000;
        } else {
            ROM_MAP_PORT = 0;
        }
    }
    addr = (u16 *) rom_addr;
    len >>= 2;



    for (i = 0; i < len; i++) {

        EPR_BUSY;
        *((volatile u16 *)0xaaa) = 0x50;
        *addr++ = *data++;
        *addr++ = *data++;
    }


    if (VER_PORT >= 3)ROM_MAP_PORT = default_rom_bank;
    EPR_BUSY;

    VBL_CATCH_OFF;
}
