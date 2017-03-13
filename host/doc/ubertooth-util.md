# UBERTOOTH-UTIL 1 "March 2017" "Project Ubertooth" "User Commands"

## NAME

ubertooth-util(1) - general purpose Ubertooth utility

## SYNOPSIS

ubertooth-util [-abcdefghijk-ellemenop-qrstuvwxyZ]

## DESCRIPTION

ubertooth-util(1) is a catch-all tool for doing generally useful things
with, to, and about Ubertooth. To get firmware revision and compile
information, run:

    ubertooth-util -vV

To fully reset Ubertooth, run:

    ubertooth-util -r

Other options are available for modifying LED states, setting channel,
and performing miscellaneous radio-related actions. Refer to the
[OPTIONS][] section for full details.

The utility also includes a simple range test tool for use with two
Uberteeth. Refer to [RANGE TEST][] for full details.

## OPTIONS

Common options:

 - `-v` :
   get firmware revision number
 - `-V` :
   get compile info
 - `-I` :
   identify ubertooth device by flashing all LEDs
 - `-d[0-1]` :
   get/set all LEDs
 - `-l[0-1]` :
   get/set USR LED
 - `-S` :
   stop current operation
 - `-r` :
   full reset
 - `-U<0-7>` :
   set ubertooth device to use

Radio options:

 - `-a[0-7]` :
   get/set power amplifier level
 - `-c[2400-2483]` :
   get/set channel in MHz
 - `-C[0-78]` :
   get/set channel
 - `-q[1-225 (RSSI threshold)]` :
   start LED spectrum analyzer
 - `-t` :
   intitiate continuous transmit test
 - `-z` :
   set squelch level

Range test:

 - `-e` :
   start repeater mode
 - `-m` :
   display range test result
 - `-n` :
   initiate range test

Miscellaneous:

 - `-f` :
   activate flash programming (DFU) mode
 - `-i` :
   activate In-System Programming (ISP) mode
 - `-b` :
   get hardware board id number
 - `-p` :
   get microcontroller Part ID
 - `-s` :
   get microcontroller serial number

## RANGE TEST

Using two Uberteeth it is possible to perform a range test. One
Ubertooth acts as a repeated by running:

    ubertooth-util -e

A second Ubertooth (the sender) initiates the range test by running:

    ubertooth-util -n

During the range test the sender Ubertooth will transmit data packets at
increasing power level, listening for repeats from the repeater
Ubertooth. The receiving Ubertooth will validate that the received data
was not corrupted in flight.

The range test results can be displayed on the sender Ubertooth's system
by running:

    ubertooth-util -m

Note that the range test transmits the Cortex M3 microcontroller's
serial number. If you do not wish to broadcast that information
wirelessly, do not use the range test functionality.

## SEE ALSO

ubertooth(7): overview of Project Ubertooth

## AUTHOR

This manual page was written by Mike Ryan.

## COPYRIGHT

ubertooth-util(1) is Copyright (c) 2010-2017. This tool is released under the
GPLv2. Refer to `COPYING` for further details.
