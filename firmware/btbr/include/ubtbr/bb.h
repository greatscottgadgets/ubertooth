#ifndef __BB_H
#define __BB_H
#include <stdint.h>
#include <ubertooth_interface.h>
#include <ubtbr/cfg.h>
#include <ubtbr/debug.h>

/* BLUETOOTH CORE SPECIFICATION Version 5.1 | Vol 2, Part B | Table 6.2 */
enum bbhdr_type_e {
	BB_TYPE_NULL	= 0,
	BB_TYPE_POLL	= 1,
	BB_TYPE_FHS	= 2,
	BB_TYPE_DM1	= 3,
	BB_TYPE_DH1	= 4,
	BB_TYPE_HV1	= 5,
	BB_TYPE_HV2	= 6,
	BB_TYPE_HV3	= 7,
	BB_TYPE_DV	= 8,
	BB_TYPE_AUX1	= 9,
	BB_TYPE_DM3	= 10,
	BB_TYPE_DH3	= 11,
	BB_TYPE_EV4	= 12,
	BB_TYPE_EV5	= 13,
	BB_TYPE_DM5	= 14,
	BB_TYPE_DH5	= 15,
};

typedef struct fhs_info_s {
	uint64_t bdaddr;
	uint32_t clk27_2;
	uint8_t lt_addr;
} fhs_info_t;

/* All sizes are excluding the 4bit preamble, which
 * is prepended to the packet by the cc2400 */
#define BB_ACCESS_CODE_SIZE 		68
#define BB_SHORT_ACCESS_CODE_SIZE 	64
#define BB_FEC13_SIZE(n)		((n)*3)
#define BB_FEC23_SIZE(n)		((n)*3/2)
#define BB_HEADER_SIZE			BB_FEC13_SIZE(18)
#define BB_FHS_PKT_SIZE 		(BB_ACCESS_CODE_SIZE+BB_HEADER_SIZE+BB_FEC23_SIZE(160))
#define BB_POLL_PKT_SIZE 		(BB_ACCESS_CODE_SIZE+BB_HEADER_SIZE)

#define GIAC 0x9e8b33

/* Declare those helpers here, to be able to inline some functions */
// Grab 6 bits from a buffer
#define EXTRACT6_0(p)   ((((((uint8_t*)(p))[0])>>0)&0x3F))
#define EXTRACT6_2(p)   ((((((uint8_t*)(p))[0])>>2)&0x3F))
#define EXTRACT6_4(p)   ((((((uint8_t*)(p))[0])>>4))|(((((uint8_t*)(p))[1])&0x3)<<4))
#define EXTRACT6_6(p)   ((((((uint8_t*)(p))[0])>>6))|(((((uint8_t*)(p))[1])&0xf)<<2))

// Grab 15 bits from a buffer
#define EXTRACT15_0(p) ( ((((uint8_t*)(p))[0]     )   ) | ((((uint8_t*)(p))[1]&0x7f)<<8) )
#define EXTRACT15_1(p) ( ((((uint8_t*)(p))[0]&0xfe)>>1) | ((((uint8_t*)(p))[1]     )<<7) )
#define EXTRACT15_2(p) ( ((((uint8_t*)(p))[0]&0xfc)>>2) | ((((uint8_t*)(p))[1]     )<<6) | ((((uint8_t*)(p))[2]&0x01)<<14) )
#define EXTRACT15_3(p) ( ((((uint8_t*)(p))[0]&0xf8)>>3) | ((((uint8_t*)(p))[1]     )<<5) | ((((uint8_t*)(p))[2]&0x03)<<13) )
#define EXTRACT15_4(p) ( ((((uint8_t*)(p))[0]&0xf0)>>4) | ((((uint8_t*)(p))[1]     )<<4) | ((((uint8_t*)(p))[2]&0x07)<<12) )
#define EXTRACT15_5(p) ( ((((uint8_t*)(p))[0]&0xe0)>>5) | ((((uint8_t*)(p))[1]     )<<3) | ((((uint8_t*)(p))[2]&0x0f)<<11) )
#define EXTRACT15_6(p) ( ((((uint8_t*)(p))[0]&0xc0)>>6) | ((((uint8_t*)(p))[1]     )<<2) | ((((uint8_t*)(p))[2]&0x1f)<<10) )
#define EXTRACT15_7(p) ( ((((uint8_t*)(p))[0]&0x80)>>7) | ((((uint8_t*)(p))[1]     )<<1) | ((((uint8_t*)(p))[2]&0x3f)<< 9) )

// Grab 10 bits from a buffer
#define EXTRACT10_0(p) ( ((((uint8_t*)(p))[0])   ) | ((((uint8_t*)(p))[1]&0x03)<<8) )
#define EXTRACT10_1(p) ( ((((uint8_t*)(p))[0])>>1) | ((((uint8_t*)(p))[1]&0x07)<<7) )
#define EXTRACT10_2(p) ( ((((uint8_t*)(p))[0])>>2) | ((((uint8_t*)(p))[1]&0x0f)<<6) )
#define EXTRACT10_3(p) ( ((((uint8_t*)(p))[0])>>3) | ((((uint8_t*)(p))[1]&0x1f)<<5) )
#define EXTRACT10_4(p) ( ((((uint8_t*)(p))[0])>>4) | ((((uint8_t*)(p))[1]&0x3f)<<4) )
#define EXTRACT10_5(p) ( ((((uint8_t*)(p))[0])>>5) | ((((uint8_t*)(p))[1]&0x7f)<<3) )
#define EXTRACT10_6(p) ( ((((uint8_t*)(p))[0])>>6) | ((((uint8_t*)(p))[1]     )<<2) )
#define EXTRACT10_7(p) ( ((((uint8_t*)(p))[0])>>7) | ((((uint8_t*)(p))[1]     )<<1) | ((((uint8_t*)(p))[2]&0x01)<< 9) )

// Encode 10 bytes in buffer (warning: buffer must be zeroed)
#define ENCODE15_0(p,val)  (((uint8_t*)(p))[0] =((val)   ),((uint8_t*)(p))[1]=((val)>>8))
#define ENCODE15_1(p,val)  (((uint8_t*)(p))[0]|=((val)<<1),((uint8_t*)(p))[1]=((val)>>7))
#define ENCODE15_2(p,val)  (((uint8_t*)(p))[0]|=((val)<<2),((uint8_t*)(p))[1]=((val)>>6),((uint8_t*)(p))[2]=((val)>>14))
#define ENCODE15_3(p,val)  (((uint8_t*)(p))[0]|=((val)<<3),((uint8_t*)(p))[1]=((val)>>5),((uint8_t*)(p))[2]=((val)>>13))
#define ENCODE15_4(p,val)  (((uint8_t*)(p))[0]|=((val)<<4),((uint8_t*)(p))[1]=((val)>>4),((uint8_t*)(p))[2]=((val)>>12))
#define ENCODE15_5(p,val)  (((uint8_t*)(p))[0]|=((val)<<5),((uint8_t*)(p))[1]=((val)>>3),((uint8_t*)(p))[2]=((val)>>11))
#define ENCODE15_6(p,val)  (((uint8_t*)(p))[0]|=((val)<<6),((uint8_t*)(p))[1]=((val)>>2),((uint8_t*)(p))[2]=((val)>>10))
#define ENCODE15_7(p,val)  (((uint8_t*)(p))[0]|=((val)<<7),((uint8_t*)(p))[1]=((val)>>1),((uint8_t*)(p))[2]=((val)>> 9))

// warning: buffer must be zeroed
#define ENCODE10_0(p,val)  (((uint8_t*)(p))[0] =((val)   ),((uint8_t*)(p))[1]=((val)>>8))
#define ENCODE10_1(p,val)  (((uint8_t*)(p))[0]|=((val)<<1),((uint8_t*)(p))[1]=((val)>>7))
#define ENCODE10_2(p,val)  (((uint8_t*)(p))[0]|=((val)<<2),((uint8_t*)(p))[1]=((val)>>6))
#define ENCODE10_3(p,val)  (((uint8_t*)(p))[0]|=((val)<<3),((uint8_t*)(p))[1]=((val)>>5))
#define ENCODE10_4(p,val)  (((uint8_t*)(p))[0]|=((val)<<4),((uint8_t*)(p))[1]=((val)>>4))
#define ENCODE10_5(p,val)  (((uint8_t*)(p))[0]|=((val)<<5),((uint8_t*)(p))[1]=((val)>>3))
#define ENCODE10_6(p,val)  (((uint8_t*)(p))[0]|=((val)<<6),((uint8_t*)(p))[1]=((val)>>2))
#define ENCODE10_7(p,val)  (((uint8_t*)(p))[0]|=((val)<<7),((uint8_t*)(p))[1]=((val)>>1),((uint8_t*)(p))[2]=((val)>>9))

#define ENCODE24_0(p,val)  (((uint8_t*)(p))[0] =((val)   ),((uint8_t*)(p))[1]=(val)>>8,((uint8_t*)(p))[2]=(val)>>16)
#define ENCODE24_2(p,val)  (((uint8_t*)(p))[0]|=((val)<<2),((uint8_t*)(p))[1]=(val)>>6,((uint8_t*)(p))[2]=(val)>>14, ((uint8_t*)(p))[3]=0x03&((val)>>22))
#define ENCODE24_4(p,val)  (((uint8_t*)(p))[0]|=((val)<<4),((uint8_t*)(p))[1]=(val)>>4,((uint8_t*)(p))[2]=(val)>>12, ((uint8_t*)(p))[3]=0x0f&((val)>>20))
#define ENCODE24_6(p,val)  (((uint8_t*)(p))[0]|=((val)<<6),((uint8_t*)(p))[1]=(val)>>2,((uint8_t*)(p))[2]=(val)>>10, ((uint8_t*)(p))[3]=0x3f&((val)>>18))

#define BYTE_ALIGN(nbits)	(((nbits)+7)>>3)

static inline uint8_t reverse8(uint8_t data)
{
	extern const uint8_t rev8_map[256];
        return rev8_map[data];
}
static inline uint16_t reverse16(uint16_t data)
{
        return (uint16_t)reverse8(data>>8) | (uint16_t)(reverse8(data&0xff)<<8);
}
static inline uint32_t reverse32(uint32_t data)
{
        return (uint32_t)reverse16(data>>16)|((uint32_t)reverse16(data&0xffff)<<16);
}

#ifdef DEBUG_CODE
/* Debug helpers */
void print_hex(uint8_t *pkt, unsigned size);
uint8_t *pack_chars(char *src, unsigned *size);
void unfec13_precal(void);
void hec_precal(void);
void fec13_slow(uint8_t *out, uint8_t *in, uint8_t nbits);
void whiten_precal_st(void);
void whiten_precal_word(void);
void fec13_precal(void);
void fec23_precal(void);
void crc_precal(void);

/* Bitstream utilities */
static inline uint8_t get_bit(const uint8_t *p, unsigned idx)
{
	return 1 & (p[idx>>3]>>(idx&7));
}
static inline void set_bit(uint8_t *p, unsigned idx, unsigned val)
{
	unsigned bi = idx&7;

	p[idx>>3] &= ~(1<<bi);
	p[idx>>3] |= ((1&val)<<bi);
}
#endif

/* HEC / CRC */
static inline uint8_t hec_compute(uint16_t data, uint8_t hec) 
{
	extern const uint8_t hec_tbl[32];

	hec = reverse8(hec);
#if 1
	hec = (hec>>5) ^ hec_tbl[(data^hec)&0x1f];
	hec = (hec>>5) ^ hec_tbl[((data>>5)^hec)&0x1f];
#else
	/* slow version */
	int i;
	uint8_t b;
	for (i=0;i<10;i++){
		b = 1&(data^hec);
		data>>=1;	
		hec>>=1;
		if (b)
			hec ^= 0xe5;
	}
#endif
	return hec;
}

static inline uint16_t crc_compute(const uint8_t *in, unsigned nbytes, uint16_t crc)
{
	extern const uint16_t crc_tbl[256];
	uint16_t i;

	for(i = 0; i < nbytes; i++)
	{
		crc = (crc>>8) ^ crc_tbl[0xff & (in[i]^crc)];
	}
	return crc;
}

/* Fast function to unfec13 the packet header  
 * in: 18*3bits = 9*6lut = 54 syms, out: 18 bits;
 * starts at 68: 8/4
 */
static inline int unfec13_hdr(uint8_t *out, uint8_t *pkt_data)
{
	extern const uint16_t unfec13_tbl[64];
	uint32_t v0, v1, v2, v3, v4, v5, v6,v7,v8;
	/* Skip 4 bits of trailer */
	uint8_t *in = pkt_data;

	/* 9 lut accesses */
	v0 = unfec13_tbl[EXTRACT6_4(in+0)]; 	// b0
	v1 = unfec13_tbl[EXTRACT6_2(in+1)];	// b1
	v2 = unfec13_tbl[EXTRACT6_0(in+2)]; 	// b2
	v3 = unfec13_tbl[EXTRACT6_6(in+2)];	
	v4 = unfec13_tbl[EXTRACT6_4(in+3)];	// b3
	v5 = unfec13_tbl[EXTRACT6_2(in+4)];	// b4
	v6 = unfec13_tbl[EXTRACT6_0(in+5)];	// b5
	v7 = unfec13_tbl[EXTRACT6_6(in+5)];	// 
	v8 = unfec13_tbl[EXTRACT6_4(in+6)]; 	// b6

	out[0] = v0|(v1<<2)|(v2<<4)|(v3<<6);
	out[1] = v4|(v5<<2)|(v6<<4)|(v7<<6);
	out[2] = v8;
	
	// bit error
	return (v0+v1+v2+v3+v4+v5+v6+v7+v8)>>14;
}

/* Whitening */
/* (Un)whiten n bits from input to output, possibly in place*/
void whiten(uint8_t* output, uint8_t* input, int length, uint8_t *statep);

int fec13(uint8_t *out, uint8_t *in, unsigned out_of, unsigned nbits);
int fec23(uint8_t *out, uint8_t *in, unsigned out_of, unsigned nbits);
int unfec13(uint8_t *out, uint8_t *in, unsigned in_of, unsigned nbits);
int unfec23(uint8_t *out, uint8_t *in, unsigned in_of, unsigned nbits);
int unfec23_10bytes(uint8_t *out, uint8_t *in);
void null_encode(uint8_t *out, uint8_t *in, unsigned out_of, unsigned byte_count);
void null_decode(uint8_t *out, uint8_t *in, unsigned in_of, unsigned byte_count);

/* Prepare the static part of the FHS payload for a paging procedure
 */
void bbpkt_fhs_prepare_payload(
	uint8_t *data, uint64_t parity, uint32_t lap, uint8_t uap, 
	uint16_t nap, uint32_t cls, uint8_t ltaddr, uint8_t has_eir);

void bbpkt_fhs_finalize_payload(uint8_t *fhs_data, uint32_t clk27_2);
void bbpkt_decode_fhs(uint8_t *data, fhs_info_t *info);

#endif
