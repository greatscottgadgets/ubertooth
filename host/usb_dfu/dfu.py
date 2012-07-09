#!/usr/bin/env python
#
# Copyright 2010-2012 Michael Ossmann, Jared Boone, Dominic Spill
# Copyright 2010 TheSeven
#
# This file is part of Project Ubertooth.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

# http://pyusb.sourceforge.net/docs/1.0/tutorial.html

import struct
import sys
import time

class Enumeration(object):
    def __init__(self, id, name):
        self._id = id
        self._name = name
        setattr(self.__class__, name, self)
        self.map[id] = self

    def __int__(self):
        return self.id

    def __repr__(self):
        return self.name

    @property
    def id(self):
        return self._id

    @property
    def name(self):
        return self._name

    @classmethod
    def create_from_map(cls):
        for id, name in cls.map.iteritems():
            cls(id, name)
        
class Request(Enumeration):
    map = {
        0: 'DETACH',
        1: 'DNLOAD',
        2: 'UPLOAD',
        3: 'GETSTATUS',
        4: 'CLRSTATUS',
        5: 'GETSTATE',
        6: 'ABORT',
    }

Request.create_from_map()

class State(Enumeration):
    map = {
         0: 'appIDLE',
         1: 'appDETACH',
         2: 'dfuIDLE',
         3: 'dfuDNLOAD_SYNC',
         4: 'dfuDNBUSY',
         5: 'dfuDNLOAD_IDLE',
         6: 'dfuMANIFEST_SYNC',
         7: 'dfuMANIFEST',
         8: 'dfuMANIFEST_WAIT_RESET',
         9: 'dfuUPLOAD_IDLE',
        10: 'dfuERROR',
    }

State.create_from_map()

class Status(Enumeration):
    map = {
        0x00: 'OK',
        0x01: 'errTARGET',
        0x02: 'errFILE',
        0x03: 'errWRITE',
        0x04: 'errERASE',
        0x05: 'errCHECK_ERASED',
        0x06: 'errPROG',
        0x07: 'errVERIFY',
        0x08: 'errADDRESS',
        0x09: 'errNOTDONE',
        0x0A: 'errFIRMWARE',
        0x0B: 'errVENDOR',
        0x0C: 'errUSBR',
        0x0D: 'errPOR',
        0x0E: 'errUNKNOWN',
        0x0F: 'errSTALLEDPKT',
    }

Status.create_from_map()

class DFU(object):
    def __init__(self, device):
        self._device = device

    def detach(self):
        self._device.ctrl_transfer(0x21, Request.DETACH, 0, 0, None)

    def download(self, block_number, data):
        self._device.ctrl_transfer(0x21, Request.DNLOAD, block_number, 0, data)

    def upload(self, block_number, length):
        data = self._device.ctrl_transfer(0xA1, Request.UPLOAD, block_number, 0, length)
        return data
    
    def get_status(self):
        status_packed = self._device.ctrl_transfer(0xA1, Request.GETSTATUS, 0, 0, 6)
        status = struct.unpack('<BBBBBB', status_packed)
        return (Status.map[status[0]], (((status[1] << 8) | status[2]) << 8) | status[3],
                State.map[status[4]], status[5])

    def clear_status(self):
        self._device.ctrl_transfer(0x21, Request.CLRSTATUS, 0, 0, None)

    def get_state(self):
        state_packed = self._device.ctrl_transfer(0xA1, Request.GETSTATE, 0, 0, 1)
        return State.map[struct.unpack('<B', state_packed)[0]]

    def abort(self):
        self._device.ctrl_transfer(0x21, Request.ABORT, 0, 0, None)

    def enter_dfu_mode(self):
        action_map = {
            State.dfuDNLOAD_SYNC: self.abort,
            State.dfuDNLOAD_IDLE: self.abort,
            State.dfuMANIFEST_SYNC: self.abort,
            State.dfuUPLOAD_IDLE: self.abort,
            State.dfuERROR: self.clear_status,
            State.appIDLE: self.detach,
            State.appDETACH: self._wait,
            State.dfuDNBUSY: self._wait,
            State.dfuMANIFEST: self.abort,
            State.dfuMANIFEST_WAIT_RESET: self._wait,
            State.dfuIDLE: self._wait
        }
        
        while True:
            state = self.get_state()
            if state == State.dfuIDLE:
                break
            action = action_map[state]
            action()

    def _wait(self):
        time.sleep(0.1)

def download(dfu, data, flash_address):
    block_size = 1 << 8
    sector_size = 1 << 12
    if flash_address & (sector_size - 1) != 0:
        raise Exception('Download must start at flash sector boundary')

    block_number = flash_address / block_size
    assert block_number * block_size == flash_address

    try:
        while len(data) > 0:
            packet, data = data[:block_size], data[block_size:]
            if len(packet) < block_size:
                packet += '\xFF' * (block_size - len(packet))
            dfu.download(block_number, packet)
            status, timeout, state, discarded = dfu.get_status()
            sys.stdout.write('.')
            sys.stdout.flush()
            block_number += 1
    finally:
        print

def upload(dfu, flash_address, length, path):
    block_size = 1 << 8
    if flash_address & (block_size - 1) != 0:
        raise Exception('Upload must start at block boundary')

    block_number = flash_address / block_size
    assert block_number * block_size == flash_address

    f = open(path, 'wb')
   
    try:
        while length > 0:
            data = dfu.upload(block_number, block_size)
            status, timeout, state, discarded = dfu.get_status()
            sys.stdout.write('.')
            sys.stdout.flush()
            if len(data) == block_size:
                f.write(data)
            else:
                raise Exception('Upload failed to read full block')
            block_number += 1
            length -= len(data)
    finally:
        f.close()
        print

def detach(dfu):
    if dfu.get_state() == State.dfuIDLE:
        dfu.detach()
        print('Detached')
    else:
        print 'In unexpected state: %s' % dfu.get_state()

def init_dfu(idVendor, idProduct):
    import usb.core
    dev = usb.core.find(idVendor=idVendor, idProduct=idProduct)
    if dev is None:
        raise RuntimeError('Device not found')

    dfu = DFU(dev)
    dev.default_timeout = 3000

    try:
        dfu.enter_dfu_mode()
    except usb.core.USBError, e:
        if len(e.args) > 0 and e.args[0] == 'Pipe error':
            raise RuntimeError('Failed to enter DFU mode. Is bootloader running?')
        else:
            raise e

    return dfu


class DfuSuffix(object):
    '''
    A class for adding and checking DFU file suffixes
    '''
    
    def __init__(self):
        # From freemyipod.org
        self.crc_table = []
        for i in range(256):
            t = i;
            for j in range(8):
                if t & 1:
                    t = (t >> 1) ^ 0xedb88320
                else:
                    t = t >> 1
            self.crc_table.append(t)
    
    def crc32(self, data):
        crc = 0xffffffff
        for byte in data:
            crc = (crc >> 8) ^ self.crc_table[(crc ^ ord(byte)) & 0xff]
        return crc
    
    def check_suffix(self, firmware):
        '''
        Check the dfu suffix
        @param firmware - binary of firmware file, including suffix
        '''
        print('Checking firmware signature')
    
        data   = firmware[:-4]
        length = ord(firmware[-5])
        suffix = firmware[-length:]
    
        # Will always have these fields
        dwCRC     = struct.unpack('<L', suffix[12:])[0]
        bLength   = struct.unpack('<B', suffix[11])[0]
        ucDfuSig  = struct.unpack('<3s', suffix[8:11])[0]
        bcdDFU    = struct.unpack('<H', suffix[6:8])[0]
        #bcdDFU, ucDfuSig, bLength, dwCRC = unpack('<H3sBL', suffix[6:])
    
        if bLength != 16:
            raise Exception("Unknown DFU suffix length: %s" % type(bLength))
    
        # We only know about dfu version 1.0/1.1
        # This needs to be smarter to support other versions if/when they exist
        if bcdDFU != 0x0100:
            raise Exception("Unknown DFU version: %d" % bcdDFU)
    
        # Suffix bytes are reversed
        if ucDfuSig != 'UFD':
            raise Exception("DFU Signature mismatch: not a DFU file")
    
        crc = self.crc32(data)
        if crc != dwCRC:
            raise Exception("CRC mismatch: calculated: 0x%x, found: 0x%x" % (crc, dwCRC))
    
        # Extract additional fields now that we know the suffix contains them
        idVendor  = struct.unpack('<H', suffix[4:6])[0]
        idProduct = struct.unpack('<H', suffix[2:4])[0]
    
        # Version information that we can't verify
        bcdDevice = struct.unpack('<H', suffix[0:2])[0]
    
        return length, idVendor, idProduct
    
    def add_suffix(self, firmware, idVendor, idProduct):
        '''
        Add a dfu suffix to a binary firmware file
        @param firmware  - binary of firmware file
        @param idVendor  - VendorId of target device
        @param idProduct - ProductId of target device
        '''
        bcdDevice = 0
        bcdDFU    = 0x0100
        ucDfuSig  = 'UFD'
        bLength   = 16
    
        suffix = struct.pack('<4H3sB', bcdDevice, idProduct, idVendor, bcdDFU,
                             ucDfuSig, bLength)
        firmware += suffix
    
        crc = self.crc32(firmware)
        firmware += struct.pack('<I', crc)
    
        return firmware