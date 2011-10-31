/*
 * Copyright 2010, 2011 Michael Ossmann
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

#include "tc13badge.h"
#include "cc2400.h"

static void spi_delay()
{
	int i;
	for (i = 13; i; i--)
		asm("nop");

}

static void wait()
{
	uint16_t i;
	for (i = 2197; i; i--)
		asm("nop");
}

void cc2400_init()
{
	/* CSN (slave select) is active low */
	CSN_SET;

	/* set up GPIO pins */
	prcr.prc2 = 1; // write enable pd0
	pd0.b5 = 1;    // CC3V3 output

	pd1.b0 = 1; // CC1V8 output
	pd1.b3 = 1; // CSN output
	/*
	pd1.b6 = 1;  // SCLK output
	pd1.b4 = 1;  // SI output
	pd1.b5 = 0;  // SO input
	pur0.b3 = 1; // enable SO pull-up
	*/

	/* activate 1V8 supply for CC2400 */
	CC1V8_SET;
	wait();

	/* activate 3V3 supply for CC2400 IO */
	CC3V3_SET;
}

void cc2400_off(void)
{
	CC3V3_CLR;
	CSN_CLR;
	CC1V8_CLR;
}

/*
 * one byte of a SPI transaction
 * caller must drop CSN
 */
uint8_t cc2400_spi_byte(uint8_t data)
{
	while (!u0c1.ti);
	u0tbl = data;
	while (!u0c1.ri);
	return u0rb;
}

/* alternative implementation, slower, without UART */
uint8_t cc2400_spi_byte_bitbang(uint8_t data)
{
	int len = 8;
	uint8_t msb = 1 << (len - 1);

	while (len--) {
		if (data & msb)
			SI_SET;
		else
			SI_CLR;
		data <<= 1;

		//spi_delay();

		SCLK_SET;
		if (SO)
			data |= 1;

		//spi_delay();

		SCLK_CLR;
	}

	return data;
}

/* read 16 bit value from a register */
uint16_t cc2400_get(uint8_t reg)
{
	uint8_t status;
	uint16_t val;

	/* start transaction by dropping CSN */
	//spi_delay();
	CSN_CLR;

	status = cc2400_spi_byte(reg | 0x80);
	val = cc2400_spi_byte(0);
	val <<= 8;
	val |= cc2400_spi_byte(0);

	/* end transaction by raising CSN */
	//spi_delay();
	CSN_SET;

	return val;
}

/* write 16 bit value to a register */
void cc2400_set(uint8_t reg, uint16_t val)
{
	uint8_t status;

	/* start transaction by dropping CSN */
	//spi_delay();
	CSN_CLR;

	status = cc2400_spi_byte(reg);
	cc2400_spi_byte((val >> 8) & 0xFF);
	cc2400_spi_byte(val & 0xFF);

	/* end transaction by raising CSN */
	//spi_delay();
	CSN_SET;
}

/* read 8 bit value from a register */
uint8_t cc2400_get8(uint8_t reg)
{
	uint8_t status;
	uint8_t val;

	/* start transaction by dropping CSN */
	//spi_delay();
	CSN_CLR;

	status = cc2400_spi_byte(reg | 0x80);
	val = cc2400_spi_byte(0);

	/* end transaction by raising CSN */
	//spi_delay();
	CSN_SET;

	return val;
}

/* write 8 bit value to a register */
void cc2400_set8(uint8_t reg, uint8_t val)
{
	uint8_t status;

	/* start transaction by dropping CSN */
	//spi_delay();
	CSN_CLR;

	status = cc2400_spi_byte(reg);
	cc2400_spi_byte(val);

	/* end transaction by raising CSN */
	//spi_delay();
	CSN_SET;
}

/* get the status */
uint8_t cc2400_status()
{
	return cc2400_strobe(0);
}

/* strobe register, return status */
uint8_t cc2400_strobe(uint8_t reg)
{
	uint8_t status;

	/* start transaction by dropping CSN */
	//spi_delay();
	CSN_CLR;

	status = cc2400_spi_byte(reg);

	/* end transaction by raising CSN */
	//spi_delay();
	CSN_SET;

	return status;
}

void cc2400_reset()
{
	cc2400_set(MAIN, 0x0000);
	while (cc2400_get(MAIN) != 0x0000);
	cc2400_set(MAIN, 0x8000);
	while (cc2400_get(MAIN) != 0x8000);
}

/* activate the CC2400's 16 MHz oscillator */
void cc2400_clock_start()
{
	cc2400_reset();
	cc2400_strobe(SXOSCON);
	while (!(cc2400_status() & XOSC16M_STABLE));
}

/* check to see if we can read a register */
void cc2400_register_test(void)
{
	while (cc2400_get(MANFIDL) != 0x133D) wait();
}

int cc2400_range_test(void)
{
	int i;
	int j;
	int tries = 28561;
	int success = 1;
	const uint8_t len = 22;
	const uint16_t channel = 2441;
	uint8_t pa = 0;
	uint8_t txbuf[22];
	uint8_t rxbuf[22];

	txbuf[0] = len - 1; // length of data (rest of payload)
	txbuf[1] = 0; // request
	txbuf[2] = 13;
	txbuf[3] = 13;
	txbuf[4] = 13;
	txbuf[5] = 13;
	txbuf[6] = 13;
	txbuf[7] = 13;
	txbuf[8] = 13;
	txbuf[9] = 13;
	txbuf[10] = 13;
	txbuf[11] = 13;
	txbuf[12] = 13;
	txbuf[13] = 13;
	txbuf[14] = 13;
	txbuf[15] = 13;
	txbuf[16] = 13;
	txbuf[17] = 13;
	txbuf[18] = pa; // request pa
	txbuf[19] = 0; // request number
	txbuf[20] = 0xff; // reply pa
	txbuf[21] = 0xff; // reply number

	// Bluetooth-like modulation
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x134b);
	cc2400_set(FSDIV,   channel);
	cc2400_set(SYNCH,   0xf9ae);
	cc2400_set(SYNCL,   0x1584);
	cc2400_set(FREND,   8 | pa);
	cc2400_set(MDMCTRL, 0x0029);
	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	for (pa = 0; pa < 5; pa++) { // could go to 7, but hard on battery
		cc2400_set(FREND, 8 | pa);
		txbuf[18] = pa;
		for (i = 0; i < 4; i++) {
			txbuf[19] = i;
			while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
			// transmit a packet
			for (j = 0; j < len; j++)
				cc2400_set8(FIFOREG, txbuf[j]);
			cc2400_strobe(STX);
		}
	}
	// sent packet, now look for repeated packet
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK));
	cc2400_set(FSDIV, channel - 1);
	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	while (1) {
		while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
		cc2400_strobe(SRX);
		while ((!(cc2400_status() & SYNC_RECEIVED)) && --tries);
		if (!tries) {
			success = 0;
			break;
		}
		for (j = 0; j < len; j++)
			rxbuf[j] = cc2400_get8(FIFOREG);
		if (cc2400_status() & STATUS_CRC_OK)
			break;
	}

	// done
	//while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK));

	// make sure rx packet is as expected
	txbuf[1] = 1; // expected value in rxbuf
	for (i = 0; i < 18; i++)
		if (rxbuf[i] != txbuf[i])
			success = 0;

	return success;
}

void init_button(void)
{
	pd3.b3 = 0;  // set p3_3 as input
	pur0.b6 = 1; // enable p3_3 pull-up resistor
}

/*
 * Initialize INT3 interrupt for button SW1.  After calling this once,
 * stop_until_button() may be used repeatedly and an interrupt service
 * routinge may be set up for other uses of the button.
 */
void int3_init(void)
{
	DISABLE_INTERRUPTS;
	int3ic = 0x00;
	inten.int3en = 1;
	//intf.b = 0xC0; // filter INT3 with f32
	intf.b = 0x00; // filter disabled for stop mode compatibility
	int3ic = 0x01; // set INT3 interrupt priority
	FOUR_NOPS;
	ENABLE_INTERRUPTS;
}

/* set GPIO pins to outputs for 13 LEDs */
void init_leds(void)
{
	pd3.b5 = 1;
	pd3.b4 = 1;
	pd1.b1 = 1;
	pd1.b2 = 1;
	pd1.b7 = 1;
	pd2.b0 = 1;
	pd2.b1 = 1;
	pd2.b2 = 1;
	pd2.b3 = 1;
	pd2.b4 = 1;
	pd2.b5 = 1;
	pd2.b6 = 1;
	pd2.b7 = 1;
}

void set_led(uint8_t led, uint8_t state)
{
	if (state) {
		switch (led) {
		case 0:
			LED1_SET;
			break;
		case 1:
			LED2_SET;
			break;
		case 2:
			LED3_SET;
			break;
		case 3:
			LED4_SET;
			break;
		case 4:
			LED5_SET;
			break;
		case 5:
			LED6_SET;
			break;
		case 6:
			LED7_SET;
			break;
		case 7:
			LED8_SET;
			break;
		case 8:
			LED9_SET;
			break;
		case 9:
			LED10_SET;
			break;
		case 10:
			LED11_SET;
			break;
		case 11:
			LED12_SET;
			break;
		case 12:
			LED13_SET;
			break;
		}
	} else {
		switch (led) {
		case 0:
			LED1_CLR;
			break;
		case 1:
			LED2_CLR;
			break;
		case 2:
			LED3_CLR;
			break;
		case 3:
			LED4_CLR;
			break;
		case 4:
			LED5_CLR;
			break;
		case 5:
			LED6_CLR;
			break;
		case 6:
			LED7_CLR;
			break;
		case 7:
			LED8_CLR;
			break;
		case 8:
			LED9_CLR;
			break;
		case 9:
			LED10_CLR;
			break;
		case 10:
			LED11_CLR;
			break;
		case 11:
			LED12_CLR;
			break;
		case 12:
			LED13_CLR;
			break;
		}
	}
}

void all_leds_on(void)
{
	LED1_SET;
	LED2_SET;
	LED3_SET;
	LED4_SET;
	LED5_SET;
	LED6_SET;
	LED7_SET;
	LED8_SET;
	LED9_SET;
	LED10_SET;
	LED11_SET;
	LED12_SET;
	LED13_SET;
}

void all_leds_off(void)
{
	LED1_CLR;
	LED2_CLR;
	LED3_CLR;
	LED4_CLR;
	LED5_CLR;
	LED6_CLR;
	LED7_CLR;
	LED8_CLR;
	LED9_CLR;
	LED10_CLR;
	LED11_CLR;
	LED12_CLR;
	LED13_CLR;
}

/* set up uart0 for CC2400 communication */
void uart_init(void)
{
	pd1.b5 = 0;    // set p1_5 as input
	s0tic = 0x00;  // disable transmit interrupt
	s0ric = 0x00;  // disable receive interrupt
	u0c1.te = 0;   // disable transmit
	u0c1.re = 0;   // disable receive
	u0mr.b = 0x01; // activate synchronous serial mode
	u0c0.b = 0x80; // transfer MSB first
	u0c1.b = 0x00;
	u0brg = 3;     // fast clock rate
	u0c1.te = 1;   // enable transmit
	u0c1.re = 1;   // enable receive
}

void uart_off(void)
{
	u0c1.te = 0; // disable transmit
	u0c1.re = 0; // disable receive
	u0mr.b = 0;  // disable uart0
}

/*
 * Enter stop mode (low power, clocks stopped) until button SW1
 * is pressed.  See Renesas Application Note REJ05B1064-0102.
 *
 * This requires that int3_init() has been executed once.
 *
 * Also, if r8c_takeover_ini() has been called, a wake-up will
 * occur if the R8C_CTL line is toggled by the LPC175x.
 */
void stop_until_button(void)
{
	prcr.prc0 = 1; // disable control register protection
	cm1.b0 = 1; // enter stop mode
	//asm("jmp ?+");
	//asm("?:");
	FOUR_NOPS;
	prcr.prc0 = 1; // disable control register protection
	cm1.b6 = 0; // no CPU clock division
	cm1.b7 = 0;
	cm0.b6 = 0; // enable division change
	prcr.prc0 = 0; // enable control register protection
}

void r8c_takeover_init(void)
{
	/* R8C_ACK is active low */
	R8C_ACK_SET;

	/* set up GPIO pins */
	prcr.prc2 = 1; // write enable pd0
	pd0.b0 = 1;    // R8C_ACK output
	pd4.b5 = 0;    // R8C_CTL input
	pur1.b1 = 1;   // enable R8C_CTL (p4_5) pull-up resistor 

	/* set up INT0 interrupt (R8C_CTL) */
	DISABLE_INTERRUPTS;
	int0ic = 0x00;
	inten.int0en = 1;
	inten.int0pl = 1; // trigger on both edges
	intf.b = 0x00; // filter disabled for stop mode compatibility
	int0ic = 0x01; // set INT3 interrupt priority
	FOUR_NOPS;
	ENABLE_INTERRUPTS;
}

/* allow LPC175x to take control */
void r8c_takeover(void)
{
	all_leds_off();
	uart_off();
	cc2400_off();
	prcr.prc2 = 1; // write enable pd0
	pd0.b5 = 0;    // CC3V3 input
	pd1.b0 = 0;    // CC1V8 input
	pd1.b3 = 0;    // CSN input
	R8C_ACK_CLR;
}

/* take control back from LPC175x */
void r8c_take_back(void)
{
	R8C_ACK_SET;
	cc2400_init();
	uart_init();
	cc2400_clock_start();
}
