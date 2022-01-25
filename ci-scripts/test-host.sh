#!/bin/bash 
host/build/ubertooth-tools/src/ubertooth-util -b -p -s
EXIT_CODE="$?"
if [ "$EXIT_CODE" == "1" ]
then
    echo "Host tool installation success! Exiting.."
    exit 0
elif [ "$EXIT_CODE" == "0" ]
then
    echo "Failed to boot Ubertooth into DFU mode! Check DFU pin jumper. Exiting.."
    exit 1
elif [ "$EXIT_CODE" == "127" ]
then
    echo "Host tool installation failed! Exiting.."
    exit $EXIT_CODE
else
    echo "Unhandled error"
    exit $EXIT_CODE
fi