/* TX task
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
#include <ubtbr/defines.h>
#include <ubtbr/cfg.h>
#include <ubtbr/codec.h>
#include <ubtbr/debug.h>
#include <ubtbr/bb_msg.h>
#include <ubtbr/ubertooth_dma.h>
#include <ubtbr/rf.h>
#include <ubtbr/hop.h>
#include <ubtbr/btphy.h>
#include <ubtbr/tdma_sched.h>
#include <ubtbr/tx_task.h>

static struct {
	tx_task_cb_t cb;
	void *cb_arg;
	bbhdr_t *user_hdr;
	bbcodec_t codec;
	uint8_t *user_data;
	unsigned data_size;
	uint8_t air_data[BYTE_ALIGN(4+54+MAX_ACL_SIZE*12)]; // to contain trailer+header+payload
	uint8_t *p;
} tx_task;

static int tx_finalize(uint8_t p1, uint8_t p2, uint16_t p3);

static void tx_fifo_cb(void *arg)
{
	unsigned len;

	if (tx_task.p == NULL)
		DIE("No p in tx_fifo_cb (size %d)\n", tx_task.data_size);

	len = MIN(tx_task.data_size, PHY_FIFO_THRESHOLD); // (128usec of tx)
	btphy_rf_fifo_write(tx_task.p, len);

	tx_task.p += len;
	tx_task.data_size -= len;
	if (tx_task.data_size == 0)
	{
		btphy_rf_disable_int();
	}
	else
	{
		// encode more if required
		bbcodec_encode_chunk(&tx_task.codec, tx_task.air_data, tx_task.user_hdr, tx_task.user_data);
	}
}

static int tx_prepare(uint8_t p1, uint8_t p2, uint16_t p3)
{
	int i;
	unsigned len;
	uint8_t chan, nslots = 1;
	uint32_t tx_clkn = btphy_cur_clkn()+1;

	/* In inquiry/paging, hop.x must be incremented before each tx
	 * of the master / RX of the slave */
	if (btphy.mode == BT_MODE_INQUIRY || btphy.mode == BT_MODE_PAGING)
		hop_increment();

	/* Hop & prepare TX (SFSON) */
	chan = hop_channel(tx_clkn);
	btphy_rf_tune_chan(2402+chan, 1);
	cc2400_strobe(SFSON);
	btphy_rf_cfg_tx();

	/* Push 32 hi bits of Access code */
	btphy_rf_fifo_write(btphy.chan_sw_hi, 4);

	/* Prepare data */
	if (tx_task.user_hdr)
	{
		bbcodec_init(&tx_task.codec,
			btphy_whiten_seed(tx_clkn),
			btphy.chan_uap, 1, 0);

		memset(tx_task.air_data, 0, sizeof(tx_task.air_data));
		/* encode the header in tx buffer & configure codec */
		bbcodec_encode_header(&tx_task.codec, tx_task.air_data, tx_task.user_hdr, btphy.chan_trailer, tx_task.user_data);

		tx_task.p = tx_task.air_data;
		tx_task.data_size = tx_task.codec.air_bytes;
		nslots = tx_task.codec.t->nslots;

		if (tx_task.user_data)
		{
			/* encode the data in tx buffer */
			bbcodec_encode_chunk(&tx_task.codec, tx_task.air_data, tx_task.user_hdr, tx_task.user_data);
		}

		/* Push start (less that 32 bytes) of message in fifo */
		len = MIN(32-4, tx_task.data_size);
		btphy_rf_fifo_write(tx_task.p, len);
		tx_task.p += len;
		tx_task.data_size -= len;
	}
	/* Schedule end of transmission */
	tdma_schedule(2*nslots, tx_finalize, 0, 0, 0, -3);

	return 0; 
}

static int tx_execute(uint8_t p1, uint8_t p2, uint16_t p3)
{
	if (!FS_TUNED())
	{
		/* TODO: Let it run for a few more usec ? */
		cprintf("TX: RF not FS_ON\n");
		// Just do nothing 
		tx_task_reset();
		// TODO: error callback
		btphy_rf_idle();
		return 0;
	}
	/* Start TX phy */
	btphy_rf_tx();
	if (tx_task.data_size)
		btphy_rf_enable_int(tx_fifo_cb, NULL, 1);
	if (btphy_cur_clkn() & 1)
		DIE("txe: wrong clkn %x", btphy_cur_clkn());

	return 0;
}

#define	TX_MAX_WAIT 1500
static int tx_finalize(uint8_t p1, uint8_t p2, uint16_t p3)
{
	/* Wait end of transmission .. */
	while (CLKN_OFFSET < TX_MAX_WAIT && (cc2400_get(FSMSTATE) & 0x1f) != STATE_STROBE_FS_ON);
	
	/* Stop phy RX */ 
	btphy_rf_idle();

	tx_task_reset();
	if(tx_task.cb)
		tx_task.cb(tx_task.cb_arg);

	return 0;
}

const struct tdma_sched_item tx_sched_set[] = {
	SCHED_ITEM(tx_prepare,		 0, 0, 0), 	SCHED_END_FRAME(),	// 3
	SCHED_ITEM(tx_execute,   	-3, 0, 0), 	SCHED_END_FRAME(),	// 0
	SCHED_END_SET()
};

void tx_task_reset(void)
{
	btphy_rf_disable_int();
	tx_task.user_hdr = NULL;
	tx_task.user_data = NULL;
	tx_task.p = NULL;
}

void tx_task_schedule(unsigned delay, tx_task_cb_t cb, void*cbarg, bbhdr_t *hdr, uint8_t *data)
{
	unsigned bitsize = 0;
	if (tx_task.user_hdr)
		DIE("txs: already got pkt");
	tx_task.cb = cb;
	tx_task.cb_arg = cbarg;
	tx_task.user_hdr = hdr;
	tx_task.user_data = data;
	tdma_schedule_set(delay, tx_sched_set, 0);
}
