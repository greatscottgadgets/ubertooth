/*
 * Copyright 2012-2017 Mike Ryan
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

#include <string.h>

#include "ubertooth.h"
#include "ubertooth_clock.h"
#include "bluetooth_le.h"
#include "ubertooth_rssi.h"
#include "ubertooth_usb.h"

extern u8 le_channel_idx;
extern u8 le_hop_amount;

u16 btle_next_hop(le_state_t *le)
{
	u16 phys = btle_channel_index_to_phys(le->channel_idx);
	le->channel_idx = (le->channel_idx + le->channel_increment) % 37;
	return phys;
}

u8 btle_channel_index(u8 channel) {
	u8 idx;
	channel /= 2;
	if (channel == 0)
		idx = 37;
	else if (channel < 12)
		idx = channel - 1;
	else if (channel == 12)
		idx = 38;
	else if (channel < 39)
		idx = channel - 2;
	else
		idx = 39;
	return idx;
}

u16 btle_channel_index_to_phys(u8 idx) {
	u16 phys;
	if (idx < 11)
		phys = 2404 + 2 * idx;
	else if (idx < 37)
		phys = 2428 + 2 * (idx - 11);
	else if (idx == 37)
		phys = 2402;
	else if (idx == 38)
		phys = 2426;
	else
		phys = 2480;
	return phys;
}

// calculate CRC
//	note 1: crc_init's bits should be in reverse order
//	note 2: output bytes are in reverse order compared to wire
//
//		example output:
//			0x6ff46e
//
//		bytes in packet will be:
//		  { 0x6e, 0xf4, 0x6f }
//
u32 btle_calc_crc(u32 crc_init, u8 *data, int len) {
	u32 state = crc_init & 0xffffff;
	u32 lfsr_mask = 0x5a6000; // 010110100110000000000000
	int i, j;

	for (i = 0; i < len; ++i) {
		u8 cur = data[i];
		for (j = 0; j < 8; ++j) {
			int next_bit = (state ^ cur) & 1;
			cur >>= 1;
			state >>= 1;
			if (next_bit) {
				state |= 1 << 23;
				state ^= lfsr_mask;
			}
		}
	}

	return state;
}

// runs the CRC in reverse to generate a CRCInit
//
//	crc should be big endian
//	the return will be big endian
//
u32 btle_reverse_crc(u32 crc, u8 *data, int len) {
	u32 state = crc;
	u32 lfsr_mask = 0xb4c000; // 101101001100000000000000
	u32 ret;
	int i, j;

	for (i = len - 1; i >= 0; --i) {
		u8 cur = data[i];
		for (j = 0; j < 8; ++j) {
			int top_bit = state >> 23;
			state = (state << 1) & 0xffffff;
			state |= top_bit ^ ((cur >> (7 - j)) & 1);
			if (top_bit)
				state ^= lfsr_mask;
		}
	}

	ret = 0;
	for (i = 0; i < 24; ++i)
		ret |= ((state >> i) & 1) << (23 - i);

	return ret;
}

u32 btle_crc_lut[256] = {
	0x000000, 0x01b4c0, 0x036980, 0x02dd40, 0x06d300, 0x0767c0, 0x05ba80, 0x040e40,
	0x0da600, 0x0c12c0, 0x0ecf80, 0x0f7b40, 0x0b7500, 0x0ac1c0, 0x081c80, 0x09a840,
	0x1b4c00, 0x1af8c0, 0x182580, 0x199140, 0x1d9f00, 0x1c2bc0, 0x1ef680, 0x1f4240,
	0x16ea00, 0x175ec0, 0x158380, 0x143740, 0x103900, 0x118dc0, 0x135080, 0x12e440,
	0x369800, 0x372cc0, 0x35f180, 0x344540, 0x304b00, 0x31ffc0, 0x332280, 0x329640,
	0x3b3e00, 0x3a8ac0, 0x385780, 0x39e340, 0x3ded00, 0x3c59c0, 0x3e8480, 0x3f3040,
	0x2dd400, 0x2c60c0, 0x2ebd80, 0x2f0940, 0x2b0700, 0x2ab3c0, 0x286e80, 0x29da40,
	0x207200, 0x21c6c0, 0x231b80, 0x22af40, 0x26a100, 0x2715c0, 0x25c880, 0x247c40,
	0x6d3000, 0x6c84c0, 0x6e5980, 0x6fed40, 0x6be300, 0x6a57c0, 0x688a80, 0x693e40,
	0x609600, 0x6122c0, 0x63ff80, 0x624b40, 0x664500, 0x67f1c0, 0x652c80, 0x649840,
	0x767c00, 0x77c8c0, 0x751580, 0x74a140, 0x70af00, 0x711bc0, 0x73c680, 0x727240,
	0x7bda00, 0x7a6ec0, 0x78b380, 0x790740, 0x7d0900, 0x7cbdc0, 0x7e6080, 0x7fd440,
	0x5ba800, 0x5a1cc0, 0x58c180, 0x597540, 0x5d7b00, 0x5ccfc0, 0x5e1280, 0x5fa640,
	0x560e00, 0x57bac0, 0x556780, 0x54d340, 0x50dd00, 0x5169c0, 0x53b480, 0x520040,
	0x40e400, 0x4150c0, 0x438d80, 0x423940, 0x463700, 0x4783c0, 0x455e80, 0x44ea40,
	0x4d4200, 0x4cf6c0, 0x4e2b80, 0x4f9f40, 0x4b9100, 0x4a25c0, 0x48f880, 0x494c40,
	0xda6000, 0xdbd4c0, 0xd90980, 0xd8bd40, 0xdcb300, 0xdd07c0, 0xdfda80, 0xde6e40,
	0xd7c600, 0xd672c0, 0xd4af80, 0xd51b40, 0xd11500, 0xd0a1c0, 0xd27c80, 0xd3c840,
	0xc12c00, 0xc098c0, 0xc24580, 0xc3f140, 0xc7ff00, 0xc64bc0, 0xc49680, 0xc52240,
	0xcc8a00, 0xcd3ec0, 0xcfe380, 0xce5740, 0xca5900, 0xcbedc0, 0xc93080, 0xc88440,
	0xecf800, 0xed4cc0, 0xef9180, 0xee2540, 0xea2b00, 0xeb9fc0, 0xe94280, 0xe8f640,
	0xe15e00, 0xe0eac0, 0xe23780, 0xe38340, 0xe78d00, 0xe639c0, 0xe4e480, 0xe55040,
	0xf7b400, 0xf600c0, 0xf4dd80, 0xf56940, 0xf16700, 0xf0d3c0, 0xf20e80, 0xf3ba40,
	0xfa1200, 0xfba6c0, 0xf97b80, 0xf8cf40, 0xfcc100, 0xfd75c0, 0xffa880, 0xfe1c40,
	0xb75000, 0xb6e4c0, 0xb43980, 0xb58d40, 0xb18300, 0xb037c0, 0xb2ea80, 0xb35e40,
	0xbaf600, 0xbb42c0, 0xb99f80, 0xb82b40, 0xbc2500, 0xbd91c0, 0xbf4c80, 0xbef840,
	0xac1c00, 0xada8c0, 0xaf7580, 0xaec140, 0xaacf00, 0xab7bc0, 0xa9a680, 0xa81240,
	0xa1ba00, 0xa00ec0, 0xa2d380, 0xa36740, 0xa76900, 0xa6ddc0, 0xa40080, 0xa5b440,
	0x81c800, 0x807cc0, 0x82a180, 0x831540, 0x871b00, 0x86afc0, 0x847280, 0x85c640,
	0x8c6e00, 0x8ddac0, 0x8f0780, 0x8eb340, 0x8abd00, 0x8b09c0, 0x89d480, 0x886040,
	0x9a8400, 0x9b30c0, 0x99ed80, 0x985940, 0x9c5700, 0x9de3c0, 0x9f3e80, 0x9e8a40,
	0x972200, 0x9696c0, 0x944b80, 0x95ff40, 0x91f100, 0x9045c0, 0x929880, 0x932c40
};

/*
 * Calculate a BTLE CRC one byte at a time. Thanks to Dominic Spill and
 * Michael Ossmann for writing and optimizing this.
 *
 * Arguments: CRCInit, pointer to start of packet, length of packet in
 * bytes
 * */
u32 btle_crcgen_lut(u32 crc_init, u8 *data, int len) {
	u32 state;
	int i;
	u8 key;

	state = crc_init & 0xffffff;
	for (i = 0; i < len; ++i) {
		key = data[i] ^ (state & 0xff);
		state = (state >> 8) ^ btle_crc_lut[key];
	}
	return state;
}




// FIXME put this in a header somewhere
extern volatile uint8_t mode;
extern volatile uint8_t modulation;
extern volatile uint8_t requested_mode;
extern volatile uint16_t channel;
extern volatile uint8_t status;
extern int8_t rssi_max;
extern int8_t rssi_min;
extern volatile uint32_t clkn;
#define CLK100NS (3125*(clkn & 0xfffff) + T0TC)

extern le_state_t le;

typedef struct _le_fsm_state_t le_fsm_state_t;
typedef void(* le_state_handler_t)(le_fsm_state_t *);

struct _le_fsm_state_t {
	uint8_t packet[DMA_SIZE];
	unsigned packet_len;

	const uint8_t *whitening;

	uint32_t clk100ns;

	le_state_handler_t handler;
	le_state_handler_t next_handler;
};


void le_state_rx(le_fsm_state_t *state);
void le_state_flush_rx(le_fsm_state_t *state);

void cc2400_idle(void);
int enqueue_le(uint8_t type, uint8_t *buf, uint32_t ts);


static void ssp_start(void) {
	// make sure the (active low) slave select signal is not active
	DIO_SSEL_SET;

	// enable SSP + RX DMA
	DIO_SSP_DMACR |= SSPDMACR_RXDMAE;
	DIO_SSP_CR1 |= SSPCR1_SSE;

	/* enable DMA */
	DMACC0Config |= DMACCxConfig_E;
	// ISER0 |= ISER0_ISE_DMA;

	// activate slave select pin
	DIO_SSEL_CLR;
}

static void ssp_stop() {
	// disable CC2400's output (active low)
	DIO_SSEL_SET;

	// disable DMA on SSP; disable SSP
	DIO_SSP_DMACR &= ~SSPDMACR_RXDMAE;
	DIO_SSP_CR1 &= ~SSPCR1_SSE;

	// disable DMA engine:
	// refer to UM10360 LPC17xx User Manual Ch 31 Sec 31.6.1, PDF page 607

	// disable DMA interrupts
	// ICER0 = ICER0_ICE_DMA;

	// disable active channels
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

	// Disable the DMA controller by writing 0 to the DMA Enable bit in the DMACConfig
	// register.
	DMACConfig &= ~DMACConfig_E;
	while (DMACConfig & DMACConfig_E);
}

static void dma_init_bytes(void *packet_buf) {
	/* DMA linked list items */
	typedef struct {
		u32 src;
		u32 dest;
		u32 next_lli;
		u32 control;
	} dma_lli;

	static dma_lli le_dma_lli[LE_MAX_LENGTH];

	int i;

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

	/* enable DMA globally */
	DMACConfig = DMACConfig_E;
	while (!(DMACConfig & DMACConfig_E));

	for (i = 0; i < LE_MAX_LENGTH; ++i) {
		le_dma_lli[i].src = (u32)&(DIO_SSP_DR);
		le_dma_lli[i].dest = (u32)packet_buf + i;
		le_dma_lli[i].next_lli = i < (LE_MAX_LENGTH - 1) ? (u32)&le_dma_lli[i+1] : 0;
		le_dma_lli[i].control = 1 |
				(0 << 12) |        /* source burst size = 1 */
				(0 << 15) |        /* destination burst size = 1 */
				(0 << 18) |        /* source width 8 bits */
				(0 << 21) |        /* destination width 8 bits */
				DMACCxControl_DI ; /* destination increment */
	}

	/* configure DMA channel 0 */
	DMACC0SrcAddr = le_dma_lli[0].src;
	DMACC0DestAddr = le_dma_lli[0].dest;
	DMACC0LLI = le_dma_lli[0].next_lli;
	DMACC0Control = le_dma_lli[0].control;
	DMACC0Config = DIO_SSP_SRC | (0x2 << 11) ; /* peripheral to memory */
}

static void le_init_radio(u32 sync) {
	u16 grmdm, mdmctrl;

	mdmctrl = 0x0040; // 250 kHz frequency deviation
	grmdm = 0x0561; // un-buffered mode, packet w/ sync word detection
	// 0 00 00 1 010 11 0 00 0 1
	//   |  |  | |   |  +--------> CRC off
	//   |  |  | |   +-----------> sync word: 32 MSB bits of SYNC_WORD
	//   |  |  | +---------------> 2 preamble bytes of 01010101
	//   |  |  +-----------------> packet mode
	//   |  +--------------------> un-buffered mode
	//   +-----------------------> sync error bits: 0

	cc2400_set(MANAND,  0x7fff);
	cc2400_set(LMTST,   0x2b22);

	cc2400_set(MDMTST0, 0x124b);
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

	cc2400_set(GRMDM,   grmdm);

	cc2400_set(SYNCL,   sync & 0xffff);
	cc2400_set(SYNCH,   (sync >> 16) & 0xffff);

	cc2400_set(FSDIV,   channel - 1); // 1 MHz IF
	cc2400_set(MDMCTRL, mdmctrl);

	while (!(cc2400_status() & XOSC16M_STABLE));
	cc2400_strobe(SFSON);
}

void le_state_wait_fs(le_fsm_state_t *state) {
	if (cc2400_status() & FS_LOCK)
		state->handler = state->next_handler;
}

void le_state_start_rx(le_fsm_state_t *state) {
	RXLED_CLR;

	cc2400_strobe(SRX);
	memset(state->packet, 0, sizeof(state->packet));
	state->packet_len = 0;
        memcpy(&state->packet[0], &le.access_address, 4);
	state->whitening = whitening_byte[btle_channel_index(channel - 2402)];
	rssi_reset();

#ifdef UBERTOOTH_ONE
	PAEN_SET;
	HGM_SET;
#endif
	state->handler = le_state_rx;
}

void le_state_rx(le_fsm_state_t *state) {
	int8_t rssi;
	uint32_t v;
	unsigned i;

	// wait until we've received at least one byte
	if (DMACC0DestAddr >= (uint32_t)state->packet + 4) {
		// wait until we have at least the length
		while (DMACC0DestAddr < (uint32_t)state->packet + 4 + 2)
			;

		state->clk100ns = CLK100NS;

		// reverse the bits and dewhiten first two bytes
		for (i = 0; i < 2; ++i) {
			v = state->packet[4+i] << 24;
			state->packet[4+i] = rbit(v) ^ state->whitening[i];
		}

		state->packet_len = (state->packet[4+1] & 0x3f) + 2 + 3;

		// abort receive on excessively long packets
		if (state->packet_len > 37 + 2 + 3) {
			state->handler = le_state_flush_rx;
			return;
		}

		// FIXME fold rssi into state?
		rssi = (int8_t)(cc2400_get(RSSI) >> 8);
		rssi_min = rssi < rssi_min ? rssi : rssi_min;
		rssi_max = rssi > rssi_max ? rssi : rssi_max;

		// probably a good packet: set RX LED
		RXLED_SET;

		// wait for rest of packet bytes
		// TODO update RSSI in meantime
		if (state->packet_len < LE_MAX_LENGTH) {
			while (DMACC0DestAddr < (uint32_t)state->packet + 4 + state->packet_len)
				;
		} else {
			while (DMACC0Config & DMACCxConfig_E)
				;
		}

		// reset the radio while we process the reset of the packet
		cc2400_strobe(SFSON);

		// dewhiten the rest of the packet
		for (i = 2; i < state->packet_len; ++i) {
			v = state->packet[4+i] << 24;
			state->packet[4+i] = rbit(v) ^ state->whitening[i];
		}

		enqueue_le(LE_PACKET, state->packet, state->clk100ns);
		state->handler = le_state_flush_rx;
	}
}

void le_state_flush_rx(le_fsm_state_t *state) {
	uint8_t tmp;

	// TODO is it safe to do this twice?
	cc2400_strobe(SFSON);

	ssp_stop();

	// flush any excess bytes from the SSP's buffer
	while (SSP1SR & SSPSR_RNE)
		tmp = (uint8_t)DIO_SSP_DR;

	dma_init_bytes(state->packet + 4);
	ssp_start();

	state->handler = le_state_wait_fs;
	state->next_handler = le_state_start_rx;
}

void le_init(le_fsm_state_t *state, uint8_t active_mode) {
	modulation = MOD_BT_LOW_ENERGY;
	mode = active_mode;

	le.link_state = LINK_LISTENING;

	// enable USB interrupts
	ISER0 = ISER0_ISE_USB;

        clkn_start();

	queue_init();
	dio_ssp_init();
	dma_init_bytes(state->packet + 4);
	ssp_start();
}

void le_deinit(void) {
	// disable USB interrupts
	ICER0 = ICER0_ICE_USB;

	// reset the radio completely
	cc2400_idle();
	ssp_stop();
        clkn_stop();
}

void le_main(uint8_t active_mode) {
	le_fsm_state_t state = { 0, };

	le_init(&state, active_mode);

	le_init_radio(rbit(le.access_address));
	state.handler = le_state_wait_fs;
	state.next_handler = le_state_start_rx;

	while (requested_mode == active_mode)
		state.handler(&state);

	le_deinit();
}

int enqueue_le(uint8_t type, uint8_t *buf, uint32_t ts) {
	usb_pkt_rx *f = usb_enqueue();

	/* fail if queue is full */
	if (f == NULL) {
		status |= FIFO_OVERFLOW;
		return 0;
	}

	f->clkn_high = 0;
	f->clk100ns = ts;

	f->channel = channel - 2402;
	f->rssi_min = rssi_min;
	f->rssi_max = rssi_max;
	f->rssi_avg = (rssi_max + rssi_min) / 2; // FIXME
	f->rssi_count = 2;

	memcpy(f->data, buf, DMA_SIZE);

	f->status = status;
	status = 0;

	return 1;
}
