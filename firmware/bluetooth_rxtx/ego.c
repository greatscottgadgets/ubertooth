/*
 * Copyright 2015 Mike Ryan
 *
 * This file is part of Project Ubertooth.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ego.h"

int enqueue_with_ts(u8 type, u8 *buf, u32 ts);
extern volatile u32 clkn;                       // clkn 3200 Hz counter
extern volatile u8 requested_mode;
extern volatile u16 channel;

/*
 * This code is meant to capture the Yuneec E-GO electric skateboard.
 */

static void msleep(uint32_t millis)
{
	uint32_t stop_at = clkn + millis * 3125 / 1000;  // millis -> clkn ticks
	do { } while (clkn < stop_at);                   // TODO: handle wrapping
}

static void ssp_start()
{
	// make sure the (active low) slave select signal is not active
	DIO_SSEL_SET;

	// enable SSP
	DIO_SSP_CR1 |= SSPCR1_SSE;

	// activate slave select pin
	DIO_SSEL_CLR;
}

static void ssp_stop()
{
	// disable CC2400's output (active low)
	DIO_SSEL_SET;

	// disable SSP
	DIO_SSP_CR1 &= ~SSPCR1_SSE;
}

void ego_rx(void)
{
	int i;
	int j;
	u8 len = 36;
	u8 rxbuf[len];
	u16 mdmctrl = 0x0026; // 150 kHz frequency deviation
	u32 rxtime;

	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x134b); // without PRNG
	cc2400_set(GRMDM,   0x04e0); // un-buffered mode, 2FSK
	// 0 00 00 1 001 11 0 00 0 0
	//      |  | |   |  +--------> CRC off
	//      |  | |   +-----------> sync word: 32 MSB bits of SYNC_WORD
	//      |  | +---------------> 1 byte of 01010101
	//      |  +-----------------> packet mode
	//      +--------------------> un-buffered mode
	cc2400_set(FSDIV,   channel - 1); // 1 MHz IF
	cc2400_set(MDMCTRL, mdmctrl);
	cc2400_set(GRDEC,  3); // 250 kbit

    // 630f9ffe86
	cc2400_set(SYNCH,   0x630f);
	cc2400_set(SYNCL,   0x9ffe);

	while (!(cc2400_status() & XOSC16M_STABLE));

	// enable USB interrupts
	ISER0 = ISER0_ISE_USB;

	dio_ssp_init();
	ssp_start();

	while (requested_mode == MODE_EGO_RX) {
		cc2400_strobe(SFSON);
		while (!(cc2400_status() & FS_LOCK));
		while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);

		cc2400_strobe(SRX);
		while (!(cc2400_status() & SYNC_RECEIVED));
		rxtime = clkn;

		RXLED_SET;

		for (j = 0; j < len; j++) {
			// make sure there are bytes ready
			while (!(SSP1SR & SSPSR_RNE)) ;
			rxbuf[j] = (u8)DIO_SSP_DR;
		}
		enqueue_with_ts(EGO_PACKET, rxbuf, rxtime);

		// msleep(50);
		RXLED_CLR;
	}

	cc2400_strobe(SRFOFF);
	ssp_stop();
	ICER0 = ICER0_ICE_USB;
}
