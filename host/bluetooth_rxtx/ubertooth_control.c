/*
 * Copyright 2010-2012 Michael Ossmann, Dominic Spill
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

#include "ubertooth_control.h"

void show_libusb_error(int error_code)
{
    switch (error_code) {
	    case LIBUSB_ERROR_TIMEOUT:
	        fprintf(stderr, "libUSB Error: Timeout (%d)\n", error_code);
	        break;
	    case LIBUSB_ERROR_NO_DEVICE:
	        fprintf(stderr, "libUSB Error: No Device, did you disconnect the ubertooth? (%d)\n", error_code);
	        break;
	    case LIBUSB_ERROR_ACCESS:
	        fprintf(stderr, "libUSB Error: Insufficient Permissions (%d)\n", error_code);
	        break;
	    default:
	        fprintf(stderr, "command error %d\n", error_code);
	        break;
	}
}

int cmd_ping(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_PING, 0, 0,
			NULL, 0, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_rx_syms(struct libusb_device_handle* devh, u16 num)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_RX_SYMBOLS, num, 0,
			NULL, 0, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_specan(struct libusb_device_handle* devh, u16 low_freq, u16 high_freq)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SPECAN,
			low_freq, high_freq, NULL, 0, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_led_specan(struct libusb_device_handle* devh, u16 rssi_threshold)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_LED_SPECAN,
			rssi_threshold, 0, NULL, 0, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_set_usrled(struct libusb_device_handle* devh, u16 state)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_USRLED, state, 0,
			NULL, 0, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_get_usrled(struct libusb_device_handle* devh)
{
	u8 state;
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_USRLED, 0, 0,
			&state, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return state;
}

int cmd_set_rxled(struct libusb_device_handle* devh, u16 state)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_RXLED, state, 0,
			NULL, 0, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_get_rxled(struct libusb_device_handle* devh)
{
	u8 state;
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_RXLED, 0, 0,
			&state, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return state;
}

int cmd_set_txled(struct libusb_device_handle* devh, u16 state)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_TXLED, state, 0,
			NULL, 0, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_get_txled(struct libusb_device_handle* devh)
{
	u8 state;
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_TXLED, 0, 0,
			&state, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return state;
}

int cmd_get_modulation(struct libusb_device_handle* devh)
{
	u8 modulation;
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_MOD, 0, 0,
			&modulation, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}

	return modulation;
}

int cmd_get_channel(struct libusb_device_handle* devh)
{
	u8 result[2];
	int r;
	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_CHANNEL, 0, 0,
			result, 2, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}

	return result[0] | (result[1] << 8);
}


int cmd_set_channel(struct libusb_device_handle* devh, u16 channel)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_CHANNEL, channel, 0,
			NULL, 0, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_get_partnum(struct libusb_device_handle* devh)
{
	u8 result[5];
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_PARTNUM, 0, 0,
			result, 5, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	if (result[0] != 0) {
		fprintf(stderr, "result not zero: %d\n", result[0]);
		return 0;
	}
	return result[1] | (result[2] << 8) | (result[3] << 16) | (result[4] << 24);
}

int cmd_get_serial(struct libusb_device_handle* devh)
{
	u8 result[17];
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_SERIAL, 0, 0,
			result, 17, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	if (result[0] != 0) {
		fprintf(stderr, "result not zero: %d\n", result[0]);
		return 0;
	}
	/* FIXME shouldn't print to stdout, should return complete serial number */
	printf("%08x", result[1] | (result[2] << 8) | (result[3] << 16) | (result[4] << 24));
	printf("%08x", result[5] | (result[6] << 8) | (result[7] << 16) | (result[8] << 24));
	printf("%08x", result[9] | (result[10] << 8) | (result[11] << 16) | (result[12] << 24));
	printf("%08x\n", result[13] | (result[14] << 8) | (result[15] << 16) | (result[16] << 24));
	return result[1] | (result[2] << 8) | (result[3] << 16) | (result[4] << 24);
}

int cmd_set_modulation(struct libusb_device_handle* devh, u16 mod)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_MOD, mod, 0,
			NULL, 0, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_set_isp(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_ISP, 0, 0,
			NULL, 0, 1000);
	/* LIBUSB_ERROR_PIPE or LIBUSB_ERROR_OTHER is expected */
	if (r && (r != LIBUSB_ERROR_PIPE) && (r != LIBUSB_ERROR_OTHER) &&
		(r != LIBUSB_ERROR_NO_DEVICE)) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_reset(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_RESET, 0, 0,
			NULL, 0, 1000);
	/* LIBUSB_ERROR_PIPE or LIBUSB_ERROR_OTHER is expected */
	if (r && (r != LIBUSB_ERROR_PIPE) && (r != LIBUSB_ERROR_OTHER) &&
		(r != LIBUSB_ERROR_NO_DEVICE)) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_stop(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_STOP, 0, 0,
			NULL, 0, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_set_paen(struct libusb_device_handle* devh, u16 state)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_PAEN, state, 0,
			NULL, 0, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_set_hgm(struct libusb_device_handle* devh, u16 state)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_HGM, state, 0,
			NULL, 0, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_tx_test(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_TX_TEST, 0, 0,
			NULL, 0, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_flash(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_FLASH, 0, 0,
			NULL, 0, 1000);
	/* LIBUSB_ERROR_PIPE or LIBUSB_ERROR_OTHER is expected */
	if ((r != LIBUSB_ERROR_PIPE) && (r != LIBUSB_ERROR_OTHER)) {
	    show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_get_palevel(struct libusb_device_handle* devh)
{
	u8 level;
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_PALEVEL, 0, 0,
			&level, 1, 3000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return level;
}

int cmd_set_palevel(struct libusb_device_handle* devh, u16 level)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_PALEVEL, level, 0,
			NULL, 0, 3000);
	if (r != LIBUSB_SUCCESS) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

int cmd_get_rangeresult(struct libusb_device_handle* devh,
		rangetest_result *rr)
{
	u8 result[5];
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_RANGE_CHECK, 0, 0,
			result, sizeof(result), 3000);
	if (r < LIBUSB_SUCCESS) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}

	rr->valid       = result[0];
	rr->request_pa  = result[1];
	rr->request_num = result[2];
	rr->reply_pa    = result[3];
	rr->reply_num   = result[4];

	return 0;
}

int cmd_range_test(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_RANGE_TEST, 0, 0,
			NULL, 0, 1000);
	if (r != LIBUSB_SUCCESS) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

int cmd_repeater(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_REPEATER, 0, 0,
			NULL, 0, 1000);
	if (r != LIBUSB_SUCCESS) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

void cmd_get_rev_num(struct libusb_device_handle* devh, char *version, u8 len)
{
	u8 result[2 + 1 + 255];
	u16 result_ver;
	int r;
	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_REV_NUM, 0, 0,
			result, sizeof(result), 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		snprintf(version, len - 1, "error: %d", r);
		version[len-1] = '\0';
		return;
	} else if (r < 0) {
		show_libusb_error(r);
		snprintf(version, len - 1, "error: %d", r);
		version[len-1] = '\0';
		return;
	}

	result_ver = result[0] | (result[1] << 8);
	if (r == 2) { // old-style SVN rev
		sprintf(version, "%u", result_ver);
	} else {
		len = MIN(r - 3, MIN(len - 1, result[2]));
		memcpy(version, &result[3], len);
		version[len] = '\0';
	}
}

int cmd_get_board_id(struct libusb_device_handle* devh)
{
	u8 board_id;
	int r;
	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_BOARD_ID, 0, 0,
			&board_id, 1, 1000);
	if (r == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "control message unsupported\n");
		return r;
	} else if (r < 0) {
		show_libusb_error(r);
		return r;
	}

	return board_id;
}

int cmd_set_squelch(struct libusb_device_handle* devh, u16 level)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_SQUELCH, level, 0, NULL, 0, 3000);
	if (r != LIBUSB_SUCCESS) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

int cmd_get_squelch(struct libusb_device_handle* devh)
{
	u8 level;
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_SQUELCH, 0, 0,
			&level, 1, 3000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return level;
}

int cmd_set_bdaddr(struct libusb_device_handle* devh, u64 address)
{
	int r, data_len;
	u64 syncword;
	data_len = 16;
	unsigned char data[data_len];

	syncword = gen_syncword(address & 0xffffff);
	//printf("syncword=%#llx\n", syncword);
	for(r=0; r < 8; r++)
		data[r] = (address >> (8*r)) & 0xff;
	for(r=0; r < 8; r++)
		data[r+8] = (syncword >> (8*r)) & 0xff;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_BDADDR, 0, 0,
		data, data_len, 1000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	} else if (r < data_len) {
		fprintf(stderr, "Only %d of %d bytes transferred\n", r, data_len);
		return 1;
	}
	return 0;
}

int cmd_start_hopping(struct libusb_device_handle* devh, int clock_offset)
{
	int r;
	u8 data[4];
	for(r=0; r < 4; r++)
		data[r] = (clock_offset >> (8*(3-r))) & 0xff;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_START_HOPPING, 0, 0,
		data, 4, 1000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

int cmd_set_clock(struct libusb_device_handle* devh, u32 clkn)
{
	int r;
	u8 data[4];
	for(r=0; r < 4; r++)
		data[r] = (clkn >> (8*r)) & 0xff;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_CLOCK, 0, 0,
		data, 4, 1000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

uint32_t cmd_get_clock(struct libusb_device_handle* devh)
{
	u32 clock = 0;
	unsigned char data[4];
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_CLOCK, 0, 0,
			data, 4, 3000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	clock = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
	printf("Read clock = 0x%x\n", clock);
	return clock;
}

int cmd_btle_sniffing(struct libusb_device_handle* devh, u16 num)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_BTLE_SNIFFING, num, 0,
			NULL, 0, 1000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

int cmd_set_afh_map(struct libusb_device_handle* devh, u8* afh_map)
{
	int r;
	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_AFHMAP, 0, 0,
		afh_map, 10, 1000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

int cmd_clear_afh_map(struct libusb_device_handle* devh)
{
	int r;
	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_CLEAR_AFHMAP, 0, 0,
		NULL, 0, 1000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

u32 cmd_get_access_address(struct libusb_device_handle* devh)
{
	u32 access_address = 0;
	unsigned char data[4];
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_ACCESS_ADDRESS, 0, 0,
			data, 4, 3000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	access_address = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
	return access_address;
}

int cmd_set_access_address(struct libusb_device_handle* devh, u32 access_address)
{
	int r;
	u8 data[4];
	for(r=0; r < 4; r++)
		data[r] = (access_address >> (8*r)) & 0xff;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_ACCESS_ADDRESS, 0, 0,
		data, 4, 1000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

int cmd_do_something(struct libusb_device_handle *devh, unsigned char *data, int len)
{
	int r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_DO_SOMETHING, 0, 0,
				data, len, 1000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}

int cmd_do_something_reply(struct libusb_device_handle* devh, unsigned char *data, int len)
{
	int r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_DO_SOMETHING_REPLY, 0, 0,
				data, len, 3000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return r;
}

int cmd_get_crc_verify(struct libusb_device_handle* devh)
{
	u8 verify;
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_GET_CRC_VERIFY, 0, 0,
			&verify, 1, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return verify;
}

int cmd_set_crc_verify(struct libusb_device_handle* devh, int verify)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_SET_CRC_VERIFY, verify, 0,
			NULL, 0, 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return 0;
}

int cmd_poll(struct libusb_device_handle* devh, usb_pkt_rx *p)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_IN, UBERTOOTH_POLL, 0, 0,
			(u8 *)p, sizeof(usb_pkt_rx), 1000);
	if (r < 0) {
		show_libusb_error(r);
		return r;
	}
	return r;
}

int cmd_btle_promisc(struct libusb_device_handle* devh)
{
	int r;

	r = libusb_control_transfer(devh, CTRL_OUT, UBERTOOTH_BTLE_PROMISC, 0, 0,
			NULL, 0, 1000);
	if (r < 0) {
		if (r == LIBUSB_ERROR_PIPE) {
			fprintf(stderr, "control message unsupported\n");
		} else {
			show_libusb_error(r);
		}
		return r;
	}
	return 0;
}
