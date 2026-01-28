/************************************************************************//**
 * \brief Flash save manager demonstration.
 *
 * This demonstration uses the saveman module to keep the data of two game save
 * slots (that are supposed to record game progress) and one score table.
 *
 * The program does the following:
 *
 * 1. Initialize the save manager.
 * 2. Print information about the flash chip layout.
 * 3. If there is no saved data, initialize the data. Else load the game data
 *    and the score table.
 * 4. If game data was loaded, print the number of times each slot was saved
 *    and check game data. We check that we saved all data to value 0x55 if
 *    count was even, and to 0xAA if count was odd.
 * 5. Print the score table.
 * 6. Increase scores and save score table.
 * 7. Slot 0 is always saved, while slot 1 is saved only if slot 0 count is
 *    even. For each saved slot:
 *    a. Increase the save count.
 *    b. Fill the save data with 0x55 if save count is odd, or 0xAA if even.
 *    c. Save the data.
 *
 * You have to run the test several times. Each run scores should be increased
 * with respect to the previous run, slot 0 number of saves should increase
 * one time, and slot 1 number of saves should increase 1 time each 2 runs.
 *
 * \author Jesus Alonso (doragasu)
 * \date 10/2022
 ****************************************************************************/

#include <genesis.h>

// Number of entries in the score table
#define NUM_SCORES 8
// Number of game progress slots
#define NUM_GAME_SLOTS 2
// Length of the data blob saved to game progress slots
#define SAVE_DATA_LEN 2048

// The demo uses 2 slots for game save and one for the score table
enum {
	SLOT_GAME_0 = 0,
	SLOT_GAME_1,
	SLOT_SCORE,
	__NUM_SLOTS
};

// A score that will be saved on the score table
struct score {
	uint32_t score;
	char name[16];
};

// The data containing the player progress
struct game_save {
	// Just a counter for demo purposes
	uint16_t count;
	// Here you should add player stats, inventory, stage...
	char data[SAVE_DATA_LEN];
};

// Just some scores to initialize the table
static const struct score score_defaults[NUM_SCORES] = {
	{1000000, "doragasu"},
	{ 900000, "Stef"},
	{ 800000, "MegaWiFi"},
	{ 700000, "The Mojon Twins"},
	{ 600000, "Davidian"},
	{ 500000, "Manu Segura"},
	{ 400000, "Kusfo"},
	{ 300000, "Alfon"},
};

// Global score table
static struct score score_table[NUM_SCORES];
// Global game save data
static struct game_save save[NUM_GAME_SLOTS];
// Line number
static uint16_t line_num;

// Prints a line
static void print_ln(const char *line)
{
	VDP_drawText(line, 1, line_num++);
}

// Prints flash chip metadata
static void flash_data_print(void)
{
	char line[44];
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

// Return 1 if memory region differs from value, 0 otherwise
static int16_t mem_val_diff(const void *mem, uint8_t val, uint16_t len)
{
	for (uint16_t i = 0; i < len; i++) {
		if (((uint8_t*)mem)[i] != val) {
			return 1;
		}
	}

	return 0;
}

static void score_table_print(void)
{
	char line[44];
	const struct score *sc;

	print_ln(" --= HIGH SCORES =--");
	for (uint16_t i = 0; i < NUM_SCORES; i++) {
		sc = &score_table[i];
		sprintf(line, "%10ld %s", sc->score, sc->name);
		print_ln(line);
	}
}

static void score_table_load(void)
{
	char line[44];

	const int16_t err = sm_load(SLOT_SCORE, score_table, sizeof(score_table));
	if (err != sizeof(score_table)) {
		sprintf(line, "score load failed (%d)", err);
		print_ln(line);
	}
}

static void score_save_test(void)
{
	char line[44];

	// Increase by 10000 points each score before saving
	for (uint16_t i = 0; i < NUM_SCORES; i++) {
		score_table[i].score += 10000;
	}

	const int16_t err = sm_save(SLOT_SCORE, score_table, sizeof(score_table));
	if (err) {
		sprintf(line, "score save failed (%d)", err);
		print_ln(line);
	}
}

static void slot_load(uint8_t slot)
{
	char line[44];

	const int16_t err = sm_load(slot, &save[slot], sizeof(struct game_save));
	if (err != sizeof(struct game_save)) {
		sprintf(line, "slot %d load failed (%d)", slot, err);
		print_ln(line);
		return;
	}

	// Print save counter for slot
	sprintf(line, "slot %d saved %d times", slot, save[slot].count);
	print_ln(line);
	// We compare agains 0x55 if count is even, or against 0xAA if odd
	const uint8_t cmp = save[slot].count & 1 ? 0xAA : 0x55;
	if (mem_val_diff(save[slot].data, cmp, SAVE_DATA_LEN)) {
		sprintf(line, "slot %d data error", slot);
	} else {
		sprintf(line, "slot %d checked successfully", slot);
	}
	print_ln(line);
}

static void load_test(void)
{
	slot_load(SLOT_GAME_0);
	slot_load(SLOT_GAME_1);
}

static void slot_save(uint8_t slot)
{
	char line[44];
	save[slot].count++;

	// Fill data with 0x55 if count is even, or with 0xAA if odd
	const uint8_t fill = save[slot].count & 1 ? 0xAA : 0x55;
	memset(save[slot].data, fill, SAVE_DATA_LEN);

	const int16_t err = sm_save(slot, &save[slot], sizeof(struct game_save));
	if (err) {
		sprintf(line, "slot %d save failed (%d)", slot, err);
	} else {
		sprintf(line, "slot %d saved successfully", slot);
	}
	print_ln(line);
}

static void slot_save_test(void)
{
	// For demo purposes, we save slot 0 always,
	// and slot 1 only if slot 0 count is even
	if (0 == (save[SLOT_GAME_0].count & 1)) {
		slot_save(SLOT_GAME_1);
	}
	slot_save(SLOT_GAME_0);
}

static void data_init(void)
{
	memcpy(score_table, score_defaults, sizeof(score_table));
	save[0].count = save[1].count = 0;

	memset(save[0].data, 0x55, SAVE_DATA_LEN);
	memset(save[1].data, 0x55, SAVE_DATA_LEN);
}

int main(__attribute__((unused)) bool hard_reset)
{
	line_num = 1;
	print_ln("Save manager test start!");

	// Init flash
	int16_t err = sm_init(__NUM_SLOTS);
	// Print flash chip metadata. We try doing it even on error cases
	flash_data_print();
	switch (err) {
	case SM_STAT_OK:
		print_ln("Save manager init OK");
		break;

	case SM_STAT_WARN_NO_DATA:
		print_ln("Save manager: no save data available");
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

	// Initialize data or load data if already available
	if (SM_STAT_WARN_NO_DATA == err) {
		data_init();
	} else {
		score_table_load();
		load_test();
	}
	// Print scores
	score_table_print();
	// Save scores
	score_save_test();
	// Save corresponding slots
	slot_save_test();

end:
	print_ln("Save manager test done");
	return 0;
}
