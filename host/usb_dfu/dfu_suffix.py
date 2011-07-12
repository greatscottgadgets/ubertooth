#!/usr/bin/env python

from struct import unpack, pack

crc_table = []

# From freemyipod.org
for i in range(256):
    t = i;
    for j in range(8):
        if t & 1:
            t = (t >> 1) ^ 0xedb88320
        else:
            t = t >> 1
    crc_table.append(t)

def crc32(data):
    crc = 0xffffffff
    for byte in data:
        crc = (crc >> 8) ^ crc_table[(crc ^ ord(byte)) & 0xff]
    return crc

def check_suffix(firmware):
    """Check the dfu suffix"""
    print('Checking firmware signature')

    data   = firmware[:-4]
    length = ord(firmware[-5])
    suffix = firmware[-length:]

    # Will always have these fields
    dwCRC     = unpack('L', suffix[12:])[0]
    bLength   = unpack('B', suffix[11])[0]
    ucDfuSig  = unpack('3s', suffix[8:11])[0]
    bcdDFU    = unpack('H', suffix[6:8])[0]
    #bcdDFU, ucDfuSig, bLength, dwCRC = unpack('H3sBL', suffix[6:])

    if bLength != 16:
        raise Exception("Unknown DFU suffix length: %s" % type(bLength))

    # We only know about dfu version 1.0/1.1
    # This needs to be smarter to support other versions if/when they exist
    if bcdDFU != 0x0100:
        raise Exception("Unknown DFU version: %d" % bcdDFU)

    # Suffix bytes are reversed
    if ucDfuSig != 'UFD':
        raise Exception("DFU Signature mismatch: not a DFU file")

    crc = crc32(data)
    if crc != dwCRC:
        raise Exception("CRC mismatch: calculated: 0x%x, found: 0x%x" % (crc, dwCRC))

    # Extract additional fields now that we know the suffix contains them
    idVendor  = unpack('H', suffix[4:6])[0]
    idProduct = unpack('H', suffix[2:4])[0]

    # Version information that we can't verify
    bcdDevice = unpack('H', suffix[0:2])[0]

    return length, idVendor, idProduct

def add_suffix(firmware, vendor, product):
    bcdDevice = 0
    idProduct = product
    idVendor  = vendor
    bcdDFU    = 0x0100
    ucDfuSig  = 'UFD'
    bLength   = 16

    suffix = pack('4H3sB', bcdDevice, idProduct, idVendor, bcdDFU, ucDfuSig, bLength)
    firmware += suffix

    crc = crc32(firmware)
    firmware += pack('L', crc)

    return firmware
