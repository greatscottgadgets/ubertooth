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

class Ubertooth(object):
    def __init__(self, device):
        self._device = device
        self._device.default_timeout = 3000
        self._device.set_configuration()

    def _cmd_specan(self, low_frequency, high_frequency):
        self._device.ctrl_transfer(0x40, 27, low_frequency, high_frequency)

    def specan(self, low_frequency, high_frequency):
        low_frequency = int(round(low_frequency / 1e6))
        high_frequency = int(round(high_frequency / 1e6))
        self._cmd_specan(low_frequency, high_frequency)

        frame = {}
        data = array('B')
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
                    frequency, rssi = struct.unpack('>Hb', item)
                    if frequency in frame:
                        frame_units = dict(((frequency * 1e6, rssi - 54) for frequency, rssi in frame.items()))
                        yield frame_units
                        frame = {}
                    frame[frequency] = rssi

if __name__ == '__main__':
    device = usb.core.find(idVendor=0xFFFF, idProduct=0x0004)
    if device is None:
        raise Exception('Device not found')

    ubertooth = Ubertooth(device)
    frame_source = ubertooth.specan(2402, 2480)

    for frame in frame_source:
        print(frame)
