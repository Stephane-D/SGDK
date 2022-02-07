/****************************************************************************
 * \brief Simple multitasking demonstration.
 *
 * This program perform parallel execution of 3 "tasks":
 * - The background task bg_tsk(), executes in user (non privileged) context,
 *   and constantly increments a 32-bit counter.
 * - The foreground task fg_tsk(), executes in privileged context and when
 *   running, once per frame prints the counter value.
 * - The vertical blanking interrupt vint_cb(), executes in exception context
 *   and prints the frame count.
 *
 * When running the example, you should see the following:
 * 1. The first row of the screen shows the frame counter, implemented in the
 *    vertical blanking interrupt. It should run non-stop.
 * 2. In the second row of the screen, you will see the count increased by the
 *    background task printed in the screen by the foreground task.
 * 3. When the count passes 0x10000, the foreground task will pend for 180
 *    frames. In the meantime the background task will continue counting, but
 *    the count value will not be updated on the screen (because the foreground
 *    task is locked).
 * 4. When the 180 frame timeout elapses, the foreground task will be unlocked
 *    and will continue printing the counter on the screen. The third row should
 *    show "TIMEOUT", to inform that the lock ended due to timeout.
 * 5. When the counter passes 0x80000, the foreground task will pend forever.
 *    Again, the background task will continue running (increasing the counter)
 *    but the value will not be updated on the screen.
 * 6. When the count passes 0x100000, the background task will unlock the
 *    foreground task. The counter value will again be updated on the screen,
 *    and the third row will show "UNLOCKED" to inform that the foreground task
 *    has been unlocked by the background task (i.e. no timeout has occurred).
 *
 * \author Jesus Alonso (doragasu)
 * \date 01/2022
 ****************************************************************************/

#include "genesis.h"

/// Counter to be incremented by background task
static volatile uint32_t count = 0;

/// Draw a hexadecimal number at specified coords
static void draw_hex(uint32_t value, uint16_t x, uint16_t y)
{
	char num[9];

	intToHex(value, num, 8);
	num[8] = '\0';
	VDP_drawText(num, x, y);
}

/// Vertical interrupt callback, just increment and print frame count
static void vint_cb(void)
{
	static uint32_t frame_count = 0;

	draw_hex(frame_count++, 1, 1);
}

/// Background task running in user mode. Will take all the available CPU
/// not used by the foreground task
static void bg_tsk(void)
{
	uint16_t posted = FALSE;
	while (TRUE) {
		count++;
		if (!posted && count >= 0x100000) {
			posted = TRUE;
			TSK_superPost(FALSE);
		}
	}
}

/// Pend and print reason when resumed
static void pend_check(uint16_t timeout)
{
	const char *message;

	VDP_drawText("PENDING ", 1, 3);
	if (TSK_superPend(timeout)) {
		message = "TIMEOUT ";
	} else {
		message = "UNLOCKED";
	}
	VDP_drawText(message, 1, 3);

}

/// Foreground task running in privileged mode. Will be awakened once per frame
static void fg_tsk(void)
{
	while (TRUE) {
		draw_hex(count, 1, 2);
		if (count > 0x10000 && count < 0x20000) {
			pend_check(180);
		} else if (count > 0x80000 && count < 0x90000) {
			pend_check(TSK_PEND_FOREVER);
		} else {
			SYS_doVBlankProcess();
		}
	}
}

/// Entry point
int main(uint16_t __attribute__((unused)) hard)
{
	// Configure vertical blanking interrupt callback
	SYS_setVIntCallback(vint_cb);
	// Configure background task as user task
	TSK_userSet(bg_tsk);

	// Start foreground task
	fg_tsk();

	// We should never reach here
	return 0;
}
