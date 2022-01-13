#!/bin/bash
cd firmware/bluetooth_rxtx
DFU_TOOL=/var/lib/jenkins/workspace/ubertooth_master/host/build/ubertooth-tools/src/ubertooth-dfu make
cd ../..