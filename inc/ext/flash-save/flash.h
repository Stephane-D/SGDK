/**
 * \file flash.h
 * \brief Low level NOR flash read/erase/write interface.
 * \author Jesus Alonso (doragasu)
 * \date 08/2022
 *
 * This module allows reading, erasing and writing data from/to the
 * cartridge flash chip. It should work with any flash chip compatible with
 * the AMD format, and implementing a CFI interface. It has been tested
 * successfully with S29GL032 and W29GL032C chips. Also it is required that
 * the cart has the WE line connected to the flash chip (that should be the
 * case on most flash carts, but is not guaranteed).
 *
 * \note The module has some limitations: addresses must be even, data pointers
 * must be word aligned, and some functions (flash_init(), flash_sector_erase()
 * and flash_program()) must be run with interrupts disabled and Z80 stopped.
 * Please also take into account that flash suffers from wear, so if you use
 * this e.g. for saving data, avoid saving very frequently and/or implement
 * a wear leveling algorithm to avoid damaging the chip.
 *
 * \note Although it is not a requirement for this module to work, it is
 * strongly recommended that the cart you use also has the RESET pin connected
 * to the console RESET signal. Otherwise, if the user pushes the RESET
 * button while the flash chip is being erased/programmed, the machine will
 * most likely freeze, requiring another press of the RESET button or a power
 * cycle.
 */

#ifndef __FLASH_H__
#define __FLASH_H__

// I usually include stdint.h, but SGDK defines some stdint types here:
#include "types.h"

// Maximum number of flash regions. CFI standard allows up to 4
#define FLASH_REGION_MAX 4

/**
 * \brief Metadata of a flash region, consisting of several sectors.
 * \{/
 */
struct flash_region {
	uint32_t start_addr;    //< Sector start address
	uint16_t num_sectors;   //< Number of sectors
	uint16_t sector_len;	//< Sector length in 256 byte units
};
/** \} */

/**
 * \brief Metadata of a flash chip, describing memory layout.
 * \{
 */
struct flash_chip {
	uint32_t len;                  //< Length of chip in bytes
	uint16_t num_regions;          //< Number of regions in chip
	struct flash_region region[FLASH_REGION_MAX]; //< Region data array
};
/** \} */

/**
 * \brief Initialise flash chip. Call this function only once, before any
 * other function in the module.
 *
 * \warning This function must be called with interrupts disabled and
 * Z80 stopped.
 */
int16_t flash_init(void);

/**
 * \brief Deinitialise flash chip.
 */
void flash_deinit(void);

/**
 * \brief Get chip metadata.
 *
 * \return An immutable reference to the chip metadata.
 */
const struct flash_chip *flash_metadata_get(void);

/**
 * \brief Erases a flash sector. Erased sectors will be read as 0xFF.
 *
 * \param[in] addr Any even address contained in the sector to erase.
 *
 * \note This function erases a complete sector. Use flash_sector_limits()
 * function to obtain the sector limits that will be erased.
 * \warning If addr is not even, this function can lock the machine.
 * \warning This function must be called with interrupts disabled and
 * Z80 stopped.
 *
 * \return The address of the next sector to the one erased on success,
 * 0 on error.
 */
uint32_t flash_sector_erase(uint32_t addr);

/**
 * \brief Programs (writes) a data buffer to the specified flash address.
 *
 * \param[in] addr Address to which data will be programmed.
 * \param[in] data Buffer containing the data to program.
 * \param[in] len  Length of the data buffer to write.
 *
 * \return The number of written bytes on success, 0 on error.
 *
 * \warning Before using this function, make sure the address range to program
 * is erased. Otherwise this function can lock the machine.
 * \warning If addr is not even, this function can lock the machine.
 * \warning If data is not word aligned, this function can lock the machine.
 * \warning If data is not in work RAM, this function can lock the machine.
 * \warning If len is odd, the last byte will not be written.
 * \warning This function must be called with interrupts disabled and
 * Z80 stopped.
 */
uint16_t flash_program(uint32_t addr, const uint8_t *data, uint16_t len);

/**
 * \brief Reads data from the specified flash address.
 *
 * \param[in]  addr Address to read data from.
 * \param[out] data Buffer used to copy readed data.
 * \param[in]  len  Length of the data block to read.
 *
 * \return The number of read bytes.
 *
 * \warning addr must be even.
 * \warning data must be word aligned.
 * \warning If len is odd, the last byte will not be read.
 */
int16_t flash_read(uint32_t addr, uint8_t *data, uint16_t len);

/**
 * \brief Copy data from a flash region to other region also in flash.
 *
 * \param[in] dst Destination address to copy data to.
 * \param[in] src Source address to copy data from
 * \param[in] len Length of the data block to copy.
 *
 * \return The number of copied bytes.
 *
 * \warning dst and src must be even.
 * \warning If len is odd, the last byte will not be copied.
 * \warning Before using this function, make sure the dst region is erased.
 * Otherwise this function can lock the machine.
 * \warning This function must be called with interrupts disabled and
 * Z80 stopped.
 */
int16_t flash_copy(uint32_t dst, uint32_t src, uint16_t len);

/**
 * \brief Obtains the sector limits (start of sector, start of next sector)
 * corresponding to the specified address.
 *
 * \return 0 on success, -1 if input address does not fit in the chip.
 *
 * \param[in]  addr  Address corresponding to the sector to get info from.
 * \param[out] start Start address of the sector. Can be NULL if this
 *             parameter is not needed.
 * \param[out] next  Start address of the sector next to the current one.
 *                   Can be NULL if this parameter is not needed.
 */
int16_t flash_sector_limits(uint32_t addr, uint32_t *start, uint32_t *next);

#endif
