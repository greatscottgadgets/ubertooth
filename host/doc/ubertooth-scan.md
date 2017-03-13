# UBERTOOTH-SCAN 1 "March 2017" "Project Ubertooth" "User Commands"

## NAME

ubertooth-scan(1) - active(Bluez) device scan and inquiry supported by Ubertooth

## SYNOPSIS

ubertooth-scan [-s] [-x]

## DESCRIPTION

This tool uses a normal Bluetooth dongle to perform Inquiry Scans and
Extended Inquiry scans of Bluetooth devices. It uses Ubertooth to
discover undiscoverable devices and can use BlueZ to scan for
discoverable devices.


## EXAMPLES

Use Ubertooth to discover devices and perform Inquiry Scan with BlueZ:

    ubertooth-scan

Use BlueZ and Ubertooth to discover devices and perform Inquiry Scan and
Extended Inquiry Scan:

    ubertooth-scan -s -x

## OPTIONS

 - `-s` :
   hci Scan - use BlueZ to scan for discoverable devices
 - `-x` :
   eXtended scan - retrieve additional information about target devices
 - `-t <seconds>` :
   scan Time - length of time to sniff packets. [Default: 20s]
 - `-e <0-4>` : 
    Maximum Access Code Errors (default: 2)
 - `-b hciN` :
    Bluetooth device (default: hci0)
 - `-U<0-7>` :
    which Ubertooth device to use

## SEE ALSO

ubertooth-rx(1): Discover devices passively with Ubertooth

ubertooth(7): overview of Project Ubertooth

## AUTHOR

This manual page was written by Mike Ryan.

## COPYRIGHT

ubertooth-scan(1) is Copyright (c) 2010-2017. This tool is released under the
GPLv2. Refer to `COPYING` for further details.
