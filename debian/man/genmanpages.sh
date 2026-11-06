#!/bin/bash


txt2man -d "${CHANGELOG_DATE}" -t UBERTOOTH-RX           -s 1 ubertooth-rx.txt         > ubertooth-rx.1
txt2man -d "${CHANGELOG_DATE}" -t UBERTOOTH-BTLE         -s 1 ubertooth-btle.txt       > ubertooth-btle.1
txt2man -d "${CHANGELOG_DATE}" -t UBERTOOTH-DEBUG        -s 1 ubertooth-debug.txt      > ubertooth-debug.1
txt2man -d "${CHANGELOG_DATE}" -t UBERTOOTH-SPECAN-UI    -s 1 ubertooth-specan-ui.txt  > ubertooth-specan-ui.1
txt2man -d "${CHANGELOG_DATE}" -t UBERTOOTH-DFU          -s 1 ubertooth-dfu.txt        > ubertooth-dfu.1
txt2man -d "${CHANGELOG_DATE}" -t UBERTOOTH-SCAN         -s 1 ubertooth-scan.txt       > ubertooth-scan.1
txt2man -d "${CHANGELOG_DATE}" -t UBERTOOTH-FOLLOW       -s 1 ubertooth-follow.txt     > ubertooth-follow.1
txt2man -d "${CHANGELOG_DATE}" -t UBERTOOTH-DUMP         -s 1 ubertooth-dump.txt       > ubertooth-dump.1
txt2man -d "${CHANGELOG_DATE}" -t UBERTOOTH-UTIL         -s 1 ubertooth-util.txt       > ubertooth-util.1
txt2man -d "${CHANGELOG_DATE}" -t UBERTOOTH-EGO          -s 1 ubertooth-ego.txt        > ubertooth-ego.1
txt2man -d "${CHANGELOG_DATE}" -t UBERTOOTH-SPECAN       -s 1 ubertooth-specan.txt     > ubertooth-specan.1
txt2man -d "${CHANGELOG_DATE}" -t UBERTOOTH-TX           -s 1 ubertooth-tx.txt         > ubertooth-tx.1
