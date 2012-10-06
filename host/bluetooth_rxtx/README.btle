ubertooth-btle
==============

Bluetooth Low Energy mode for Ubertooth

Note that this software is EXTREMELY EXPERIMENTAL. It has worked for me
under controlled conditions while I developed it. Anyone who wants to do
anything serious with this is totally crazy (in a good way).


PREREQUISITES
=============

 - an Ubertooth
 - a device which you want to sniff that is totally yours

Always use the latest bluetooth_rxtx firmware and host code from git.
Instructions for building the firmware are in the README in the firmware
directory.

The user should have some familiarity with the technical details of
Bluetooth Low Energy. Please refer to the Bluetooth Core Spec 4.0 for
more information.


USAGE
=====

ubertooth-btle supports two major modes: connection following and
promiscuous mode. At present time, the two modes are very similar.

In both modes, raw packets are hex dumped to the terminal and (if
specified) a dump file.

The only way to exit either of these modes is to reset the device:

    ubertooth-util -r

Connection Following
--------------------

The Ubertooth will sit on one of the advertising channels and wait for a
connection packet. When it sees a connection packet (type 0x05), it will
begin channel hopping and follow that connection.

To enter the mode, run:

    ubertooth-btle -f

Set your LE device to discoverable mode. You should see advertising
packets that look something like this:

    systime=1349412883 freq=2402 addr=8e89bed6 delta_t=38.441 ms
    00 17 ab cd ef 01 22 00 02 01 06 03 02 0d 18 06 ff 6b 00 03 00 00 02
    0a 00 c2 87 64

The first byte (00) indicates that this is a connectable undirected
advertising event (core spec 4.0 page 2203).

At this point, attempt to connect to the device using your PC or phone.
If you're lucky, you'll see a packet that looks like this:

    systime=1349413162 freq=2402 addr=8e89bed6 delta_t=0.409 ms
    05 22 99 88 77 66 02 00 ab cd ef 01 22 00 12 8b 9a af a3 df 00 03 0e
    00 0f 00 00 00 80 0c ff ff ff ff 1f ab 80 ff 0f

Followed by data packets of this form:

    systime=1349413162 freq=2450 addr=af9a8b12 delta_t=39.675 ms
    0d 00 44 a5 22

Notice that the frequency has changed from 2402 MHz (the advertising
channel) to 2450 MHz (a data channel).

The packet beginning with 05 is a connection request event (core spec
4.0 page 2206). When the Ubertooth sees this, it immediately begins
following the connection.

The packet beginning with 0d is an empty data packet. It consists of a 2
byte header, 0 byte body, and 3 byte CRC.

Promiscuous Mode
----------------

Set your Ubertooth to your favorite data channel:

    ubertooth-util -c2404

It also helps to adjust the squelch, though you will have to play with
the value to find something that works:

    ubertooth-util -z-50

Then enter promiscuous mode:

    ubertooth-btle -p

Establish a connection with the LE device you wish to sniff. You should
begin to see data packets. Many of them will be garbage, but eventually
you'll see repeating access addresses.

    systime=1349414653 freq=2404 addr=bc1d023e delta_t=64.037 ms
    01 00 af dd 10

    systime=1349414653 freq=2404 addr=f3474c81 delta_t=121.582 ms
    01 00 25 98 8b

    systime=1349414653 freq=2404 addr=9bfc3bf3 delta_t=0.062 ms
    01 00 58 ec c3

    systime=1349414678 freq=2404 addr=506545d9 delta_t=1387.338 ms
    01 00 20 56 86

    systime=1349414680 freq=2404 addr=506545d9 delta_t=1387.783 ms
    01 00 20 56 86

    systime=1349414681 freq=2404 addr=506544ed delta_t=1387.436 ms
    01 00 20 56 86

    systime=1349414683 freq=2404 addr=506545d9 delta_t=1387.557 ms
    01 00 20 56 86

Once the access address has been located, the Ubertooth will
automatically begin following it. The CRC init will be recovered, and
CRC verification automatically enabled. At this point you should see
data packets with contents other than 01 00:

    systime=1349414683 freq=2404 addr=506545d9 delta_t=0.352 ms
    05 00 f3 50 86

    systime=1349414683 freq=2404 addr=506545d9 delta_t=693.326 ms
    0d 00 55 5d 86

The Ubertooth then recovers the hop interval and hop amount, finally
entering connection following mode (as though a connect were observed).
Packets should stream quickly at this point.

Miscellaneous Usage
-------------------

CRC verification is enabled by default. To change this setting, use the
-v flag (you should adjust the squelch if you disable it):

    ubertooth-btle -v0

The access address being used is normally auto-detected, either from the
connection packet or recovered in promiscuous mode. If you wish to
explicitly set the access address to follow, use the -a flag:

    ubertooth-btle -a01234567


THEORY OF OPERATION
===================

In order to follow a connection, we need to know four values:

 1. Access address
 2. CRC init
 3. Hop interval
 4. Hop amount

In connection following mode, these values are extracted from the
connection packet. In promiscuous mode, we recover them by exploiting
properties of LE packets.

Connection following mode begins by looking for empty data packets,
which have a predictable form. From there it extracts candidate access
addresses.

Once an access address has been seen five times, we assume it is valid
and represents an active connection. We sit on a data channel dumping
packets with this access address.

The CRC init is used to initialize an LFSR. The output of this LFSR is
XOR'd with the packet data and fed back into the front, a process which
is fully reversible. We extract the CRC init by considering data packets
and running the LFSR in reverse.

The hop interval is recovered by observing that the hop sequence
completes a full cycle once every 37 * 1.25 * hop_interval milliseconds.
We sit on a data channel and calculate the time between two consecutive
packets. We directly calculate the hop interval using this formula:

    hop_interval = delta_t / (37 * 1.25)

Finally the hop amount is recovered by measuring the interarrival time
of packets on two data channels (index 0 and 1). We wait for a packet on
channel index 0, then jump to channel index 1 and measure the time it
takes for a second packet to arrive.

From the interarrival time, we can calculate the number of channels
hopped between the first and second packet:

    channels_hopped = delta_t / (1.25 * hop_interval)

We use a lookup table to map this value to the hop amount.

At this point, we have all four values needed to follow a connection,
and we enter connection following mode as though we observed the initial
connect packet.


BUGS
====

The code always assumes all data channels are in use. I have never
observed otherwise, but it's within spec to use fewer.


AUTHOR
======

This code was mostly written by Mike Ryan <mikeryan@lacklustre.net> over
the course of many sleepless nights.

This could not have been done without the tremendous moral and technical
support of Dominic Spill and Mike Ossmann. Thanks also to Will Code,
Jared Boone, Mike Kershaw (dragorn), and the rest of the Ubertooth team.

A major tip o' the hat is due to the Bluetooth SIG. The Bluetooth Core
Spec 4.0 is an amazingly clear and readable technical reference.
