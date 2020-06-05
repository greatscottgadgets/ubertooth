#ifndef __SC_DEBUG_H
#define __SC_DEBUG_H
#include "cfg.h"
#include "ubertooth_usb.h"
#include <ubtbr/console.h>

#define cputc console_putc

void flash_leds(int count, int delay);
void xUSB_ERROR(void);
void xTDMA_ERROR(void);
void xDMA_ERROR(void);
void xOTHER_ERROR(void);
void xWIN(void);
void cprintf(char *fmt, ...);
void early_printf(char *fmt, ...);
void print_hex(uint8_t *pkt, unsigned size);

static inline unsigned get_lr(void)
{
	uint32_t lr;

	__asm__ __volatile__ (
	"\tcpy    %0, lr\n"
	: "=r" (lr));

	return lr;
}
void __attribute__((noreturn)) die (char *fmt, ...);
#define DIE(fmt, ...)	die("DIE|"fmt"\n", ##__VA_ARGS__)

//#define BB_DEBUG(fmt, ...) cprintf(fmt, ##__VA_ARGS__)
#define BB_DEBUG(fmt, ...)
	
#endif
