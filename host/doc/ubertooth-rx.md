# UBERTOOTH-RX 1 "March 2017" "Project Ubertooth" "User Commands"

## NAME

ubertooth-rx(1) - Classic Bluetooth discovery, sniffing, and decoding

## SYNOPSIS

    ubertooth-rx [ -l <lap> [ -u <uap ] ]
    ubertooth-rx -z

## DESCRIPTION

ubertooth-rx(1) is the primary interface into Classic Bluetooth (BR)
functionality provided by Ubertooth. It has two main modes of operation:
piconet following and survey mode. In either mode, ubertooth-rx(1) is
able to discover undiscoverable devices. See [DISCOVERING UNDISCOVERABLE
DEVICES][].

In piconet following mode, the tool will follow the first piconet it
fully identifies. In survey mode the device will attempt to identify all
piconets in a given area and display them after either a timeout or
manual interruption.

Piconet following is the main mode entered when no arguments are passed
to the command or a LAP and optionally a UAP are provided. If no
arguments are passed, the tool will attempt to calculate the UAP for any
observed LAPs. If a LAP is passed, the UAP will be calculated for that
specific LAP. Once a LAP and UAP have been recovered, the tool will
attempt to recover the clock value, and if that succeeds it will follow
that piconet.

Survey mode, entered using `-z`, will record all LAPs and attempt to
calculate the UAPs for any observed LAPs. This mode can be combined with
a timeout using `-t`, and it can be interrupted at any time using
`ctrl-C`.

## EXAMPLES

Follow the first piconet whose LAP, UAP, and clock are recovered from
the air:

    ubertooth-rx

For a given LAP, calculate the UAP and recover the clock, then follow:

    ubertooth-rx -l 112233

For a given LAP and UAP, recover the clock then follow:

    ubertooth-rx -l 112233 -u ab

Enter survey mode for 20 seconds, and print out the BD ADDRs of all
observed piconets:

    ubertooth-rx -z -t 20

## OPTIONS

Major modes:

 - `-l <lap>` :
   Limit UAP recovery, clock recovery, and piconet following to a given
   LAP. Format is 3 bytes / 6 hex characters.

 - `-u <uap>` :
   Limit clock recovery and piconet following to a given UAP. Must be
   used in conjunction with `-l`. Format is 1 byte / 2 hex characters.

 - `-z` :
   Survey mode: recover all LAP and UAP pairs and display them. Will run
   indefinitely until interrupted with `ctrl-C` unless paired with `-t`.

Options:

 - `-i <input>` :
   Input file. If not specified will perform live capture using
   Ubertooth.

 - `-c <0-79>` :
   Fixed channel for all major modes. If not specified will sweep
   through all channels.

 - `-e <0-4>` :
   Maximum access code bit errors. [Default: 2]

 - `-t <seconds>` :
   Timeout in seconds. If not specified will run indefinitely. Suggested
   values for `-z`: 20-60 seconds.

Output options:

 - `-r <file.pcapng>` :
   Capture packets to PcapNG
 - `-q <file.pcap>` :
   Capture packets to PCAP
 - `-d <file.bin>` :
   Capture packets to binary file suitable for use with `-i`.

Miscellaneous:

 - `-V` :
   Version information

 - `-U<0-7>` :
   Which Ubertooth device to use

## DISCOVERING UNDISCOVERABLE DEVICES

Classic Bluetooth piocnets are defined by the Lower Address Part (LAP)
and Upper Address Part (UAP) of the master device. These are elements
of the master device's Bluetooth Address (BD ADDR).

Consider the following BD ADDR:

    22:44:66:88:AA:BB

The lower address part (LAP) is the lower 24 bits, so `88:AA:BB`. In the
context of this tool, the value is written `88AABB`. The upper address
part is the next 8 bits, so `66`. The `22:44` is called the
Non-significant Address Part (NAP) and as you might imagine it is not
significant.

In piconet following mode, the tool will recover LAP values from the air
and attempt to calculate the UAP from those. It will go on to follow the
piconet if it can recover the clock value. In survey mode, the tool will
simply recover LAP and UAP values.

To convert LAP + UAP pairs back into Bluetooth addresses, do the reverse
of the above. For example, if the tool recovers a LAP of 36A2B4 and a
UAP of 98, the associated Bluetooth address is `??:??:98:36:A2:B4`. Any
value can be substituted into the ?? slots and most Bluetooth tools will
still work. For example, `hcitool name 00:00:98:36:A2:B4` will establish
a connection to the device and return its name.

This attack works against discoverable and undiscoverable devices alike.

## SEE ALSO

ubertooth-scan(1): active device scanning and inquiry using Ubertooth
and BlueZ

ubertooth(7): overview of Project Ubertooth

D. Spill and A. Bittau. "BlueSniff: Eve Meets Alice and Bluetooth."
USENIX WOOT 2007.

## AUTHOR

This manual page was written by Mike Ryan.

## COPYRIGHT

ubertooth-rx(1) is Copyright (c) 2010-2017 Michael Ossmann, Dominic
Spill, and others. This tool is released under the GPLv2. Refer to
`COPYING` for further details.
