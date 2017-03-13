# UBERTOOTH 7 "March 2017" "Project Ubertooth" "Project Ubertooth Overview"

## NAME

ubertooth(7) - Project Ubertooth

## OVERVIEW

Ubertooth is a platform for Bluetooth experimentation. It is able to
sniff Bluetooth Smart (BLE), discover undiscoverable classic Bluetooth
devices, and perform rudimentary sniffing of classic Bluetooth devices.

Ubertooth is **not** a Bluetooth dongle and it will not show up as one
when running `hciconfig`.

## COMMAND LINE TOOLS

The following command line tools included in this package are the
primary means of using Ubertooth. Refer to their manual pages for
example usage and comprehensive documentation.

Useful commands:

 - ubertooth-btle(1) : BLE sniffing and other fun
 - ubertooth-rx(1) : Classic Bluetooth device discovery and rudimentary sniffing
 - ubertooth-scan(1) : Active scanning of undiscoverable devices
 - ubertooth-afh(1) : Detecting the AFH map of a Classic Bluetooth piconet
 - ubertooth-ego(1) : Yuneec E-Go electric skateboard sniffing

Utility commands:

 - ubertooth-dfu(1) : Firmware update tool
 - ubertooth-dump(1) : Dumping raw RF symbols to disk
 - ubertooth-util(1) : "Everything else"

Less useful commands:

 - ubertooth-debug(1) : Peeking and poking registers on the CC2400
 - ubertooth-specan(1) : Raw RSSI values used by graphical specan

## SUPPORT

Ubertooth is an open source project maintained primarily by volunteers.
Please keep that in mind when seeking support.

If you are having issues with a tool or believe you have found a bug,
please file an issue on the project GitHub:
<https://github.com/greatscottgadgets/ubertooth>

For interactive support, join the `#ubertooth` IRC channel on Freenode.

## COPYRIGHT

The code and documentation for Project Ubertooth are Copyright 2010-2017
and are available under the terms of the GPLv2. Refer to `COPYING` for
details. Ubertooth is a registered trademark of Great Scott Gadgets.

## AUTHOR

This manual page was written by Mike Ryan.
