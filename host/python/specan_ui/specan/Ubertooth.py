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
        frame_size = len(frequency_axis)
        buffer_size = frame_size * 3
        frequency_index_map = dict(((int(round(frequency_axis[index] / 1e6)), index) for index in range(frame_size)))

        low = int(round(low_frequency / 1e6))
        high = int(round(high_frequency / 1e6))
        args = ["ubertooth-specan", "-d", "-", "-l%d" % low, "-u%d" % high]
        self.proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        default_raw_rssi = -128
        rssi_offset = -54
        rssi_values = numpy.empty((bin_count,), dtype=numpy.float32)
        rssi_values.fill(default_raw_rssi + rssi_offset)

        # Give it a chance to time out if it fails to find Ubertooth
        time.sleep(0.5)
        if self.proc.poll() is not None:
            print("Could not open Ubertooth device")
            print("Failed to run: ", ' '.join(args))
            return
        while self.proc.poll() is None:
            data = self.proc.stdout.read(buffer_size)
            while len(data) >= 3:
                frequency, raw_rssi_value = struct.unpack('>Hb', data[:3])
                data = data[3:]
                if frequency >= low and frequency <= high:
                    index = frequency_index_map[frequency]
                    if index == 0:
                        # new frame, pause as a frame limiter!
                        time.sleep(0.013)  # I regret nothing

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
    ubertooth = Ubertooth()
    frame_source = ubertooth.specan(2.402e9, 2.480e9)

    try:
        for frame in frame_source:
            print(frame)
    except KeyboardInterrupt:
        pass
    finally:
        ubertooth.close()
