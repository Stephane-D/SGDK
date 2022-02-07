/**
 *  \file everdrive.h
 *  \brief Everdrive support
 *  \author Krikzz
 *  \date XX/20XX
 *
 * This unit provides support for everdrive flash cart
 */

#ifndef _EVERDRIVE
#define _EVERDRIVE

#if (MODULE_EVERDRIVE != 0)

//config register bits
#define _SS 0
#define _FULL_SPEED 1
#define _SPI16 2
#define _GAME_MODE 3
#define _SMS_MODE 4
#define _HARD_RESET 5
#define _RAM_MODE_1 6
#define _RAM_ON 7
#define _VBL_CATCH 8
#define _MEGAKEY_ON 9
#define _MEGAKEY_REGION_1 10
#define _SSF_MODE_ON 11
#define _RAM_FS 12
#define _CART 13

//state register bits
#define _SPI_READY 0
#define _RY 1
#define _SMS_KEY 2
#define _SD_CART 3

//everdrive hardware registers
#define SPI_PORT *((volatile u16*) (0xA13000))
#define CFG_PORT *((volatile u16*) (0xA13002))
#define VBL_PORT *((volatile u16*) (0xA13004))
#define SRAM_BANK_PORT *((volatile u16*) (0xA13006))
#define VER_PORT *((volatile u16*) (0xA13008))
#define ROM_MAP_PORT *((volatile u16*) (0xA1300a))


#define CFGC(bit)(cfg &= ~(1 << bit), CFG_PORT = cfg)
#define CFGS(bit)(cfg |= (1 << bit), CFG_PORT = cfg)

#define IS_RY (CFG_PORT & (1 << _RY))
#define IS_SPI_READY (CFG_PORT & (1 << _SPI_READY))
#define IS_SMS_KEY_PRESSED (CFG_PORT & (1 << _SMS_KEY))
#define IS_SD_SLOT_EMPTY (CFG_PORT & (1 << _SD_CART))

#define SPI_HI_SPEED_ON CFGS(_FULL_SPEED)
#define SPI_HI_SPEED_OFF CFGC(_FULL_SPEED)

#define SPI16_ON CFGS(_SPI16);
#define SPI16_OFF CFGC(_SPI16);

#define SS_ON CFGC(_SS)
#define SS_OFF CFGS(_SS)

#define CART_ON CFGC(_CART)
#define CART_OFF CFGS(_CART)

#define RAM_ON CFGS(_RAM_ON);
#define RAM_OFF CFGC(_RAM_ON);

#define VBL_CATCH_ON CFGS(_VBL_CATCH);
#define VBL_CATCH_OFF CFGC(_VBL_CATCH);

#define SPI_BUSY while(!IS_SPI_READY)
#define EPR_BUSY while(!IS_RY)



extern u16 cfg;


//SD/MMC card initialization. should be run just one times, aer this cart will be ready for work
//will return 0 success
u8 evd_mmcInit();


//read block (512b) from SD/MMC card. mmc_addr should be multiple to 512
//will return 0 success
u8 evd_mmcRdBlock(u32 mmc_addr, u8 *stor);


//write block (512b) to SD/MMC card. mmc_addr should be multiple to 512
//will return 0 success
u8 evd_mmcWrBlock(u32 mmc_addr, u8 *data_ptr);


//erase flash memry sector(64kb). rom_addr should be multiple to 64k.
//code of this function should be placed in ram because rom memory inaccessible while erase process
//WARNING! this function may damage cart bios if sectors in range 0 - 0x40000 will be erased
void evd_eprEraseBlock(u32 rom_addr);


//write data to flash memory. len should be multiple to 4.
//each byte of flah memory should be erased before writeing by evd_eprEraseBlock
//code of this function should be placed in ram because rom memory inaccessible while writeing process
//WARNING! this function may damage cart bios if memory will be writen in area 0 - 0x40000
void evd_eprProgBlock(u16 *data, u32 rom_addr, u32 len);


//everdrive initialization.
//def_rom_bank = 0 if app placed in 0-0x400000 area, 1 if in 0x400000-0x800000 arae
//_is_ram_app = 0 if app assembled for work in rom, 1 if app assembleed for work in ram
void evd_init(u16 def_rom_bank, u8 _is_ram_app);


#endif // MODULE_EVERDRIVE

#endif
