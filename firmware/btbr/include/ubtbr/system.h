#ifndef __SYSTEM_H
#define __SYSTEM_H
#include <stdint.h>

static inline uint32_t irq_save_disable(void)
{
	uint32_t primask;

	__asm__ __volatile__ (
	"\tmrs    %0, primask\n"
	"\tcpsid  i\n"
	: "=r" (primask)
	:
	: "memory");

	return primask;
}

static inline void irq_restore(uint32_t primask)
{
	__asm__ __volatile__ (
	"msr primask, %0"
	:
	: "r"(primask)
	: "memory");
}

#endif
