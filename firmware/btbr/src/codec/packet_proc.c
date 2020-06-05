/* Packet processing
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
#include <ubtbr/cfg.h>
#include <ubtbr/bb.h>

/* Encode static fields of fhs payload */
void bbpkt_fhs_prepare_payload(
	uint8_t *data, uint64_t parity, uint32_t lap, uint8_t uap, 
	uint16_t nap, uint32_t cls, uint8_t ltaddr, uint8_t has_eir)
{
	data[0] = 0xff & parity;
	data[1] = 0xff & (parity>>8);
	data[2] = 0xff & (parity>>16);
	data[3] = 0xff & (parity>>24);
	data[4] = (0x3 & (parity>>32)) | (0xfc & (lap<<2));
	data[5] = 0xff & (lap >> 6);
	data[6] = 0xff & (lap >> 14);
	data[7] = (0x03 & (lap >> 22))
		| ((has_eir&1) << 2) 	// EIR
//		| (0 << 3) 	// RES = 0
		| (1 << 4) 	// SR = 1
		| (2 << 6);	// SP = 2
	data[8] = uap;
	data[9] = 0xff & nap;
	data[10] = 0xff & (nap>>8);
	data[11] = 0xff & (cls);
	data[12] = 0xff & (cls>>8);
	data[13] = 0xff & (cls>>16);
	// 3 low bits are ltaddr
	data[14] = ltaddr;
}

/* Write last 32 bits of payload (ltaddr,clkn,psmode)
 * compute crc
 * whiten 
 * write the fec23 encoded payload in air_data
 */
void bbpkt_fhs_finalize_payload(uint8_t *fhs_data, uint32_t clk27_2)
{
	uint8_t tmp[20];
	uint16_t crc;
	fhs_data[14] = (fhs_data[14]&7) | (0xf8 & (clk27_2<<3));
	fhs_data[15] = 0xff & (clk27_2>>5);
	fhs_data[16] = 0xff & (clk27_2>>13);
	// 3 high bits are psmode = 0
	fhs_data[17] = 0x1f & (clk27_2>>21);
}

/* FHS contains 144 info bits + 16 bits of CRC: 
 * LSB                                                                 MSB
 * 0     4                    8             14                      18   20
 * |  34   | 24 | 1 | 1 |2 |2 | 8 |16 | 24  |  3   |26  |      3     |16 |
 * |Parity | Lap|EIR|Res|SR|SP|UAP|NAP|Class|ltaddr|clk2|PageScanMode|CRC|
 */
void bbpkt_decode_fhs(uint8_t *data, fhs_info_t *info)
{
	//uint64_t parity;
	uint32_t lap, clk27_2;
	//uint32_t cls;
	uint16_t nap;
	//uint8_t eir, res, sr, sp, psmode;
	uint8_t uap, ltaddr;

	/* 34 low bits of syncword */
	//parity = 0x3ffffffff & (data[0]|(data[1]<<8)|(data[2]<<16)|(data[3]<<24)|((uint64_t)(data[4]&3)<<32));

	/* 24 low bits of address */
	lap = (data[4]>>2)|(data[5]<<6)|(data[6]<<14)|((data[7]&3)<<22);
	// eir = 1&(data[7]>>2);	// Extended Inquiry Response follows
	// res = 1&(data[7]>>3);	// Reserved
	// sr =  3&(data[7]>>4);	// Page scan repetition mode
	// sp =  3&(data[7]>>6);	// Must be 2
	uap = data[8];
	nap = data[9]|((uint16_t)data[10]<<8);
	//cls = data[11]|((uint32_t)data[12]<<8)|((uint32_t)data[13]<<16);
	ltaddr = data[14]&7;
	clk27_2 = (data[14]>>3)|(data[15]<<5)|(data[16]<<13)|((data[17]&0x1f)<<21);
	//psmode = data[17]>>5;

	info->bdaddr = (lap|((uint64_t)uap<<24)|((uint64_t)nap<<32));
	info->clk27_2= clk27_2;;
	info->lt_addr = ltaddr;

	//BB_DEBUG("Valid fhs: parity = %llx, bdaddr=%04x:%02x:%06x\n", (unsigned long long)parity, nap, uap, lap);
	//BB_DEBUG("\teir=%d, res=%d, sr=%d, sp=%d, ltaddr=%d, clk27=%d, cls=0x%x, psmode=%d\n", eir, res, sr, sp, ltaddr, clk27_2, cls, psmode);
}
