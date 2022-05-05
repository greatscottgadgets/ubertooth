#!/bin/bash
usbhub --disable-i2c --hub D9D1 power state --port 4 --reset
sleep 1s
host/build/ubertooth-tools/src/ubertooth-dfu -d firmware/bluetooth_rxtx/bluetooth_rxtx.dfu -r
DFU_STATUS="$?"
if [ "$DFU_STATUS" == "0" ]
then
    echo "DFU installation success! Checking device..."
    sleep 1s
    host/build/ubertooth-tools/src/ubertooth-util -b -p -s
    DEVICE_STATUS="$?"
    if [ "$DEVICE_STATUS" == "0" ]
    then
        echo "Device found! Exiting."
        exit $DEVICE_STATUS
    elif [ "$DEVICE_STATUS" == "1" ]
    then
        echo "Could not find device! Broken firmware? Exiting."
        exit 99
    else
        echo "Unhandled device error!"
        exit $DEVICE_STATUS
    fi
elif [ "$DFU_STATUS" == "1" ]
then
    echo "No Ubertooth found! Disconnected? Exiting."
    exit $DFU_STATUS
elif [ "$DFU_STATUS" == "127" ]
then
    echo "Host tool installation failed! Exiting."
    exit $DFU_STATUS
else
    echo "Unhandled DFU error!"
    exit $DFU_STATUS
fi