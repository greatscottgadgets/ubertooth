/*
 * Copyright 2013
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
#include <stdio.h>

/* Mark unused variables to avoid gcc/clang warnings */
#define UNUSED(x) (void)(x)

/*
 * based on http://www.ti.com/lit/ds/symlink/cc2400.pdf
 */

int value;
int size;
int verbose;
FILE *fp;

static void
bits_init (FILE * f, char *name, int v, int s, int vb)
{
  char format[16];
  value = v;
  size = s * 8 - 1;
  verbose = vb;
  fp = f;


  if (name)
    {
      sprintf (format, "%%s = 0x%%0%dX\n", s * 2);
      fprintf (fp, format, name, value);
    }
}

static void
bits (int high, int low, char *name, char *mode, char *desc)
{
  int i;
  int mask = 1;
  char c;

  if (strcmp (name, "-") == 0 && verbose < 2)
    return;

  mask <<= size;

  for (i = size; i >= 0; i--, mask >>= 1)
    {
      if (i > high || i < low)
	c = '.';
      else if (value & mask)
	c = '1';
      else
	c = '0';

      fprintf (fp, "%c", c);
      if (i % 8 == 0)
	fprintf (fp, " ");
    }
  if (*mode)
    fprintf (fp, "(%2s)", mode);
  if (*name)
    fprintf (fp, " %s", name);
  if (*desc)
    fprintf (fp, " (%s)", desc);
  fprintf (fp, "\n");
}

struct reg_t
{
  unsigned char num;
  char *name;
  void (*f) (unsigned short v);
};

static void
cc2400_main (unsigned short v)
{
  bits (15, 15, "RESETN", "RW", "");
  bits (14, 10, "-", "W0", "");
  bits (9, 9, "FS_FORCE_EN", "RW", "");
  bits (8, 8, "RXN_TX", "RW", (v & (1 << 8)) ? "TX" : "RX");
  bits (7, 4, "-", "W0", "");
  bits (3, 3, "-", "W0", "");
  bits (2, 2, "-", "W0", "");
  bits (1, 1, "XOSC16M_BYPASS", "RW", "");
  bits (0, 0, "XOSC16M_EN", "RW", "");
}

static void
cc2400_fsctrl (unsigned short v)
{
  char description[64];

  bits (15, 6, "-", "W0", "");
  sprintf (description, "periods=%d", 1 << (((v & 0x30) >> 4) + 6));
  bits (5, 4, "LOCK_THRESHOLD", "RW", description);
  bits (3, 3, "CAL_DONE", "RO", "");
  bits (2, 2, "CAL_RUNNING", "RO", "");
  sprintf (description, "periods=%d", 2 + (v & 0x2));
  bits (1, 1, "LOCK_LENGTH", "RW", description);
  bits (0, 0, "LOCK_STATUS", "RO", "");
}

static void
cc2400_fsdiv (unsigned short v)
{
  char description[64];

  sprintf (description, "fc = %d MHz", v & 0xfff);

  bits (15, 12, "-", "W0", "");
  bits (11, 10, "FREQ[11:10]", "RO", "");
  bits (9, 0, "FREQ[9:0]", "RW", description);
}

static void
cc2400_mdmctrl (unsigned short v)
{
  char description[64];

  bits (15, 13, "-", "W0", "");
  sprintf (description, "%0.3f kHz", 15.625 * ((v >> 7) & 0x3F));
  bits (12, 7, "MOD_OFFSET", "RW", description);
  sprintf (description, "%0.4f kHz", 3.9062 * (v & 0x7F));
  bits (6, 0, "MOD_DEV", "RW", description);
}

static void
cc2400_agcctrl (unsigned short v)
{
  char description[64];
  sprintf (description, "%d", (v >> 8));
  bits (15, 8, "VGA_GAIN", "RW", description);
  bits (7, 4, "-", "W0", "");
  bits (3, 3, "AGC_LOCKED", "RO", "");
  bits (2, 2, "AGC_LOCK", "RW", "");
  bits (1, 1, "AGC_SYNC_LOCK", "RW", "");
  bits (0, 0, "VGA_GAIN_OE", "RW", "");
}

static void
cc2400_frend (unsigned short v)
{
  char description[64];
  sprintf (description, "%d", (v & 0x7));
  bits (15, 4, "-", "W0", "");
  bits (3, 3, "-", "W1", "");
  bits (2, 0, "PA_LEVEL", "RW", description);
}

static void
cc2400_rssi (unsigned short v)
{
  char description[64];

  sprintf (description, "%d dB", ((signed short) v) >> 8);
  bits (15, 8, "RSSI_VAL", "RO", description);
  sprintf (description, "%d dB", 4 * (((signed char) v) >> 2));
  bits (7, 2, "RSSI_CS_THRES", "RW", description);
  sprintf (description, "%d bits", v & 0x3);
  bits (1, 0, "RSSI_FILT", "RW", description);
}

static void
cc2400_frequest (unsigned short v)
{
  char description[64];

  sprintf (description, "%d", ((signed short) v) >> 8);
  bits (15, 8, "RX_FREQ_OFFSET", "RO", description);
  bits (7, 0, "-", "W0", "");
}

static void
cc2400_iocfg (unsigned short v)
{
  char *description[] =
    { "Off", "Output AGC status", "Output ADC I and Q values",
    "Output I/Q after digital down-mixing and channel filtering",
    "Output RX signal magnitude / frequency unfiltered",
    "Output RX signal magnitude / frequency filtered",
    "Output RSSI / RX frequency offset estimation", "Input DAC values"
  };

  bits (15, 15, "-", "W0", "");
  bits (14, 9, "GPIO6_CFG", "RW", "");
  bits (8, 3, "GPIO1_CFG", "RW", "");
  bits (2, 0, "HSSD_SRC", "RW", description[v & 0x7]);
}

static void
cc2400_fsmtc (unsigned short v)
{
  char description[64];

  sprintf (description, "%d us", 5 * ((v >> 13) & 0x7));
  bits (15, 13, "TC_RXON2AGCEN", "RW", description);
  sprintf (description, "%d us", (v >> 10) & 0x7);
  bits (12, 10, "TC_PAON2AGCEN", "RW", description);
  bits (9, 6, "-", "RW", "");
  sprintf (description, "%d us", (v >> 3) & 0x7);
  bits (5, 3, "END2SWITCH", "RW", description);
  sprintf (description, "%d us", v & 0x7);
  bits (5, 3, "TC_TXEND2PAOFF", "RW", description);
}

static void
cc2400_reserved (unsigned short v)
{
  UNUSED(v);
  bits (15, 5, "RES", "RW", "");
  bits (4, 0, "RES", "RW", "");
}

static void
cc2400_manand (unsigned short v)
{
  UNUSED(v);
  bits (15, 15, "VGA_RESET_N", "RW", "");
  bits (14, 14, "LOCK_STATUS", "RW", "");
  bits (13, 13, "BALUN_CTRL", "RW", "");
  bits (12, 12, "RXTX", "RW", "");
  bits (11, 11, "PRE_PD", "RW", "");
  bits (10, 10, "PA_N_PD", "RW", "");
  bits (9, 9, "PA_P_PD", "RW", "");
  bits (8, 8, "DAC_LPF_PD", "RW", "");
  bits (7, 7, "BIAS_PD", "RW", "");
  bits (6, 6, "XOSC16M_PD", "RW", "");
  bits (5, 5, "CHP_PD", "RW", "");
  bits (4, 4, "FS_PD", "RW", "");
  bits (3, 3, "ADC_PD", "RW", "");
  bits (2, 2, "VGA_PD", "RW", "");
  bits (1, 1, "RXBPF_PD", "RW", "");
  bits (0, 0, "LNAMIX_PD", "RW", "");
}

static void
cc2400_fsmstate (unsigned short v)
{
  bits (15, 13, "-", "W0", "");
  bits (12, 8, "FSM_STATE_BKPT", "RW",
	((v >> 8) & 0xF) == 0 ? "disabled" : "");
  bits (7, 5, "-", "W0", "");
  bits (4, 0, "FSM_CUR_STATE", "RO", "");
}

static void
cc2400_adctst (unsigned short v)
{
  char description[64];

  bits (15, 15, "-", "W0", "");
  sprintf (description, "%d", (v >> 8) & 0x3F);
  bits (14, 8, "ADC_I", "RO", description);
  bits (7, 7, "-", "W0", "");
  sprintf (description, "%d", v & 0x3F);
  bits (6, 0, "ADC_Q", "RO", description);
}

static void
cc2400_rxbpftst (unsigned short v)
{
  char description[64];
  bits (15, 15, "-", "W0", "");
  bits (14, 14, "RXBPF_CAP_OE", "RW", "");
  sprintf(description,"%d",(v>>7)&0x7F);
  bits (13, 7, "RXBPF_CAP_O", "RW", description);
  sprintf(description,"%d",v&0x7F);
  bits (6, 0, "RXBPF_CAP_RES", "RW", description);
}

static void
cc2400_pamtst (unsigned short v)
{
  char description[64];
  char *testmode[] = {"Output IQ from RxMIX", "Input IQ to BPF", "Output IQ from VGA", "Input IQ to ADC", "Output IQ from LPF", "Input IQ to TxMIX", "Output PN from Prescaler", "Connects TX IF to RX IF and simultaneously the ATEST1 pin to the internal VC node"};
  char *current[] = {"1.72 mA", "1.88 mA", "2.05 mA", "2.21 mA"}; 

  bits(15,13,"-","W0","");
  bits(12,12,"VC_IN_TEST_EN","RW","");
  bits(11,11,"ATESTMOD_PD","WO","");
  bits(10,8,"ATESTMOD_MODE","RW",testmode[(v>>8)&7]);
  bits(7,7,"-","W0","");
  bits(6,5,"TXMIX_CAP_ARRAY","RW","");
  bits(4,3,"TXMIX_CURRENT","RW",current[(v>>3)&3]);
  sprintf(description,"%+d",(v&0x7)-3);
  bits(2,0,"PA_CURRENT","RW",description);
}

static void
cc2400_lmtst (unsigned short v)
{
  char description[64];
  bits(15,14,"-","W0","");
  bits(13,13,"RXMIX_HGM", "RW","");
  sprintf(description,"%d uA",((v>>11)&0x3)*4+12);
  bits(12,11,"RXMIX_TAIL", "RW",description);
  sprintf(description,"%d uA",((v>>9)&0x3)*4+82);
  bits(10,9,"RXMIX_CVM", "RW",description);
  switch ((v>>7)&0x3) {
     case 0:
       strcpy(description,"360");
       break;
     case 1:
       strcpy(description,"720");
       break;
     case 2:
       strcpy(description,"900");
       break;
     case 3:
       strcpy(description,"1260");
       break;
  }
  strcat(description," uA");
  bits(8,7,"RXMIX_CURRENT", "RW",description);
  switch ((v>>5)&0x3) {
     case 0:
       strcpy(description,"OFF");
       break;
     case 1:
     case 2:
     case 3:
       sprintf(description,"0.%d pF",(v>>5)&0x3);
       break;
  }
  bits(6,5,"LNA_CAP_ARRAY", "RW",description);
  bits(4,4,"LNA_LOWGAIN", "RW",(v>>4)&1?"7 dB":"19 dB");
  switch ((v>>2)&0x3) {
     case 0:
       strcpy(description,"OFF");
       break;
     case 1:
       strcpy(description,"100 uA");
       break;
     case 2:
       strcpy(description,"300 uA");
       break;
     case 3:
       strcpy(description,"1000 uA");
       break;
  }
  bits(3,2,"LNA_GAIN", "RW",description);
  switch (v&0x3) {
     case 0:
       strcpy(description,"240 uA");
       break;
     case 1:
       strcpy(description,"480 uA");
       break;
     case 2:
       strcpy(description,"640 uA");
       break;
     case 3:
       strcpy(description,"1280 uA");
       break;
  }
  bits(1,0,"LNA_CURRENT", "RW",description);
}
static void
cc2400_manor (unsigned short v)
{
  UNUSED(v);
  bits(15,15,"VGA_RESET_N","RW","");
  bits(14,14,"LOCK_STATUS","RW","");
  bits(13,13,"BALUN_CTRL","RW","");
  bits(12,12,"RXTX","RW","");
  bits(11,11,"PRE_PD","RW","");
  bits(10,10,"PA_N_PD","RW","");
  bits(9,9,"PA_P_PD","RW","");
  bits(8,8,"DAC_LPF_PD","RW","");
  bits(7,7,"BIAS_PD","RW","");
  bits(6,6,"XOSC16M_PD","RW","");
  bits(5,5,"CHP_PD","RW","");
  bits(4,4,"FS_PD","RW","");
  bits(3,3,"ADC_PD","RW","");
  bits(2,2,"VGA_PD","RW","");
  bits(1,1,"RXBPF_PD","RW","");
  bits(0,0,"LNAMIX_PD","RW","");
}
static void
cc2400_mdmtst0 (unsigned short v)
{
  char description[64];
  bits(15,14,"-","W0","");
  bits(13,13,"TX_PRNG","RW","");
  bits(12,12,"TX_1MHZ_OFFSET_N","RW","");
  bits(11,11,"INVERT_DATA","RW","");
  bits(10,10,"AFC_ADJUST_ON_PACKET","RW","");
  sprintf(description,"%d pairs",1<<((v>>8)&0x3));
  bits(9,8,"AFC_SETTLING","RW",description);
  sprintf(description,"%d",v&0xff);
  bits(7,0,"AFC_SETTLING","RW",description);
}
static void
cc2400_mdmtst1 (unsigned short v)
{
  char description[64];
  bits(15,7,"-","W0","");
  sprintf(description,"%d",v&0x7f);
  bits(6,0,"BSYNC_THRESHOLD","RW",description);
}
static void
cc2400_dactst (unsigned short v)
{
  char *src[] = {"Normal","Override","From ADC","I/Q after digital down-mixing and channel filtering", "Full-spectrum White Noise","RX signal magnitude/frequency", "RSSI/RX frequency offset estimation","HSSD Module"};
  char description[64];
  bits(15,15,"-","W0","");
  bits(14,12,"DAC_SRC","RW",src[(v>>12)&0x7]);
  sprintf(description,"%d",(v>>6)&0x3F);
  bits(11,6,"DAC_I_O","RW",description);
  sprintf(description,"%d",v&0x3F);
  bits(5,0,"DAC_Q_O","RW",description);
}
static void
cc2400_agctst0 (unsigned short v)
{
  char description[64];

  if (((v>>13)&0x7) == 0)
    strcpy(description,"Disabled");
  else
    sprintf(description,"%d * 8 MHz clock cycles",(v>>13)&0x7);
  bits(15,13,"AGC_SETTLE_BLANK_DN","RW",description);
  sprintf(description,"%d",(v>>11)&0x3);
  bits(12,11,"AGC_WIN_SIZE","RW",description);
  sprintf(description,"%d",(v>>7)&0xF);
  bits(10,7,"AGC_SETTLE_PEAK","RW",description);
  sprintf(description,"%d",(v>>3)&0xF);
  bits(6,3,"AGC_SETTLE_ADC","RW",description);
  sprintf(description,"%d",(v)&0x3);
  bits(2,0,"AGC_ATTEMPTS","RW",description);
}
static void
cc2400_agctst1 (unsigned short v)
{
  char description[64];
  bits(15,15,"-","W0","");
  bits(14,14,"AGC_VAR_GAIN_SAT","RW",(v&0x4000)?"-3/-5":"-1/-3");
  if (((v>>11)&7) == 0)
    strcpy(description,"Disabled");
  else
    sprintf(description,"%d * 8 MHz clock cycles",(v>>11)&7);
  bits(13,11,"AGC_SETTLE_BLANK_UP","RW",description);
  bits(10,10,"PEAKDET_CUR_BOOST","RW","");
  sprintf(description,"%d",(v>>6)&7);
  bits(9,6,"AGC_MULT_SLOW","RW",description);
  sprintf(description,"%d",(v>>2)&7);
  bits(5,2,"AGC_SETTLE_FIXED","RW",description);
  sprintf(description,"%d",v&3);
  bits(1,0,"AGC_SETTLE_VAR","RW",description);
}
static void
cc2400_agctst2 (unsigned short v)
{
  char description[64];
  bits(15,14,"-","W0","");
  if (((v>>12)&3) == 0)
    strcpy(description,"Disabled");
  else
    sprintf(description,"%d Fixed/Variable enable",(v>>12)&3);
  bits(13,12,"AGC_BACKEND_BLANKING","RW",description);
  sprintf(description,"%d",(v>>9)&7);
  bits(11,9,"AGC_ADJUST_M3DB","RW",description);
  sprintf(description,"%d",(v>>6)&7);
  bits(8,6,"AGC_ADJUST_M1DB","RW",description);
  sprintf(description,"%d",(v>>3)&7);
  bits(5,3,"AGC_ADJUST_P3DB","RW",description);
  sprintf(description,"%d",v&7);
  bits(2,0,"AGC_ADJUST_P1DB","RW",description);
}
static void
cc2400_fstst0 (unsigned short v)
{
  char description[64];
  char *rxmixbuf[] = {"690uA","980uA","1.16mA","1.44mA"};

  bits(15,14,"RXMIXBUF_CUR","RW",rxmixbuf[(v>>14)&3]);
  bits(13,12,"TXMIXBUF_CUR","RW",rxmixbuf[(v>>12)&3]);
  bits(11,11,"VCO_ARRAY_SETTLE_LONG","RW","");
  bits(10,10,"VCO_ARRAY_OE","RW","");
  sprintf(description,"%d",(v>>5)&0xF);
  bits(9,5,"VCO_ARRAY_O","RW",description);
  sprintf(description,"%d",v&0xF);
  bits(4,0,"VCO_ARRAY_RES","RO",description);
}
static void
cc2400_fstst1 (unsigned short v)
{
  char description[64];
  bits(15,15,"RXBPF_LOCUR","RW",(v&0x8000)?"3uA":"4uA");
  bits(14,14,"RXBPF_MIDCUR","RW",(v&0x4000)?"3uA":"4uA");
  sprintf(description,"%d",(v>>10)&0xF);
  bits(13,10,"VCO_CURRENT_REF","RW",description);
  sprintf(description,"%d",(v>>4)&0x3F);
  bits(9,4,"VCO_CURRENT_K","RW",description);
  bits(3,3,"VCO_DAC_EN","RW","");
  sprintf(description,"%d",v&0x3);
  bits(2,0,"VCO_DAC_VAL","RW","");
}
static void
cc2400_fstst2 (unsigned short v)
{
  char *speed[] = {"Normal","Undefined","Half Speed","Undefined"};
  char description[64];
  bits(15,15,"-","W0","");
  bits(14,13,"VCO_CURCAL_SPEED","RW",speed[(v>>13)&3]);
  bits(12,12,"VCO_CURRENT_OE","RW","");
  sprintf(description,"%d",(v>>6)&0x3f);
  bits(11,6,"VCO_CURRENT_O","RW",description);
  sprintf(description,"%d",v&0x3f);
  bits(5,0,"VCO_CURRENT_RES","RO",description);
}
static void
cc2400_fstst3 (unsigned short v)
{
  char *period[] = {"0.25us","0.5us","1us","4us"};
  char description[64];
  bits(15,14,"-","W0","");
  bits(13,13,"CHP_TEST_UP","RW","");
  bits(12,12,"CHP_TEST_DN","RW","");
  bits(11,11,"CHP_DISABLE","RW","");
  bits(10,10,"PD_DELAY","RW",((v>>11)&1)?"Long":"Short");
  bits(9,8,"CHP_STEP_PERIOD","RW",period[(v>>8)&3]);
  sprintf(description,"%d",(v>>4)&0xF);
  bits(7,4,"STOP_CHP_CURRENT","RW",description);
  sprintf(description,"%d",v&0xF);
  bits(3,0,"START_CHP_CURRENT","RW",description);
}
static void
cc2400_manfidl (unsigned short v)
{
  char description[64];
  sprintf(description,"0x%X",(v>>12)&0xF);
  bits(15,12,"PARTNUM","RO",description);
  sprintf(description,"0x%X",v&0xFFF);
  bits(11,0,"MANFID","RO",description);
}
static void
cc2400_manfidh (unsigned short v)
{
  char description[64];
  sprintf(description,"0x%X",(v>>12)&0xF);
  bits(15,12,"VERSION","RO",description);
  sprintf(description,"0x%X",v&0xFFF);
  bits(11,0,"PARTNUM","RO",description);
}
static void
cc2400_grmdm (unsigned short v)
{
  char *pinmode[] = {"Unbuffered","Buffered","HSSD test","Unused"};
  char *prebytes[] = {"0","1","2","4","8","16","32","Infinitely On"};
  char *syncwordsize[] = {"8","16","24","32"};
  char *dataformat[] = {"NRZ","Manchester","8/10","Reserved"};
  char description[64];
  bits(15,15,"-","W0","");
  sprintf(description,"%d",(v>>13)&3);
  bits(14,13,"SYNC_ERRBITS_ALLOWED","RW",description);
  bits(12,11,"PIN_MODE","RW",pinmode[(v>>11)&3]);
  bits(10,10,"PACKET_MODE","RW","");
  bits(9,7,"PRE_BYTES","RW",prebytes[(v>>7)&7]);
  bits(6,5,"SYNC_WORD_SIZE","RW",syncwordsize[(v>>5)&3]);
  bits(4,4,"CRC_ON","RW","");
  bits(3,2,"DATA_FORMAT","RW",dataformat[(v>>2)&3]);
  bits(1,1,"MODULATION_FORMAT","RW",((v>>1)&1)?"Reserved":"FSK/GFSK");
  bits(0,0,"TX_GAUSSIAN_FILTER","RW","");
}
static void
cc2400_grdec (unsigned short v)
{
  char *decshift[] = {"0","1","-2","-1"};
  char *channeldec[] = {"1MHz","500kHz","250kHz","125kHz"};
  char description[64];
  bits(15,13,"-","W0","");
  bits(12,12,"IND_SATURATION","RO","");
  bits(11,10,"DEC_SHIFT","RW",decshift[(v>>10)&3]);
  bits(9,8,"CHANNEL_DEC","RW",channeldec[(v>>8)&3]);
  sprintf(description,"%d",v&0xFF);
  bits(7,0,"DEC_VAL","RW",description);
}
static void
cc2400_pktstatus (unsigned short v)
{
  UNUSED(v);
  bits(15,11,"-","W0","");
  bits(10,10,"SYNC_WORD_RECEIVED","RO","");
  bits(9,9,"CRC_OK","RO","");
  bits(8,8,"-","RO","");
  bits(7,0,"-","RO","");
}
static void
cc2400_int (unsigned short v)
{
  char description[64];
  bits(15,8,"-","W0","");
  bits(7,7,"-","RW","");
  bits(6,6,"PKT_POLARITY","RW","");
  bits(5,5,"FIFO_POLARITY","RW","");
  sprintf(description,"%d",v&0x1F);
  bits(4,0,"FIFO_THRESHOLD","RW",description);
}
static void
cc2400_syncl (unsigned short v)
{
  char description[64];
  sprintf(description,"0x%04X",v);
  bits(15,0,"SYNCWORD_LOWER","RW",description);
}
static void
cc2400_synch (unsigned short v)
{
  char description[64];
  sprintf(description,"0x%04X",v);
  bits(15,0,"SYNCWORD_UPPER","RW",description);
}

static void
cc2400_res24 (unsigned short v)
{
  char description[64];
  bits(15,14,"-","W0","");
  sprintf(description,"%d",(v>>10)&0xF);
  bits(13,10,"-","RW",description);
  sprintf(description,"%d",(v>>7)&0x7);
  bits(9,7,"-","RW",description);
  sprintf(description,"%d",v&0x3F);
  bits(6,0,"-","RW",description);
}
static void
cc2400_res25 (unsigned short v)
{
  char description[64];
  bits(15,12,"-","W0","");
  sprintf(description,"%d",v&0xFFF);
  bits(11,0,"-","RW",description);
}
static void
cc2400_res26 (unsigned short v)
{
  char description[64];
  sprintf(description,"%d",(v>>10)&0x3F);
  bits(15,10,"-","RW",description);
  sprintf(description,"%d",v&0x1FF);
  bits(9,0,"-","RW",description);
}
static void
cc2400_res27 (unsigned short v)
{
  char description[64];
  sprintf(description,"%d",(v>>8)&0xFF);
  bits(15,8,"-","RO",description);
  sprintf(description,"%d",(v>>3)&0xF);
  bits(7,3,"-","RW",description);
  sprintf(description,"%d",v&7);
  bits(2,0,"-","RW",description);
}
static void
cc2400_res28 (unsigned short v)
{
  char description[64];
  bits(15,15,"-","RW","");
  sprintf(description,"%d",(v>>13)&3);
  bits(14,13,"-","RW",description);
  sprintf(description,"%d",(v>>7)&0x3F);
  bits(12,7,"-","RW",description);
  sprintf(description,"%d",v&0x3F);
  bits(6,0,"-","RW",description);
}
static void
cc2400_res29 (unsigned short v)
{
  char description[64];
  bits(15,8,"-","W0","");
  sprintf(description,"%d",(v>>3)&0x1F);
  bits(7,3,"-","RW",description);
  sprintf(description,"%d",v&7);
  bits(2,0,"-","RW",description);
}
static void
cc2400_res2a (unsigned short v)
{
  char description[64];
  bits(15,11,"-","W0","");
  bits(10,10,"-","RW","");
  sprintf(description,"%d",v&0x3FF);
  bits(9,0,"-","RW",description);
}
static void
cc2400_res2b (unsigned short v)
{
  char description[64];
  bits(15,14,"-","W0","");
  bits(13,13,"-","RW","");
  bits(12,12,"-","RO","");
  sprintf(description,"%d",v&0x7FF);
  bits(11,0,"-","RO",description);
}
static struct reg_t cc2400[] = {
  {0x00, "%MAIN", cc2400_main},
  {0x01, "%FSCTRL", cc2400_fsctrl},
  {0x02, "%FSDIV", cc2400_fsdiv},
  {0x03, "%MDMCTRL", cc2400_mdmctrl},
  {0x04, "%AGCCTRL", cc2400_agcctrl},
  {0x05, "%FREND", cc2400_frend},
  {0x06, "%RSSI", cc2400_rssi},
  {0x07, "%FREQUEST", cc2400_frequest},
  {0x08, "%IOCFG", cc2400_iocfg},
  /* 0x09 - 0x0a are Unused */
  {0x0b, "%FSMTC", cc2400_fsmtc},
  {0x0c, "%RESERVED", cc2400_reserved},
  {0x0d, "%MANAND", cc2400_manand},
  {0x0e, "%FSMSTATE", cc2400_fsmstate},
  {0x0f, "%ADCTST", cc2400_adctst},
  {0x10, "%RXBPFTST", cc2400_rxbpftst},
  {0x11, "%PAMTST", cc2400_pamtst},
  {0x12, "%LMTST", cc2400_lmtst},
  {0x13, "%MANOR", cc2400_manor},
  {0x14, "%MDMTST0", cc2400_mdmtst0},
  {0x15, "%MDMTST0", cc2400_mdmtst1},
  {0x16, "%DACTST", cc2400_dactst},
  {0x17, "%AGCTST0", cc2400_agctst0},
  {0x18, "%AGCTST1", cc2400_agctst1},
  {0x19, "%AGCTST2", cc2400_agctst2},
  {0x1a, "%FSTST0", cc2400_fstst0},
  {0x1b, "%FSTST1", cc2400_fstst1},
  {0x1c, "%FSTST2", cc2400_fstst2},
  {0x1d, "%FSTST3", cc2400_fstst3},
  {0x1e, "%MANFIDL", cc2400_manfidl},
  {0x1f, "%MANFIDH", cc2400_manfidh},
  {0x20, "%GRMDM", cc2400_grmdm},
  {0x21, "%GRDEC", cc2400_grdec},
  {0x22, "%PKTSTATUS", cc2400_pktstatus},
  {0x23, "%INT", cc2400_int},
  /* 0x24 - 0x2B are Reserved, but documented at the bit level */
  {0x24, "%R24", cc2400_res24},
  {0x25, "%R25", cc2400_res25},
  {0x26, "%R26", cc2400_res26},
  {0x27, "%R27", cc2400_res27},
  {0x28, "%R28", cc2400_res28},
  {0x29, "%R29", cc2400_res29},
  {0x2a, "%R2A", cc2400_res2a},
  {0x2b, "%R2B", cc2400_res2b},
  {0x2c, "%SYNCL", cc2400_syncl},
  {0x2d, "%SYNCH", cc2400_synch},
  /* 0x2C - 0x5F undocumented */
  {0x60, "%SXOSCON", NULL},
  {0x61, "%SFSON", NULL},
  {0x62, "%SRX", NULL},
  {0x63, "%STX", NULL},
  {0x64, "%SRFOFF", NULL},
  {0x65, "%SXOSCOFF", NULL},
  /* 0x66 - 0x6F are Reserved, but not documented at the bit level */
  {0x66, "%R66", NULL},
  {0x67, "%R67", NULL},
  {0x68, "%R68", NULL},
  {0x69, "%R69", NULL},
  {0x6a, "%R6A", NULL},
  {0x6b, "%R6B", NULL},
  {0x6c, "%R6C", NULL},
  {0x6d, "%R6D", NULL},
  {0x6e, "%R6E", NULL},
  {0x6f, "%R6F", NULL},
  {0x70, "%FIFOREG", NULL},
  /* End of list marker */
  {0, NULL, NULL}
};

int cc2400_name2reg(char *name)
{
  int i;
  int r = -1;

  for (i=0; r < 0 && cc2400[i].name; i++)
    if (strncmp(cc2400[i].name,name,strlen(cc2400[i].name)) == 0) {
      r = cc2400[i].num;
      break;
    }

  return r;
}

char *cc2400_reg2name(int r)
{
  return cc2400[r].name;
}

void
cc2400_decode (FILE * fp, int r, unsigned short v, int verbose)
{
  int i = 0;

  for (i = 0; cc2400[i].name; i++)
    if (cc2400[i].num == r)
      break;

  bits_init (fp, cc2400[i].name, v, sizeof (v), verbose);

  if (cc2400[i].f == NULL)
    {
      if (! cc2400[i].name)
        fprintf (fp, "%%r%02x = 0x%04X\n", r, v);

      if (verbose > 0)
	bits (sizeof (v) * 8 - 1, 0, "", "", "");
    }
  else if (verbose > 0)
    (cc2400[i].f) (v);
}

#ifdef CC2400_DEBUG
/*
 * Test driver to run through all posible registers and values
 */
main ()
{
  int reg,value,verbosity=1;

  // for (verbosity=0;verbosity<3;verbosity++)
    for (reg=0;reg<=0xFF;reg++)
      for (value=0;value<=0xFFFF;value++)
        cc2400_decode (stdout, reg, value, verbosity);
}
#endif
