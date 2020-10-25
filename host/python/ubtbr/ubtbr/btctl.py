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

# Commands
BTCTL_DEBUG 		= 0
# Host -> Device
BTCTL_RESET_REQ 	= 20
BTCTL_IDLE_REQ 		= 21
BTCTL_SET_FREQ_OFF_REQ 	= 22
BTCTL_SET_BDADDR_REQ 	= 23
BTCTL_INQUIRY_REQ	= 24
BTCTL_PAGING_REQ	= 25
BTCTL_TXTEST_REQ	= 26
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
BTCTL_STATE_TEST	= 4
BTCTL_STATE_INQUIRY_SCAN = 5
BTCTL_STATE_PAGE_SCAN	= 6

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
	BTCTL_STATE_TEST:	"TEST"
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
	def __init__(self, llid, data, flow=1, bt_type = None):
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

	@staticmethod
	def unpack(type_, data):
		tp = acl_type_find(type_)
		assert(tp is not None)
		bt_type,hlen,plen,fec = tp
		if hlen == 1:
			hdr = data[0]
			data=data[1:]
		else:
			hdr, = unpack("<H", data[:2])
			data=data[2:]
		llid = 3&hdr
		flow = 1&(hdr>>2)
		size = hdr>>3
		return BTCtlACLPkt(llid, data, flow, bt_type)

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

	def _handle_pkt(self, pkt):
		if pkt.bb_hdr.type == FHS:
			self._handle_fhs(pkt)
		else:
			acl = pkt.bt_data
			if acl.llid == LLID_LMP:
				self._handle_lmp(pkt)
			else:
				self._handle_l2cap(pkt)

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

	def _start(self):
		self._bt.send_monitor_cmd(self._bdaddr)

	def _handle_state(self, state, reason):
		lt_addr = reason>>5
		reason &= 0x1f;
		if state == BTCTL_STATE_PAGE_SCAN:
			log.info("Monitor started")
		elif state == BTCTL_STATE_CONNECTED:
			self._ready = True
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
		self._send_cmd(BTCTL_SET_FREQ_OFF_REQ, pack("<B", off))

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
