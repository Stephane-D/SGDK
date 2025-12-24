#include "config.h"

#if (MODULE_MEGAWIFI == 1)

#include "ext/mw/16c550.h"
#include "task.h"

// Should consider if console is PAL or NTSC
#define UART_MS_TO_FRAMES(ms)	(((ms)*60/500 + 1)/2)

/// Shadow copy of the UART registers
UartShadow sh;

void uart_init(void) {
	// Set line to BR,8N1. LCR[7] must be set to access DLX registers
	UART_LCR = 0x83;
	UART_DLM = UART_DLM_VAL;
	UART_DLL = UART_DLL_VAL;
	uart_set(LCR, 0x03);

	// Enable auto RTS/CTS.
	uart_set(MCR, 0x22);

	// Enable FIFOs, set trigger level to 14 bytes.
	// NOTE: Even though trigger level is 14 bytes, RTS is de-asserted when
	// receiving the first bit of the 16th byte entering the FIFO. See Fig. 9
	// of the SC16C550B datasheet.
	UART_FCR = 0xC1;
	// Reset FIFOs
	uart_set(FCR, 0xC7);

	// Set IER default value (for the shadow register to load).
	uart_set(IER, 0x00);

	// Ready to go! Interrupt and DMA modes were not configured since the
	// Megadrive console lacks interrupt/DMA control pins on cart connector
	// (shame on Masami Ishikawa for not including a single interrupt line!).

    uart_line_sync();

    uart_reset();
	//// Power down and Program not active (required for the module to boot)
	uart_clr_bits(MCR, MW__PRG);
//
	//// Try accessing UART scratch pad register to see if it is installed
	//uart_test(UART_SPR, 0x55); //return MW_ERR
	//uart_test(UART_SPR, 0xAA);
	//// Wait a bit and take module out of resest
	
	uart_start(); 
}

void uart_reset(void) {
	uart_set_bits(MCR, MW__RESET);
}

void uart_start(void) {
	TSK_superPend(MS_TO_FRAMES(30));
	uart_clr_bits(MCR, MW__RESET);
	TSK_superPend(UART_MS_TO_FRAMES(1000));
	uart_set_bits(MCR, MW__PRG);
	uart_reset_fifos(); 
}

u16 uart_get_buff_length(void) {
	return UART_BUFLEN;
}

u16 uart_get_tx_fifo_length(void) {
	return UART_TX_FIFO_LEN;
}
bool uart_is_present(void){
	// Try accessing UART scratch pad register to see if it is installed
	uart_test(UART_SPR, 0x55);
	bool res1 = UART_SPR == 0x55;
	uart_test(UART_SPR, 0xAA);
	return res1 && UART_SPR == 0xAA;
}
bool uart_tx_ready()	{ return UART_LSR & 0x20; }
bool uart_rx_ready()	{ return UART_LSR & 0x01; }
void uart_putc(u8 c)	{ UART_RHR = c; }
u8 uart_getc()		{ return UART_RHR; }
void uart_reset_fifos()	{ uart_set_bits(FCR, 0x07); }

void uart_line_sync(void)
{
	for (int i = 0; i < 256; i++) {
		if (uart_tx_ready()) {
			uart_putc(0x55);
		}
	}
}

#endif // MODULE_MEGAWIFI
