#include "genesis.h"

#include "lipsum.h"

#define _STR(token) #token
#define STR(token) _STR(token)

#define TEST_ADDR 0x3FFFC0

__attribute__((aligned(2))) static const char lipsum[] = LIPSUM;

static char load_buf[16384];

static void print_ln(const char *line)
{
	static uint16_t line_num = 1;
	VDP_drawText(line, 1, line_num++);
}

static void flash_data_print(void)
{
	char line[40];
	const struct flash_chip *flash = flash_metadata_get();

	sprintf(line, "Flash length: %ld", flash->len);
	print_ln(line);

	sprintf(line, "%d regions:", flash->num_regions);
	print_ln(line);

	for (int16_t i = 0; i < flash->num_regions; i++) {
		sprintf(line, " START: %06lX, SLEN: %ld, NS: %d",
				flash->region[i].start_addr,
				(uint32_t)flash->region[i].sector_len * 256,
				flash->region[i].num_sectors);
		print_ln(line);
	}
}

// memcmp not available on SGDK
static int8_t _memcmp(const char *m1, const char *m2, uint16_t len)
{
	int8_t dif = 0;
	for (uint16_t i = 0; i < len && !dif; i++) {
		dif = m1[i] - m2[i];
	}

	return dif;
}

static void load(uint8_t slot, uint16_t slot_len)
{
	char line[44];
	int16_t readed;

	readed = sm_load(slot, load_buf, slot_len);
	if (readed != slot_len) {
		if (readed < 0) {
			sprintf(line, "slot %d has no data", slot);
		} else {
			sprintf(line, "slot %d wanted %d, got %d", slot,
					slot_len, readed);
		}
	} else {
		if (0 == _memcmp(lipsum, load_buf, slot_len)) {
			sprintf(line, "slot %d(%d) matches", slot, slot_len);
		} else {
			sprintf(line, "slot %d does not match", slot);
		}
	}
	print_ln(line);
}

static void load_test(uint16_t sl0_len, uint16_t sl1_len, uint16_t sl2_len)
{
	if (sl0_len) {
		load(0, sl0_len);
	}
	if (sl1_len) {
		load(1, sl1_len);
	}
	if (sl2_len) {
		load(2, sl2_len);
	}
}

static int16_t save(uint8_t slot, uint16_t len)
{
	char line[44];
	const int16_t err = sm_save(slot, lipsum, len);

	sprintf(line, "slot %d(%d) save returned %d", slot, len, err);
	print_ln(line);

	return err;
}

static void save_test(uint16_t sl0_len, uint16_t sl1_len, uint16_t sl2_len)
{
	int16_t err = 0;

	if (sl0_len) {
		err = save(0, sl0_len);
	}
	if (!err && sl1_len) {
		err = save(1, sl1_len);
	}
	if (!err && sl2_len) {
		err = save(2, sl2_len);
	}
}

static void delete_test(uint8_t slot)
{
	char line[44];

	int16_t err = sm_delete(slot);

	sprintf(line, "sm_delete(%d) returned %d", slot, err);
	print_ln(line);
}

int main(__attribute__((unused)) bool hard_reset)
{
	print_ln("Save manager test start!");

	// Init flash, restrit top 64 KiB (for compatibility with the WiFi BL)
	int16_t err = sm_init(3, 0x3F0000);
	flash_data_print();
	switch (err) {
	case SM_STAT_OK:
		print_ln("Save manager init OK");
		break;

	case SM_STAT_WARN_NO_DATA:
		print_ln("Save manager: no save data");
		break;

	case SM_STAT_HW_ERR:
		print_ln("Save manager reported HW error");
		goto end;

	case SM_STAT_PARAM_ERR:
		print_ln("Save manager invalid parameter");
		goto end;

	default:
		print_ln("Save manager unknown error");
		goto end;
	}

	load_test(384, 3000, 7000);
//	delete_test(1);
	save_test(384, 3000, 7000);

end:
	print_ln("Save manager test done");
	return 0;
}
