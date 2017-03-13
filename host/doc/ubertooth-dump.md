# UBERTOOTH-DUMP 1 "March 2017" "Project Ubertooth" "User Commands"

## NAME

ubertooth-dump(1) - output a continuous stream of received bits

## SYNOPSIS

    ubertooth-dump [-b] [-c | -l] [-d <filename.bin>]

## DESCRIPTION

This tool dumps the raw bitstream received from the CC2400 radio to
STDOUT or a file (if specified with `-d`). It can configure the radio to
demodulate Classic Bluetooth with `-c` (default), or Bluetooth Low
Energy (BLE) with `-l`. Optionally it can decode the binary data and
print ASCII 0 and 1 using `-b`.'

## OPTIONS

 - `-b` :
   Print ASCII 0 and 1 instead of raw data
 - `-c` :
   Classic Bluetooth modulation (default)
 - `-l` :
   Bluetooth Low Energy (BLE) modulation
 - `-d <filename.bin>` :
   Dump to file instead of stdout
 - `-U <0-7>` :
   which Ubertooth device to use

## SEE ALSO

ubertooth(7): overview of Project Ubertooth

## AUTHOR

This manual page was written by Mike Ryan.

## COPYRIGHT

ubertooth-dump(1) is Copyright (c) 2010-2017. This tool is released under the
GPLv2. Refer to `COPYING` for further details.
