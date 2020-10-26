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

Change bdaddr of the Ubertooth
> bdaddr AA:BB:CC:DD:EE:FF

You can stop the current command with Ctrl+C.

## Monitor mode:
This mode allows to monitor a connection as it is being established.
It targets a race-condition in the pairing procedure: we receive the FHS packet before the legitimate slave.
With the FHS, we can synchronize to the piconet before the two actual participants, then record the packets they exchange.

In the beginning of a connection, devices will use the LMP protocol to exchange they capabilities and negociate encryption. 
The LMP packets sent at the beginning of a connection are not encrypted.
Hence, we can decode those LMP packets and display them. We can also handle LMP_SET_AFH to follow AFH.

We can still follow the piconet after encryption is enabled.
Though, ubertooth-btbr won't be able to decrypt the encrypted packets, and they will appear as "RX Raw". 
Also, we won't be able to decrypt LMP_SET_AFH packets, and to update the AFH map.

### Usage
For instance, say you want to monitor the pairing process between your phone and a peripheral.
All you need is the BD_ADDR of the slave peripheral.
First, start monitoring pages directed to peripheral 11:22:33:44:55:66:
> monitor 11:22:33:44:55:66

Then use your phone to connect to the slave. In ubertooth-btbr, you should see the FHS, followed by LMP traffic of the two devices.
If you don't receive the FHS, this means you have lost the race. Try again.

### Limitations:
- You need to win the race: if the actual slave receive the FHS before the Ubertooth, monitor will fail.
- This does not breaks encryption.
- Some packets will be lost. To reduce the packet loss: get closer of the targets, use a better antenna, use several ubertooth devices.
