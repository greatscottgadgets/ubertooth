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

import struct
import numpy
import time
import subprocess

class Ubertooth(object):

    def __init__(self):
        self.proc = None

    def specan(self, low_frequency, high_frequency):
        spacing_hz = 1e6
        bin_count = int(round((high_frequency - low_frequency) / spacing_hz)) + 1
        frequency_axis = numpy.linspace(low_frequency, high_frequency, num=bin_count, endpoint=True)
        frequency_index_map = dict(((int(round(frequency_axis[index] / 1e6)), index) for index in range(len(frequency_axis))))

        low = int(round(low_frequency / 1e6))
        high = int(round(high_frequency / 1e6))
        args = ["ubertooth-specan", "-d", "-", "-l%d"%low, "-u%d"%high]
        self.proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        default_raw_rssi = -128
        rssi_offset = -54
        rssi_values = numpy.empty((bin_count,), dtype=numpy.float32)
        rssi_values.fill(default_raw_rssi + rssi_offset)

        data = ''
        last_index = None
        # Give it a chance to time out if it fails to find Ubertooth
        time.sleep(0.5)
        if self.proc.poll() is not None:
            print "Could not open Ubertooth device"
            print "Failed to run: ", ' '.join(args)
            return
        while self.proc.poll() is None:
            buf = self.proc.stdout.read(512)
            data += buf
            while len(data) >= 64:
                header, block, data = data[:14], data[14:64], data[64:]
                #pkt_type, status, channel, clkn_high, clk100ns, reserved = \
                #                                struct.unpack('<BBBBI6s', header)
                #clk = (clkn_high << 32) | clk100ns
                #clk_seconds = float(clk) * 100e-9
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
        if self.proc and not self.proc.poll():
            self.proc.terminate()
            if self.proc.poll() is not None:
                self.proc.kill()
        self.proc = None

if __name__ == '__main__':
    device = Ubertooth()
    frame_source = ubertooth.specan(2.402e9, 2.480e9)

    try:
        for frame in frame_source:
            print(frame)
    except KeyboardInterrupt:
        pass
    finally:
        ubertooth.close()
