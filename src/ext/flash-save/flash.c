#include "config.h"

#if (MODULE_FLASHSAVE != 0)

#include "types.h"
#include "ext/flash-save/flash.h"

// Put function in .data (RAM) instead of the default .text
#define RAM_SECT __attribute__((section(".ramprog")))
// Avoid function to be inlined by compiler
#define NO_INL __attribute__((noinline))

// Base address for the CFI data
#define CFI_BASE 0x10

// Offset in the CFI data buffer
#define CFI_LENGTH_OFF (0x27 - CFI_BASE)
#define CFI_NUM_REGIONS_OFF (0x2C - CFI_BASE)
#define CFI_REGION_DATA_OFF (0x2D - CFI_BASE)

static struct flash_chip flash = {};

RAM_SECT static void bus_write8(uint32_t addr, uint8_t data)
{
	volatile uint8_t *p_addr = (uint8_t*)addr;

	*p_addr = data;
}

RAM_SECT static uint8_t bus_read8(uint32_t addr)
{
	volatile uint8_t *p_addr = (uint8_t*)addr;

	return *p_addr;
}

RAM_SECT static void bus_write16(uint32_t addr, uint16_t data)
{
	volatile uint16_t *p_addr = (uint16_t*)addr;

	*p_addr = data;
}

RAM_SECT static uint16_t bus_read16(uint32_t addr)
{
	volatile uint16_t *p_addr = (uint16_t*)addr;

	return *p_addr;
}

RAM_SECT static void unlock(void)
{
	bus_write8(0xAAB, 0xAA);
	bus_write8(0x555, 0x55);
}

RAM_SECT static void reset(void)
{
	bus_write8(1, 0xF0);
}

RAM_SECT static void data_poll(uint32_t addr, uint16_t data)
{
	while (bus_read16(addr) != data);
}

// buf must be 45 byte long or greater
RAM_SECT NO_INL static void cfi_read(uint8_t *buf)
{
	// CFI Query
	// NOTE: all chips I have seen accept the CFI Query command while in
	// autoselect mode, but entering autoselect is not required.
	bus_write8(0xAB, 0x98);

	// Read from 0x10 to 0x3C (45 bytes) in byte mode
	for (uint16_t i = 0, addr = 0x21 ; i < 45; i++, addr += 2) {
		buf[i] = bus_read8(addr);
	}

	// Exit CFI Query
	reset();
}

static int16_t metadata_populate(const uint8_t *cfi)
{
	flash.len = 1<<cfi[CFI_LENGTH_OFF];
	flash.num_regions = cfi[CFI_NUM_REGIONS_OFF];

	uint32_t start_addr = 0;
	const uint8_t *cfi_reg = cfi + CFI_REGION_DATA_OFF;
	for (uint16_t i = 0; i < flash.num_regions; i++) {
		struct flash_region *reg = &flash.region[i];

		reg->start_addr = start_addr;
		reg->num_sectors = *cfi_reg++ + 1;;
		reg->num_sectors |= 256 * *cfi_reg++;
		reg->sector_len = *cfi_reg++;
		reg->sector_len |= 256 * *cfi_reg++;

		start_addr += ((uint32_t)reg->sector_len * 256) * reg->num_sectors;
	}

	// The end of computed sector lengths should match the flash length
	if (start_addr != flash.len) {
		return -1;
	}

	return 0;
}

int16_t flash_init(void)
{
	static const uint8_t cfi_check[] = {
		0x51, 0x52, 0x59, 0x02, 0x00
	};
	uint8_t cfi[45];

	// Read CFI data
	cfi_read(cfi);

	// Check response contains QRY and algorithm is AMD compatible
	for (uint16_t i = 0; i < sizeof(cfi_check); i++) {
		if (cfi[i] != cfi_check[i]) {
			// No flash chip installed, or chip not supported
			return -1;
		}
	}

	// Check number of regions is coherent and populate metadata
	const uint8_t num_regions = cfi[CFI_NUM_REGIONS_OFF];
	if (!num_regions || num_regions > FLASH_REGION_MAX) {
		// Between 1 and 4 regions are supported
		return -1;
	}

	return metadata_populate(cfi);
}

const struct flash_chip *flash_metadata_get(void)
{
	return &flash;
}

void flash_deinit(void)
{
	// Nothing to do!
}

int16_t flash_read(uint32_t addr, uint8_t *data, uint16_t len)
{
	// Convert length to words
	uint16_t wlen = len >> 1;
	uint16_t *wbuf = (uint16_t*)data;

	// TODO: could use DMA here
	for (uint16_t i = 0; i < wlen; i++, addr += 2) {
		wbuf[i] = bus_read16(addr);
	}

	return wlen<<1;
}

RAM_SECT NO_INL static void word_program(uint32_t addr, uint16_t value)
{
	unlock();
	bus_write8(0xAAB, 0xA0);
	bus_write16(addr, value);
	data_poll(addr, value);
}

uint16_t flash_program(uint32_t addr, const uint8_t *data, uint16_t len)
{
	const uint16_t wlen = len>>1;
	const uint16_t *wbuf = (uint16_t*)data;

	for (uint16_t i = 0; i < wlen; i++, addr += 2) {
		word_program(addr, wbuf[i]);
	}

	return len;
}

RAM_SECT static void erase_unlock(void)
{
	unlock();
	bus_write8(0xAAB, 0x80);
	bus_write8(0xAAB, 0xAA);
	bus_write8(0x555, 0x55);
}

RAM_SECT NO_INL static void sect_erase(uint32_t addr)
{
	erase_unlock();
	bus_write8(addr + 1, 0x30);
	data_poll(addr, 0xFFFF);
}

int16_t flash_sector_limits(uint32_t addr, uint32_t *start, uint32_t *next)
{
	if (addr >= flash.len) {
		// Requested address does not fit in chip
		return -1;
	}

	// Find in which region is the address
	uint16_t reg_num = flash.num_regions - 1;
	while (addr < flash.region[reg_num].start_addr) {
		reg_num--;
	}

	// Compute limits
	const struct flash_region *reg = &flash.region[reg_num];
	const uint32_t sect_len = ((uint32_t)reg->sector_len) * 256;
	const uint32_t base = addr - reg->start_addr;
	const uint32_t trim = (base & ~(sect_len - 1)) + reg->start_addr;
	if (start) {
		*start = trim;
	}
	if (next) {
		*next = trim + sect_len;
	}

	return 0;
}

uint32_t flash_sector_erase(uint32_t addr)
{
	sect_erase(addr);

	uint32_t next;
	const int16_t err = flash_sector_limits(addr, 0, &next);
	if (err) {
		return 0;
	}

	return next;
}

int16_t flash_copy(uint32_t dst, uint32_t src, uint16_t len)
{
	// Convert to word len
	uint16_t wlen = len>>1;

	// Copy words one by one
	while (wlen--) {
		uint16_t scratch = bus_read16(src);
		word_program(dst, scratch);
		src += 2;
		dst += 2;
	}

	return len;
}

#endif // MODULE_FLASHSAVE