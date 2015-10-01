#!/usr/bin/env python

# Copyright 2013 Mike Ryan
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

import getopt
import re
import sys
from subprocess import Popen, PIPE


def main():
    try:
        opts, args = getopt.getopt(
            sys.argv[1:], "h",
            [
                "help",
                "list-interfaces",
                "list-dlts",
                "config",
                "capture",
                "interface=",
                "fifo=",
                "channel=",
            ])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(str(err))  # will print something like "option -a not recognized"
        usage()
        sys.exit(2)

    interface = ''
    fifo = None
    channel = "37"
    do_list_dlts = False
    do_config = False
    do_capture = False

    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o == "--list-interfaces":
            list_interfaces()
            exit(0)
        elif o == "--list-dlts":
            do_list_dlts = True
        elif o == "--config":
            do_config = True
        elif o == "--capture":
            do_capture = True
        elif o == "--interface":
            interface = a
        elif o == "--fifo":
            fifo = a
        elif o == "--channel":
            channel = a
        else:
            assert False, "unhandled option"

    # every operation down here depends on having an interface
    m = re.match('ubertooth(\d+)', interface)
    if not m:
        exit(1)
    interface = m.group(1)

    if do_list_dlts:
        list_dlts()
    elif do_config:
        config()
    elif do_capture:
        if fifo is None:
            print("Must specify fifo!")
            exit(1)
        capture(interface, fifo, channel)


def usage():
    print("Usage: %s <--list-interfaces | --list-dlts | --config | --capture>" % sys.argv[0])


def list_interfaces():
    proc = Popen(['ubertooth-util', '-s'], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    out = proc.communicate()[0]
    lines = out.split('\n')

    interfaces = []

    for line in lines:
        p = line.split()
        if len(p) == 0 or p[0] == 'ubertooth-util':
            break

        if p[0] == 'Serial':
            interfaces.append(p[2])
        elif re.match('[0-9a-f]+', p[0]):
            interfaces.append(p[0])

    for i in range(len(interfaces)):
        print("interface {value=ubertooth%d}{display=Ubertooth One %s}" % (i, interfaces[i]))


def list_dlts():
    print("dlt {number=147}{name=USER0}{display=Bluetooth Low Energy}")


def config():
    args = []
    args.append((0, '--channel', 'Advertising Channel', 'selector'))

    values = []
    values.append((0, "37", "37", "true"))
    values.append((0, "38", "38", "false"))
    values.append((0, "39", "39", "false"))

    for arg in args:
        print("arg {number=%d}{call=%s}{display=%s}{type=%s}" % arg)

    for value in values:
        print("value {arg=%d}{value=%s}{display=%s}{default=%s}" % value)


def capture(interface, fifo, channel):
    p = Popen([
        "ubertooth-btle", "-f",
        "-U%s" % interface,
        "-c", fifo,
        "-A", channel,
    ])
    p.wait()

if __name__ == "__main__":
    main()
