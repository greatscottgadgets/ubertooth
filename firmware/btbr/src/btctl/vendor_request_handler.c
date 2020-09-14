/*
 * Copyright 2012 Dominic Spill
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
#include <ubertooth.h>
#include <ubertooth_interface.h>
#include <ubtbr/cfg.h>
#include <ubtbr/ubertooth_usb.h>
#include <ubtbr/ubertooth_dma.h>
#include <ubtbr/btphy.h>
#include <ubtbr/btctl_intf.h>

static void request_reset(void)
{
	btctl_rx_enqueue(btctl_msg_alloc(BTCTL_RESET_REQ));
}

int vendor_request_handler(uint8_t request, uint16_t* request_params, uint8_t* data, int* data_len)
{
	uint32_t clock;
	size_t length; // string length
	usb_pkt_rx* p = NULL;
	uint16_t reg_val;
	uint8_t i;
	unsigned data_in_len = request_params[2];

	switch (request) {

	case UBERTOOTH_PING:
		*data_len = 0;
		break;

	case UBERTOOTH_GET_USRLED:
		data[0] = (USRLED) ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_USRLED:
		if (request_params[0])
			USRLED_SET;
		else
			USRLED_CLR;
		break;

	case UBERTOOTH_GET_RXLED:
		data[0] = (RXLED) ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_RXLED:
		if (request_params[0])
			RXLED_SET;
		else
			RXLED_CLR;
		break;

	case UBERTOOTH_GET_TXLED:
		data[0] = (TXLED) ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_TXLED:
		if (request_params[0])
			TXLED_SET;
		else
			TXLED_CLR;
		break;

	case UBERTOOTH_GET_1V8:
		data[0] = (CC1V8) ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_1V8:
		if (request_params[0])
			CC1V8_SET;
		else
			CC1V8_CLR;
		break;

	case UBERTOOTH_GET_PARTNUM:
		get_part_num(data, data_len);
		break;

	case UBERTOOTH_RESET:
		request_reset();
		break;

	case UBERTOOTH_GET_SERIAL:
		get_device_serial(data, data_len);
		break;

#ifdef UBERTOOTH_ONE
	case UBERTOOTH_GET_PAEN:
		data[0] = (PAEN) ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_PAEN:
		if (request_params[0])
			PAEN_SET;
		else
			PAEN_CLR;
		break;

	case UBERTOOTH_GET_HGM:
		data[0] = (HGM) ? 1 : 0;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_HGM:
		if (request_params[0])
			HGM_SET;
		else
			HGM_CLR;
		break;

	case UBERTOOTH_GET_PALEVEL:
		data[0] = cc2400_get(FREND) & 0x7;
		*data_len = 1;
		break;

	case UBERTOOTH_SET_PALEVEL:
		if( request_params[0] < 8 ) {
			cc2400_set(FREND, 8 | request_params[0]);
		} else {
			return 0;
		}
		break;
#endif

	case UBERTOOTH_SET_ISP:
		set_isp();
		*data_len = 0; /* should never return */
		break;

	case UBERTOOTH_FLASH:
		bootloader_ctrl = DFU_MODE;
		request_reset();
		break;

	case UBERTOOTH_GET_REV_NUM:
		data[0] = 0x00;
		data[1] = 0x00;

		length = (u8)strlen(GIT_REVISION);
		data[2] = length;

		memcpy(&data[3], GIT_REVISION, length);

		*data_len = 2 + 1 + length;
		break;

	case UBERTOOTH_GET_BOARD_ID:
		data[0] = BOARD_ID;
		*data_len = 1;
		break;

	default:
		return 0;
	}
	return 1;
}

