/* Bluetooth codec
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
#include <ubtbr/defines.h>
#include <ubtbr/bb.h>
#include <ubtbr/codec.h>

#include <ubtbr/btphy.h>

const bbcodec_types_t bbcodec_acl_types[16] = {
/* 				ns   	hlen 	plen 	fec23 	crc	*/
	[BB_TYPE_NULL]	= {	1,	0,	0,	0,	0},
	[BB_TYPE_POLL]	= {	1,	0,	0,	0,	0},
	[BB_TYPE_FHS]	= {	1,	0,	18,	1,	1},
	[BB_TYPE_DM1]	= {	1,	1,	17,	1,	1},
	[BB_TYPE_DH1]	= {	1,	1,	27,	0,	1},
	[BB_TYPE_DM3]	= {	3,	2,	121,	1,	1},
	[BB_TYPE_DH3]	= {	3,	2,	183,	0,	1},
	[BB_TYPE_DM5]	= {	5,	2,	224,	1,	1},
	[BB_TYPE_DH5]	= {	5,	2,	339,	0,	1} 
};

/* Unwhiten n bits from packed input into packed output. 
 * update whiten seed in packet.
 */
static inline void bbcodec_unwhiten(uint8_t *whiten_state, uint8_t *out, uint8_t *in, unsigned bit_count )
{
	whiten(out, in, bit_count, whiten_state);
}

/* Payload must contain at least two decoded bytes */
static void bbcodec_calc_payload_length(bbcodec_t *codec, uint8_t *payload)
{
	const bbcodec_types_t *t = codec->t;
	unsigned payload_bits, hdr_len;

	if (t->payload_bytes != 0 && payload == NULL)
		DIE("No payload for type %p\n", codec->t);

	hdr_len = t->payload_header_bytes;

	if (codec->rx_raw)
	{
		codec->payload_length = t->payload_bytes;
	}
	else
	{
		switch(hdr_len)
		{
		case 1:
			codec->payload_length = payload[0]>>3;
			break;
		case 2:
			codec->payload_length = 0x3ff & ((payload[0]|(payload[1]<<8))>>3);
			break;
		default: // 0
			codec->payload_length = t->payload_bytes;
			break;
		}
		if (codec->payload_length > t->payload_bytes)
		{
			BB_DEBUG("(Bad size %d for type %d)", codec->payload_length,
				t->payload_bytes);
			codec->payload_length = 0;
		}
	}
	codec->coded_total = (hdr_len + codec->payload_length);
	if (t->has_crc)
		codec->coded_total += 2;
	if (t->has_fec23)
		payload_bits = codec->coded_total*12;
	else
		payload_bits = codec->coded_total*8;
	codec->air_bytes = BYTE_ALIGN(58+payload_bits);
}

/* Set codec type according to header type
 * Decode payload length in payload_header if any */
static void bbcodec_set_payload_type(bbcodec_t *codec, unsigned type)
{
	/* FIXME: only acl supported */
	codec->t = &bbcodec_acl_types[type&0xf];

	if (codec->t->nslots == 0)
	{
		cprintf("(Nyi plt %d)", type);
		/* Fallback to null.. */
		codec->t = &bbcodec_acl_types[0];
	}
}

int bbcodec_decode_header(bbcodec_t *codec, bbhdr_t *out_hdr, uint8_t *in_data)
{
	int be;
	uint8_t hdr_bytes[3];
	uint32_t h32, hdr_data;
	uint8_t hec;

	/* Unfec13 header */
	be = unfec13_hdr(hdr_bytes, in_data);
	if (be>3)
	{
		BB_DEBUG("(hdr be: %d)", be);
		return -1;
	}

	/* Unwhiten header */
	if (codec->use_whiten)
		bbcodec_unwhiten(&codec->whiten_state, hdr_bytes, hdr_bytes, 18);

	/* Decode header */
	h32 = hdr_bytes[0]|((uint32_t)hdr_bytes[1]<<8)|((uint32_t)hdr_bytes[2]<<16);
	hdr_data = h32&0x3ff;
	out_hdr->hec =(h32>>10);
	out_hdr->lt_addr = hdr_data & 7;
	out_hdr->type   = (hdr_data>>3) & 0xf;
	out_hdr->flags  = (hdr_data>>7) & 7;

	/* Check HEC */
	hec = hec_compute(hdr_data, codec->uap);
	/* FIXME: Why on earth is the high bit wrong half of the time ? */
	if ((hec&0x7f) != (0x7f&out_hdr->hec))
	{
		BB_DEBUG("(heccal %x, pkt %x)", hec, out_hdr->hec);
		return -1;
	}
	codec->air_off_b = 58;

	bbcodec_set_payload_type(codec, out_hdr->type);

	return 0;
}

/* encode pkt's header in given buffer
 * data: packet buffer, must be zeroed by caller
 */
void bbcodec_encode_header(bbcodec_t *codec, uint8_t *air_data, bbhdr_t *in_hdr, uint8_t trailer, uint8_t *in_data)
{
	uint32_t h32;
	uint8_t hdr_bytes[3];
	// Write 4bits sw_trailer + 18*3 bits header in air_data
	
	/* encode the header */
	h32 = 7 & in_hdr->lt_addr;
	h32 |= (uint32_t)(0xf & in_hdr->type) << 3;
	h32 |= (uint32_t)(0x7 & in_hdr->flags) << 7;
	h32 |= (uint32_t)hec_compute(h32, codec->uap) << 10;

	hdr_bytes[0] = 0xff & h32;
	hdr_bytes[1] = 0xff & (h32>>8);
	hdr_bytes[2] = 0xff & (h32>>16);

	/* whiten the header */
	if (codec->use_whiten)
		bbcodec_unwhiten(&codec->whiten_state, hdr_bytes, hdr_bytes, 18);

	/* Write trailer of syncword in the buffer */
	air_data[0] = trailer;

	/* fec13 & write the header at position 4 */
	fec13(air_data, hdr_bytes, 4, 18);

	/* encoder output position */
	codec->air_off_b = 58;

	/* Configure codec type */
	bbcodec_set_payload_type(codec, in_hdr->type);

	/* Set codec type / payload length */
	bbcodec_calc_payload_length(codec, in_data);
}

void bbcodec_encode_chunk(bbcodec_t *codec, uint8_t *air_data, bbhdr_t *in_hdr, uint8_t *in_data)
{
	const bbcodec_types_t *t = codec->t;
	uint8_t tmp[CODEC_TX_CHUNK_SIZE], *inp;
	unsigned byte_left, crc_num, byte_num, out_bitlen;

	/* Current input pointer */
	inp = in_data + codec->coded_pos;

	/* total encode bytes left including crc */
	byte_left = codec->coded_total - codec->coded_pos;

	/* nothing to do */
	if (byte_left == 0)
		return;

	/* If the crc calculation is not done yet */
	if (!codec->rx_raw && t->has_crc && byte_left > 2)
	{
		/* Num bytes we're going to crc */
		crc_num = MIN(byte_left-2, CODEC_TX_CHUNK_SIZE);

		codec->crc_state = crc_compute(inp, crc_num, codec->crc_state);

		/* If the crc calculation is done, write crc in in_data */
		if (codec->coded_pos+byte_left == codec->coded_total)
		{
			in_data[codec->coded_total-2] = 0xff & codec->crc_state;
			in_data[codec->coded_total-1] = 0xff & (codec->crc_state>>8);
		}
	}

	/* Num bytes we're going to encode */
	byte_num = MIN(byte_left, CODEC_TX_CHUNK_SIZE);

	/* whiten the encode bytes */
	if (codec->use_whiten)
		bbcodec_unwhiten(&codec->whiten_state, tmp, inp, byte_num*8);

	/* Encode data in the air buffer */
	if (t->has_fec23)
	{
		fec23(air_data+(codec->air_off_b>>3), tmp, codec->air_off_b&7, byte_num*8);
		out_bitlen = byte_num*12;
	}
	else
	{
		null_encode(air_data+(codec->air_off_b>>3), tmp, codec->air_off_b&7, byte_num);
		out_bitlen = byte_num*8;
	}

	/* Update input position */
	codec->coded_pos += byte_num;

	/* Update output position */
	codec->air_off_b += out_bitlen;
}

static void bbcodec_decode_update_crc(bbcodec_t *codec, const uint8_t *data)
{
	const uint8_t *crc_data = data+codec->crc_pos;
	unsigned crc_num = MIN(codec->coded_total-2,codec->coded_pos)-codec->crc_pos;

	if (crc_num)
	{
		codec->crc_state = crc_compute(crc_data, crc_num, codec->crc_state);
		codec->crc_pos += crc_num;
	}
}

/* Decode 10 host bytes */
int bbcodec_decode_chunk(bbcodec_t *codec, uint8_t *out, bbhdr_t *in_hdr, uint8_t *in_data, unsigned in_length)
{
	const bbcodec_types_t *t = codec->t;
	int bit_av, in_bitlen, first_chunk = codec->coded_pos == 0;
	uint8_t *outp;
	unsigned byte_num;

	/* Check if there is a payload to decode */
	if (t->payload_bytes == 0)
		return BBCODEC_DONE;

	/* First chunk: we do not know coded position,
	 * Just do one full chunk */
	if (first_chunk)
	{
		
		byte_num = CODEC_RX_CHUNK_SIZE;
		outp = out;
	}
	else{
		/* FIXME: hacky assert that payload length was decoded */
		if(codec->coded_total == 0)
			DIE("dec_nc: unk pl len\n");
		byte_num = MIN(CODEC_RX_CHUNK_SIZE,codec->coded_total - codec->coded_pos);
		outp = out+codec->coded_pos;
	}

	if (byte_num == 0)
		return BBCODEC_DONE;

	bit_av = in_length*8 - codec->air_off_b;
	if (bit_av < 0)
		DIE("dec_nc: error av %d of %d\n", in_length, codec->air_off_b);

	if (t->has_fec23)
	{
		in_bitlen = 12*byte_num;
		if (bit_av < in_bitlen)
			return BBCODEC_SHORT;
		/* TODO: return unfec23 error for pkt->flags ?
		 * For now, we're just relying on CRC to check validity of message. */
		unfec23(outp, in_data, codec->air_off_b, 8*byte_num);
	}
	else
	{
		in_bitlen = 8*byte_num;
		if (bit_av < in_bitlen)
			return BBCODEC_SHORT;
		null_decode(outp, in_data, codec->air_off_b, byte_num);
	}
	if (codec->use_whiten)
		bbcodec_unwhiten(&codec->whiten_state, outp, outp, byte_num*8);

	codec->air_off_b += in_bitlen;
	codec->coded_pos += byte_num;

	if (first_chunk)
	{
		/* Read payload length */
		bbcodec_calc_payload_length(codec, out);
	}
	/* update crc */
	if (!codec->rx_raw && t->has_crc)
	{
		bbcodec_decode_update_crc(codec, out);
	}
	if (codec->coded_pos >= codec->coded_total)
	{
		return BBCODEC_DONE;
	}
	return 0;
}

/* Finalize decoding of the packet type,
 * must be called after all chunks are received
 */
uint8_t bbcodec_decode_finalize(bbcodec_t *codec, uint8_t *out, uint16_t *out_size)
{
	uint16_t crc;
	uint8_t *outp;
	uint8_t rc = 0;
	const bbcodec_types_t *t = codec->t;

	/* Check if there is a payload to decode */
	if (codec->coded_pos < codec->coded_total)
	{
		DIE("Pkt not decoded? %d:%d\n",
			codec->coded_pos, codec->coded_total);
	}
	if (!codec->rx_raw && t->has_crc)
	{
		/* Verify crc */
		crc = out[codec->coded_total-2]|(out[codec->coded_total-1]<<8);
		if (crc!=codec->crc_state)
		{
			BB_DEBUG("(bad crc: cal=%x, crc=%x)", codec->crc_state, crc);
		}
		else{
			rc |= 1<<BBPKT_F_GOOD_CRC;
		}
		*out_size = codec->coded_total-2;
	}
	else
	{
		*out_size = codec->coded_total;
	}

	return rc;
}
