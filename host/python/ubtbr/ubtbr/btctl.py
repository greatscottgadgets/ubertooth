import usb1
import sys
from binascii import hexlify, unhexlify
from threading import Thread, Event, Lock
from queue import Queue
from struct import pack, unpack
from time import sleep
from ubtbr.lmp import *
import logging
log = logging.getLogger("btctl")
log.setLevel(logging.DEBUG)
log_handler = logging.StreamHandler(sys.stderr)
log_handler.setLevel(logging.DEBUG)
log_formatter = logging.Formatter('%(asctime)s | %(levelname)s | %(message)s',
	"%H:%M:%S")
log_handler.setFormatter(log_formatter)
log.addHandler(log_handler)

UBERTOOTH_VENDOR_ID = 0x1d50
UBERTOOTH_PRODUCT_ID = 0x6002

### BTCTL interface
BBPKT_F_HAS_PKT		= 0
BBPKT_F_HAS_HDR		= 1
BBPKT_F_HAS_CRC		= 2
BBPKT_F_GOOD_CRC	= 3

# Commands
BTCTL_DEBUG 		= 0
# Host -> Device
BTCTL_RESET_REQ 	= 20
BTCTL_IDLE_REQ 		= 21
BTCTL_SET_FREQ_OFF_REQ 	= 22
BTCTL_SET_BDADDR_REQ 	= 23
BTCTL_INQUIRY_REQ	= 24
BTCTL_PAGING_REQ	= 25
BTCTL_SET_MAX_AC_ERRORS_REQ	= 26
BTCTL_TX_ACL_REQ	= 27
BTCTL_INQUIRY_SCAN_REQ	= 28
BTCTL_PAGE_SCAN_REQ	= 29
BTCTL_SET_EIR_REQ	= 30
BTCTL_SET_AFH_REQ	= 31
BTCTL_MONITOR_REQ	= 32
# Device -> Host 
BTCTL_RX_PKT		= 40
BTCTL_STATE_RESP	= 41

# States
BTCTL_STATE_STANDBY	= 0
BTCTL_STATE_INQUIRY	= 1
BTCTL_STATE_PAGE	= 2
BTCTL_STATE_CONNECTED	= 3
BTCTL_STATE_INQUIRY_SCAN = 4
BTCTL_STATE_PAGE_SCAN	= 5

# Reasons
BTCTL_REASON_SUCCESS	= 0
BTCTL_REASON_TIMEOUT	= 1
# REASON_PAGED indicates that device entered CONNECTED state,
# but the slave did not answer yet
BTCTL_REASON_PAGED	= 2

STATE_TO_STR = {
	BTCTL_STATE_STANDBY:	"STANDBY",
	BTCTL_STATE_INQUIRY:	"INQUIRY",
	BTCTL_STATE_PAGE:	"PAGE",
}

REASON_TO_STR = {
	BTCTL_REASON_SUCCESS:	"SUCCESS",
	BTCTL_REASON_TIMEOUT:	"TIMEOUT",
	BTCTL_REASON_PAGED:	"PAGED"
}


# BT definitions
NULL = 0
POLL = 1
FHS = 2
DM1 = 3
DH1 = 4
DM3 = 10
DH3 = 11 
DM5 = 14
DH5 = 15 

ACL_TYPES_TO_STR = {
	NULL:	"NULL",
	POLL:	"POLL",
	FHS:	"FHS",
	DM1:	"DM1",
	DH1:	"DH1",
	DM3:	"DM3",
	DH3:	"DH3",
	DM5:	"DM5",
	DH5:	"DH5"
}

ACL_TYPE_TBL = [ 
# 	Type, hlen 	plen	fec23
	(DM1,	1,	17,	1	),
	(DM3,	2,	121,	1	),
	(DM5,	2,	224,	1	),
	(DH1,	1,	27,	0	),
	(DH3,	2,	183,	0	),
	(DH5,	2,	339,	0	)
]

LLID_L2CAP_CONT		= 1
LLID_L2CAP_START	= 2
LLID_LMP	 	= 3

LLID_TO_STR = {
	LLID_L2CAP_CONT:	"L2CAP_CONT",
	LLID_L2CAP_START:	"L2CAP_START",
	LLID_LMP:		"LMP"
}

def reverse8(num):
	rev8_tbl = [
		0x00,0x80,0x40,0xc0,0x20,0xa0,0x60,0xe0,0x10,0x90,0x50,0xd0,0x30,0xb0,0x70,0xf0,
		0x08,0x88,0x48,0xc8,0x28,0xa8,0x68,0xe8,0x18,0x98,0x58,0xd8,0x38,0xb8,0x78,0xf8,
		0x04,0x84,0x44,0xc4,0x24,0xa4,0x64,0xe4,0x14,0x94,0x54,0xd4,0x34,0xb4,0x74,0xf4,
		0x0c,0x8c,0x4c,0xcc,0x2c,0xac,0x6c,0xec,0x1c,0x9c,0x5c,0xdc,0x3c,0xbc,0x7c,0xfc,
		0x02,0x82,0x42,0xc2,0x22,0xa2,0x62,0xe2,0x12,0x92,0x52,0xd2,0x32,0xb2,0x72,0xf2,
		0x0a,0x8a,0x4a,0xca,0x2a,0xaa,0x6a,0xea,0x1a,0x9a,0x5a,0xda,0x3a,0xba,0x7a,0xfa,
		0x06,0x86,0x46,0xc6,0x26,0xa6,0x66,0xe6,0x16,0x96,0x56,0xd6,0x36,0xb6,0x76,0xf6,
		0x0e,0x8e,0x4e,0xce,0x2e,0xae,0x6e,0xee,0x1e,0x9e,0x5e,0xde,0x3e,0xbe,0x7e,0xfe,
		0x01,0x81,0x41,0xc1,0x21,0xa1,0x61,0xe1,0x11,0x91,0x51,0xd1,0x31,0xb1,0x71,0xf1,
		0x09,0x89,0x49,0xc9,0x29,0xa9,0x69,0xe9,0x19,0x99,0x59,0xd9,0x39,0xb9,0x79,0xf9,
		0x05,0x85,0x45,0xc5,0x25,0xa5,0x65,0xe5,0x15,0x95,0x55,0xd5,0x35,0xb5,0x75,0xf5,
		0x0d,0x8d,0x4d,0xcd,0x2d,0xad,0x6d,0xed,0x1d,0x9d,0x5d,0xdd,0x3d,0xbd,0x7d,0xfd,
		0x03,0x83,0x43,0xc3,0x23,0xa3,0x63,0xe3,0x13,0x93,0x53,0xd3,0x33,0xb3,0x73,0xf3,
		0x0b,0x8b,0x4b,0xcb,0x2b,0xab,0x6b,0xeb,0x1b,0x9b,0x5b,0xdb,0x3b,0xbb,0x7b,0xfb,
		0x07,0x87,0x47,0xc7,0x27,0xa7,0x67,0xe7,0x17,0x97,0x57,0xd7,0x37,0xb7,0x77,0xf7,
		0x0f,0x8f,0x4f,0xcf,0x2f,0xaf,0x6f,0xef,0x1f,0x9f,0x5f,0xdf,0x3f,0xbf,0x7f,0xff,
	]
	#r = 0
	#for i in range(8):
	#	r |= (num&1)<<(7-i)
	#	num>>=1
	#return r
	return rev8_tbl[num]

def crc_compute(byts, crc):
	crc_tbl = [
		0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
		0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
		0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
		0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
		0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
		0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
		0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
		0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
		0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
		0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
		0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
		0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
		0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
		0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
		0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
		0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
		0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
		0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
		0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
		0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
		0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
		0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
		0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
		0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
		0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
		0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
		0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
		0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
		0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
		0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
		0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
		0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78,
	]
	for b in byts:
		crc = (crc>>8) ^ crc_tbl[0xff & (b^crc)]
	return crc

def acl_type_for_size(size):
	for tp in ACL_TYPE_TBL:
		t,h,p,fec = tp
		if p >= size:
			return tp

def acl_type_find(type_):
	for tp in ACL_TYPE_TBL:
		t,h,p,fec = tp
		if t == type_:
			return tp

def eprint(msg):
	sys.stderr.write(msg)
	sys.stderr.flush()

def data_to_str(data):
	return " ".join("%02x"%b for b in data)

def print_state(state, reason):
	ss = STATE_TO_STR.get(state) or str(state)
	rs = REASON_TO_STR.get(reason) or str(reason)
	log.info ("Device state: %s, reason %s"%(ss, rs))

class BBHdr:
	def __init__(self, lt_addr, type_, flags=0, hec=0):
		self.lt_addr = lt_addr
		self.type = type_
		self.flags = flags
		self.hec = hec

	@staticmethod
	def unpack(data):
		"""
		typedef struct bthdr_s {
			uint8_t lt_addr;
			uint8_t type;
			uint8_t flags;
			uint8_t hec;
		} __attribute__((packed)) bbhdr_t;
		"""
		lt_addr, type_, flags, hec = data
		return BBHdr(lt_addr, type_, flags, hec)

	def pack(self):
		return bytes((self.lt_addr, self.type, self.flags, self.hec))

	def flagstr(self):
		s = ""
		if self.flags & 1:
			s += "F" # flow
		if self.flags & 2:
			s += "A" # arqn
		if self.flags & 4:
			s += "S" # seqn
		return s

	def __str__(self):
		t = ACL_TYPES_TO_STR[self.type]
		if t is None: t = "%d"%t
		s = "BBHdr(lt_addr=%d, type=%s, flags=%s)"%(
			self.lt_addr, t, self.flagstr())
		return s

class BTCtlEIR:
	EIR_FLAGS = 1
	EIR_INCOMPLETE_U16_LIST = 2
	EIR_COMPLETE_U16_LIST = 3
	EIR_INCOMPLETE_U32_LIST = 4
	EIR_COMPLETE_U32_LIST = 5
	EIR_INCOMPLETE_U128_LIST = 6
	EIR_COMPLETE_U128_LIST = 7
	EIR_SHORTENED_LOCAL_NAME = 8
	EIR_COMPLETE_LOCAL_NAME = 9
	# TODO: many more
	TYPE_NAME = {
		EIR_FLAGS: "FLAGS",
		EIR_INCOMPLETE_U16_LIST: "INCOMPLETE_U16_LIST",
		EIR_COMPLETE_U16_LIST: "COMPLETE_U16_LIST",
		EIR_INCOMPLETE_U32_LIST: "INCOMPLETE_U32_LIST",
		EIR_COMPLETE_U32_LIST: "COMPLETE_U32_LIST",
		EIR_INCOMPLETE_U128_LIST: "INCOMPLETE_U128_LIST",
		EIR_COMPLETE_U128_LIST: "COMPLETE_U128_LIST",
		EIR_SHORTENED_LOCAL_NAME: "SHORTENED_LOCAL_NAME",
		EIR_COMPLETE_LOCAL_NAME: "COMPLETE_LOCAL_NAME",
	}
	
	def __init__(self, fields):
		self.fields = fields

	@staticmethod
	def unpack(data):
		buf = data
		fields = []
		while (buf):
			l,t = buf[:2]
			dat,buf = buf[2:1+l], buf[1+l:]
			if (1+len(dat) != l):
				print ("Malformed EIR!");
				break
			fields.append((t,dat))
		return BTCtlEIR(fields)

	def pack(self):
		r = b''
		for t, dat in self.fields:
			r += bytes((len(dat)+1,t))+dat
		return r

	def __str__(self):
		s = ""
		for t, dat in self.fields:
			if s: s += "\n"
			tn = BTCtlEIR.TYPE_NAME.get(t)	
			if tn is None: tn = "0x%x"%t
			if t in (BTCtlEIR.EIR_SHORTENED_LOCAL_NAME,
				BTCtlEIR.EIR_COMPLETE_LOCAL_NAME):
				ts = "'%s'"%(dat.decode())
			else:
				ts = hexlify(dat).decode()
			s += "%s: %s"%(tn, ts)
		return s


class BTCtlFHSPkt:
	def __init__(self, parity, bdaddr, cls, ltaddr, clk27_2):
		self.parity = parity
		self.bdaddr = bdaddr
		self.cls = cls
		self.ltaddr = ltaddr
		self.clk27_2 = clk27_2

	@staticmethod
	def unpack(data):
		parity = 0x3ffffffff & ( data[0]|(data[1]<<8)
			|(data[2]<<16)|(data[3]<<24)|((data[4]&3)<<32))
		lap = (   (data[4]>>2)
			| (data[5]<<6)
			| (data[6]<<14)
			| ((3&data[7])<<22))
		uap = data[8]
		nap = data[9]|(data[10]<<8)
		bdaddr = lap|(uap<<24)|(nap<<32)

		cls = data[11]|(data[12]<<8)|(data[13]<<16)
		ltaddr = data[14]&7
		clk27_2 = (data[14]>>3)|(data[15]<<5)|(data[16]<<13)|((data[17]&0x1f)<<21)
	
		return BTCtlFHSPkt(parity, bdaddr, cls, ltaddr, clk27_2)

	def __str__(self):
		return "FHS: bdaddr=%x parity=%x cls=%x ltaddr=%x clk=%x"%(
			self.bdaddr, self.parity,self.cls,self.ltaddr, self.clk27_2)
		
class BTCtlACLPkt:
	def __init__(self, llid, data, flow=1, bt_type = None, raw_data=None, raw_size=0):
		self.llid = llid
		self.data = data
		self.flow = flow
		if bt_type is None:
			# Determine bb type according to data size
			size = len(self.data)
			tp = acl_type_for_size(size)
		else:
			tp = acl_type_find(bt_type)
		bt_type, hlen, plen, fec = tp
		self.bt_type = bt_type
		self.hlen = hlen
		# Raw RX packet fields
		self.raw_data = raw_data
		self.raw_size = raw_size

	@staticmethod
	def unpack(type_, inp):
		tp = acl_type_find(type_)
		assert(tp is not None)
		bt_type,hlen,plen,fec = tp
		if len(inp) < hlen:
			hdr = 0
		else:
			if hlen == 1:
				hdr = inp[0]
				data=inp[1:]
			else:
				hdr, = unpack("<H", inp[:2])
				data=inp[2:]
		llid = 3&hdr
		flow = 1&(hdr>>2)
		size = hdr>>3
		return BTCtlACLPkt(llid, data, flow, bt_type, raw_data=inp, raw_size=hlen+size)

	def pack(self):
		# Pack payload header
		data_hdr = pack("<"+(self.hlen == 1 and "B" or "H"),
			(len(self.data)<<3) | ((self.flow&1)<<2) | (self.llid&3))
		return data_hdr + self.data

	def __str__(self):
		s = "ACL(llid=%s, flow=%d, len=%d): %s"%(
			LLID_TO_STR.get(self.llid) or str(self.llid), self.flow, len(self.data),
			data_to_str(self.data))
		return s

class BTCtlRxPkt:
	def __init__(self, clkn, chan, flags, bb_hdr, bt_data):
		self.clkn = clkn
		self.chan = chan
		self.flags = flags
		self.bb_hdr = bb_hdr
		self.bt_data = bt_data

	@staticmethod
	def unpack(data):
		"""
		 typedef struct {
			 uint32_t clkn;          // Clkn sampled at start of rx
			 uint8_t chan;           // Channel number
			 uint8_t flags;          // Decode flags
			 uint16_t data_size;     // Size of bt_data field
			 bbhdr_t bb_hdr;         // decoded header
			 uint8_t bt_data[0];     // maximum data size for an ACL packet
		 } __attribute__((packed)) btctl_rx_pkt_t;
		"""
		hdr,bb_hdr,bt_data = data[:8],data[8:12],data[12:]
		clkn,chan,flags,data_size = unpack("<IBBH", hdr)
		assert(data_size==len(bt_data))
		bb_hdr = BBHdr.unpack(bb_hdr)
		t = bb_hdr.type
		if t == FHS:
			bt_data = BTCtlFHSPkt.unpack(bt_data)
		else:
			bt_data = BTCtlACLPkt.unpack(t, bt_data)
		return BTCtlRxPkt(clkn,chan,flags,bb_hdr,bt_data)

	def __str__(self):
		s = "RX(clkn=%d, chan=%d):"%(
			self.clkn, self.chan)
		s += "\n  %s"%self.bb_hdr
		s += "\n    %s"%self.bt_data
		return s

class BTCtlCmd(Thread):
	def __init__(self, bt):
		super().__init__()
		self._bt = bt
		self._msg_q = Queue()
		self._ready = False
		self._done = False
		self._bt.register_msg_handler(self._put_msg)

	def _start(self):
		pass

	def stop_allowed(self):
		return True

	def stop(self):
		self._bt.send_idle_cmd()

	def _handle_state(self, state, reason):
		print_state(state)

	def _handle_fhs(self, pkt):
		log.info("RX FHS: %s"%(pkt.bt_data))

	def _handle_lmp(self, pkt):
		log.info("RX LMP: %s"%(pkt.bt_data))

	def _handle_l2cap(self, pkt):
		log.info("RX L2CAP: %s"%pkt.bt_data)

	def _handle_raw(self, pkt):
		log.info("RX Raw: %s"%pkt)

	def _handle_pkt(self, pkt):
		if pkt.bb_hdr.type == FHS:
			self._handle_fhs(pkt)
		else:
			acl = pkt.bt_data
			if pkt.flags & (1<<BBPKT_F_GOOD_CRC):
				if acl.llid == LLID_LMP:
					self._handle_lmp(pkt)
				else:
					self._handle_l2cap(pkt)
			else:
				self._handle_raw(pkt)

	def _put_msg(self, t, data):
		self._msg_q.put((t,data))

	def run(self):
		self._start()
		while not self._done:
			t, msg = self._msg_q.get()
			if t == BTCTL_STATE_RESP:
				self._handle_state(*msg)
			elif t == BTCTL_RX_PKT:
				if not self._ready:
					log.warn ("Not ready for msg %d %s"%(t,msg))
				else:
					self._handle_pkt(msg)
			else:
				log.warning("nyi msg %d %s"%(t,msg))
		log.debug("%s done"%self)
		self._bt.unregister_msg_handler(self._put_msg)
		return

	def done(self):
		return self._done

	def __str__(self):
		return self.__class__.__name__

class BTCtlSuperCmd(Thread):
	def __init__(self, bt):
		super().__init__()
		self._bt = bt
		self._stopped = Event()
		self._cmd = None

	def _start_cmd(self, cls, *args):
		if self._cmd is not None:
			log.info ("Not idle")
			return
		log.info("Starting %s"%cls.__name__)
		args = [self._bt]+list(args)
		self._cmd = cls(*args)
		self._cmd.start()
		return self._cmd

	def _stop_cmd(self):
		if self._cmd is not None:
			self._cmd.stop()
			self._cmd.join()
			self._cmd = None

	def stop(self):
		self._stopped.set()

	def stopped(self):
		return self._stopped.isSet()

	def run(self):
		while not self.stopped():
			self.run_once()
		self._stop_cmd()
		log.info("%s stopped"%self.__class__.__name__)

class BTCtlInquiryCmd(BTCtlCmd):
	def _start(self):
		self._bt.send_inquiry_cmd()

	def _handle_state(self, state, reason):
		if state == BTCTL_STATE_INQUIRY:
			self._ready = True
			log.info ("Inquiry state Ready")
			return
		if state != BTCTL_STATE_STANDBY:
			log.info("Invalid state!")
			print_state(state, reason)
		log.info("Inquiry done")
		self._done = True

	def _handle_l2cap(self, pkt):
		acl = pkt.bt_data
		eir = BTCtlEIR.unpack(pkt.bt_data.data)
		log.info("%s\n%s\n"%(acl,eir))

class BTCtlInquiryScanCmd(BTCtlCmd):
	def _start(self):
		self._bt.send_inquiry_scan_cmd()

	def _handle_state(self, state, reason):
		if state == BTCTL_STATE_INQUIRY_SCAN:
			self._ready = True
			log.info ("Inquiry Scan state Ready")
			return
		if state != BTCTL_STATE_STANDBY:
			log.info("Invalid state!")
			print_state(state, reason)
		log.info("Inquiry scan done")
		self._done = True

class BTCtlPageScanCmd(BTCtlCmd):
	def _start(self):
		self._bt.send_page_scan_cmd()
		self._lmp = LMPSlave(self)

	def send_acl(self, llid, data, flow=1):
		self._bt.send_acl_cmd(llid, data, flow, self.lt_addr)

	def stop_allowed(self):
		# Avoid interrupting existing LMP transactions
		return self._done or not self._ready

	def _handle_state(self, state, reason):
		lt_addr = reason>>5
		reason &= 0x1f;
		if state == BTCTL_STATE_PAGE_SCAN:
			log.info ("Page Scan state Ready")
		elif state == BTCTL_STATE_CONNECTED:
			if reason == BTCTL_REASON_SUCCESS:
				self._ready = True
				self.lt_addr = lt_addr
				log.info("Connection ready, lt_addr=%d"%self.lt_addr)
			else:
				log.info("Connection status %d"%reason)
		else:
			if state != BTCTL_STATE_STANDBY:
				log.info("Invalid state!")
				print_state(state, reason)
			log.info("Page scan done")
			self._done = True

	def _handle_lmp(self, pkt):
		acl = pkt.bt_data
		self._lmp.receive(pkt.clkn, acl.data)

	def _handle_l2cap(self, pkt):
		acl = pkt.bt_data
		log.info("RX L2CAP: %s"%acl)

	def handle_setup_complete(self):
		log.info("LMP setup complete")
		pass

class BTCtlPagingCmd(BTCtlCmd):
	LMP_CLS = LMPMaster
	def __init__(self, bt, bdaddr):
		super().__init__(bt)
		self._bdaddr = bdaddr
		self._lmp = self.LMP_CLS(self)
		self.lt_addr = None

	def _start(self):
		self._bt.send_paging_cmd(self._bdaddr)

	def send_acl(self, llid, data, flow=1):
		self._bt.send_acl_cmd(llid, data, flow, self.lt_addr)

	def _handle_state(self, state, reason):
		lt_addr = reason>>5
		reason &= 0x1f;
		if state == BTCTL_STATE_PAGE:
			log.info("Paging started")
		elif state == BTCTL_STATE_CONNECTED:
			if reason == BTCTL_REASON_SUCCESS:
				self._ready = True
				self.lt_addr = lt_addr
				log.info("Connection ready, lt_addr=%d"%self.lt_addr)
				self._lmp.start()
			else:
				log.info ("Connection status %d"%reason)
		else:
			if state != BTCTL_STATE_STANDBY:
				log.info("Invalid state!")
				print_state(state, reason)
			self._done = True
			log.info("Paging done")

	def handle_setup_complete(self):
		log.info("LMP setup complete")

	def _handle_lmp(self, pkt):
		acl = pkt.bt_data
		self._lmp.receive(pkt.clkn, acl.data)

class BTCtlDiscoverableCmd(BTCtlSuperCmd):
	def run_once(self):
		self._cmd = self._start_cmd(BTCtlInquiryScanCmd)
		sleep(2)
		self._stop_cmd()
		self._cmd = self._start_cmd(BTCtlPageScanCmd)
		sleep(2)
		# If a connection is running, wait for it to complete
		while not self._cmd.stop_allowed() and not self.stopped():
			sleep(0.5)
		self._stop_cmd()

class BTCtlMonitorCmd(BTCtlCmd):
	def __init__(self, bt, slave_bdaddr):
		super().__init__(bt)
		self._bdaddr = slave_bdaddr
		self._uap = 0 # needs master bdaddr

	def _start(self):
		self._bt.send_monitor_cmd(self._bdaddr)

	def _handle_state(self, state, reason):
		lt_addr = reason>>5
		reason &= 0x1f;
		if state == BTCTL_STATE_PAGE_SCAN:
			log.info("Monitor started")
			self._ready = True
		elif state == BTCTL_STATE_CONNECTED:
			log.info ("Connection status %d"%reason)
		else:
			if state != BTCTL_STATE_STANDBY:
				log.info("Invalid state!")
				print_state(state, reason)
			self._done = True
			log.info("Monitor done")

	def _handle_fhs(self, pkt):
		role = "slave" if (pkt.clkn & 2) else "master"
		log.info("RX FHS (%6s:%7x): %s"%(role,pkt.clkn,pkt.bt_data))
		self._uap = 0xff&(pkt.bt_data.bdaddr>>24)

	def _handle_raw(self, pkt):
		acl = pkt.bt_data
		# Check CRC to handle LMP
		data = acl.raw_data[:acl.raw_size+2]
		if len(data) < acl.raw_size+2:
			return self._handle_bad(pkt)
		crc = u16(data[-2:])
		crc_cal = crc_compute(data[:-2], reverse8(self._uap)<<8)
		if crc != crc_cal:
			return self._handle_bad(pkt)
		# Fix acl data
		acl.data = data[acl.hlen:-2]
		if acl.llid == LLID_LMP:
			self._handle_lmp(pkt)
		else:
			self._handle_l2cap(pkt)

	def _handle_bad(self, pkt):
		role = "slave" if (pkt.clkn & 2) else "master"
		log.info("RX Raw (%6s:%7x): %s | %s"%(role,pkt.clkn, pkt.bb_hdr, hexlify(pkt.bt_data.raw_data)))

	def _handle_lmp(self, pkt):
		pdu = pkt.bt_data.data
		if not pdu: return
		op = u8(pdu[0])>>1
		if op == LMP_SET_AFH:
			data = pdu[1:]
			instant = u32(data[:4])
			mode = data[4]
			chan_map = data[5:]
			self._bt.send_set_afh_cmd(instant<<1, mode, chan_map)
			log.info("AFH req: instant=%d, (cur %d), mode=%d"%(
				instant<<1, pkt.clkn, mode))
		role = "slave" if (pkt.clkn & 2) else "master"
		log.info("RX LMP (%6s:%7x): %s | %s"%(role,pkt.clkn, pkt.bb_hdr, pdu2str(pdu)))

	def _handle_l2cap(self, pkt):
		role = "slave" if (pkt.clkn & 2) else "master"
		log.info("RX L2CAP (%6s:%7x): %s"%(role,pkt.clkn,pkt))

class BTCtl:
	DATA_IN = 0x82
	DATA_OUT = 0x05
	def __init__(self, usb):
		self._usb = usb
		self._rx_thread = None
		self._con = False
		self._con_lock = Lock()
		self._rx_stopped = Event()
		self._rx_stopped.clear()
		self._msg_handler = None

	@staticmethod
	def find():
		context = usb1.USBContext()
		handle = context.openByVendorIDAndProductID(
			UBERTOOTH_VENDOR_ID, UBERTOOTH_PRODUCT_ID,
			skip_on_error = True)
		if handle is None:
			raise Exception("Ubertooth device not found")
			sys.exit(1)
		return BTCtl(handle)

	def register_msg_handler(self, h):
		assert(self._msg_handler is None)
		self._msg_handler = h

	def unregister_msg_handler(self, h):
		assert(self._msg_handler == h)
		self._msg_handler = None

	def _print_console(self, msg):
		msg = "\x1b[32;1m%s\x1b[0m"%msg.decode()
		eprint(msg)

	def _print_debug(self, msg):
		msg = "\x1b[31;1m%s\x1b[0m"%msg.decode()
		eprint(msg)

	def _handle_default(self, t, msg):
		if t == BTCTL_STATE_RESP:
			print_state(*msg)
		else:
			log.info("No cmd for message %d"%t)

	def _handle_msg(self, data):
		t,msg =data[0], data[4:]
		if t == BTCTL_DEBUG:
			self._print_console(msg)
		else:
			if t == BTCTL_RX_PKT:
				msg = BTCtlRxPkt.unpack(msg)
			if self._msg_handler is not None:
				self._msg_handler(t,msg)
			else:
				self._handle_default(t, msg)

	def _rx_thread_main(self):
		msg = None
		msg_size = 0
		log.info("Rx thread started")
		while not self._rx_stopped.isSet():
			try:
				data = self._usb.bulkRead(self.DATA_IN, 64,100)
			except usb1.USBErrorTimeout:
				continue
			t = data[:1]
			#log.info ("got data (l:%d, t:%s): %s"%(len(data),repr(t),repr(data)))
			if t == b'P':
				self._print_debug(data[1:].strip(b"\x00"))
			else:
				if  msg is None:
					assert(t == b'S')
					msg_size, = unpack("<H",data[2:4])
					msg = data[4:]
					assert (len(data) == min(64, 4+msg_size))
				else:
					assert(t == b'C')
					assert(len(data) == min(64, 1+msg_size-len(msg)))
					msg += data[1:]
				if len(msg) == msg_size:
					self._handle_msg(msg)
					msg = None
		log.debug("Rx thread stopped")

	def connect(self):
		if self.connected():
			log.warning("Already connected")
			return
		self._usb.claimInterface(0)
		self._rx_thread = Thread(target=self._rx_thread_main)
		self._rx_thread.start()
		log.info("USB connected")
		self._con = True
		self.send_idle_cmd()
		sleep(1)

	def connected(self):
		return self._con

	def close(self):
		if not self.connected():
			log.warning("Not connected")
			return
		self.send_idle_cmd()
		self._rx_stopped.set()
		self._rx_thread.join()
		self._usb.close()
		self._con = False

	def _send_usb_bulk(self, bulk):
		assert(len(bulk)<=64)
		self._usb.bulkWrite(self.DATA_OUT, bulk)

	def _send_usb(self, data):
		self._con_lock.acquire()
		usb_hdr = b"S\x00"+pack("<H", len(data))
		chunk,data = data[:60],data[60:]
		self._send_usb_bulk(usb_hdr+chunk)
		while data:
			chunk,data = data[:63],data[63:]
			self._send_usb_bulk(b'C'+chunk)
		self._con_lock.release()

	def _send_cmd(self, t, data=b''):
		hdr = pack("<I", t)
		self._send_usb(hdr+data)

	def send_debug_cmd(self, msg):
		msg = msg[:256]
		self._send_cmd(BTCTL_DEBUG, msg)

	def send_idle_cmd(self):
		log.info("Send idle")
		self._send_cmd(BTCTL_IDLE_REQ)

	def send_set_freq_off_cmd(self, off):
		self._send_cmd(BTCTL_SET_FREQ_OFF_REQ, p16(off))

	def send_set_max_ac_errors_cmd(self, max_ac_errors):
		self._send_cmd(BTCTL_SET_MAX_AC_ERRORS_REQ, p16(max_ac_errors))

	def send_set_bdaddr_cmd(self, bdaddr):
		self._send_cmd(BTCTL_SET_BDADDR_REQ, pack("<Q", bdaddr))

	def send_inquiry_cmd(self):
		self._send_cmd(BTCTL_INQUIRY_REQ)

	def send_inquiry_scan_cmd(self):
		self._send_cmd(BTCTL_INQUIRY_SCAN_REQ)

	def send_paging_cmd(self, bdaddr):
		self._send_cmd(BTCTL_PAGING_REQ, pack("<Q", bdaddr))

	def send_monitor_cmd(self, bdaddr):
		self._send_cmd(BTCTL_MONITOR_REQ, pack("<Q", bdaddr))

	def send_page_scan_cmd(self):
		self._send_cmd(BTCTL_PAGE_SCAN_REQ)

	def send_acl_cmd(self, llid, data, flow=1, lt_addr=1, flags=0, bt_type=None):
		acl = BTCtlACLPkt(llid, data, flow, bt_type)
		bb_hdr = BBHdr(lt_addr, acl.bt_type, flags)
		payload = bb_hdr.pack() + acl.pack()
		log.debug("send bb %s, acl: %s"%(bb_hdr,acl))
		self._send_cmd(BTCTL_TX_ACL_REQ, payload)

	def send_set_eir_cmd(self, eir_data):
		acl = BTCtlACLPkt(LLID_L2CAP_START, eir_data, 1)
		bb_hdr = BBHdr(0, acl.bt_type)	# send on lt_addr 0
		payload = bb_hdr.pack() + acl.pack()
		log.debug("Set eir %s, acl: %s"%(bb_hdr,acl))
		self._send_cmd(BTCTL_SET_EIR_REQ, payload)

	def send_set_afh_cmd(self, instant, mode, afh_map):
		cmd = pack("<IB", instant,mode)+afh_map
		log.debug("send set afh: %s"%hexlify(cmd))
		self._send_cmd(BTCTL_SET_AFH_REQ, cmd)
