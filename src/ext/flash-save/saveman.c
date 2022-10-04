/*
 * Some concepts to help understanding the code:
 * - Sector: one of the two flash areas (sectors) used to store the data.
 *   A sector can be written in a word granularity, but must be erased in its
 *   entirety.
 * - Sector header (sector_hdr): describes the data stored in one sector. This
 *   data is stored at the beginning of a sector, so it is at flash (not RAM).
 *   At any given moment, only one sector should have a valid header. The only
 *   exception is when one sector is full, code switches to the other sector,
 *   and there is a power cut just after the new sector is written and before
 *   the old sector is erased. In the rare case this happens, the condition is
 *   detected and corrected by the init routines.
 * - Sector metadata (sector_metadata): just the address of the beginning of a
 *   sector that can be accessed as the address value itself or as a pointer
 *   to the sector header. Also includes sector limit address.
 * - Blob (save_blob): data written to the flash memory, consisting of save data
 *   plus some metadata.
 * - Slot: Number of independent data segments that can be saved. E.g. if you
 *   have 3 slots, your game can have 3 independent save files. When you write
 *   a slot, it gets saved. If you write again to the same slot, it gets
 *   overwritten. But if you write to other different slot, the first one
 *   preserves its data. Slot management in this module is a bit tricky, and
 *   special care must be taken when a sector is filled and data must be written
 *   to the other one: in addition to the data currently being saved, all the
 *   data saved in the other slots must be copied too!
 */

#include "config.h"
#include "types.h"

#include "sys.h"
#include "z80_ctrl.h"
#include "memory.h"

#if (MODULE_FLASHSAVE != 0)

#include "ext/flash-save/flash.h"
#include "ext/flash-save/saveman.h"

// Compatibility layer definitions
#define ALLOC(len) MEM_alloc(len)
#define DEALLOC(chunk) do{MEM_free(chunk);}while(0)
#define CRITICAL_ENTER SYS_disableInts();const bool z80_taken=Z80_getAndRequestBus(TRUE)
#define CRITICAL_EXIT if (z80_taken) Z80_releaseBus();SYS_enableInts()

// Magic number at the beginning of sectors
#define MAGIC_NUM 0x44534D21

#ifndef MIN
#define MIN(a, b) (a) < (b) ? (a) : (b)
#endif

// Header of a sector with data stored on flash
struct sector_hdr {
	uint32_t magic;
	uint8_t num_slots; // Number of save slots supported
	uint8_t reserved;  // Reserved, set to 0xFF
};

// Metadata describing a flash sector used for saving data
struct sector_metadata {
	union {
		volatile struct sector_hdr *hdr;
		uint32_t addr;
	};
	uint32_t limit;
};

// Module local data
struct save_manager {
	bool cur_sect; // Sector in use (0 or 1)
	struct sector_metadata sect[2];
	uint32_t last_addr;   // Blob offset for the last slot
	uint32_t blob_addr[]; // Blob offset for each slot
};

// Contains one save blob: metadata + data
struct save_blob {
	uint8_t slot;
	union {
		// Note flags are negated to be able to clear them from flash
		uint8_t flags;
		struct {
			uint8_t done:1;
			uint8_t invalid:1;
			uint8_t cleared:1;
		};
	};
	uint16_t save_len;
	uint16_t data[];
};

// Module global data
static struct save_manager *sm = NULL;

static uint16_t align(uint16_t number, uint16_t power)
{
	const uint16_t mask = (1<<power) - 1;

	if (number & mask) {
		number += (1<<power);
	}

	number &= ~mask;

	return number;
}

static uint16_t save_data_off(void)
{
	return align(sizeof(struct sector_hdr), 2);
}

static int16_t sector_addrs_set(uint32_t max_length_restrict)
{
	const struct flash_chip *flash = flash_metadata_get();

	if (!max_length_restrict) {
		max_length_restrict = MIN(flash->len, 0x400000);
	}
	// We allocate the last two sectors
	flash_sector_limits(max_length_restrict - 1, &sm->sect[1].addr,
			&sm->sect[1].limit);
	// Sector end should match restricted length
	if (sm->sect[1].limit != max_length_restrict) {
		return SM_STAT_PARAM_ERR;
	}

	uint32_t sector_len = sm->sect[1].limit - sm->sect[1].addr;
	flash_sector_limits(sm->sect[1].addr - 1, &sm->sect[0].addr,
			&sm->sect[0].limit);
	// This driver requires both sectors to have the same length
	if (sector_len != sm->sect[1].addr - sm->sect[0].addr) {
		return SM_STAT_PARAM_ERR;
	}

	return SM_STAT_OK;
}

static int16_t sector_init(uint16_t sect_num, uint8_t num_slots)
{
	struct sector_hdr hdr = {
		.magic = MAGIC_NUM,
		.num_slots = num_slots,
		.reserved = 0xFF
	};

	CRITICAL_ENTER;
	// Skip magic to ensure we write it the last
	uint16_t write_len = sizeof(struct sector_hdr) - sizeof(uint32_t);
	uint8_t *write_addr = ((uint8_t*)&hdr) + sizeof(uint32_t);
	int16_t err = write_len != flash_program(sm->sect[sect_num].addr +
			sizeof(uint32_t), write_addr, write_len);
	if (!err) {
		// Write magic
		write_len = sizeof(uint32_t);
		write_addr = ((uint8_t*)&hdr);
		err = sizeof(uint32_t) != flash_program(sm->sect[sect_num].addr,
					write_addr, write_len);
	}
	CRITICAL_EXIT;

	return err;
}

int16_t sm_clear(uint8_t num_slots)
{
	CRITICAL_ENTER;
	int16_t err = (0 == flash_sector_erase(sm->sect[0].addr)) ||
			(0 == flash_sector_erase(sm->sect[1].addr));
	CRITICAL_EXIT;
	if (err) {
		return SM_STAT_HW_ERR;
	}

	// Initialize first sector for use
	err = sector_init(0, num_slots);
	if (err) {
		return SM_STAT_HW_ERR;
	}

	// Mark sector 0 as in use
	sm->cur_sect = 0;

	return SM_STAT_OK;
}

static uint16_t safe_sect_erase(uint32_t addr)
{
	CRITICAL_ENTER;
	const int16_t err = flash_sector_erase(addr);
	CRITICAL_EXIT;

	return err;
}

// Returns 0 if region is erased (all values are 0xFF), or the address of the
// first position free if the zone is not erased
static uint32_t erase_check(uint32_t start_addr, uint32_t limit_addr)
{
	uint16_t *pos = (uint16_t*)start_addr;
	uint16_t *end = (uint16_t*)limit_addr;
	uint32_t result = 0;

	while (pos < end) {
		if (*pos++ != 0xFFFF) {
			result = (uint32_t)pos;
		}
	}

	// If there is a dirty zone, advance to first non-dirty word
	if (result) {
		result += 2;
	}

	return result;
}

static uint16_t blob_len(uint16_t save_len)
{
	return sizeof(struct save_blob) + align(save_len, 2);
}

static uint32_t invalid_mark(volatile const struct save_blob *blob)
{
	struct save_blob invalid = *blob;
	uint32_t blob_addr = (uint32_t)blob;
	invalid.done = 0; // Reminder: flags are reversed
	invalid.invalid = 0;
	invalid.slot = 0; // We need an invalid slot to avoid it being skipped
	uint32_t clean = 0;

	const int16_t err = 2 != flash_program(blob_addr, (uint8_t*)&invalid, 2);
	if (!err) {
		clean = blob_addr + blob_len(blob->save_len);
	}

	return clean;
}

// Write the save length corresponding to the clean address
static int16_t skip_write(uint32_t blob_addr, uint32_t clean)
{
	clean = align(clean, 2);
	uint16_t save_len = clean - blob_addr - sizeof(struct save_blob);

	return 2 != flash_program(blob_addr + 2, (uint8_t*)&save_len, 2);
}

// Perform recovery. If there is dangling data beyond the correctly saved info,
// and assuming normal operation (only this driver is writing to flash and no
// bugs) this can only happen if power was cut while saving data. In this case
// the recovery process must distinguish two different cases:
// 1. save_len was written and only flags/slot were not updated. In this case
//    we just mark the blob as invalid. In this case we must also check the
//    detected dirty area is equal or smaller than the written save_len.
//    Otherwise there is a critical error we cannot currently recover. Note we
//    could try skipping only the dirty zone (that might be smaller than the
//    specified length), but this is tricky since we only can clear bits to 0
//    in the written length, we cannot set bits to 1.
// 2. save_len was not written. In this case we write the found dirty len to
//    save_len and then mark the blob as dirty.
static uint32_t recovery(uint32_t addr, uint32_t clear)
{
	volatile const struct save_blob *blob = (struct save_blob*)addr;
	uint32_t clean_addr = 0;

	if (blob->save_len != 0xFFFF) {
		if (clear <= (addr + blob_len(blob->save_len))) {
			// Blob is not correct, mark it as invalid
			clean_addr = invalid_mark(blob);
		}
	} else {
		int16_t err = skip_write(addr, clear);
		if (!err) {
			clean_addr = invalid_mark(blob);
		}
	}

	return clean_addr;
}

// Returns 1 of blob empty or has error
static int16_t blob_checks(volatile const struct save_blob *blob)
{
	int16_t stat = 0;

	if ((blob->slot >= sm->sect[sm->cur_sect].hdr->num_slots) ||
			0xFFFF == blob->save_len || blob->done) {
		stat = 1;
	}

	return stat;
}

// Populates sm->last_off and sm->blob_off[]
static int16_t slot_pos_update(const struct sector_metadata *sect)
{
	uint16_t i;

	uint32_t addr = sm->last_addr;
	volatile const struct save_blob *blob;

	// Advance until the first word that either has no data or has some
	// invalid data.
	while (addr < sect->limit) {
		blob = (struct save_blob*)addr;
		if (blob_checks(blob)) {
			break;
		}
		// Remember flags are inverted, so this is for valid blobs
		if (blob->invalid) {
			sm->blob_addr[blob->slot] = blob->cleared ? addr : 0;
		}
		addr += blob_len(blob->save_len);
	}
	if (addr >= sect->limit) {
		// Unrecoverable error
		return SM_STAT_ERR;
	}

	sm->last_addr = addr;

	// Check if there is unused data written from addr to the end of
	// the sector. If there is, start the data recovery process
	const uint32_t clear = erase_check(addr, sect->limit);
	if (clear) {
		sm->last_addr = recovery(addr, clear);
	}

	// Check if at least one slot has data available
	for (i = 0; i < sect->hdr->num_slots && 0 == sm->blob_addr[i]; i++);
	if (i == sect->hdr->num_slots) {
		return SM_STAT_WARN_NO_DATA;
	}

	return SM_STAT_OK;
}

static int16_t savedata_probe(uint8_t num_slots)
{
	int16_t err = SM_STAT_OK;
	struct sector_metadata *sect = sm->sect;

	if (MAGIC_NUM == sect[0].hdr->magic) {
		sm->cur_sect = 0;
		// The only case in which the unused sector is not clear, is
		// if there was a power cut while changing slot. If this
		// happens, clear the unused sector.
		if (0xFFFFFFFF != sect[1].hdr->magic) {
			if (safe_sect_erase(sect[1].addr)) {
				return SM_STAT_HW_ERR;
			}
		}
	} else if (MAGIC_NUM == sect[1].hdr->magic) {
		sm->cur_sect = 1;
		if (0xFFFFFFFF != sect[0].hdr->magic) {
			if (safe_sect_erase(sect[0].addr)) {
				return SM_STAT_HW_ERR;
			}
		}
	} else {
		// No sector with game saves found, initialize flash
		if (sm_clear(num_slots)) {
			return SM_STAT_HW_ERR;
		}
		err = SM_STAT_WARN_NO_DATA;
		sm->cur_sect = 0;
	}

	sm->last_addr = sm->sect[sm->cur_sect].addr + save_data_off();

	// When we reach here, sect is pointing to a valid sector in use.
	// Now we have to seek the last blob for each slot
	if (SM_STAT_WARN_NO_DATA != err) {
		err = slot_pos_update(&sm->sect[sm->cur_sect]);
	}

	return err;
}

int16_t sm_init(uint8_t num_slots, uint32_t max_length_restrict)
{
	// We require num_slots to be between 1 and 254
	if (0 == num_slots || 0xFF == num_slots) {
		return SM_STAT_PARAM_ERR;
	}
	if (!sm) {
		// Don't forget to also allocate cur_blob for each slot
		sm = ALLOC(sizeof(struct save_manager) +
				num_slots * sizeof(uint16_t));
		if (!sm) {
			return SM_STAT_HW_ERR;
		}
	}

	CRITICAL_ENTER;
	int16_t err = flash_init();
	CRITICAL_EXIT;
	if (err) {
		sm_deinit();
		return SM_STAT_HW_ERR;
	}

	err = sector_addrs_set(max_length_restrict);
	if (err) {
		return err;
	}

	// Mark all slots as unused
	for (uint16_t i = 0; i < num_slots; i++) {
		sm->blob_addr[i] = 0;
	}

	err = savedata_probe(num_slots);
	if (err) {
		return err;
	}

	return SM_STAT_OK;
}

int16_t sm_load(uint8_t slot, void *save_data, uint16_t len)
{
	uint8_t *buf = save_data;
	if (slot > sm->sect[sm->cur_sect].hdr->num_slots) {
		return SM_STAT_PARAM_ERR;
	}
	if (((uint32_t)save_data) & 1) {
		// save_data must be word aligned
		return SM_STAT_PARAM_ERR;
	}
	if (!sm->blob_addr[slot]) {
		return SM_STAT_ERR;
	}

	volatile const struct save_blob *blob =
		(struct save_blob*)sm->blob_addr[slot];
	const uint16_t slen = MIN(blob->save_len, len);
	const uint32_t addr = (uint32_t)blob->data;
	flash_read(addr, save_data, slen);
	// For odd lengths, we need to read one word more
	// and copy the MSB into buffer
	if (slen & 1) {
		uint8_t last[2];
		flash_read(addr + (slen & ~1), last, 2);
		buf[slen - 1] = last[0];
	}

	return slen;
}

// Returns lenght of copied blob, 0 on error
static uint16_t blob_copy(uint32_t dst, uint32_t blob_addr)
{
	volatile const struct save_blob *blob = (struct save_blob*)blob_addr;
	uint16_t len = blob_len(blob->save_len);

	int16_t err = len != flash_copy(dst, blob_addr, len);

	return err ? 0 : len;
}

static int16_t sector_cross_begin(uint8_t slot)
{
	int16_t err = SM_STAT_OK;
	uint16_t len;
	const bool new_sect = sm->cur_sect ^ 1;
	const struct sector_metadata *ns = &sm->sect[new_sect];
	const struct sector_metadata *os = &sm->sect[sm->cur_sect];
	const uint8_t num_slots = os->hdr->num_slots;

	// Things get a bit convoluted here. First we have to copy data for
	// all the slots excepting the one we were originally writing to
	uint32_t save_addr = ns->addr + save_data_off();
	sm->last_addr = save_addr; // Mark as no used blobs
	CRITICAL_ENTER;
	for (int16_t i = 0; !err && i < num_slots; i++) {
		if (i != slot && sm->blob_addr[i]) {
			len = blob_copy(save_addr, sm->blob_addr[i]);
			if (len) {
				sm->blob_addr[i] = save_addr;
				save_addr += len;
			} else {
				err = TRUE;
			}
		}
	}

	sm->last_addr = save_addr;
	if (!err) {
		// Save all new sector metadata, excepting the magic number
		struct sector_hdr hdr = {
			.num_slots = num_slots,
			.reserved = 0xFF
		};
		err = 2 == flash_program(ns->addr + 4, ((uint8_t*)&hdr) + 4, 2) ?
			SM_STAT_OK : SM_STAT_HW_ERR;
	}
	CRITICAL_EXIT;

	return err;
}

static int16_t blob_save(uint8_t slot, const void *save_data, uint16_t len)
{
	struct save_blob *blob = (struct save_blob*)sm->last_addr;

	CRITICAL_ENTER;
	// Write data
	int16_t err = len == flash_program((uint32_t)blob->data,
			save_data, len) ? SM_STAT_OK : SM_STAT_HW_ERR;

	// Write metadata header, first length, then slot and done flag
	if (!err) {
		struct save_blob hdr = {
			.slot = slot,
			.flags = 0xFF,
			.save_len = len
		};
		hdr.done = 0;
		if (2 != flash_program(((uint32_t)blob) + 2, ((uint8_t*)&hdr) + 2, 2)) {
			err = SM_STAT_HW_ERR;
		}
		if (!err && (2 != flash_program((uint32_t)blob, (uint8_t*)&hdr, 2))) {
			err = SM_STAT_HW_ERR;
		}
	}
	CRITICAL_EXIT;

	if (!err) {
		// Set the current blob address for the corresponding slot
		sm->blob_addr[slot] = sm->last_addr;
		// Advance the next unused blob
		sm->last_addr += blob_len(len);
	}

	return err;
}

static int16_t sector_cross_end(void)
{
	const struct sector_metadata *old_sect = &sm->sect[sm->cur_sect];
	const struct sector_metadata *sect = &sm->sect[sm->cur_sect ^ 1];
	const uint32_t magic = MAGIC_NUM;

	CRITICAL_ENTER;
	// Write magic number
	int16_t err = sizeof(magic) != flash_program(sect->addr,
			(uint8_t*)&magic, sizeof(magic));

	if (!err) {
		// Erase old sector
		err = flash_sector_erase(old_sect->addr);
	}
	if (!err) {
		// Done, change current sector
		sm->cur_sect ^= 1;
	}
	CRITICAL_EXIT;

	return err ? SM_STAT_HW_ERR : SM_STAT_OK;
}

static int16_t sector_cross(uint8_t slot, const void *save_data, uint16_t len)
{
	int16_t err = sector_cross_begin(slot);

	if (!err) {
		err = blob_save(slot, save_data, len);
	}
	if (!err) {
		err = sector_cross_end();
	}

	return err;
}

int16_t sm_save(uint8_t slot, const void *save_data, uint16_t len)
{
	int16_t err = SM_STAT_OK;

	if (slot >= sm->sect[sm->cur_sect].hdr->num_slots) {
		return SM_STAT_PARAM_ERR;
	}
	if (((uint32_t)save_data) & 1) {
		// save_data must be word aligned
		return SM_STAT_PARAM_ERR;
	}

	// Check if we have to cross sector
	if ((sm->last_addr + blob_len(len)) >= sm->sect[sm->cur_sect].limit) {
		err = sector_cross(slot, save_data, len);
	} else {
		err = blob_save(slot, save_data, len);
	}


	return err;
}

int16_t sm_delete(uint8_t slot)
{
	if (slot >= sm->sect[sm->cur_sect].hdr->num_slots) {
		return SM_STAT_PARAM_ERR;
	}
	if (!sm->blob_addr[slot]) {
		// Slot already empty
		return SM_STAT_OK;
	}

	// Update the blob cleared flag
	volatile const struct save_blob *blob =
		(struct save_blob*)sm->blob_addr[slot];

	struct save_blob new_blob = *blob;
	new_blob.cleared = 0; // Remember flags are reversed

	int16_t err = 2 != flash_program((uint32_t)blob, (uint8_t*)&new_blob, 2);
	if (!err) {
		// Mark slot as cleared
		sm->blob_addr[slot] = 0;
	}

	return err ? SM_STAT_HW_ERR : SM_STAT_OK;
}

void sm_deinit(void)
{
	if (sm) {
		DEALLOC(sm);
		sm = NULL;
	}
}

#endif // MODULE_FLASHSAVE