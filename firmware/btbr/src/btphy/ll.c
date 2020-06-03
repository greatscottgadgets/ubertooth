/* Link Layer
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
#include <string.h>
#include <ubtbr/cfg.h>
#include <ubtbr/btctl_intf.h>
#include <ubtbr/btphy.h>
#include <ubtbr/bb.h>
#include <ubtbr/ll.h>

#ifdef USE_LL_DEBUG
#define LL_DEBUG(c)	console_putc(c)
#else
#define LL_DEBUG(c)	
#endif

#define SUPERVISION_TO  (1*CLKN_RATE) // 1 second

/* Process rx msg at Link layer
 * return: 0 if nothing to do
 *	   1 if tx required
 * 	  <0 on error
 */
int ll_process_rx(link_layer_t *ll, msg_t *msg)
{
	uint32_t clk_diff;
	int rc = ll->is_master != 0;	// master always tx
	int do_free_msg = 1;
	btctl_rx_pkt_t *pkt;
	btctl_hdr_t *h = (btctl_hdr_t*)msg->data;

	/* RX packet */
	if (h->type != BTCTL_RX_PKT)
		DIE("rx : expect rx pkt");
	pkt = (btctl_rx_pkt_t *)h->data;

	/* No packet received, update timeout */
	if (!BBPKT_HAS_HDR(pkt))
	{
		LL_DEBUG('x');
		/* Check supervision timeout */
		clk_diff = btphy_cur_clkn() - ll->clkn_last_rx;
		if (clk_diff > SUPERVISION_TO)
		{
			cprintf("LS To (con for %d sec, %d pkt rx)\n",
				(btphy_cur_clkn()-ll->clkn_start-SUPERVISION_TO)/CLKN_RATE,
				ll->rx_count);
			//btctl_set_state(BTCTL_STATE_STANDBY, BTCTL_REASON_TIMEOUT);
			rc = -BTCTL_REASON_TIMEOUT;
		}
		goto end;
	}

	/* Check for connection establishment */
	if (!ll->is_connected)
	{
		ll->is_connected = 1;
		/* Inform host of connection establishment */
		btctl_set_state(BTCTL_STATE_CONNECTED, BTCTL_REASON_SUCCESS|(ll->lt_addr<<5));
	}
	/* Update supervision timeout / stats */
	ll->clkn_last_rx = btphy_cur_clkn();
	ll->rx_count ++;

	/* Check if we must discard the packet */
	if (!ll->is_master && pkt->bb_hdr.lt_addr != ll->lt_addr)
	{
		// TODO: broadcast/sco/monitor..
		LL_DEBUG('0'+pkt->bb_hdr.lt_addr);
		goto end;
	}

	/* Handle remote ARQN */
	if (pkt->bb_hdr.flags & (1<<BTHDR_ARQN))
	{
		LL_DEBUG('a');
		/* Stop retransmitting when we receive an ACK */
		if (ll->cur_tx_msg != NULL)
		{
			msg_free(ll->cur_tx_msg);
			ll->cur_tx_msg = NULL;
			/* Update my sequence number */
			ll->loc_seqn ^= 1;
		}
	}
	if (!(pkt->bb_hdr.flags & (1<<BTHDR_FLOW)))
	{
		LL_DEBUG('s');
	}

	/* Handle remote SEQN */
	if (BBPKT_HAS_CRC(pkt))
	{
		// We already received this message
		if ((1&(pkt->bb_hdr.flags>>BTHDR_SEQN)) != ll->rmt_seqn)
		{
			LL_DEBUG('i');
			// just ignore it & ack in next slot
			ll->loc_arqn = 1;
		}
		else
		{
			if (BBPKT_GOOD_CRC(pkt))
			{
				LL_DEBUG('c');
				/* Send the message to host */
				if (btctl_tx_enqueue(msg) != 0)
				{
					DIE("txq full in master\n");
				}
				ll->rmt_seqn ^= 1;
				ll->loc_arqn = 1;
				do_free_msg = 0;
			}
			else{
				LL_DEBUG('b');
				// else ignore the packet 
			}
		}
	}
	// DEBUG
	else {
		if (pkt->bb_hdr.type == BB_TYPE_POLL)
		{
			LL_DEBUG('p');
		}
#ifdef USE_LL_DEBUG
		else if (pkt->bb_hdr.type == BB_TYPE_NULL)
		{
			LL_DEBUG('n');
		}
		else{
			LL_DEBUG('?');
		}
#endif
	}
	/* We can get there in two cases:
	 * - We're the master and will TX everytime
	 * - We're the slave and we must ack/crc
	 * In both case we'll tx
	 */
	if (ll->is_master || pkt->bb_hdr.type == BB_TYPE_POLL || ll->loc_arqn)
	{
		rc = 1;
	}
end:
	if (do_free_msg)
		msg_free(msg);

	return rc;
}

void ll_prepare_tx(link_layer_t *ll, bbhdr_t **tx_hdr, uint8_t **tx_data)
{
	btctl_hdr_t *btctl_hdr;
	btctl_tx_pkt_t *tx_pkt;

	if (ll->cur_tx_msg == NULL)
	{
		/* Try dequeue next acl tx packet */
		ll->cur_tx_msg=btctl_acl_tx_dequeue();
		if (ll->cur_tx_msg){
			LL_DEBUG('Q');
		}
	}
	// If no packet needs to be sent, send POLL/NULL instead
	if (ll->cur_tx_msg == NULL)
	{
		ll->tx_hdr.lt_addr = ll->lt_addr;
		if (ll->is_master)
		{
			LL_DEBUG('P');
			ll->tx_hdr.type = BB_TYPE_POLL;
		}
		else
		{
			LL_DEBUG('N');
			ll->tx_hdr.type = BB_TYPE_NULL;
		}
		*tx_hdr = &ll->tx_hdr;
		*tx_data = NULL;
	}
	else{
		LL_DEBUG('T');
		btctl_hdr = (btctl_hdr_t*)ll->cur_tx_msg->data;
		if (btctl_hdr->type != BTCTL_TX_ACL_REQ)
			DIE("BTCTL_TX_ACL_REQ expected\n");
		tx_pkt = (btctl_tx_pkt_t*)btctl_hdr->data;
		*tx_hdr =  &tx_pkt->bb_hdr;
		*tx_data = tx_pkt->bt_data;
	}

	/* Write ll flags in tx header */
	(*tx_hdr)->flags =
		(ll->loc_arqn<<BTHDR_ARQN)
		 | (ll->loc_seqn<<BTHDR_SEQN)
		 | (1<<BTHDR_FLOW);
	ll->loc_arqn = 0;
}

void ll_reset(link_layer_t *ll)
{
	// Todo : free msg_t*
	if (ll->cur_tx_msg)
	{
		msg_free(ll->cur_tx_msg);
		ll->cur_tx_msg = NULL;
	}
}

void ll_init(link_layer_t *ll, uint8_t is_master, uint8_t lt_addr)
{
	ll->is_master = is_master;
	ll->is_connected = 0;

	/* Link flags */
	ll->lt_addr = lt_addr;
	ll->rmt_seqn = 1;
	ll->loc_seqn = 1;
	ll->loc_arqn = 0;

	/* TX message */
	ll->cur_tx_msg = NULL;

	/* Stats / timeout */
	ll->rx_count = 0;
	ll->clkn_last_rx = ll->clkn_start = btphy_cur_clkn();
}
