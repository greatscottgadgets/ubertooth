#ifndef __BTPHY_LL_H
#define __BTPHY_LL_H

typedef struct {
	uint8_t is_connected;
	uint8_t is_master;

	/* Stats / supervision */
	uint32_t clkn_start;
	uint32_t clkn_last_rx;
	uint32_t rx_count;
	
	// ll flags
	uint8_t lt_addr;
	uint8_t rmt_seqn;
	uint8_t loc_seqn;
	uint8_t loc_arqn;

	/* Current tx message */
	bbhdr_t tx_hdr;
	msg_t *cur_tx_msg;
	bbhdr_t *cur_tx_hdr;
	uint8_t *cur_tx_data;
} link_layer_t;

void ll_init(link_layer_t *ll, uint8_t is_master, uint8_t lt_addr);
void ll_reset(link_layer_t *ll);
int ll_process_rx(link_layer_t *ll, msg_t *msg);
void ll_prepare_tx(link_layer_t *ll, bbhdr_t **tx_hdr, uint8_t **tx_data);

#endif
