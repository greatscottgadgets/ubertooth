#ifndef __HOP_H
#define __HOP_H
#include <stdint.h>
#include <ubertooth_interface.h>

typedef struct hop_state_s {
	uint8_t a27_23, a22_19, C, E;
	uint16_t a18_10;
	uint8_t x;
	/* frequency register bank */
	uint8_t basic_bank[NUM_BREDR_CHANNELS];
	uint8_t afh_bank[NUM_BREDR_CHANNELS];
	uint8_t afh_chan_count;
	uint8_t afh_enabled;
	uint8_t *bank;
	uint8_t chan_count;
} hop_state_t;

void hop_init(uint32_t address);
uint8_t hop_basic(uint32_t clk);
uint8_t hop_inquiry(uint32_t clk);
uint8_t hop_channel(uint32_t clk);
void hop_cfg_afh(uint8_t* buf);

/* FIXME ?*/
extern hop_state_t hop_state;

static inline uint8_t perm5(uint8_t z, uint8_t p_high, uint16_t p_low)
{
	extern uint8_t perm5_lut[2][4096];
	uint16_t p = (p_low&0x1ff)|((p_high&0x1f)<<9);

	z &= 0x1f;
	z = perm5_lut[0][(((p>>7))<<5)|z];
	z = perm5_lut[1][((0x7f&(p>>0))<<5)|z];

	return z;
}


/* This function computes the input value X to be sent to hop_selection_kernel when the master
has clock clk, during page scan.

A slave in PAGE_SCAN mode listens for ID(1) messages for a duration of PAGE_WINDOW (>=10ms)
every PAGE_INTERVAL (either 0s, 1.28s or 2.56s), with slow frequency-hopping that only
changes every 1.28s. The used frequency is calculated by hop_selection_kernel from the slave's
BT-MAC (inputs A,B,C,D,E,F) and CLK[12:16] (input X).

To establish connection the master must send ID(1) on the correct frequency.
Knowing the slave BT-MAC (but not its CLK), the master needs to try all possible X values (2**5 = 32 options).
On every master slot, the master tries two frequencies corresponding to two guesses for X (slave CLK[12:16]),
one try on each clock of the slot. If any of them is correct, the slave will respond with ID(2)
on the corresponding clock of the following slot.

Since PAGE_WINDOW might be as short as 10ms, only 16 X values might fit a single window.
For slaves whose PAGE_INTERVAL is long (1.28s or 2.56s), CLK[12:16] will change across windows.
We want to avoid trying X values that correspond to the same guess for the slave's clock,
i.e., if X was rejected on time T, then X+1 will be rejected on T+1.28s, X+2 will be rejected on T+2.56s, etc.
Therefore we calculate a series of X values to try on each master clock (keeping in mind only
even slots are the master's), in a way that:

1. all possible X values are tried repeatedly within a 1.28s interval;
2. if the 16 values tried during the 10ms window starting at time T were (X0,...,X15), then none of the values (X0+1,...,X15+1) will be tried on time T+1.28s;
3. if the 16 values tried during the 10ms window starting at time T were (X0,...,X15), then none of the values (X0+2,...,X15+2) will be tried on time T+2.56s.

* CLK[5:2,0] is a simple incrementing counter (jumping over slave slots indicated by CLK[1]==1) to satisfy (1);
* CLK[16:12]*17 ensures adding 17 every 1.28s to satisfy (2);
* CLK[16:13]*16 ensures adding 17+17+16=50 == 18 (mod 32) every 2.56s to satisfy (3). */
inline uint8_t calc_iterate_slave_page_scan_phase(uint32_t clk)
{
	// 	X = ( CLK[5...2,0] + CLK[16...12]*17 + CLK[16...13]*16 ) mod 32
	return ((((clk>>1) & 0x1e) | (clk&1) ) + ((clk>>12)&0x1f)*17 + ((clk>>13)&0xf)*16)%32  ;
}

/* When we recive an ID(1) acknoledgment, that is, ID(2) From the slave device we transition into PAGE_RESPONSE
we freeze this value and use a frozen value of this +N (see Master Page Response Substate hopping scheme control word)
("our" N is actually hop_state.x which is also used in bbcodec) */
inline void hop_freeze_clock(uint32_t clk)
{
	hop_state.x = calc_iterate_slave_page_scan_phase(clk);
}

/* This function increment the x variable of for paging/inquiry hopping.
 * It must be called before each master's transmission. */
static inline void hop_increment(void)
{
	hop_state.x++;
}
#endif
