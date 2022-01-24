#!/bin/bash
usbhub --hub D9D1 power state --port 4 --reset
sleep 1s
host/build/ubertooth-tools/src/ubertooth-dfu -d firmware/bluetooth_rxtx/bluetooth_rxtx.dfu
EXIT_CODE="$?"
if [ "$EXIT_CODE" == "0" ]
then
    echo "DFU installation success! Exiting.."
    exit $EXIT_CODE
elif [ "$EXIT_CODE" == "1" ]
then
    echo "No Ubertooth found! Disconnected? Exiting.."
    exit $EXIT_CODE
elif [ "$EXIT_CODE" == "127" ]
then
    echo "Host tool installation failed! Exiting.."
    exit $EXIT_CODE
else
    echo "Unhandled error"
    exit $EXIT_CODE
fi