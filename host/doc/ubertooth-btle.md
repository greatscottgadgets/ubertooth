# UBERTOOTH-BTLE 1 "July 2018" "Project Ubertooth" "User Commands"

## NAME

ubertooth-btle -- Bluetooth Low Energy (BLE) sniffing and more

## SYNOPSIS

    ubertooth-btle -f [-A 37|38|39] [-r output.pcapng]

## DESCRIPTION

`ubertooth-btle` is a tool for doing Fun Stuff(TM) with BLE. It can do
the following things:

 - Sniff connections
 - Interfere with connections
 - Send advertising packets (experimental)

Sniffing connections is the most robust feature supported by
`ubertooth-btle`. It has two primary modes of operation: follow mode and
promiscuous mode.

Follow mode is the preferred mode for general use. In this mode,
Ubertooth will listen on one of three advertising channels waiting for a
BLE connection to be established. When a connection is established,
Ubertooth will hop along the data channels, passively capturing the data
sent between the central and peripheral. After the connection
terminates, Ubertooth will return to the advertising channel and wait
for another connection.

No-follow mode is similar to follow mode, but it only logs advertising
packets and will not follow connections as they are established.

Promiscuous mode is an experimental mode for sniffing connections after
they have already been established. This mode can be used to sniff
long-lived connections.

When sniffing, Ubertooth can only operate in either follow mode or
promiscuous mode, but not both at the same time. If you are unsure which
mode to use, use follow mode.

By default, Ubertooth will follow any connection it observes. You can
limit this to following a specific Bluetooth Address (BD ADDR) using the
`-t` command line flag. For example, the following command will only
sniff connections where the central or peripheral's BD ADDR is
`22:44:66:88:AA:CC`:

    ubertooth-btle -f -t22:44:66:88:AA:CC

`-t` can also take a mask length in CIDR-like notation. Masks can be
between 1 and 48 bits long, with a 48 bit mask matching the entire
address. Using a /24 mask will filter on just the OUI. For example, to
limit sniffing to just TI devices with the OUI 00:1A:7D, use the
following:

    ubertooth-btle -t 00:1A:7D:00:00:00/24

Filters persist until they are explicitly cleared or the system restarts
(either via `ubertooth-util -r` or unplug/replug). To clear a filter,
use the special filter `none`. Example:

    ubertooth-btle -t none

When filtering, previous versions of the firmware would still log all
advertising packets but only follow connections based on the filter
parameters. As of 2018-06-R1, advertising packets that do not match the
filter are dropped.

In all sniffing modes, Ubertooth can log data to PCAP or PcapNG with a
variety of pseudoheaders. The recommended logging format is PcapNG
(`-r`) or PCAP with LE Pseudoheader (`-q`). For compatibility with
crackle (see [USING WITH CRACKLE][]), use PCAP with PPI (`-c`).

Interfering with connections is a feature for causing intentional
interference with newly established or long-lived connections. When this
attack succeeds, the BLE connection between the central and peripheral
will be terminated. Pair the `-i` or `-I` flag with `-f` to interfere
with new connections or `-p` to interfere with long-lived connections.
Note that causing intentional interference may be illegal in your
jurisdiction. Check your local laws before using this feature.

Finally, `ubertooth-btle` supports transmitting advertising packets with
a specified BD ADDR. This feature, referred to as faux slave mode, is
experimental and may not function as intended. Use at your own risk.

## EXAMPLES

Sniff all connections on advertising channel 38, logging all data to
PcapNG:

    ubertooth-btle -f -A 38 -r log.pcapng

Log advertising packets without following connections:

    ubertooth-btle -n

Interfere with connections recovered with promiscuous mode:

    ubertooth-btle -p -I

Send advertising packets using BD ADDR `22:44:66:88:AA:CC`:

    ubertooth-btle -s22:44:66:88:AA:CC

## OPTIONS

 - `-h` :
   Displays help message

Major modes:

 - `-f` :
   Follow mode: sniff connections as they are established
 - `-n` :
   No-follow mode: log advertising packets but don't follow connections
 - `-p` :
   Promiscuous mode: sniff already-established connections
 - `-s<BD ADDR>` : 
   Inject advertising packets using specified BD ADDR

Interference (pair with `-f` or `-p`):

 - `-i` :
   Interfere with one connection and return to idle
 - `-I` :
   Interfere continuously with many connections

Filtering:

 - `-t<BD ADDR>` :
   Limit connection following and interference in follow mode to the
   specified BD ADDR

Logging:

 - `-r <output.pcapng>` :
   Log to PcapNG (preferred)
 - `-q <output.pcap>` :
   Log to PCAP with `DLT_BLUETOOTH_LE_LL_WITH_PHDR`
 - `-c <output.pcap>` :
   Log to PCAP with PPI (for compatibility with crackle(1))

Miscellaneous:

 - `-A <37|38|39>` :
   Which advertising channel to use in follow mode (default: 37)
 - `-a[address]` :
   Get or set access address in promiscuous mode
 - `-v[01]` :
   Get or set CRC verification (default: 0)
 - `-x<0-32>` :
   Allow n access address violations (default: 32). Filtering occurs on
   host.

Data source:

 - `-U<0-7>` :
   Which Ubertooth to use

## USING WITH CRACKLE

`crackle` is a tool for cracking the BLE key exchange and decrypting
encrypted data. To capture data for use with `crackle`, sniff
connections in follow mode using `-f` and log data to PCAP/PPI using
`-c`. Example:

    ubertooth-btle -f -c crack.pcap

Refer to `crackle` documentation for further details.

## SEE ALSO

crackle(1): https://github.com/mikeryan/crackle

## COPYRIGHT

`ubertooth-btle` is Copyright (C) 2012-2018 Mike Ryan. This tool is
released under the GPLv2. Refer to COPYING for futher details.
