#ifndef __BTPHY_H
#define __BTPHY_H
#include <stdint.h>
#include <ubertooth_interface.h>
#include <ubtbr/cfg.h>
#include <ubtbr/queue.h>

#define CLKN_RATE 3200
#define PERIPH_CLK_RATE	50000000			// 50Mhz
#define PERIPH_CLK_NS (1000000000/PERIPH_CLK_RATE)	// 20ns
#define CLK100NS_SECOND (1000000000/100)
/* clkn_offset prescale value (from peripheral clock -> 100ns) */
#define CLKN_OFFSET_PRESCALE_VAL 	(100/PERIPH_CLK_NS)
/* clk_offset match register value */
#define CLKN_OFFSET_RESET_VAL ((PERIPH_CLK_RATE/CLKN_OFFSET_PRESCALE_VAL)/CLKN_RATE)
#define MASTER_CLKN	T1TC
#define CLKN_OFFSET	T0TC

typedef enum btphy_mode_e {
	BT_MODE_INQUIRY,
	BT_MODE_PAGING,
	BT_MODE_INQUIRY_SCAN,
	BT_MODE_PAGE_SCAN,
	BT_MODE_MASTER,
	BT_MODE_SLAVE,
} btphy_mode_t ; 

typedef struct btphy_s {
	btphy_mode_t mode;
	uint32_t master_clkn; 
	uint32_t slave_clkn; 
	int slave_clkn_delay;
	int clkn_delayed;
	uint32_t chan_lap;
	uint8_t chan_uap;
	uint64_t chan_sw;
	uint8_t chan_trailer;
	uint8_t chan_sw_lo[4];
	uint8_t chan_sw_hi[4];
	uint32_t my_lap;
	uint8_t  my_uap;
	uint16_t my_nap;
	uint64_t my_sw;
} btphy_t;

typedef void (*btphy_timer_fn_t)(void *arg);

extern btphy_t btphy;

#define CUR_MASTER_SLOT_IDX()	(btphy.master_clkn&3)
#define CUR_SLAVE_SLOT_IDX()	(btphy.slave_clkn&3)

static inline uint32_t btphy_cur_clkn(void)
{
	switch(btphy.mode)
	{
	case BT_MODE_INQUIRY_SCAN:
	case BT_MODE_PAGE_SCAN:
	case BT_MODE_SLAVE:
		return btphy.slave_clkn;
	default:
		return btphy.master_clkn;
	}
}

void btphy_init(void);
void btphy_set_mode(btphy_mode_t mode, uint32_t lap, uint8_t uap);
void btphy_set_bdaddr(uint64_t bdaddr);
uint8_t btphy_whiten_seed(uint32_t clk);
void btphy_adj_clkn_delay(int delay);
void btphy_cancel_clkn_delay(void);
void btphy_timer_add(uint32_t instant, btphy_timer_fn_t cb, void *cb_arg, uint8_t anyway);

#endif
