# UBERTOOTH-EGO 1 "March 2017" "Project Ubertooth" "User Commands"

## NAME

ubertooth-ego(1) - Yuneec E-Go sniffing

## SYNOPSIS

    ubertooth-ego -f

## DESCRIPTION

The Yuneec E-Go is an electric skateboard controlled by a wireless
handheld remote. This handheld remote uses a custom protocol implemented
in 2.4 GHz RF. ubertooth-ego(1) can sniff this protocol and interfere
with its operation.

## EXAMPLES

Sniff the connection between an E-Go and its remote:

    ubertooth-ego -f

Receive all E-Go packets on 2408 MHz:

    ubertooth-ego -r -c 2408

Interfere with a connection:

    ubertooth-ego -i

## OPTIONS

Major modes:

 - `-f` :
   Sniff a connection by hopping along channels
 - `-r` :
   Continuously receive packets on a single channel (see `-c`)
 - `-i` :
   Interfere with a connection

Options:

 - `-c` :
   Channel to use for continuous receive (suggested: 2408, 2418, 2423, 2469)
 - `-U<0-7>` :
   Which Ubertooth device to use

## SEE ALSO

ubertooth(7): overview of Project Ubertooth

## AUTHOR

This manual page was written by Mike Ryan.

## COPYRIGHT

ubertooth-ego(1) is Copyright (c) 2015 Mike Ryan. This tool is released
under the GPLv2. Refer to `COPYING` for further details.
