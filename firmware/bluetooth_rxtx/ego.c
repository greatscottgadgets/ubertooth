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

/*
 * This code performs several functions related to the Yuneec E-GO electric
 * skateboard:
 *
 *  - connection following
 *  - continuous RX on a single channel
 *  - jamming
 */

int enqueue_with_ts(u8 type, u8 *buf, u32 ts);
#define CLK100NS (3125*(clkn & 0xfffff) + T0TC)
extern volatile u32 clkn; // TODO: replace with timer1
extern volatile u8 requested_mode;
extern volatile u16 channel;

uint16_t channels[4] = { 2408, 2418, 2423, 2469 };

typedef enum _ego_state_t {
	EGO_ST_INIT = 0,
	EGO_ST_START_RX,
	EGO_ST_CAP,
	EGO_ST_SLEEP,
	EGO_ST_START_JAMMING,
	EGO_ST_JAMMING,
} ego_state_t;

typedef struct _ego_fsm_state_t {
	ego_state_t state;
	int channel_index;
	u32 sleep_until;
	int timer_active;

	// used by jamming
	int packet_observed;
	u32 anchor;
} ego_fsm_state_t;

typedef void (*ego_st_handler)(ego_fsm_state_t *);

#define EGO_PACKET_LEN 36
typedef struct _ego_packet_t {
	u8 rxbuf[EGO_PACKET_LEN];
	u32 rxtime;
} ego_packet_t;

static void ssp_start(void) {
	// make sure the (active low) slave select signal is not active
	DIO_SSEL_SET;

	// enable SSP
	DIO_SSP_CR1 |= SSPCR1_SSE;

	// activate slave select pin
	DIO_SSEL_CLR;
}

static void ssp_stop() {
	// disable CC2400's output (active low)
	DIO_SSEL_SET;

	// disable SSP
	DIO_SSP_CR1 &= ~SSPCR1_SSE;
}

static void ego_init(void) {
	// enable USB interrupts
	ISER0 = ISER0_ISE_USB;

	dio_ssp_init();
}

static void ego_deinit(void) {
	cc2400_strobe(SRFOFF);
	ssp_stop(); // TODO disable SSP
	ICER0 = ICER0_ICE_USB;
}

static void rf_on(void) {
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x134b); // without PRNG
	cc2400_set(GRMDM,   0x04c0); // un-buffered mode, 2FSK
	// 0 00 00 1 001 10 0 00 0 0
	//      |  | |   |  +--------> CRC off
	//      |  | |   +-----------> sync word: 24 MSB bits of SYNC_WORD
	//      |  | +---------------> 1 byte of 01010101
	//      |  +-----------------> packet mode
	//      +--------------------> un-buffered mode
	cc2400_set(FSDIV,   channel - 1); // 1 MHz IF
	cc2400_set(MDMCTRL, 0x0026); // 150 kHz frequency deviation
	cc2400_set(GRDEC,  3);       // 250 kbit

    // 630f9ffe86
	cc2400_set(SYNCH,   0x630f);
	cc2400_set(SYNCL,   0x9ffe);

	while (!(cc2400_status() & XOSC16M_STABLE));

	ssp_start();

	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);

	cc2400_strobe(SRX);
}

void do_rx(ego_packet_t *packet) {
	int i;
	for (i = 0; i < EGO_PACKET_LEN; i++) {
		// make sure there are bytes ready
		while (!(SSP1SR & SSPSR_RNE)) ;
		packet->rxbuf[i] = (u8)DIO_SSP_DR;
	}
}

static inline int sync_received(void) {
	return cc2400_status() & SYNC_RECEIVED;
}


/////////////
// states

// do nothing
void nop_state(ego_fsm_state_t *state) {
}

// used in follow and jam mode, override the channel supplied by user
void init_state(ego_fsm_state_t *state) {
	state->channel_index = 0;
	channel = channels[state->channel_index];
	state->state = EGO_ST_START_RX;
}

void start_rf_state(ego_fsm_state_t *state) {
	rf_on();
	state->state = EGO_ST_CAP;
}

void cap_state(ego_fsm_state_t *state) {
	ego_packet_t packet = {
		.rxtime = CLK100NS,
	};

	if (state->timer_active && CLK100NS >= state->sleep_until) {
		state->sleep_until = CLK100NS + 4*1000*10; // 4 ms
		state->state = EGO_ST_SLEEP;
	}

	if (sync_received()) {
		RXLED_SET;
		do_rx(&packet);
		enqueue_with_ts(EGO_PACKET, packet.rxbuf, packet.rxtime);
		RXLED_CLR;

		state->sleep_until = packet.rxtime + 6*1000*10; // 6 ms
		state->state = EGO_ST_SLEEP;
	}

	// kill RF on state change
	if (state->state != EGO_ST_CAP) {
		cc2400_strobe(SRFOFF);
		ssp_stop();
		state->timer_active = 1;
	}
}

void sleep_state(ego_fsm_state_t *state) {
	if (CLK100NS >= state->sleep_until) {
		// change channel
		state->channel_index = (state->channel_index + 1) % 4;
		channel = channels[state->channel_index];

		// set 7 ms timeout for RX
		state->sleep_until = CLK100NS + 7*1000*10; // 7 ms
		state->timer_active = 1;

		state->state = EGO_ST_START_RX;
	}
}

// continuous cap states (reuses START_RX state)
void continuous_init_state(ego_fsm_state_t *state) {
	state->state = EGO_ST_START_RX;
}

void continuous_cap_state(ego_fsm_state_t *state) {
	ego_packet_t packet = {
		.rxtime = CLK100NS,
	};

	if (sync_received()) {
		RXLED_SET;
		do_rx(&packet);
		enqueue_with_ts(EGO_PACKET, packet.rxbuf, packet.rxtime);
		RXLED_CLR;

		// restart cap with radio warm
		cc2400_strobe(SFSON);
		while (!(cc2400_status() & FS_LOCK));
		while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
		cc2400_strobe(SRX);
	}
}

// jammer states
void jam_cap_state(ego_fsm_state_t *state) {
	if (sync_received()) {
		state->state = EGO_ST_START_JAMMING;
		state->packet_observed = 1;
		state->anchor = CLK100NS;
	}
	if (state->timer_active && CLK100NS >= state->sleep_until) {
		state->state = EGO_ST_START_JAMMING;
		state->packet_observed = 0;
		state->anchor += 11*1000*10; // 11 ms hop interval
	}

	// state changed, kill radio
	if (state->state != EGO_ST_CAP) {
		cc2400_strobe(SRFOFF);
		ssp_stop();
	}
}

void start_jamming_state(ego_fsm_state_t *state) {
#ifdef TX_ENABLE
	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);
	cc2400_set(MDMTST0, 0x334b); // with PRNG
	// cc2400_set(GRMDM,   0x04e0); // un-buffered mode, 2FSK
	cc2400_set(GRMDM,   0x04c0); // un-buffered mode, 2FSK
	// 0 00 00 1 001 10 0 00 0 0
	//      |  | |   |  +--------> CRC off
	//      |  | |   +-----------> sync word: 24 MSB bits of SYNC_WORD
	//      |  | +---------------> 1 byte of 01010101
	//      |  +-----------------> packet mode
	//      +--------------------> un-buffered mode
	cc2400_set(FSDIV,   channel); // no IF for TX
	cc2400_set(MDMCTRL, 0x0026); // 150 kHz frequency deviation
	cc2400_set(GRDEC,  3);       // 250 kbit
	cc2400_set(FREND, 0xf);

	while (!(cc2400_status() & XOSC16M_STABLE));

	cc2400_strobe(SFSON);
	while (!(cc2400_status() & FS_LOCK));
	while ((cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);

#ifdef UBERTOOTH_ONE
	PAEN_SET;
#endif

	cc2400_strobe(STX);
	TXLED_SET;
#endif

	state->state = EGO_ST_JAMMING;
	state->sleep_until = state->anchor + 2*1000*10;
}

void jamming_state(ego_fsm_state_t *state) {
	if (CLK100NS >= state->sleep_until) {
		cc2400_strobe(SRFOFF);
#ifdef UBERTOOTH_ONE
		PAEN_CLR;
#endif
		TXLED_CLR;

		// change channel
		state->channel_index = (state->channel_index + 1) % 4;
		channel = channels[state->channel_index];

		state->state = EGO_ST_SLEEP;
		state->sleep_until = state->anchor + 6*1000*10; // 6 ms
	}
}

void jam_sleep_state(ego_fsm_state_t *state) {
	if (CLK100NS >= state->sleep_until) {
		state->state = EGO_ST_START_RX;
		state->timer_active = 1;
		state->sleep_until = state->anchor + 11*1000*10; // 11 ms
	}
}

void ego_main(ego_mode_t mode) {
	ego_st_handler *handler; // set depending on mode
	ego_fsm_state_t state = {
		.state = EGO_ST_INIT,
		.channel_index = 0,
		.timer_active = 0,
	};

	// hopping connection following
	ego_st_handler follow_handler[] = {
		init_state,
		start_rf_state,
		cap_state,
		sleep_state,
		nop_state,
		nop_state,
		nop_state,
	};

	// continuous rx on a single channel
	ego_st_handler continuous_rx_handler[] = {
		continuous_init_state, // do not override user channel
		start_rf_state,
		continuous_cap_state,
		nop_state,
		nop_state,
		nop_state,
	};

	// jamming
	ego_st_handler jam_handler[] = {
		init_state,
		start_rf_state,
		jam_cap_state,
		jam_sleep_state,
		start_jamming_state,
		jamming_state,
	};

	switch (mode) {
		case EGO_FOLLOW:
			handler = follow_handler;
			break;
		case EGO_CONTINUOUS_RX:
			handler = continuous_rx_handler;
			break;
#ifdef TX_ENABLE
		case EGO_JAM:
			handler = jam_handler;
			break;
#endif
		default: // should never happen
			requested_mode = MODE_IDLE;
			return;
	}

	ego_init();

	while (1) {
		if (requested_mode != MODE_EGO)
			break;
		handler[state.state](&state);
	}

	ego_deinit();
}
