/* Btctl interface
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
#include <stdlib.h>
#include <string.h>
#include <ubtbr/btctl_intf.h>
#include <ubtbr/btphy.h>
#include <ubtbr/system.h>
#include <ubtbr/rf.h>
#include <ubtbr/hop.h>
#include <ubtbr/inquiry_state.h>
#include <ubtbr/paging_state.h>
#include <ubtbr/master_state.h>
#include <ubtbr/inquiry_scan_state.h>
#include <ubtbr/page_scan_state.h>
#include <ubtbr/monitor_state.h>

btctl_t btctl;

static void btctl_flush_acl_tx_queue(void)
{
	msg_t *msg;

	/* Flush the acl tx queue */
	while ((msg = safe_dequeue(&btctl.acl_tx_q)) != NULL)
	{
		msg_free(msg);
	}
}

static void btctl_handle_debug_req(msg_t *msg)
{
	btctl_hdr_t *hdr = (btctl_hdr_t*)msg->data;

	cprintf("debug req: %s\n", (char*)hdr->data);
}

static void btctl_handle_reset_req(msg_t *msg)
{
	reset();
}

static void btctl_handle_idle_req(msg_t *msg)
{
	uint32_t wait_clkn;

	/* Cancel everything we can */
	btctl_set_state(BTCTL_STATE_STANDBY, BTCTL_REASON_SUCCESS);

	/* Let some time to tasks to complete (FIXME: wrap)*/
	wait_clkn = MASTER_CLKN+30;

	while (MASTER_CLKN < wait_clkn)
		usb_work();
}

static void btctl_handle_set_freq_off_req(msg_t *msg)
{
	btctl_hdr_t *hdr = (btctl_hdr_t*)msg->data;
	btctl_set_reg_req_t *req = (btctl_set_reg_req_t*)hdr->data;

	btphy_rf_set_freq_off(req->reg&0x3f);
}

static void btctl_handle_set_max_ac_errors_req(msg_t *msg)
{
	btctl_hdr_t *hdr = (btctl_hdr_t*)msg->data;
	btctl_set_reg_req_t *req = (btctl_set_reg_req_t*)hdr->data;

	btphy_rf_set_max_ac_errors(req->reg&3);
}

static void btctl_handle_set_bdaddr_req(msg_t *msg)
{
	btctl_hdr_t *hdr = (btctl_hdr_t*)msg->data;
	btctl_set_bdaddr_req_t *req = (btctl_set_bdaddr_req_t*)hdr->data;

	btphy_set_bdaddr(req->bdaddr);
}

static void btctl_handle_inquiry_req(msg_t *msg)
{
	inquiry_state_setup();
}

static void btctl_handle_paging_req(msg_t *msg)
{
	btctl_hdr_t *hdr = (btctl_hdr_t*)msg->data;
	btctl_paging_req_t *req = (btctl_paging_req_t*)hdr->data;
	uint32_t lap = req->bdaddr & 0xffffff;
	uint8_t uap = (req->bdaddr >> 24) & 0xff;

	btctl_flush_acl_tx_queue();

	/* Start paging */
	paging_state_setup(lap, uap);
}

static void btctl_handle_monitor_req(msg_t *msg)
{
	btctl_hdr_t *hdr = (btctl_hdr_t*)msg->data;
	btctl_paging_req_t *req = (btctl_paging_req_t*)hdr->data;

	/* Start paging monitor */
	monitor1_state_setup(req->bdaddr);
}

static void btctl_handle_tx_acl_req(msg_t *msg)
{
	btctl_acl_tx_enqueue(msg);
}

static void btctl_handle_inquiry_scan_req(msg_t *msg)
{
	inquiry_scan_state_setup();
}

static void btctl_handle_page_scan_req(msg_t *msg)
{
	btctl_flush_acl_tx_queue();
	page_scan_state_setup();
}

static void btctl_handle_set_afh_req(msg_t *msg)
{
	static uint8_t afh_buf[11];
	uint32_t flags = irq_save_disable();
	btctl_hdr_t *hdr = (btctl_hdr_t*)msg->data;
	btctl_set_afh_req_t *req = (btctl_set_afh_req_t*)hdr->data;

	afh_buf[0] = req->mode;
	memcpy(afh_buf+1, req->map, 10);
	//hop_set_afh_map(req->map);
	btphy_timer_add(req->instant, (btphy_timer_fn_t)hop_cfg_afh, afh_buf, 0);
	irq_restore(flags);
}

static void btctl_handle_set_eir_req(msg_t *msg)
{
	btctl_hdr_t *h = (btctl_hdr_t*)msg->data;

	if (btctl.state != BTCTL_STATE_STANDBY)
	{
		cprintf("Cannot change eir_msg now\n");
		msg_free(msg);
		return;
	}
	if (btctl.eir_msg)
	{
		msg_free(btctl.eir_msg);
	}
	cprintf("EIR updated\n");
	btctl.eir_msg = msg;
	btctl.eir_pkt = (btctl_tx_pkt_t *)h->data;
}

static void btctl_handle_msg(msg_t *msg)
{
	btctl_hdr_t *hdr = (btctl_hdr_t*)msg->data;

	switch(hdr->type)
	{
	case BTCTL_DEBUG:
		btctl_handle_debug_req(msg);
		break;
	case BTCTL_RESET_REQ:
		btctl_handle_reset_req(msg);
		break;
	case BTCTL_IDLE_REQ:
		btctl_handle_idle_req(msg);
		break;
	case BTCTL_SET_FREQ_OFF_REQ:
		btctl_handle_set_freq_off_req(msg);
		break;
	case BTCTL_SET_MAX_AC_ERRORS_REQ:
		btctl_handle_set_max_ac_errors_req(msg);
		break;
	case BTCTL_SET_BDADDR_REQ:
		btctl_handle_set_bdaddr_req(msg);
		break;
	case BTCTL_INQUIRY_REQ:
		btctl_handle_inquiry_req(msg);
		break;
	case BTCTL_PAGING_REQ:
		btctl_handle_paging_req(msg);
		break;
	case BTCTL_TX_ACL_REQ:
		btctl_handle_tx_acl_req(msg);
		// don't free tx messages
		goto end_nofree;
	case BTCTL_INQUIRY_SCAN_REQ:
		btctl_handle_inquiry_scan_req(msg);
		break;
	case BTCTL_PAGE_SCAN_REQ:
		btctl_handle_page_scan_req(msg);
		break;
	case BTCTL_MONITOR_REQ:
		btctl_handle_monitor_req(msg);
		break;
	case BTCTL_SET_EIR_REQ:
		btctl_handle_set_eir_req(msg);
		// don't free the eir message
		goto end_nofree;
	case BTCTL_SET_AFH_REQ:
		btctl_handle_set_afh_req(msg);
		break;
	default:
		cprintf("Invalid message type %d\n", hdr->type);
		break;
	}
	btctl_mem_free(msg);
end_nofree:
	return;
}

int btctl_work(void)
{
	msg_t *msg = btctl_rx_dequeue();

	if (msg == NULL)
		return 0;

	btctl_handle_msg(msg);

	return 1;
}

void btctl_send_state_resp(btctl_state_t state, btctl_reason_t reason)
{
	msg_t *msg = btctl_msg_alloc(BTCTL_STATE_RESP);
	btctl_state_resp_t *resp;

	resp = (btctl_state_resp_t*)msg_put(msg, sizeof(*resp));
	resp->state = state;
	resp->reason = reason;
	btctl_tx_enqueue(msg);
}

static inline int btctl_state_transition_valid(btctl_state_t cur, btctl_state_t next)
{
	/* All states can go standby */
	if (next == BTCTL_STATE_STANDBY)
		return 1;
	/* Connected state must come from page/page scan */
	if (next == BTCTL_STATE_CONNECTED)
		return cur == BTCTL_STATE_PAGE
			|| cur == BTCTL_STATE_PAGE_SCAN
			|| cur == BTCTL_STATE_CONNECTED;
	/* Others states must come from standby */
	return cur == BTCTL_STATE_STANDBY;
}

static const char* btctl_state_name(btctl_state_t state)
{
	const char *name;
	static const char* btctl_state_name_tbl[BTCTL_STATE_COUNT] = {
		[BTCTL_STATE_STANDBY]	= "STANDBY", 
		[BTCTL_STATE_INQUIRY]	= "INQUIRY",
		[BTCTL_STATE_PAGE]	= "PAGE",
		[BTCTL_STATE_CONNECTED]	= "CONNECTED",
		[BTCTL_STATE_INQUIRY_SCAN] = "INQUIRY_SCAN",
		[BTCTL_STATE_PAGE_SCAN] = "PAGE_SCAN"
	};
	if (state >= BTCTL_STATE_COUNT)
		return "Invalid";

	name = btctl_state_name_tbl[state];
	if (!name)
		name = "TODO";
	return name;
}

void btctl_set_state(btctl_state_t state, btctl_reason_t reason)
{
	uint32_t flags = irq_save_disable();
	btctl_state_t old_state = btctl.state;

#if 1
	cprintf("State %s -> %s\n",
		btctl_state_name(btctl.state),
		btctl_state_name(state));
#endif

	if (!btctl_state_transition_valid(old_state, state))
	{
		irq_restore(flags);
		DIE("Invalid state transition %s -> %s\n",
			btctl_state_name(old_state),
			btctl_state_name(state));
	}
	btctl.state = state;
	irq_restore(flags);
	if (!(old_state == BTCTL_STATE_STANDBY && state == BTCTL_STATE_STANDBY))
		btctl_send_state_resp(state, reason);
}

void btctl_init(void)
{
	btctl.state = BTCTL_STATE_STANDBY;
	btctl.eir_msg = NULL;
	btctl.eir_pkt = NULL;
	msg_queue_init(&btctl.rx_q);
	msg_queue_init(&btctl.tx_q);
	msg_queue_init(&btctl.acl_tx_q);
}
