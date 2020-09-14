# UBTBR firmware

This firmware implements partial support of the Bluetooth BR/EDR baseband layer.

## Supported features

- Inquiry/Inquiry scan: Perform inquiry to discover nearby devices, or reply to inquiries to be visible.
- Page/Page scan: Establish a connection as a master, or allow connection as slave.
- Link-Layer: Poor-man link-layer to maintain a connection.
- Monitoring: Spy on pairing procedures of remote devices

## Limitations / TODOs
- Due to limitations of the cc2400, only BR packets are supported.
- Packet types supported: ID, NULL, POLL, FHS, DM*, DH*.
- Link-layer only supports ACL packets.
- Master side of link-layer drains too much battery of the slave (FIXME: POLL less).
- Role-switching and related LMP procedures are not yet implemented.
- Encryption of packets is not yet implemented.

## Host code
The *ubtbr* python modules contains the host control interface and a minimal implementation of the LMP protocol. 

UBTBR can be controlled using the *ubtbr* python module, and the ubertooth-btbr tool.

See host/python/ubtbr/README.
