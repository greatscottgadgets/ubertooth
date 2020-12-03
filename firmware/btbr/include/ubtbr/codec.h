#ifndef __CODEC_H
#define __CODEC_H
#include <stdint.h>
#include <ubtbr/bb.h>

#define MAX_ACL_SIZE 344

/* FIXME: pkt_type interpretation depends on channel's type.
*/
typedef struct bbcodec_types_s {
	uint8_t nslots;		// Slot occupancy
	uint8_t payload_header_bytes;	// Payload header bytes
	uint16_t payload_bytes;	// Payload bytes len
	uint8_t has_fec23;	// Payload is fec23 encoded
	uint8_t has_crc;	// CRC is applied on payload
} bbcodec_types_t;

typedef struct bbcodec_s {
	// common params 
	uint8_t uap;	
	int use_whiten;
	int rx_raw;
	uint8_t whiten_state;
	uint16_t crc_state;
	uint16_t crc_pos;
	// codec type
	const bbcodec_types_t *t;

	/* packet size */
	unsigned payload_length; // length of payload from spec or payload's header
	unsigned air_bytes;	// total bytes required for rx/tx, incl. (58bits header)+encoded(payload)
	unsigned coded_total;	// total payload bytes including payload_header, payload_daya, crc

	// decoding state
	unsigned air_off_b;	// air bit offset (dec: in, enc: out)
	unsigned coded_pos;	// coded payload position
} bbcodec_t;

static inline void bbcodec_init(bbcodec_t *codec,
		uint32_t whiten_init,
		uint8_t uap,
		int use_whiten,
		int rx_raw)
{
	codec->uap = uap;
	codec->use_whiten = use_whiten;
	codec->rx_raw = rx_raw;
	codec->crc_state = reverse8(uap)<<8;
	codec->crc_pos = 0;
	codec->whiten_state = whiten_init;
	codec->t = NULL;
	codec->payload_length = 0;
	codec->air_off_b = 0;
	codec->air_bytes = 0;
	codec->coded_total = 0;
	codec->coded_pos = 0;
}
/* Decode 10 host bytes at the time */
#define CODEC_RX_CHUNK_SIZE 10
/* Encode 80 host bytes at the time */
#define CODEC_TX_CHUNK_SIZE 80
#define BBCODEC_DONE  1
#define BBCODEC_SHORT 2

/* Codec Interface */
void bbcodec_encode_header(bbcodec_t *codec, uint8_t *air_data, bbhdr_t *in_hdr, uint8_t trailer, uint8_t *in_data);
void bbcodec_encode_chunk(bbcodec_t *codec, uint8_t *air_data, bbhdr_t *in_hdr, uint8_t *in_data);

int bbcodec_decode_header(bbcodec_t *codec, bbhdr_t *out_hdr, uint8_t *in_data);
int bbcodec_decode_chunk(bbcodec_t *codec, uint8_t *out, bbhdr_t *in_hdr, uint8_t *in_data, unsigned in_length);
uint8_t bbcodec_decode_finalize(bbcodec_t *codec, uint8_t *out, uint16_t *out_size);
#endif
