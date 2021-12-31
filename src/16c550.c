#include "16c550.h"

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
}

