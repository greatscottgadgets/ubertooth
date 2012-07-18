#!/usr/bin/env python
#
# Copyright 2011 Jared Boone
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

import usb.core
import struct
import sys
import time
from array import array
import numpy

class Ubertooth(object):
    STATE_IDLE   = 0
    STATE_ACTIVE = 1
    
    def __init__(self, device):
        self._device = device
        self._device.default_timeout = 3000
        self._device.set_configuration()
        self._state = self.STATE_IDLE

    def _cmd_specan(self, low_frequency, high_frequency):
        low_frequency = int(round(low_frequency / 1e6))
        high_frequency = int(round(high_frequency / 1e6))
        self._device.ctrl_transfer(0x40, 27, low_frequency, high_frequency)
        self._state = self.STATE_ACTIVE

    def specan(self, low_frequency, high_frequency):
        spacing_hz = 1e6
        bin_count = int(round((high_frequency - low_frequency) / spacing_hz)) + 1
        frequency_axis = numpy.linspace(low_frequency, high_frequency, num=bin_count, endpoint=True)
        frequency_index_map = dict(((int(round(frequency_axis[index] / 1e6)), index) for index in range(len(frequency_axis))))
        
        self._cmd_specan(low_frequency, high_frequency)
        
        default_raw_rssi = -128
        rssi_offset = -54
        rssi_values = numpy.empty((bin_count,), dtype=numpy.float32)
        rssi_values.fill(default_raw_rssi + rssi_offset)
        
        data = array('B')
        last_index = None
        while True:
            buffer = self._device.read(0x82, 64)
            data += buffer
            while len(data) >= 64:
                header, block, data = data[:14], data[14:64], data[64:]
                pkt_type, status, channel, clkn_high, clk100ns, reserved = struct.unpack('<BBBBI6s', header)
                clk = (clkn_high << 32) | clk100ns
                clk_seconds = float(clk) * 100e-9
                while len(block) >= 3:
                    item, block = block[:3], block[3:]
                    frequency, raw_rssi_value = struct.unpack('>Hb', item)
                    index = frequency_index_map[frequency]
                    if index == 0:
                        # We started a new frame, send the existing frame
                        yield (frequency_axis, rssi_values)
                        rssi_values.fill(default_raw_rssi + rssi_offset)
                    rssi_values[index] = raw_rssi_value + rssi_offset
                    
    def close(self):
        if self._state != self.STATE_IDLE:
            self._device.ctrl_transfer(0x40, 21)
            self._state = self.STATE_IDLE
    
if __name__ == '__main__':
    ids = [
        (0x1D50, 0x6002), # UBERTOOTH_ONE
        (0x1D50, 0x6000), # UBERTOOTH_ZERO
        (0xFFFF, 0x0004)  # TC13BADGE / Ubertooth with older firmware
    ]
    for vendor, product in ids:
        device = usb.core.find(idVendor=vendor, idProduct=product)
        if device:
            break
    else:
        raise Exception('Device not found')

    ubertooth = Ubertooth(device)
    frame_source = ubertooth.specan(2.402e9, 2.480e9)

    try:
        for frame in frame_source:
            print(frame)
    except KeyboardInterrupt:
        pass
    finally:
        ubertooth.close()
