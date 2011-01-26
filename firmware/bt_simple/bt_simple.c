/*
 * Copyright 2010 Michael Ossmann
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
#include "usb_serial.h"

/* crude USB serial command interface */
#define NO_COMMAND 0
#define LED 1
#define GET 2
#define TXTEST 3
#define RXTEST 4

static u8 command = NO_COMMAND;

/* DMA buffers */
#define DMA_SIZE 32
u8 rxbuf1[DMA_SIZE];
u8 rxbuf2[DMA_SIZE];

/*
 * The active buffer is the one with an active DMA transfer.
 * The idle buffer is the one we can read/write between transfers.
 */
u8 *active_rxbuf = &rxbuf1[0];
u8 *idle_rxbuf = &rxbuf2[0];

/* DMA linked list items */
typedef struct {
	u32 src;
	u32 dest;
	u32 next_lli;
	u32 control;
} dma_lli;

dma_lli rx_dma_lli1;
dma_lli rx_dma_lli2;

/* rx terminal count and error interrupt counters */
volatile u32 rx_tc;
volatile u32 rx_err;

inline static void newline()
{
	VCOM_putchar('\r');
	VCOM_putchar('\n');
}

static void dma_init()
{
	/* power up GPDMA controller */
	PCONP |= PCONP_PCGPDMA;

	/* zero out channel configs and clear interrupts */
	DMACC0Config = 0;
	DMACC1Config = 0;
	DMACC2Config = 0;
	DMACC3Config = 0;
	DMACC4Config = 0;
	DMACC5Config = 0;
	DMACC6Config = 0;
	DMACC7Config = 0;
	DMACIntTCClear = 0xFF;
	DMACIntErrClr = 0xFF;

	/* DMA linked lists */
	rx_dma_lli1.src = (u32)&(DIO_SSP_DR);
	rx_dma_lli1.dest = (u32)&rxbuf1[0];
	rx_dma_lli1.next_lli = (u32)&rx_dma_lli2;
	rx_dma_lli1.control = (DMA_SIZE) |
			(1 << 12) |        /* source burst size = 4 */
			(1 << 15) |        /* destination burst size = 4 */
			(0 << 18) |        /* source width 8 bits */
			(0 << 21) |        /* destination width 8 bits */
			DMACCxControl_DI | /* destination increment */
			DMACCxControl_I;   /* terminal count interrupt enable */

	rx_dma_lli2.src = (u32)&(DIO_SSP_DR);
	rx_dma_lli2.dest = (u32)&rxbuf2[0];
	rx_dma_lli2.next_lli = (u32)&rx_dma_lli1;
	rx_dma_lli2.control = (DMA_SIZE) |
			(1 << 12) |        /* source burst size = 4 */
			(1 << 15) |        /* destination burst size = 4 */
			(0 << 18) |        /* source width 8 bits */
			(0 << 21) |        /* destination width 8 bits */
			DMACCxControl_DI | /* destination increment */
			DMACCxControl_I;   /* terminal count interrupt enable */

	/* disable DMA interrupts */
	ISER0 &= ~ISER0_ISE_DMA;

	/* enable DMA globally */
	DMACConfig = DMACConfig_E;
	while (!(DMACConfig & DMACConfig_E));

	/* configure DMA channel 1 */
	DMACC0SrcAddr = rx_dma_lli1.src;
	DMACC0DestAddr = rx_dma_lli1.dest;
	DMACC0LLI = rx_dma_lli1.next_lli;
	DMACC0Control = rx_dma_lli1.control;
	DMACC0Config =
			DIO_SSP_SRC |
			(0x2 << 11) |           /* peripheral to memory */
			DMACCxConfig_IE |       /* allow error interrupts */
			DMACCxConfig_ITC;       /* allow terminal count interrupts */

	/* reset interrupt counters */
	rx_tc = 0;
	rx_err = 0;
}

void DMA_IRQHandler()
{
	/* interrupt on channel 0 */
	if (DMACIntStat & (1 << 0)) {
		if (DMACIntTCStat & (1 << 0)) {
			DMACIntTCClear |= (1 << 0);
			rx_tc++;
		}
		if (DMACIntErrStat & (1 << 0)) {
			DMACIntErrClr |= (1 << 0);
			rx_err++;
		}
	}
}

static void dio_ssp_start()
{
	/* make sure the (active low) slave select signal is not active */
	DIO_SSEL_SET;

	/* enable rx DMA on DIO_SSP */
	DIO_SSP_DMACR |= SSPDMACR_RXDMAE;
	DIO_SSP_CR1 |= SSPCR1_SSE;
	
	/* enable DMA */
	DMACC0Config |= DMACCxConfig_E;
	ISER0 |= ISER0_ISE_DMA;

	/* activate slave select pin */
	DIO_SSEL_CLR;
}

/* start un-buffered bluetooth rx */
void cc2400_rx()
{
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x134b); // without PRNG
	cc2400_set(GRMDM,   0x0101); // un-buffered mode, GFSK
	cc2400_set(FSDIV,   0x0988); // 2440 MHz + 1 MHz IF = 2441 MHz
	cc2400_set(MDMCTRL, 0x0029); // 160 kHz frequency deviation
	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	cc2400_strobe(SRX);
}

/* I really should just get printf working. */
void hexdump(u8 byte)
{
	u8 low_nibble = byte & 0x0F;
	u8 high_nibble = (byte >> 4) & 0x0F;

	if (high_nibble > 9)
		VCOM_putchar('A' + high_nibble - 10);
	else
		VCOM_putchar('0' + high_nibble);

	if (low_nibble > 9)
		VCOM_putchar('A' + low_nibble - 10);
	else
		VCOM_putchar('0' + low_nibble);
}

/*
 * We should be using memory to USB DMA or direct SSP to USB DMA, but this is
 * fine for now.
 */
void dump_buf(u8 *buf, int len)
{
	int i;

	for (i = 0; i < len; i++)
		hexdump(*buf++);

	newline();
}

void dump_buf_raw(u8 *buf, int len)
{
	int i;

	for (i = 0; i < len; i++)
		VCOM_putchar(*buf++);
}

void cc2400_print(u8 reg)
{
	u8 i;
	u16 val = cc2400_get(reg);

	hexdump(val >> 8);
	hexdump(val & 0xFF);

	newline();
}

void cc2400_txtest()
{
	cc2400_print(MANAND);
	cc2400_print(LMTST);
	cc2400_print(MDMTST0);
	cc2400_print(FSDIV);
	cc2400_print(MDMCTRL);

	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x334b); // with PRNG
	cc2400_set(FSDIV,   0x0989); // 2441 MHz
	cc2400_set(MDMCTRL, 0x0029); // 160 kHz frequency deviation
	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	cc2400_strobe(STX);
	newline();

	cc2400_print(MANAND);
	cc2400_print(LMTST);
	cc2400_print(MDMTST0);
	cc2400_print(FSDIV);
	cc2400_print(MDMCTRL);
}

void cc2400_rxtest()
{
	u8 *tmp = NULL;
	int i;

	dio_ssp_init();
	dma_init();
	dio_ssp_start();
	cc2400_rx();

	while (1) {
		/* wait for DMA transfer */
		while ((rx_tc == 0) && (rx_err == 0));
		rx_tc = 0;
		rx_err = 0;

		/* swap buffers */
		tmp = active_rxbuf;
		active_rxbuf = idle_rxbuf;
		idle_rxbuf = tmp;

		dump_buf_raw(idle_rxbuf, DMA_SIZE);
	}
	/* should have some method to turn off */
}

int main()
{
	int c;

	ubertooth_init();
	usb_serial_init();

	while (1) {
		c = VCOM_getchar();
		if (c != EOF) {
			if (command == LED) {
				if (c == '1') {
					VCOM_putchar('1');
					USRLED_SET;
					TXLED_SET;
					RXLED_SET;
				}
				else if (c == '0') {
					VCOM_putchar('0');
					USRLED_CLR;
					TXLED_CLR;
					RXLED_CLR;
				} else {
					VCOM_putchar('.');
				}
				newline();
				command = NO_COMMAND;
			} else if (command == GET) {
				if (c == '4') {
					VCOM_putchar('4');
					cc2400_print(AGCCTRL);
				} else {
					VCOM_putchar('.');
					newline();
				}
				command = NO_COMMAND;
			} else {
				if (c == 'l') {
					command = LED;
					VCOM_putchar('l');
				} else if (c == 'g') {
					command = GET;
					VCOM_putchar('g');
				} else if (c == 't') {
					command = TXTEST;
					VCOM_putchar('t');
					newline();
					cc2400_txtest();
					command = NO_COMMAND;
				} else if (c == 'r') {
					command = RXTEST;
					cc2400_rxtest();
					newline();
					command = NO_COMMAND;
				} else {
					command = NO_COMMAND;
					VCOM_putchar('.');
					newline();
				}
			}
		}
	}
}
