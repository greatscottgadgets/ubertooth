# Ubtbr
A python library to control the ubtbr firmware.
Implements communication with the firmware, and provide a simple
LMP layer to setup an unencrypted connection.

# Ubertooth-btbr

The *ubertooth-ubtbr* tool provide a command-line interface to most functionnalities of ubtbr:

Start an inquiry:
> inquiry

Connect to a device:
> page 11:22:33:44:55:66

or 
> page 112233445566

Become visible:
> inquiry_scan

Become connectable:
> page_scan

Become both visible and connectable:
> discoverable

Ping the device with L2CAP echo transfers:
> l2ping 11:22:33:44:55:66

You can stop the current command with Ctrl+C.

## Monitor mode:
This mode allows monitoring the pairing process between two Bluetooth devices.
It targets a race-condition in the pairing procedure: we receive the FHS packet before the legitimate slave.
With the FHS, we can synchronize to the piconet before the two actual participants, then receive LMP traffic
that is exchanged in the clear (until activation of encryption).

For instance, say you want to monitor the pairing process between your phone and a peripheral.
All you need is the BD_ADDR of the peripheral.
First, start monitoring pages directed to peripheral 11:22:33:44:55:66:
> monitor 11:22:33:44:55:66

Then use your phone to pair to the peripheral. In ubertooth-btbr, you should see the FHS, followed by LMP traffic of the two devices.
If you don't receive the FHS, this means you have lost the race. Try again.

### Limitations:
- You need to win the race: if the actual slave receive the FHS before the Ubertooth, monitor will fail.
- This only works for the early state of the connection, before encryption is enabled. Usually, you won't see anymore packets after a LMP_START_ENCRYPTION_REQ.
- Some packets get lots, even before AFH is enabled. To reduce the packet loss: get closer of the targets, use a better antenna, or contribute to the btbr firmware to improve its performances.
- AFH is not fully implemented yet, so you might lose more packets after AFH establishment.
