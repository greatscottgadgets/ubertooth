# Ubtbr
A small python library to interact with the ubtbr firwmare.
Implements communication with the firmware, and provide a simple
LMP layer to setup an unencrypted connection.

The ubertooth-ubtbr.py tool provide a command-line interface to most functionnalities of ubtbr.

To start an inquiry:
>>> inquiry

To connect to a device:
>>> page 11:22:33:44:55:66

or 

>>> 112233445566

To be visible and connectable:

>>> discoverable

To ping the device with L2CAP echo transfers:

>>> l2ping

You can stop the current command with Ctrl+C.
