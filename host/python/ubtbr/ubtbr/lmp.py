from time import time
import logging
from binascii import unhexlify, hexlify
from struct import pack, unpack
log = logging.getLogger("btctl")
LLID_LMP = 3

EXT_OPCODE = 0x100

# LMP opcodes
LMP_NAME_REQ                     = 1
LMP_NAME_RES                     = 2
LMP_ACCEPTED                     = 3
LMP_NOT_ACCEPTED                 = 4
LMP_CLKOFFSET_REQ                = 5
LMP_CLKOFFSET_RES                = 6
LMP_DETACH                       = 7
LMP_IN_RAND                      = 8
LMP_COMB_KEY                     = 9
LMP_UNIT_KEY                     = 10
LMP_AU_RAND                      = 11
LMP_SRES                         = 12
LMP_TEMP_RAND                    = 13
LMP_TEMP_KEY                     = 14
LMP_ENCRYPTION_MODE_REQ          = 15
LMP_ENCRYPTION_KEY_SIZE_REQ      = 16
LMP_START_ENCRYPTION_REQ         = 17
LMP_STOP_ENCRYPTION_REQ          = 18
LMP_SWITCH_REQ                   = 19
LMP_HOLD                         = 20
LMP_HOLD_REQ                     = 21
LMP_SNIFF                        = 22
LMP_SNIFF_REQ                    = 23
LMP_UNSNIFF_REQ                  = 24
LMP_PARK_REQ                     = 25
LMP_SET_BROADCAST_SCAN_WINDOW    = 27
LMP_MODIFY_BEACON                = 28
LMP_UNPARK_BD_ADDR_REQ           = 29
LMP_UNPARK_PM_ADDR_REQ           = 30
LMP_POWER_CONTROL_REQ            = 31
LMP_POWER_CONTROL_RES            = 32
LMP_MAX_POWER                    = 33
LMP_MIN_POWER                    = 34
LMP_AUTO_RATE                    = 35
LMP_PREFERRED_RATE               = 36
LMP_VERSION_REQ                  = 37
LMP_VERSION_RES                  = 38
LMP_FEATURES_REQ                 = 39
LMP_FEATURES_RES                 = 40
LMP_QUALITY_OF_SERVICE           = 41
LMP_QUALITY_OF_SERVICE_REQ       = 42
LMP_SCO_LINK_REQ                 = 43
LMP_REMOVE_SCO_LINK_REQ          = 44
LMP_MAX_SLOT                     = 45
LMP_MAX_SLOT_REQ                 = 46
LMP_TIMING_ACCURACY_REQ          = 47
LMP_TIMING_ACCURACY_RES          = 48
LMP_SETUP_COMPLETE               = 49
LMP_USE_SEMI_PERMANENT_KEY       = 50
LMP_HOST_CONNECTION_REQ          = 51
LMP_SLOT_OFFSET                  = 52
LMP_PAGE_MODE_REQ                = 53
LMP_PAGE_SCAN_MODE_REQ           = 54
LMP_SUPERVISION_TIMEOUT          = 55
LMP_TEST_ACTIVATE                = 56
LMP_TEST_CONTROL                 = 57
LMP_ENCRYPTION_KEY_SIZE_MASK_REQ = 58
LMP_ENCRYPTION_KEY_SIZE_MASK_RES = 59
LMP_SET_AFH                      = 60
LMP_ENCAPSULATED_HEADER          = 61
LMP_ENCAPSULATED_PAYLOAD         = 62
LMP_SIMPLE_PAIRING_CONFIRM       = 63
LMP_SIMPLE_PAIRING_NUMBER        = 64
LMP_DHKEY_CHECK                  = 65
LMP_ESCAPE_1                     = 124
LMP_ESCAPE_2                     = 125
LMP_ESCAPE_3                     = 126
LMP_ESCAPE_4                     = 127

# LMP extended opcodes
LMP_ACCEPTED_EXT                = 1
LMP_NOT_ACCEPTED_EXT            = 2
LMP_FEATURES_REQ_EXT            = 3
LMP_FEATURES_RES_EXT            = 4
LMP_CLK_ADJ						= 5
LMP_PACKET_TYPE_TABLE_REQ       = 11
LMP_ESCO_LINK_REQ               = 12
LMP_REMOVE_ESCO_LINK_REQ        = 13
LMP_CHANNEL_CLASSIFICATION_REQ  = 16
LMP_CHANNEL_CLASSIFICATION      = 17
LMP_SNIFF_SUBRATING_REQ         = 21
LMP_SNIFF_SUBRATING_RES         = 22
LMP_PAUSE_ENCRYPTION_REQ        = 23
LMP_RESUME_ENCRYPTION_REQ       = 24
LMP_IO_CAPABILITY_REQ           = 25
LMP_IO_CAPABILITY_RES           = 26
LMP_NUMERIC_COMPARISON_FAILED   = 27
LMP_PASSKEY_FAILED              = 28
LMP_OOB_FAILED                  = 29
LMP_KEYPRESS_NOTIFICATION       = 30
LMP_POWER_CONTROL_INC           = 31
LMP_POWER_CONTROL_DEC           = 32

LMP_OP2STR = {
	LMP_NAME_REQ: "LMP_NAME_REQ",
	LMP_NAME_RES: "LMP_NAME_RES",
	LMP_ACCEPTED: "LMP_ACCEPTED",
	LMP_NOT_ACCEPTED: "LMP_NOT_ACCEPTED",
	LMP_CLKOFFSET_REQ: "LMP_CLKOFFSET_REQ",
	LMP_CLKOFFSET_RES: "LMP_CLKOFFSET_RES",
	LMP_DETACH: "LMP_DETACH",
	LMP_IN_RAND: "LMP_IN_RAND",
	LMP_COMB_KEY: "LMP_COMB_KEY",
	LMP_UNIT_KEY: "LMP_UNIT_KEY",
	LMP_AU_RAND: "LMP_AU_RAND",
	LMP_SRES: "LMP_SRES",
	LMP_TEMP_RAND: "LMP_TEMP_RAND",
	LMP_TEMP_KEY: "LMP_TEMP_KEY",
	LMP_ENCRYPTION_MODE_REQ: "LMP_ENCRYPTION_MODE_REQ",
	LMP_ENCRYPTION_KEY_SIZE_REQ: "LMP_ENCRYPTION_KEY_SIZE_REQ",
	LMP_START_ENCRYPTION_REQ: "LMP_START_ENCRYPTION_REQ",
	LMP_STOP_ENCRYPTION_REQ: "LMP_STOP_ENCRYPTION_REQ",
	LMP_SWITCH_REQ: "LMP_SWITCH_REQ",
	LMP_HOLD: "LMP_HOLD",
	LMP_HOLD_REQ: "LMP_HOLD_REQ",
	LMP_SNIFF: "LMP_SNIFF",
	LMP_SNIFF_REQ: "LMP_SNIFF_REQ",
	LMP_UNSNIFF_REQ: "LMP_UNSNIFF_REQ",
	LMP_PARK_REQ: "LMP_PARK_REQ",
	LMP_SET_BROADCAST_SCAN_WINDOW: "LMP_SET_BROADCAST_SCAN_WINDOW",
	LMP_MODIFY_BEACON: "LMP_MODIFY_BEACON",
	LMP_UNPARK_BD_ADDR_REQ: "LMP_UNPARK_BD_ADDR_REQ",
	LMP_UNPARK_PM_ADDR_REQ: "LMP_UNPARK_PM_ADDR_REQ",
	LMP_POWER_CONTROL_REQ: "LMP_POWER_CONTROL_REQ",
	LMP_POWER_CONTROL_RES: "LMP_POWER_CONTROL_RES",
	LMP_MAX_POWER: "LMP_MAX_POWER",
	LMP_MIN_POWER: "LMP_MIN_POWER",
	LMP_AUTO_RATE: "LMP_AUTO_RATE",
	LMP_PREFERRED_RATE: "LMP_PREFERRED_RATE",
	LMP_VERSION_REQ: "LMP_VERSION_REQ",
	LMP_VERSION_RES: "LMP_VERSION_RES",
	LMP_FEATURES_REQ: "LMP_FEATURES_REQ",
	LMP_FEATURES_RES: "LMP_FEATURES_RES",
	LMP_QUALITY_OF_SERVICE: "LMP_QUALITY_OF_SERVICE",
	LMP_QUALITY_OF_SERVICE_REQ: "LMP_QUALITY_OF_SERVICE_REQ",
	LMP_SCO_LINK_REQ: "LMP_SCO_LINK_REQ",
	LMP_REMOVE_SCO_LINK_REQ: "LMP_REMOVE_SCO_LINK_REQ",
	LMP_MAX_SLOT: "LMP_MAX_SLOT",
	LMP_MAX_SLOT_REQ: "LMP_MAX_SLOT_REQ",
	LMP_TIMING_ACCURACY_REQ: "LMP_TIMING_ACCURACY_REQ",
	LMP_TIMING_ACCURACY_RES: "LMP_TIMING_ACCURACY_RES",
	LMP_SETUP_COMPLETE: "LMP_SETUP_COMPLETE",
	LMP_USE_SEMI_PERMANENT_KEY: "LMP_USE_SEMI_PERMANENT_KEY",
	LMP_HOST_CONNECTION_REQ: "LMP_HOST_CONNECTION_REQ",
	LMP_SLOT_OFFSET: "LMP_SLOT_OFFSET",
	LMP_PAGE_MODE_REQ: "LMP_PAGE_MODE_REQ",
	LMP_PAGE_SCAN_MODE_REQ: "LMP_PAGE_SCAN_MODE_REQ",
	LMP_SUPERVISION_TIMEOUT: "LMP_SUPERVISION_TIMEOUT",
	LMP_TEST_ACTIVATE: "LMP_TEST_ACTIVATE",
	LMP_TEST_CONTROL: "LMP_TEST_CONTROL",
	LMP_ENCRYPTION_KEY_SIZE_MASK_REQ: "LMP_ENCRYPTION_KEY_SIZE_MASK_REQ",
	LMP_ENCRYPTION_KEY_SIZE_MASK_RES: "LMP_ENCRYPTION_KEY_SIZE_MASK_RES",
	LMP_SET_AFH: "LMP_SET_AFH",
	LMP_ENCAPSULATED_HEADER: "LMP_ENCAPSULATED_HEADER",
	LMP_ENCAPSULATED_PAYLOAD: "LMP_ENCAPSULATED_PAYLOAD",
	LMP_SIMPLE_PAIRING_CONFIRM: "LMP_SIMPLE_PAIRING_CONFIRM",
	LMP_SIMPLE_PAIRING_NUMBER: "LMP_SIMPLE_PAIRING_NUMBER",
	LMP_DHKEY_CHECK: "LMP_DHKEY_CHECK",
	LMP_ESCAPE_1: "LMP_ESCAPE_1",
	LMP_ESCAPE_2: "LMP_ESCAPE_2",
	LMP_ESCAPE_3: "LMP_ESCAPE_3",
	LMP_ESCAPE_4: "LMP_ESCAPE_4",
}

LMP_OPEXT2STR = {
	LMP_ACCEPTED_EXT: "LMP_ACCEPTED_EXT",
	LMP_NOT_ACCEPTED_EXT: "LMP_NOT_ACCEPTED_EXT",
	LMP_FEATURES_REQ_EXT: "LMP_FEATURES_REQ_EXT",
	LMP_FEATURES_RES_EXT: "LMP_FEATURES_RES_EXT",
	LMP_PACKET_TYPE_TABLE_REQ: "LMP_PACKET_TYPE_TABLE_REQ",
	LMP_ESCO_LINK_REQ: "LMP_ESCO_LINK_REQ",
	LMP_REMOVE_ESCO_LINK_REQ: "LMP_REMOVE_ESCO_LINK_REQ",
	LMP_CHANNEL_CLASSIFICATION_REQ: "LMP_CHANNEL_CLASSIFICATION_REQ",
	LMP_CHANNEL_CLASSIFICATION: "LMP_CHANNEL_CLASSIFICATION",
	LMP_SNIFF_SUBRATING_REQ: "LMP_SNIFF_SUBRATING_REQ",
	LMP_SNIFF_SUBRATING_RES: "LMP_SNIFF_SUBRATING_RES",
	LMP_PAUSE_ENCRYPTION_REQ: "LMP_PAUSE_ENCRYPTION_REQ",
	LMP_RESUME_ENCRYPTION_REQ: "LMP_RESUME_ENCRYPTION_REQ",
	LMP_IO_CAPABILITY_REQ: "LMP_IO_CAPABILITY_REQ",
	LMP_IO_CAPABILITY_RES: "LMP_IO_CAPABILITY_RES",
	LMP_NUMERIC_COMPARISON_FAILED: "LMP_NUMERIC_COMPARISON_FAILED",
	LMP_PASSKEY_FAILED: "LMP_PASSKEY_FAILED",
	LMP_OOB_FAILED: "LMP_OOB_FAILED",
	LMP_KEYPRESS_NOTIFICATION: "LMP_KEYPRESS_NOTIFICATION",
	LMP_POWER_CONTROL_INC: "LMP_POWER_CONTROL_INC",
	LMP_POWER_CONTROL_DEC: "LMP_POWER_CONTROL_DEC",
}

def u8(d):	return d
def u16(d):
	return unpack("<H", d)[0]
def u32(d):
	return unpack("<I", d)[0]
def p8(n):	return pack("<B", n)
def p16(n):	return pack("<H", n)
def p32(n):	return pack("<I", n)
def p64(n):	return pack("<Q", n)

def pdu2str(pdu):
	opcode = u8(pdu[0])
	tid = opcode & 1
	opcode >>= 1

	if opcode == LMP_ESCAPE_4:
		op = u8(pdu[1])
		opstr = LMP_OPEXT2STR.get(op)
	else:
		opstr = LMP_OP2STR.get(opcode)
	opstr = "\x1b[33;1m%s\x1b[0m"%opstr
	return ("(tid %d) %-40s | %s"%(tid, opstr, hexlify(pdu)))

class LMP:
	# Features supported (Core v5.2 | Vol 2, Part C, table 3.2 p585)
	#FEATURES = b"\x03\x00\x00\x00\x08\x08\x19\x00" # With AFH
	FEATURES = b"\x03\x00\x00\x00\x00\x00\x19\x00" # No AFH

	def __init__(self, con, debug=True):
		self._debug = debug
		self._con = con
		self._start_time = time()
		self._clkn = 0

	def time(self):
		return time()-self._start_time

	# FSM per LMP_state
	def receive(self, clkn, pdu):
		self._clkn = clkn
		# Parse lmp message
		opcode = u8(pdu[0])
		tid = opcode & 1
		opcode >>= 1

		if opcode == LMP_ESCAPE_4:
			op = u8(pdu[1]) | EXT_OPCODE
			data = pdu[2:]
		else:
			op = opcode
			data = pdu[1:]

		if self._debug:
			log.info("%d|%.2f sec | <<< lmp_rx (state=%d): %s"%(clkn,self.time(),self._state, pdu2str(pdu)))

		fsm = self._FSM[self._state]
		reg = fsm.get(op)
		if reg is None:
			log.warn("Unhandled opcode 0x%x"%op)
			return False
		else:
			handler = reg[0]
			if len(reg) > 1:
				next_state = reg[1]
			else:
				next_state = self._state
			retval = handler(op, data) or 0
			if retval is not None:
				if retval < 0:
					return retval
				elif retval > 0:
					next_state = retval
			self.set_state(next_state)

	def set_state(self, state):
		if (state != self._state):
			log.info("%.2f sec | Switch state %d -> %d"%(self.time(), self._state, state))
			self._state = state

	def lmp_down(self, pdu):
		self._con.send_acl(LLID_LMP, pdu)

	def pack_lmp(self, opcode, data, tid=0):
		pdu = p8((opcode<<1)|(tid&1)) + data
		pdu = pdu.ljust(17, b"\x00")
		return pdu

	def lmp_send(self, opcode, data, tid=0):
		pdu = self.pack_lmp(opcode, data, tid=tid)
		if self._debug:
			log.info("%.2f sec | >>> lmp_tx (state=%d): %s"%(self.time(),self._state, pdu2str(pdu)))
		return self.lmp_down(pdu)

	def handle_name_req(self, op, data):
		offset = u8(data[0])
		name = b"Ubertooth"
		return self.lmp_send_name_res(offset, len(name), name[offset:offset+14])

	def handle_feat_req(self, op, data):	
		log.info("handle_feat_req")
		self.lmp_send_feat(False)

	def handle_feat_req_ext(self, op, data):
		log.info("handle_feat_req_ext")
		self.lmp_send_feat_ext(False, num=u8(data[0]))

	def handle_vers_req(self, op, data):	
		log.info("handle_vers_req")
		self.lmp_send_version(False)

	def handle_io_cap_req(self, op, data):
		log.info("handle_io_cap_req")
		self.lmp_send_io_cap(False)

	def lmp_send_conn_req(self, **kwargs):
		return self.lmp_send(LMP_HOST_CONNECTION_REQ, b'', **kwargs)

	def lmp_send_name_req(self, offset, **kwargs):
		return self.lmp_send(LMP_NAME_REQ, p8(offset), **kwargs)

	def lmp_send_name_res(self, poffset, psize, payload, **kwargs):
		assert(len(payload)<=14)
		data = p8(poffset) + p8(psize) + payload
		return self.lmp_send(LMP_NAME_RES, data, **kwargs)

	def lmp_send_accepted(self, opcode, **kwargs):
		return self.lmp_send(LMP_ACCEPTED, p8(opcode), **kwargs)

	def lmp_send_not_accepted(self, opcode, data=b'', **kwargs):
		return self.lmp_send(LMP_NOT_ACCEPTED, p8(opcode)+data, **kwargs)

	def lmp_send_io_cap(self, is_req, **kwargs):
		# ext opcode
		if is_req:
			data = p8(LMP_IO_CAPABILITY_REQ)
		else: 
			data = p8(LMP_IO_CAPABILITY_RES)	
		data += b"\x01\x00\x03"
		return self.lmp_send(LMP_ESCAPE_4, data, **kwargs)

	def lmp_send_encap_header(self, size, minor=1,major=1,**kwargs):
		payload = p8(minor)+p8(major) # version
		payload += p8(size)
		return self.lmp_send(LMP_ENCAPSULATED_HEADER, payload, **kwargs)

	def lmp_send_encap_payload(self, data, **kwargs):
		assert(len(data) == 16)
		return self.lmp_send(LMP_ENCAPSULATED_PAYLOAD, data, **kwargs)

	def lmp_send_version(self, is_req, **kwargs):
		# Opcode
		if is_req:	op = LMP_VERSION_REQ
		else:		op = LMP_VERSION_RES
		data = p8(1)		# version num
		data += p16(0xffff)	# Company identifier: none
		data += p16(2020)	# sub version num: 2020
		return self.lmp_send(op, data, **kwargs)
		
	def lmp_send_feat(self, is_req, **kwargs):
		if is_req:	op = LMP_FEATURES_REQ
		else:		op = LMP_FEATURES_RES
		return self.lmp_send(op, self.FEATURES, **kwargs)
		
	def lmp_send_feat_ext(self, is_req, num=1, **kwargs):
		# Ext opcode
		if is_req:	data = p8(LMP_FEATURES_REQ_EXT)
		else:		data = p8(LMP_FEATURES_RES_EXT)
		data += p8(num)	# page 
		data += p8(num)	# Max supported page
		data = data.ljust(11, b"\x00")	# We support none 
		return self.lmp_send(LMP_ESCAPE_4, data, **kwargs)

	def lmp_send_setup_complete(self, **kwargs):
		return self.lmp_send(LMP_SETUP_COMPLETE, b'', **kwargs)

	def lmp_send_set_afh(self, instant, mode, cmap):
		assert(len(cmap)==10)
		log.info("Send AFH req: instant=%d, (cur %d), mode=%d"%(
			instant, self._clkn, mode))
		return self.lmp_send(LMP_SET_AFH, p32(instant>>1)+p8(mode)+cmap)

class LMPMaster(LMP):
	def __init__(self, con):
		self._FSM = {
			# First state: fetch device infos
			1: {
				LMP_VERSION_REQ: 	(self.handle_vers_req,),
				LMP_FEATURES_REQ:	(self.handle_feat_req,),
				EXT_OPCODE|LMP_FEATURES_REQ_EXT:	(self.handle_feat_req_ext,),
				LMP_IO_CAPABILITY_REQ: 	(self.handle_io_cap_req,),
				LMP_NAME_REQ: 		(self.handle_name_req,),
				LMP_FEATURES_RES:	(self.handle_info_res,),
				EXT_OPCODE|LMP_FEATURES_RES_EXT: (self.handle_info_res,),
				LMP_VERSION_RES: 	(self.handle_info_res,),
				LMP_IO_CAPABILITY_RES: 	(self.handle_info_res,),
				LMP_NAME_RES: 		(self.handle_info_res,),
				LMP_SLOT_OFFSET:	(self.handle_slot_offset,),
				LMP_SWITCH_REQ:		(self.handle_switch_req,),
				LMP_ACCEPTED:		(self.handle_accepted,),
				LMP_NOT_ACCEPTED:		(self.handle_not_accepted,),
				LMP_SETUP_COMPLETE:	(self.handle_setup_complete,),
				LMP_ENCRYPTION_KEY_SIZE_MASK_RES: (self.handle_info_res,),
			},
		}
		self._state = 1
		self.rmt_features = None
		self.rmt_features_ext = None
		self.rmt_iocap = None
		self.rmt_version = None
		self.rmt_name = None
		self.rmt_enc_key_size_mask = None
		super().__init__(con)

	def start(self):
		self.send_info_req()

	def handle_not_accepted(self, op, data):
		pass

	def handle_accepted(self, op, data):
		pass

	def handle_slot_offset(self, op, data):
		offset = u16(data[:2])
		bdaddr = data[2:][::-1]
		log.info("slot_offset from %s: %dusec\n"%(
			hexlify(bdaddr).decode(),offset))

	def handle_switch_req(self, op, data):
		instant = u32(data)
		log.info("switch req: instant: %d (in %d ticks)"%(
			instant, (instant<<1)-self._clkn))
		
	def send_info_req(self):
		if self.rmt_features is None:
			self.lmp_send_feat(True)
			return
		if self.rmt_features_ext is None:
			self.lmp_send_feat_ext(True)
			return
		if self.rmt_version is None:
			self.lmp_send_version(True)
			return
		if self.rmt_name is None:
			self.lmp_send_name_req(0)
			return
		if self.rmt_enc_key_size_mask is None:
			self.lmp_send(LMP_ENCRYPTION_KEY_SIZE_MASK_REQ, b'')
			return
		self.lmp_send(LMP_HOST_CONNECTION_REQ,b'')

	def handle_info_res(self, op, data):
		if op == LMP_FEATURES_RES:
			self.rmt_features = data
		elif op == LMP_VERSION_RES:
			self.rmt_version = data
		elif op == EXT_OPCODE|LMP_FEATURES_RES_EXT:
			self.rmt_features_ext = data
		elif op == LMP_NAME_RES:
			self.rmt_name = data
		elif op == LMP_ENCRYPTION_KEY_SIZE_MASK_RES:
			self.rmt_enc_key_size_mask = data
		self.send_info_req()

	def handle_setup_complete(self, op, data):
		self.lmp_send(LMP_SETUP_COMPLETE, b"\x00"*16)
		self._con.handle_setup_complete()

class LMPSlave(LMP):
	def __init__(self, con):
		self._FSM = {
			# First state: wait for establishment message
			1: {
				LMP_DETACH: 		(self.handle_detach,),
				LMP_VERSION_REQ: 	(self.handle_vers_req,),
				LMP_FEATURES_REQ:	(self.handle_feat_req,),
				EXT_OPCODE|LMP_FEATURES_REQ_EXT:	(self.handle_feat_req_ext,),
				LMP_NAME_REQ: 		(self.handle_name_req,),
				LMP_HOST_CONNECTION_REQ: (self.handle_host_connection_req,),
				LMP_SETUP_COMPLETE: 	(self.handle_setup_complete,),
				LMP_SET_AFH:		(self.handle_set_afh,),
			},
		}
		self._state = 1
		super().__init__(con)

	def handle_host_connection_req(self, op, data):
		self.lmp_send_accepted(op)

	def handle_detach(self, op, data):
		self._con.stop()
		pass

	def handle_setup_complete(self, op, data):
		self.lmp_send_setup_complete()
		self._con.handle_setup_complete()

	def handle_set_afh(self, op, data):
		instant = u32(data[:4])
		mode = data[4]
		chan_map = data[5:]
		log.info("AFH req: instant=%d, (cur %d), mode=%d"%(
			instant<<1, self._clkn, mode))
		self.lmp_send_accepted(op)
		self._con._bt.send_set_afh_cmd(instant<<1, mode, chan_map)

	def start(self):
		pass
