#!/usr/bin/env python

# http://pyusb.sourceforge.net/docs/1.0/tutorial.html

import usb.core
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
            state = dfu.get_state()
            print state
            if state == State.dfuIDLE:
                break
            action = action_map[state]
            action()

    def _wait(self):
        time.sleep(0.1)

def download(dfu, path, flash_address):
    block_size = 1 << 8
    sector_size = 1 << 12
    if flash_address & (sector_size - 1) != 0:
        raise Exception('Download must start at flash sector boundary')

    block_number = flash_address / block_size
    assert block_number * block_size == flash_address

    f = open(path, 'rb')
    data = f.read()

    while len(data) > 0:
        packet, data = data[:block_size], data[block_size:]
        if len(packet) < block_size:
            packet += '\xFF' * (block_size - len(packet))
        dfu.download(block_number, packet)
        status, timeout, state, discarded = dfu.get_status()
        #print '%08x' % (block_number * block_size), status, state
        block_number += 1

    f.close()

def upload(dfu, flash_address, length, path):
    block_size = 1 << 8
    if flash_address & (block_size - 1) != 0:
        raise Exception('Upload must start at block boundary')

    block_number = flash_address / block_size
    assert block_number * block_size == flash_address

    f = open(path, 'wb')
    
    while length > 0:
        data = dfu.upload(block_number, block_size)
        status, timeout, state, discarded = dfu.get_status()
        print '%08x' % (block_number * block_size), status, state
        if len(data) == block_size:
            f.write(data)
        else:
            raise Exception('Upload failed to read full block')
        block_number += 1
        length -= len(data)

    f.close()

dev = usb.core.find(idVendor=0xFFFF, idProduct=0x0004)
if dev is None:
    raise Exception('Device not found')
dfu = DFU(dev)

dfu.enter_dfu_mode()

bootloader_offset = 0x0
bootloader_size = 0x4000
application_offset = bootloader_offset + bootloader_size
application_size = (512 * 1024) - application_offset

try:
    if sys.argv[1] == 'read':
        upload(dfu, application_offset, application_size, sys.argv[2])
    elif sys.argv[1] == 'write':
        download(dfu, sys.argv[2], application_offset)
except Exception, e:
    print e
    print dfu.get_status()

