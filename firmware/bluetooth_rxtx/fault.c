/*
 * Copyright 2019 Mike Ryan
 *
 * This file is part of Project Ubertooth and is released under the
 * terms of the GPL. Refer to COPYING for more information.
 */

#include "ubertooth.h"

// delay for about 30 ms (very rough)
static void delay_30ms(void) {
	volatile int i;
	for (i = 0; i < 3000000; ++i)
		;
}

// flash LEDs a certain number of times
static void flash_leds(int count) {
	int i;

	for (i = 0; i < count; ++i) {
		USRLED_SET;
		RXLED_SET;
		TXLED_SET;
		delay_30ms();
		USRLED_CLR;
		RXLED_CLR;
		TXLED_CLR;
		delay_30ms();
	}
	delay_30ms();
	delay_30ms();
	delay_30ms();
}

void HardFault_Handler(void) {
	uint32_t val = SCB_HFSR;

	while (1) {
		flash_leds(2);

		if (val & (1 << 1)) {
			flash_leds(3); // vector table read
		} else if (val & (1 << 30)) {
			flash_leds(4); // forced (should never happen)
		}

		delay_30ms();
		delay_30ms();
		delay_30ms();
	}
}

void MemManagement_Handler(void) {
	int i;
	int offset[5] = { 7, 4, 3, 1, 0 };
	uint8_t mmsr = SCB_MMSR;

	while (1) {
		flash_leds(3);

		for (i = 0; i < 5; ++i) {
			if (mmsr & (1 << offset[i])) {
				flash_leds(i+1);
				break;
			}
		}

		delay_30ms();
		delay_30ms();
		delay_30ms();
	}
}

void BusFault_Handler(void) {
	int i;
	int offset[6] = { 7, 4, 3, 2, 1, 0 };
	uint8_t bfsr = SCB_BFSR;

	while (1) {
		flash_leds(4);

		for (i = 0; i < 6; ++i) {
			if (bfsr & (1 << offset[i])) {
				flash_leds(i+1);
				break;
			}
		}

		delay_30ms();
		delay_30ms();
		delay_30ms();
	}
}

void UsageFault_Handler(void) {
	int i;
	int offset[6] = { 9, 8, 3, 2, 1, 0 };
	uint16_t ufsr = SCB_UFSR;

	while (1) {
		flash_leds(5);

		for (i = 0; i < 6; ++i) {
			if (ufsr & (1 << offset[i])) {
				flash_leds(i+1);
				break;
			}
		}

		delay_30ms();
		delay_30ms();
		delay_30ms();
	}
}
