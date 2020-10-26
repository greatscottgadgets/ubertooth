/* RF driver
 *
 * Copyright 2020 Etienne Helluy-Lafont, Univ. Lille, CNRS.
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
#include <ubertooth.h>
#include <ubtbr/cfg.h>
#include <ubtbr/debug.h>
#include <ubtbr/rf.h>
#include <ubtbr/ubertooth_dma.h>
#include <ubtbr/system.h>

volatile rf_state_t rf_state = {0};

/* Interrupt handler for GIO6 */
void EINT3_IRQHandler(void)
{
	/* Clear interrupt */
	IO2IntClr |= PIN_GIO6;

	if (!rf_state.int_handler)
	{
		DIE("No phy int handler");
	}
	rf_state.int_handler(rf_state.int_arg);
}

void btphy_rf_enable_int(btbr_int_cb_t cb, void*cb_arg, int tx)
{
	/* clear pending */
	ICPR0 = ISER0_ISE_EINT3;
	IO2IntClr |= PIN_GIO6;

	rf_state.int_handler = cb;
	rf_state.int_arg = cb_arg;

	/* Configure interrupt on GIO6 rising edge */
	if (tx)
		IO2IntEnR |= PIN_GIO6;
	else
		IO2IntEnF |= PIN_GIO6;

	/* Enable */
	ISER0 = ISER0_ISE_EINT3;
}

void btphy_rf_disable_int(void)
{
	/* Mask interrupt */
	ICER0 = ISER0_ISE_EINT3;

	IO2IntEnR &= ~PIN_GIO6;
	IO2IntEnF &= ~PIN_GIO6;

	/* Clear pending interrupt */
	IO2IntClr |= PIN_GIO6;
	ICPR0 = ISER0_ISE_EINT3;
}

void btphy_rf_set_freq_off(uint8_t off)
{
	uint32_t flags = irq_save_disable();

	rf_state.freq_off_reg = off;
	cc2400_set(MDMCTRL, 0x0029|(rf_state.freq_off_reg<<7)); // 160 kHz frequency deviation
	irq_restore(flags);
	cprintf("freq off reg: %d\n", rf_state.freq_off_reg);
}

void btphy_rf_set_max_ac_errors(uint8_t max_ac_errors)
{
	rf_state.max_ac_errors = max_ac_errors;
	cprintf("max ac errors: %d\n", rf_state.max_ac_errors);
}

void btphy_rf_cfg_sync(uint32_t sync)
{
	cc2400_set(SYNCL,   sync & 0xffff);
	cc2400_set(SYNCH,   (sync >> 16) & 0xffff);
}

void btphy_rf_init(void)
{
	uint32_t mdmtst0, iocfg;

	/* Initialize dma here */
	dma_poweron();
	dio_ssp_init();

	rf_state.max_ac_errors = MAX_AC_ERRORS_DEFAULT;

	btphy_rf_idle();
	WAIT_CC2400_STATE(STATE_IDLE);

	// Bluetooth-like modulation
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);    // LNA and receive mixers test register
	mdmtst0 = 0x34b;
#ifndef USE_TX_1MHZ_OFF
	mdmtst0 |= 0x1000;
#endif
	// 1      2      4b
	// 00 0 1 0 0 10 01001011
	//    | | | | |  +---------> AFC_DELTA = ??
	//    | | | | +------------> AFC settling = 4 pairs (8 bit preamble)
	//    | | | +--------------> no AFC adjust on packet
	//    | | +----------------> do not invert data
	//    | +------------------> TX IF freq 1 0Hz
	//    +--------------------> PRNG off
	//
	// ref: CC2400 datasheet page 67
	// AFC settling explained page 41/42
	cc2400_set(MDMTST0, mdmtst0);
	cc2400_set(FREND,   0b1011);	// Amplifier level (-7 dBm, picked from hat)
	cc2400_set(MDMCTRL, 0x0029|(rf_state.freq_off_reg<<7)); // 160 kHz frequency deviation
	cc2400_set(INT,     PHY_FIFO_THRESHOLD);	// FIFO_THRESHOLD

	btphy_rf_disable_int();

#ifdef UBERTOOTH_ONE
	PAEN_SET;
	HGM_SET;
#endif
}

static void spi_delay() {
	volatile int delay_counter;
	delay_counter = 10;
	while (--delay_counter);
}

/* write multiple bytes to SPI */
static void btphy_rf_cc2400_fifo_write(u8 len, u8 *data) {
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

	/* Push reversed bytes to save a few cycles */
	for (i = 0; i < len; ++i) {
		temp = data[i];
		for (j = 0; j < 8; ++j) {
			if (temp & 1)
				MOSI_SET;
			else
				MOSI_CLR;
			temp >>= 1;
			SCLK_SET;
			SCLK_CLR;
		}
	}

	spi_delay();
	/* end transaction by raising CSN */
	CSN_SET;
}

void btphy_rf_fifo_write(uint8_t *data, unsigned len)
{
	/* Bufferize message  */
	btphy_rf_cc2400_fifo_write(len, data);
}

void btphy_rf_off(void)
{
	btphy_rf_idle();
	WAIT_CC2400_STATE(STATE_IDLE);

#ifdef UBERTOOTH_ONE
	PAEN_CLR;
	HGM_CLR;
#endif
}

void btphy_rf_tune_chan(uint16_t channel, int tx)
{
	/* Wait for state IDLE before changing FSDIV */
	WAIT_CC2400_STATE(STATE_IDLE);

	/* Retune */
#ifdef USE_TX_1MHZ_OFF
	channel -= 1;
#else
	if (!tx)
		channel -= 1;
#endif
	cc2400_set(FSDIV, channel);
}
