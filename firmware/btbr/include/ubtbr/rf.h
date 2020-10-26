#ifndef __BTPHY_RF_H
#define __BTPHY_RF_H
#include <stdint.h>
#include "ubertooth.h"

/* Num of bytes left in fifo when FIFO_EMPTY int trigger 64usec */
#define PHY_MIN_FIFO_BYTES 16
#define PHY_FIFO_THRESHOLD (32-PHY_MIN_FIFO_BYTES)

#define WAIT_CC2400_STATE(state) while((cc2400_get(FSMSTATE) & 0x1f) != (state));
#define FS_TUNED() (cc2400_status() & FS_LOCK)
#define WAIT_FS_TUNED() while(!FS_TUNED())

#define RF_EXPECTED_RX_CLKN_OFFSET 610	// (29usec of rf warmup + 32usec of sw)

typedef void (*btbr_int_cb_t)(void *arg);

#define MAX_AC_ERRORS_DEFAULT	1
typedef struct {
	uint16_t freq_off_reg;
	uint16_t max_ac_errors;
	btbr_int_cb_t int_handler;
	void *int_arg;
} rf_state_t;

extern volatile rf_state_t rf_state;

void btphy_rf_init(void);
void btphy_rf_off(void);
void btphy_rf_set_freq_off(uint8_t off);
void btphy_rf_set_max_ac_errors(uint8_t max_ac_errors);
void btphy_rf_cfg_sync(uint32_t sync);
void btphy_rf_tune_chan(uint16_t channel, int tx);
void btphy_rf_fifo_write(uint8_t *data, unsigned len);

void btphy_rf_enable_int(btbr_int_cb_t cb, void*cb_arg, int tx);
void btphy_rf_disable_int(void);

static inline void btphy_rf_idle(void)
{
	cc2400_strobe(SRFOFF);
	TXLED_CLR;
	RXLED_CLR;
}

static inline void btphy_rf_tx(void)
{
	cc2400_strobe(STX);
	TXLED_SET;
}
static inline void btphy_rf_rx(void)
{
	cc2400_strobe(SRX);
	RXLED_SET;
}

/* cc2400 configure for un-buffered rx */
static inline void btphy_rf_cfg_rx(void)
{
	/* un-buffered mode, packet w/ sync word detection */
	cc2400_set(GRMDM,   0x4E1|(rf_state.max_ac_errors<<13));
	// 0 XX 00 1 001 11 0 00 0 1
	//   |  |  | |   |  +--------> CRC off
	//   |  |  | |   +-----------> sync word: 32 MSB bits of SYNC_WORD
	//   |  |  | +---------------> 1 preamble bytes of (0)1010101
	//   |  |  +-----------------> packet mode
	//   |  +--------------------> un-buffered mode // use sync word to trigger 
	//   +-----------------------> sync error bits allowed: N
	cc2400_set(IOCFG, 0x170|(GIO_PKT<<9));
}

/* cc2400 configure for buffered tx */
static inline void btphy_rf_cfg_tx(void)
{
	cc2400_set(GRMDM,   0x0CE1);
	// 0 00 01 1 001 11 0 00 0 1
	//      |  | |   |  +--------> CRC off
	//      |  | |   +-----------> sync word: 32 MSB bits of SYNC_WORD
	//      |  | +---------------> 1 preamble bytes of (0)1010101
	//      |  +-----------------> packet mode / sync word detection
	//      +--------------------> buffered mode
	cc2400_set(IOCFG, 0x170|(GIO_FIFO_EMPTY<<9));
}
#endif
