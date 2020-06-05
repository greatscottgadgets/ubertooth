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

/* This function increment the x variable of for paging/inquiry hopping.
 * It must be called before each master's transmission. */
static inline void hop_increment(void)
{
	hop_state.x++;
}
#endif
