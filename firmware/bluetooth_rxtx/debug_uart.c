#include <stdarg.h>
#include <stdio.h>

#include "ubertooth.h"

char debug_buffer[256];

void debug_uart_init(int flow_control) {
	// power on UART1 peripheral
	PCONP |= PCONP_PCUART1;

	// 8N1, enable access to divisor latches
	U1LCR = 0b10000011;

	// divisor: 11, fractional: 3/13. final baud: 115,411
	U1DLL = 11;
	U1DLM = 0;
	U1FDR = (3 << 0) | (13 << 4);

	// block access to divisor latches
	U1LCR &= ~0b10000000;

	// enable auto RTS/CTS
	if (flow_control)
		U1MCR = 0b11000000;
	else
		U1MCR = 0;

	// enable FIFO and DMA
	// U1FCR = 0b1001;
	U1FCR = 0b0001; // XXX no DMA, just FIFO

	// set P0.15 as TXD1, with pullup
	PINSEL0  = (PINSEL0  & ~(0b11 << 30)) | (0b01 << 30);
	PINMODE0 = (PINMODE0 & ~(0b11 << 30)) | (0b00 << 30);

	// set P0.16 as RXD1, with pullup
	PINSEL1  = (PINSEL1  & ~(0b11 <<  0)) | (0b01 <<  0);
	PINMODE1 = (PINMODE1 & ~(0b11 <<  0)) | (0b00 <<  0);

	if (flow_control) {
		// set P0.17 as CTS1, no pullup/down
		PINSEL1  = (PINSEL1  & ~(0b11 <<  2)) | (0b01 <<  2);
		PINMODE1 = (PINMODE1 & ~(0b11 <<  2)) | (0b10 <<  2);

		// set P0.22 as RTS1, no pullup/down
		PINSEL1  = (PINSEL1  & ~(0b11 << 12)) | (0b01 << 12);
		PINMODE1 = (PINMODE1 & ~(0b11 << 12)) | (0b10 << 12);
	}
}

// synchronously write a string to debug UART
// does not start any DMA
void debug_write(const char *str) {
	unsigned i;

	for (i = 0; str[i]; ++i) {
		while ((U1LSR & U1LSR_THRE) == 0)
			;
		U1THR = str[i];
	}
}

void debug_printf(char *fmt, ...) {
	va_list ap;
	void *ret;

	va_start(ap, fmt);
	vsnprintf(debug_buffer, sizeof(debug_buffer) - 1, fmt, ap);
	va_end(ap);
	debug_buffer[sizeof(debug_buffer) - 1] = 0;

	// TODO convert this to DMA
	char *p;
	for (p = debug_buffer; *p; ++p) {
		while (!(U1LSR & U1LSR_THRE))
			;
		U1THR = *p;
	}
}
