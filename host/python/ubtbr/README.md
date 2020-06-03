# Ubtbr
A python library to control the ubtbr firmware.
Implements communication with the firmware, and provide a simple
LMP layer to setup an unencrypted connection.

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
