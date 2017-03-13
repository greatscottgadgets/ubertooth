# ubertooth-debug 1 "March 2017" "Project Ubertooth" "User Commands"

## NAME

ubertooth-debug(1) - Classic Bluetooth discovery, sniffing, and decoding

## SYNOPSIS

    ubertooth-debug -r <number>[,<number>[,...]]

## DESCRIPTION

ubertooth-debug(1) is a tool for reading register values from the CC2400 chip
on Ubertooth One. It is primarily designed to assist firmware development but
may be of interest to curious hackers.

## OPTIONS

Options:

 - `-r <reg>[,<reg> [,[...]]]` :
   Read one or more registers from the CC2400 on Ubertooth
 - `-r <start>-<end>` :
   Read a consecutive set of registers from the CC2400 on Ubertooth
 - `-v <0-2>` :
   Sets the level of verbosity, default is 1.
 - `-U<0-7>` :
   Which Ubertooth device to use

## EXAMPLES

To read the MANOR register, use:

    ubertooth-debug -r 19

Registers can also be specified by name, the following will
also read the MANOR register:

    ubertooth-debug -r %manor

Read a range of registers:

    ubertooth-debug -r 19-22

Dump all registers with:

    ubertooth-debug -r 0x00-0x70

## SEE ALSO

ubertooth(7): overview of Project Ubertooth

## AUTHOR

This manual page was written by Dominic Spill.

## COPYRIGHT

ubertooth-debug(1) is Copyright (c) 2010-2017. This tool is released under the
GPLv2. Refer to `COPYING` for further details.
