/*
 * Copyright 2017 Mike Ryan
 *
 * This file is part of Project Ubertooth and is released under the
 * terms of the GPL. Refer to COPYING for more information.
 */

#include <stdlib.h>
#include <string.h>

#include "ubertooth.h"
#include "ubertooth_clock.h"
#include "ubertooth_dma.h"
#include "ubertooth_usb.h"
#include "bluetooth_le.h"
#include "queue.h"

// current time, from timer1
#define NOW T1TC
#define USEC(X) (X*10)
#define MSEC(X) (X*10000)

//////////////////////
// global state

extern le_state_t le; // FIXME - refactor this struct
volatile uint16_t rf_channel;
uint8_t le_dma_dest[2];

extern volatile uint8_t mode;
extern volatile uint8_t requested_mode;

////////////////////
// buffers

// packet buffers live in a pool. a minimum of one buffer is always
// being used, either waiting to receive or actively receiving a packet
// (current_rxbuf). once a packet is received, it is placed into the
// packet queue. the main loop pulls packets from this queue and
// processes them, and then returns the buffers back to the pool by
// calling buffer_release()

#define LE_BUFFER_POOL_SIZE 4
typedef struct _le_rx_t {
	uint8_t data[2 + 255 + 3];  // header + PDU + CRC
	unsigned size;              // total data length (known after header rx)
	unsigned pos;               // current input byte offset
	uint32_t timestamp;         // timestamp taken after first byte rx
	unsigned channel;           // physical channel
	uint32_t access_address;    // access address
	int available;              // 1 if available, 0 in use
	int8_t rssi_min, rssi_max;  // min and max RSSI observed values
	int rssi_sum;               // running sum of all RSSI values
} le_rx_t;

// pool of all buffers
static le_rx_t le_buffer_pool[LE_BUFFER_POOL_SIZE];

// buffer waiting for or actively receiving packet
static le_rx_t *current_rxbuf = NULL;

// received packets, waiting to be processed
queue_t packet_queue;

//////////////////////
// code

// pre-declarations for utility stuff
static void timer1_start(void);
static void timer1_stop(void);
static void timer1_set_match(uint32_t match);
static void timer1_clear_match(void);
static void timer1_wait_fs_lock(void);
static void blink(int tx, int rx, int usr);
static void le_dma_init(void);
static void le_cc2400_strobe_rx(void);
static uint8_t dewhiten_length(unsigned channel, uint8_t data);

// resets the state of all available buffers
static void buffers_init(void) {
	int i;

	for (i = 0; i < LE_BUFFER_POOL_SIZE; ++i)
		le_buffer_pool[i].available = 1;
}

// clear a buffer for new data
static void buffer_clear(le_rx_t *buf) {
	buf->pos = 0;
	buf->size = 0;
	memset(buf->data, 0, sizeof(buf->data));
	buf->rssi_min = INT8_MAX;
	buf->rssi_max = INT8_MIN;
	buf->rssi_sum = 0;
}

// get a packet buffer
// returns a pointer to a buffer if available
// returns NULL otherwise
static le_rx_t *buffer_get(void) {
	int i;

	for (i = 0; i < LE_BUFFER_POOL_SIZE; ++i) {
		if (le_buffer_pool[i].available) {
			le_buffer_pool[i].available = 0;
			buffer_clear(&le_buffer_pool[i]);
			return &le_buffer_pool[i];
		}
	}

	return NULL;
}

// release a buffer back to the pool
static void buffer_release(le_rx_t *buffer) {
	buffer->available = 1;
}

// DMA handler
// called once per byte. handles all incoming data, but only minimally
// processes received data. at the end of a packet, it enqueues the
// received packet, fetches a new buffer, and restarts RX.
void le_DMA_IRQHandler(void) {
	unsigned pos;
	int8_t rssi;

	// channel 0
	if (DMACIntStat & (1 << 0)) {
		// terminal count - byte received
		if (DMACIntTCStat & (1 << 0)) {
			DMACIntTCClear = (1 << 0);

			// poll RSSI
			rssi = (int8_t)(cc2400_get(RSSI) >> 8);
			current_rxbuf->rssi_sum += rssi;
			if (rssi < current_rxbuf->rssi_min) current_rxbuf->rssi_min = rssi;
			if (rssi > current_rxbuf->rssi_max) current_rxbuf->rssi_max = rssi;

			// grab byte from DMA buffer
			pos = current_rxbuf->pos;
			current_rxbuf->data[pos] = le_dma_dest[pos & 1]; // dirty hack
			pos += 1;
			current_rxbuf->pos = pos;

			if (pos == 1) {
				current_rxbuf->timestamp = NOW - USEC(8 + 32); // packet starts at preamble
				current_rxbuf->channel = rf_channel;
				current_rxbuf->access_address = le.access_address;
			}

			// get length from header
			if (pos == 2) {
				uint8_t length = dewhiten_length(current_rxbuf->channel, current_rxbuf->data[1]);
				current_rxbuf->size = length + 2 + 3; // two bytes for header and three for CRC
			}

			// finished packet - state transition
			if (pos > 2 && pos >= current_rxbuf->size) {
				// stop the CC2400 before flushing SSP
				cc2400_strobe(SFSON);

				// stop DMA and flush SSP
				DIO_SSP_DMACR &= ~SSPDMACR_RXDMAE;
				while (SSP1SR & SSPSR_RNE) {
					uint8_t tmp = (uint8_t)DIO_SSP_DR;
				}

				// blink RX LED
				blink(0, 1, 0);

				// TODO error transition on queue_insert
				queue_insert(&packet_queue, current_rxbuf);

				// get a new packet
				// TODO handle error transition
				current_rxbuf = buffer_get();

				// restart DMA and SSP
				le_dma_init();
				dio_ssp_start();

				// wait for FS_LOCK in background
				timer1_wait_fs_lock();
			}
		}

		// error - transition to error state
		if (DMACIntErrStat & (1 << 0)) {
			// TODO error state transition
			DMACIntErrClr = (1 << 0);
		}
	}
}

static void le_dma_init(void) {
	int i;

	// DMA linked list items
	typedef struct {
		uint32_t src;
		uint32_t dest;
		uint32_t next_lli;
		uint32_t control;
	} dma_lli;
	static dma_lli le_dma_lli[2];

	// power up GPDMA controller
	PCONP |= PCONP_PCGPDMA;

	dma_disable();

	// enable DMA globally
	DMACConfig = DMACConfig_E;
	while (!(DMACConfig & DMACConfig_E));

	for (i = 0; i < 2; ++i) {
		le_dma_lli[i].src = (uint32_t)&(DIO_SSP_DR);
		le_dma_lli[i].dest = (uint32_t)&le_dma_dest[i];
		le_dma_lli[i].next_lli = (uint32_t)&le_dma_lli[1-i]; // two elements pointing back at each other
		le_dma_lli[i].control = 1 |
				(0 << 12) |        // source burst size = 1
				(0 << 15) |        // destination burst size = 1
				(0 << 18) |        // source width 8 bits
				(0 << 21) |        // destination width 8 bits
				DMACCxControl_I;   // terminal count interrupt enable
	}

	// configure DMA channel 0
	DMACC0SrcAddr = le_dma_lli[0].src;
	DMACC0DestAddr = le_dma_lli[0].dest;
	DMACC0LLI = le_dma_lli[0].next_lli;
	DMACC0Control = le_dma_lli[0].control;
	DMACC0Config =
			DIO_SSP_SRC |
			(0x2 << 11) |     // peripheral to memory
			DMACCxConfig_IE | // allow error interrupts
			DMACCxConfig_ITC; // allow terminal count interrupts
}

// initalize USB, SSP, and DMA
static void le_sys_init(void) {
	usb_queue_init(); // USB FIFO FIXME replace with safer queue
	dio_ssp_init();   // init SSP and raise !CS (self-routed GPIO)
	le_dma_init();    // prepare DMA + interrupts
	dio_ssp_start();  // enable SSP + DMA
}

// initialize RF and strobe FSON
static void le_cc2400_init_rf(void) {
	u16 grmdm, mdmctrl;
	uint32_t sync = rbit(le.access_address);

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

	cc2400_set(FSDIV,   rf_channel - 1); // 1 MHz IF
	cc2400_set(MDMCTRL, mdmctrl);

	// XOSC16M should always be stable, but leave this test anyway
	while (!(cc2400_status() & XOSC16M_STABLE));

	// wait for FS_LOCK in background
	cc2400_strobe(SFSON);
	timer1_wait_fs_lock();
}

// strobe RX and enable PA
static void le_cc2400_strobe_rx(void) {
	cc2400_strobe(SRX);
#ifdef UBERTOOTH_ONE
	PAEN_SET;
	HGM_SET;
#endif
}

///////
// timer stuff

static void timer1_start(void) {
	T1TCR = TCR_Counter_Reset;
	T1PR = 4; // 100 ns
	T1TCR = TCR_Counter_Enable;

	// set up interrupt handler
	ISER0 = ISER0_ISE_TIMER1;
}

static void timer1_stop(void) {
	T1TCR = TCR_Counter_Reset;

	// clear interrupt handler
	ICER0 = ICER0_ICE_TIMER1;
}

static void timer1_set_match(uint32_t match) {
	T1MR0 = match;
	T1MCR |= TMCR_MR0I;
}

static void timer1_clear_match(void) {
	T1MCR &= ~TMCR_MR0I;
}

static void timer1_wait_fs_lock(void) {
	T1MR2 = NOW + USEC(3);
	T1MCR |= TMCR_MR2I;
}

void TIMER1_IRQHandler(void) {
	if (T1IR & TIR_MR0_Interrupt) {
		// ack the interrupt
		T1IR = TIR_MR0_Interrupt;
		timer1_clear_match(); // prevent interrupt from firing a second time

		// TODO and then.... ?
		// insert hopping logic here
	}

	// LEDs
	if (T1IR & TIR_MR1_Interrupt) {
		T1IR = TIR_MR1_Interrupt;
		T1MCR &= ~TMCR_MR1I;

		TXLED_CLR;
		RXLED_CLR;
		USRLED_CLR;
	}

	// check FS_LOCK
	if (T1IR & TIR_MR2_Interrupt) {
		T1IR = TIR_MR2_Interrupt;

		// if FS is locked, strobe RX and clear interrupt
		if (cc2400_status() & FS_LOCK) {
			le_cc2400_strobe_rx();
			T1MCR &= ~TMCR_MR2I;
		}

		// if FS is not locked, check again in 3 us
		else {
			timer1_wait_fs_lock();
		}
	}
}

static void blink(int tx, int rx, int usr) {
	if (tx)
		TXLED_SET;
	if (rx)
		RXLED_SET;
	if (usr)
		USRLED_SET;

	// blink for 10 ms
	T1MR1 = NOW + MSEC(10);
	T1MCR |= TMCR_MR1I;
}

// helper function to dewhiten length from whitened data (only used
// during DMA)
static uint8_t dewhiten_length(unsigned channel, uint8_t data) {
	unsigned int i, bit;
	int idx = whitening_index[btle_channel_index(channel)];
	uint8_t out = 0;

	// length is second byte of packet
	idx = (idx + 8) % sizeof(whitening);

	for (i = 0; i < 8; ++i) {
		bit = (data >> (7-i)) & 1;
		bit ^= whitening[idx];
		idx = (idx + 1) % sizeof(whitening);
		out |= bit << i;
	}

	return out;
}

// enqueue a packet for USB
// FIXME this is cribbed from existing code, but does not have enough
// room for larger LE packets
static int usb_enqueue_le(le_rx_t *packet) {
	usb_pkt_rx* f = usb_enqueue();

	// fail if queue is full
	if (f == NULL) {
		return 0;
	}

	f->pkt_type = LE_PACKET;

	f->clkn_high = 0;
	f->clk100ns = packet->timestamp;

	f->channel = (uint8_t)((packet->channel - 2402) & 0xff);
	f->rssi_avg = packet->rssi_sum / packet->size;
	f->rssi_min = packet->rssi_min;
	f->rssi_max = packet->rssi_max;
	f->rssi_count = 0;

	memcpy(f->data, &packet->access_address, 4);
	memcpy(f->data+4, packet->data, DMA_SIZE-4);

	f->status = 0;

	return 1;
}

void le_phy_main(void) {
	// disable USB interrupts -- we poll them below
	// n.b., they should not be enabled but let's be careful
	ICER0 = ICER0_ICE_USB;
	// disable clkn and timer0
	clkn_disable();

	buffers_init();
	queue_init(&packet_queue);
	timer1_start();

	current_rxbuf = buffer_get();
	rf_channel = 2402;
	le_sys_init();
	le_cc2400_init_rf();

	while (requested_mode == MODE_BT_FOLLOW_LE) {
		le_rx_t *packet = NULL;
		if (queue_remove(&packet_queue, (void **)&packet)) {
			le_dewhiten(packet->data, packet->size, packet->channel);
			usb_enqueue_le(packet);

			// TODO process packet

			buffer_release(packet);
		}

		// polled USB handling
		handle_usb(0);

		// XXX maybe LED light show?
	}

	timer1_stop();

	// reset state
	RXLED_CLR;
	TXLED_CLR;
	USRLED_CLR;
	clkn_init();

	// TODO kill CC2400
}
