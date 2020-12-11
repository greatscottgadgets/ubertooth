/*
 * Copyright 2020 Mike Ryan
 *
 * This file is part of Project Ubertooth and is released under the
 * terms of the GPL. Refer to COPYING for more information.
 */

#include <stdlib.h>

#include "ubertooth.h"

extern volatile uint8_t requested_mode;

#define NOW T2TC

static void timer2_start(void) {
	PCONP |= PCONP_PCTIM2;
	T2TCR = TCR_Counter_Reset;
	T2PR = 50000-1; // 1 ms
	T2TCR = TCR_Counter_Enable;

	// set up interrupt handler
	ISER0 = ISER0_ISE_TIMER2;
}

static void timer2_stop(void) {
	T2TCR = TCR_Counter_Reset;

	// clear interrupt handler
	ICER0 = ICER0_ICE_TIMER2;

	PCONP &= ~PCONP_PCTIM2;
}

#define BLINK (NOW + 400 + (rand() % 1200))

void xmas_main(void) {
	// enable USB interrupts
	ISER0 = ISER0_ISE_USB;

	timer2_start();

	T2MR0 = BLINK;
	T2MR1 = BLINK;
	T2MR2 = BLINK;

	T2MCR |= TMCR_MR0I | TMCR_MR1I | TMCR_MR2I;

	while (requested_mode == MODE_XMAS)
		asm("WFI"); // wait for interrupt

	timer2_stop();

	// disable USB interrupts
	ICER0 = ICER0_ICE_USB;
}

void TIMER2_IRQHandler(void) {
	if (T2IR & TIR_MR0_Interrupt) {
		T2IR = TIR_MR0_Interrupt;
		if (USRLED) USRLED_CLR; else USRLED_SET;
		T2MR0 = BLINK;
	}
	if (T2IR & TIR_MR1_Interrupt) {
		T2IR = TIR_MR1_Interrupt;
		if (RXLED) RXLED_CLR; else RXLED_SET;
		T2MR1 = BLINK;
	}
	if (T2IR & TIR_MR2_Interrupt) {
		T2IR = TIR_MR2_Interrupt;
		if (TXLED) TXLED_CLR; else TXLED_SET;
		T2MR2 = BLINK;
	}
}
