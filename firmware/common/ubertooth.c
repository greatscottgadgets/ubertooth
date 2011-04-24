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

#include "ubertooth.h"

/* delay a number of seconds while on internal oscillator (4 MHz) */
void wait(u8 seconds)
{
	u32 i = 400000 * seconds;
	while (--i);
}

/*
 * This should be called very early by every firmware in order to ensure safe
 * operating conditions for the CC2400.
 */
void gpio_init()
{
	/* 
	 * Set all pins for GPIO.  This shouldn't be necessary after a reset, but
	 * we might get called at other times.
	 */
	PINSEL0 = 0;
	PINSEL1 = 0;
	PINSEL2 = 0;
	PINSEL3 = 0;
	PINSEL4 = 0;
	PINSEL7 = 0;
	PINSEL8 = 0;
	PINSEL9 = 0;
	PINSEL10 = 0;

	/* set certain pins as outputs, all others inputs */
#ifdef UBERTOOTH_ZERO
	FIO0DIR = PIN_USRLED;
	FIO1DIR = (PIN_CC3V3 | PIN_RX | PIN_TX | PIN_CSN |
			PIN_SCLK | PIN_MOSI | PIN_CC1V8 | PIN_BTGR);
	FIO2DIR = PIN_SSEL0;
	FIO3DIR = 0;
	FIO4DIR = (PIN_RXLED | PIN_TXLED);
#endif
#ifdef UBERTOOTH_ONE
	FIO0DIR = 0;
	FIO1DIR = (PIN_USRLED | PIN_RXLED | PIN_TXLED | PIN_CC3V3 |
			PIN_RX | PIN_CC1V8 | PIN_BTGR);
	FIO2DIR = (PIN_CSN | PIN_SCLK | PIN_MOSI | PIN_PAEN | PIN_HGM);
	FIO3DIR = 0;
	FIO4DIR = (PIN_TX | PIN_SSEL1);

	/* set P2.9 as USB_CONNECT */
	PINSEL4 = (PINSEL4 & ~(3 << 18)) | (1 << 18);
#endif

	/* set all outputs low */
	FIO0PIN = 0;
	FIO1PIN = 0;
	FIO2PIN = 0;
	FIO3PIN = 0;
	FIO4PIN = 0;
}

/*
 * Every application that uses the main oscillator (including any that use both
 * USB and the CC2400) should start with this.
 */
void ubertooth_init()
{
	gpio_init();
	cc2400_init();
	clock_start();
}

/* configure SSP for CC2400's secondary serial data interface */
void dio_ssp_init()
{
#ifdef UBERTOOTH_ZERO
	/* set P0.15 as SCK0 */
	PINSEL0 = (PINSEL0 & ~(3 << 30)) | (2 << 30);

	/* set P1.16 as SSEL0 */
	PINSEL1 = (PINSEL1 & ~(3 << 0)) | (2 << 0);

	/* set P1.17 as MISO0 */
	PINSEL1 = (PINSEL1 & ~(3 << 2)) | (2 << 2);

	/* set P1.18 as MOSI0 */
	PINSEL1 = (PINSEL1 & ~(3 << 4)) | (2 << 4);
#endif
#ifdef UBERTOOTH_ONE
	/* set P0.7 as SCK1 */
	PINSEL0 = (PINSEL0 & ~(3 << 14)) | (2 << 14);

	/* set P0.6 as SSEL1 */
	PINSEL0 = (PINSEL0 & ~(3 << 12)) | (2 << 12);

	/* set P0.8 as MISO1 */
	PINSEL0 = (PINSEL0 & ~(3 << 16)) | (2 << 16);

	/* set P0.9 as MOSI1 */
	PINSEL0 = (PINSEL0 & ~(3 << 18)) | (2 << 18);
#endif

	/*
	 * DIO_SSEL is a GPIO output connected directly to the SSEL input on the
	 * microcontroller for the CC2400's un-buffered (DIO/DCLK) serial
	 * interface.  Since the CC2400 doesn't have a slave select output, we
	 * control it with this.  DIO_SSEL should already be configured by
	 * gpio_init().  We set it high by default because it is an active low
	 * signal.
	 */
	DIO_SSEL_SET;

	/* configure DIO_SSP */
	DIO_SSP_CR0 = (0x7 /* 8 bit transfer */ | SSPCR0_CPOL | SSPCR0_CPHA);
	DIO_SSP_CR1 = (SSPCR1_MS | SSPCR1_SOD);
}

void atest_init()
{
	/*
	 * ADC can optionally be configured for ATEST1 and ATEST2, but for now we
	 * set them as floating inputs.
	 */

	/* P0.25 is ATEST1, P0.26 is ATEST2 */
	PINSEL1 &= ~((0x3 << 20) | (0x3 << 18)); // set as GPIO
	FIO0DIR &= ~((0x3 << 25)); // set as input
	PINMODE1 |= ((0x5 << 19)); // no pull-up/pull-down
	PINMODE1 &= ~((0x5 << 18)); // no pull-up/pull-down
}

void cc2400_init()
{
	atest_init();

	/* activate 1V8 supply for CC2400 */
	CC1V8_SET;
	wait(1); //FIXME only need to wait 50us

	/* CSN (slave select) is active low */
	CSN_SET;

	/* activate 3V3 supply for CC2400 IO */
	CC3V3_SET;
}

static void spi_delay()
{
	u32 i = 10;
	while (--i);
}

/*
 * This is a single SPI transaction of variable length, usually 8 or 24 bits.
 * The CC2400 also supports longer transactions (e.g. for the FIFO), but we
 * haven't implemented anything longer than 32 bits.
 *
 * We're bit-banging because:
 *
 * 1. We're using one SPI peripheral for the CC2400's unbuffered data
 *    interace.
 * 2. We're saving the second SPI peripheral for an expansion port.
 * 3. The CC2400 needs CSN held low for the entire transaction which the
 *    LPC17xx SPI peripheral won't do without some workaround anyway.
 */
u32 cc2400_spi(u8 len, u32 data)
{
	u32 msb = 1 << (len - 1);

	/* start transaction by dropping CSN */
	CSN_CLR;

	while (len--) {
		if (data & msb)
			MOSI_SET;
		else
			MOSI_CLR;
		data <<= 1;

		spi_delay();

		SCLK_SET;
		if (MISO)
			data |= 1;

		spi_delay();

		SCLK_CLR;
	}

	spi_delay();

	/* end transaction by raising CSN */
	CSN_SET;

	return data;
}

/* read the value from a register */
u16 cc2400_get(u8 reg)
{
	u32 in;
	u16 val;

	u32 out = (reg | 0x80) << 16;
	in = cc2400_spi(24, out);
	val = in & 0xFFFF;

	return val;
}

/* write a value to a register */
void cc2400_set(u8 reg, u32 val)
{
	u32 out = (reg << 16) | val;
	cc2400_spi(24, out);
}

/* get the status */
u8 cc2400_status()
{
	return cc2400_spi(8, 0);
}

/* strobe register, return status */
u8 cc2400_strobe(u8 reg)
{
	return cc2400_spi(8, reg);
}

/*
 * Warning: This should only be called when running on the internal oscillator.
 * Otherwise use clock_start().
 */
void cc2400_reset()
{
	cc2400_set(MAIN, 0x0000);
	while (cc2400_get(MAIN) != 0x0000);
	cc2400_set(MAIN, 0x8000);
	while (cc2400_get(MAIN) != 0x8000);
}

/* activate the CC2400's 16 MHz oscillator and sync LPC175x to it */
void clock_start()
{
	/* configure flash accelerator for higher clock rate */
	FLASHCFG = (0x03A | (FLASHTIM << 12));

	/* switch to the internal oscillator if necessary */
	CLKSRCSEL = 0;

	/* disconnect PLL0 */
	PLL0CON &= ~PLL0CON_PLLC0;
	PLL0FEED_SEQUENCE;
	while (PLL0STAT & PLL0STAT_PLLC0_STAT);

	/* turn off PLL0 */
	PLL0CON &= ~PLL0CON_PLLE0;
	PLL0FEED_SEQUENCE;
	while (PLL0STAT & PLL0STAT_PLLE0_STAT);

	/* temporarily set CPU clock divider to 1 */
	CCLKCFG = 0;

	/* configure CC2400 oscillator */
	cc2400_reset();
	cc2400_set(IOCFG, (GIO_ZERO << 9) | (GIO_CLK_16M << 3));
	cc2400_strobe(SXOSCON);
	while (!(cc2400_status() & XOSC16M_STABLE));

	/* activate main oscillator */
	SCS = SCS_OSCEN;
	while (!(SCS & SCS_OSCSTAT));

	/*
	 * errata sheet says we must select peripheral clock before enabling and
	 * connecting PLL0
 	 */
	PCLKSEL0  = (2 << 2); /* TIMER0 at cclk/2 (50 MHz) */
	PCLKSEL1  = 0;

	/* switch to main oscillator */
	CLKSRCSEL = 1;

	/* configure PLL0 */
	PLL0CFG = (MSEL0 << 0) | (NSEL0 << 16);
	PLL0FEED_SEQUENCE;

	/* turn on PLL0 */
	PLL0CON |= PLL0CON_PLLE0;
	PLL0FEED_SEQUENCE;
	while (!(PLL0STAT & PLL0STAT_PLLE0_STAT));

	/* set CPU clock divider */
	CCLKCFG = CCLKSEL;

	/* connect PLL0 */
	PLL0CON |= PLL0CON_PLLC0;
	PLL0FEED_SEQUENCE;
	while (!(PLL0STAT & PLL0STAT_PLLC0_STAT));

	/* configure PLL1 */
	PLL1CFG = (MSEL1 << 0) | (PSEL1 << 5);
	PLL1FEED_SEQUENCE;

	/* turn on PLL1 */
	PLL1CON |= PLL1CON_PLLE1;
	PLL1FEED_SEQUENCE;
	while (!(PLL1STAT & PLL1STAT_PLLE1_STAT));
	while (!(PLL1STAT & PLL1STAT_PLOCK1));

	/* connect PLL1 */
	PLL1CON |= PLL1CON_PLLC1;
	PLL1FEED_SEQUENCE;
	while (!(PLL1STAT & PLL1STAT_PLLC1_STAT));
}


/* reset the LPC17xx, the cc2400 will be handled by the boot code */
void reset()
{
	/* Enable the watchdog with reset enabled */
	USRLED_CLR;
	WDMOD |= WDMOD_WDEN | WDMOD_WDRESET;
	WDFEED_SEQUENCE;
	
	/* Set watchdog timeout to 256us (minimum) */
	
	/* sleep for 1s (minimum) */
	wait(1);
}

//FIXME ssp
//FIXME tx/rx
