#!/bin/bash
set -e
cd firmware/bluetooth_rxtx
DFU_TOOL=../../host/build/ubertooth-tools/src/ubertooth-dfu make
cd ../..
