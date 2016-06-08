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

#define IAP_LOCATION 0x1FFF1FF1
const IAP_ENTRY iap_entry = (IAP_ENTRY)IAP_LOCATION;

/* delay a number of seconds while on internal oscillator (4 MHz) */
void wait(u8 seconds)
{
	wait_us(seconds * 1000000);
}

/* delay a number of milliseconds while on internal oscillator (4 MHz) */
void wait_ms(u32 ms)
{
	wait_us(ms * 1000);
}

/* efficiently reverse the bits of a 32-bit word */
u32 rbit(u32 value) {
  u32 result = 0;
  asm("rbit %0, %1" : "=r" (result) : "r" (value));
  return result;
}

/* delay a number of microseconds while on internal oscillator (4 MHz) */
/* we only have a resolution of 1000/400, so to the nearest 2.5        */
static volatile u32 wait_us_counter;
void wait_us(u32 us)
{
	/* This is binary multiply by ~0.3999, i.e, multiply by
	   0.011011011b. The loop also contains 6 instructions at -Os, so
	   why this factor works is not at all related to the comment
	   above ;-) */
	wait_us_counter =
		(us>>2) + (us>>3) + (us>>6) + (us>>7) + (us>>10) + (us>>11);
	while(--wait_us_counter);
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
	all_pins_off();

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
#endif
#ifdef TC13BADGE
	/*
	 * Leave all the CC2400 pins configured as inputs until cc2400_init() is
	 * called so that we don't interfere with the R8C's control of the CC2400.
	 */
	FIO0DIR = 0;
	FIO1DIR = PIN_R8C_CTL;
	FIO2DIR = 0;
	FIO3DIR = 0;
	FIO4DIR = 0;
#endif

	/* set all outputs low */
	FIO0PIN = 0;
	FIO1PIN = 0;
	FIO2PIN = 0;
	FIO3PIN = 0;
	FIO4PIN = 0;

#ifdef TC13BADGE
	/* R8C_CTL is active low */
	R8C_CTL_SET;
#endif
}

void all_pins_off(void)
{
	/* configure all pins for GPIO */
	PINSEL0 = 0;
	PINSEL1 = 0;
	PINSEL2 = 0;
	PINSEL3 = 0;
	PINSEL4 = 0;
	PINSEL7 = 0;
	PINSEL9 = 0;
	PINSEL10 = 0;

	/* configure all pins as inputs */
	FIO0DIR = 0;
	FIO1DIR = 0;
	FIO2DIR = 0;
	FIO3DIR = 0;
	FIO4DIR = 0;

	/* pull-up on every pin */
	PINMODE0 = 0;
	PINMODE1 = 0;
	PINMODE2 = 0;
	PINMODE3 = 0;
	PINMODE4 = 0;
	PINMODE7 = 0;
	PINMODE9 = 0;

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
#if defined UBERTOOTH_ONE || defined TC13BADGE
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
	FIO0DIR &= ~(0x3 << 25); // set as input
	PINMODE1 |= (0x5 << 19); // no pull-up/pull-down
	PINMODE1 &= ~(0x5 << 18); // no pull-up/pull-down
}

void cc2400_init()
{
#ifdef TC13BADGE
	/* request CC2400 control from the R8C */
	r8c_takeover();

	/* configure output pins for CC2400 control */
	FIO0DIR = 0;
	FIO1DIR = (PIN_CC3V3 | PIN_CC1V8 | PIN_CSN | PIN_SCLK | PIN_MOSI |
			PIN_R8C_CTL);
	FIO2DIR = (PIN_CSN | PIN_SCLK | PIN_MOSI);
	FIO3DIR = 0;
	FIO4DIR = PIN_SSEL1;

	/* set all outputs low */
	FIO0PIN = 0;
	FIO1PIN = 0; /* assuming we have already asserted R8C_CTL low */
	FIO2PIN = 0;
	FIO3PIN = 0;
	FIO4PIN = 0;
#else
	atest_init();
#endif

	/* activate 1V8 supply for CC2400 */
	CC1V8_SET;
	wait_us(50);

	/* CSN (slave select) is active low */
	CSN_SET;

	/* activate 3V3 supply for CC2400 IO */
	CC3V3_SET;

	/* initialise various cc2400 settings - see datasheet pg63 */
	cc2400_set(MANAND,  0x7fff);
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

		SCLK_SET;
		if (MISO)
			data |= 1;

		SCLK_CLR;
	}

	/* end transaction by raising CSN */
	CSN_SET;

	return data;
}

/* read 16 bit value from a register */
u16 cc2400_get(u8 reg)
{
	u32 in;

	u32 out = (reg | 0x80) << 16;
	in = cc2400_spi(24, out);
	return in & 0xFFFF;
}

/* write 16 bit value to a register */
void cc2400_set(u8 reg, u16 val)
{
	u32 out = (reg << 16) | val;
	cc2400_spi(24, out);
}

/* read 8 bit value from a register */
u8 cc2400_get8(u8 reg)
{
	u16 in;

	u16 out = (reg | 0x80) << 8;
	in = cc2400_spi(16, out);
	return in & 0xFF;
}

/* write 8 bit value to a register */
void cc2400_set8(u8 reg, u8 val)
{
	u32 out = (reg << 8) | val;
	cc2400_spi(16, out);
}

static volatile u32 delay_counter;
static void spi_delay() {
       delay_counter = 10;
       while (--delay_counter);
}


/* write multiple bytes to SPI */
void cc2400_fifo_write(u8 len, u8 *data) {
	u8 msb = 1 << 7;
	u8 reg = FIFOREG;
	u8 i, j, temp;

	/* start transaction by dropping CSN */
	CSN_CLR;

	for (i = 0; i < 8; ++i) {
		if (reg & msb)
			MOSI_SET;
		else
			MOSI_CLR;
		reg <<= 1;
		SCLK_SET;
		SCLK_CLR;
	}

	for (i = 0; i < len; ++i) {
		temp = data[i];
		for (j = 0; j < 8; ++j) {
			if (temp & msb)
				MOSI_SET;
			else
				MOSI_CLR;
			temp <<= 1;
			SCLK_SET;
			SCLK_CLR;
		}
	}

	// this is necessary to clock in the last byte
	for (i = 0; i < 8; ++i) {
		SCLK_SET;
		SCLK_CLR;
	}
	
	spi_delay();
	/* end transaction by raising CSN */
	CSN_SET;
}

/* read multiple bytes from SPI */
void cc2400_fifo_read(u8 len, u8 *buf) {
	u8 msb = 1 << 7;
	u8 i, j, temp, reg;
	// Set first bit because it's a read
	reg = 0x80 | FIFOREG;

	/* start transaction by dropping CSN */
	CSN_CLR;

	for (i = 0; i < 8; ++i) {
		if (reg & msb)
			MOSI_SET;
		else
			MOSI_CLR;
		reg <<= 1;
		SCLK_SET;
		SCLK_CLR;
	}

	for (i = 0; i < len; ++i) {
		temp = 0;
		for (j = 0; j < 8; ++j) {
			spi_delay();
			SCLK_SET;
			temp <<= 1;
			if (MISO)
				temp |= 1;
			spi_delay();
			SCLK_CLR;
		}
		buf[i] = temp;
	}

	/* end transaction by raising CSN */
	spi_delay();
	CSN_SET;
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

	/* configure CC2400 oscillator, output carrier sense on GIO6 */
	cc2400_reset();
	cc2400_set(IOCFG, (GIO_CARRIER_SENSE_N << 9) | (GIO_CLK_16M << 3));
	cc2400_strobe(SXOSCON);
	while (!(cc2400_status() & XOSC16M_STABLE));

	/* activate main oscillator */
	SCS = SCS_OSCEN;
	while (!(SCS & SCS_OSCSTAT));

	/*
	 * errata sheet says we must select peripheral clock before enabling and
	 * connecting PLL0
 	 */
#ifdef TC13BADGE
	PCLKSEL0  = (1 << 2); /* TIMER0 at cclk (30 MHz) */
#else
	PCLKSEL0  = (2 << 2); /* TIMER0 at cclk/2 (50 MHz) */
#endif
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
	all_pins_off();

	/* Enable the watchdog with reset enabled */
	USRLED_CLR;
	WDMOD |= WDMOD_WDEN | WDMOD_WDRESET;
	WDFEED_SEQUENCE;
	
	/* Set watchdog timeout to 256us (minimum) */
	
	/* sleep for 1s (minimum) */
	wait(1);
}

/* take control of R8C microcontroller and CC2400 on the ToorCon 13 badge */
#ifdef TC13BADGE
void r8c_takeover(void)
{
	/* wait until user presses SW1 while USB is connected */
	while (SW1 || !VBUS) {
		/* turn off power to all peripherals except GPIO */
		PCONP = PCONP_PCGPIO;

		/* enable interrupt on falling edge of P2.8 (SW1) */
		IO2IntEnF |= (0x1 << 8);
		ISER0 |= ISER0_ISE_EINT3;

		/* go into power-down mode and wait for interrupt */
		SCB_SCR = SCB_SCR_SLEEPDEEP;
		PCON = (PCON_PM0 | PCON_BOGD);
		asm("WFI");

		/* disable interrupt */
		IO2IntEnF &= ~(0x1 << 8);
		ICER0 |= ICER0_ICE_EINT3;

		/* turn power back on for peripherals we use */
		PCONP = (PCONP_PCTIM0 | PCONP_PCSSP1 | PCONP_PCGPIO | PCONP_PCSSP0);
	}

	/* drop R8C_CTL to let the R8C know we are taking over */
	R8C_CTL_CLR;

	/* wait for the R8C to acknowledge the takeover */
	while (R8C_ACK);
}

/* interrupt handler for SW1 */
void EINT3_IRQHandler(void)
{
	/* clear interrupt */
	IO2IntClr |= (0x1 << 8);
}
#endif

/*
 * Tune the CC2400 to a frequency in MHz for RX.  The calling function should
 * ensure that the CC2400 is in the IDLE state.
 */
void cc2400_tune_rx(uint16_t channel)
{
	cc2400_set(FSDIV, channel - 1);
}

/*
 * Tune the CC2400 to a frequency in MHz for TX.  The calling function should
 * ensure that the CC2400 is in the IDLE state.
 */
void cc2400_tune_tx(uint16_t channel)
{
	cc2400_set(FSDIV, channel);
}

/* Hop to a new RX frequency in MHz.  Assumes starting from RX or TX state. */
void cc2400_hop_rx(uint16_t channel)
{
	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK));
	cc2400_tune_rx(channel);
	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	cc2400_strobe(SRX);
}

/* Hop to a new TX frequency in MHz.  Assumes starting from RX or TX state. */
void cc2400_hop_tx(uint16_t channel)
{
	cc2400_strobe(SRFOFF);
	while ((cc2400_status() & FS_LOCK));
	cc2400_tune_rx(channel);
	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	cc2400_strobe(SRX);
}

void get_part_num(uint8_t *buffer, int *len)
{
	u32 command[5];
	u32 result[5];
	command[0] = 54; /* read part number */
	iap_entry(command, result);
	buffer[0] = result[0] & 0xFF; /* status */
	buffer[1] = result[1] & 0xFF;
	buffer[2] = (result[1] >> 8) & 0xFF;
	buffer[3] = (result[1] >> 16) & 0xFF;
	buffer[4] = (result[1] >> 24) & 0xFF;
	*len = 5;
	
}

void get_device_serial(uint8_t *buffer, int *len)
{
	u32 command[5];
	u32 result[5];
	command[0] = 58; /* read device serial number */
	iap_entry(command, result);
	buffer[0] = result[0] & 0xFF; /* status */
	buffer[1] = result[1] & 0xFF;
	buffer[2] = (result[1] >> 8) & 0xFF;
	buffer[3] = (result[1] >> 16) & 0xFF;
	buffer[4] = (result[1] >> 24) & 0xFF;
	buffer[5] = result[2] & 0xFF;
	buffer[6] = (result[2] >> 8) & 0xFF;
	buffer[7] = (result[2] >> 16) & 0xFF;
	buffer[8] = (result[2] >> 24) & 0xFF;
	buffer[9] = result[3] & 0xFF;
	buffer[10] = (result[3] >> 8) & 0xFF;
	buffer[11] = (result[3] >> 16) & 0xFF;
	buffer[12] = (result[3] >> 24) & 0xFF;
	buffer[13] = result[4] & 0xFF;
	buffer[14] = (result[4] >> 8) & 0xFF;
	buffer[15] = (result[4] >> 16) & 0xFF;
	buffer[16] = (result[4] >> 24) & 0xFF;
	*len = 17;
}

void set_isp(void)
{
	u32 command[5];
	u32 result[5];
	command[0] = 57;
	iap_entry(command, result);
}

//FIXME ssp
//FIXME tx/rx
