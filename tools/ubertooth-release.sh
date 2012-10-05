#!/bin/bash

if test $# -lt 1 ; then
	echo "Run from the base of the Ubertooth repository, on the release branch"
    echo "usage: ./ubertooth-release.sh <previous release> [release branch name]"
    exit
fi

if test $# -lt 2 ; then
	branch=`git rev-parse --abbrev-ref HEAD`
else
	branch=${2}
	git checkout ${2}
fi

# FIXME you'll need to update these:
top="`pwd`/../release"

releasename="ubertooth-${branch}"
targetdir="${top}/${releasename}"
# Previous release, this is laziness to not have to rebuild the hardware files
originaldir="${top}/ubertooth-${1}"

mkdir -p ${targetdir}
git archive --format=tar  HEAD | (cd ${targetdir} && tar xf -)

sed -e "s/GIT_DESCRIBE=\".*\"/GIT_DESCRIBE=\"${branch}\"/" -i ${targetdir}/firmware/common.mk

############################
# Documentation
############################
cd ${targetdir}/web
make

############################
# Software
############################
export BOARD=UBERTOOTH_ONE
mkdir ${dir}/ubertooth-one-firmware-bin
cd ${targetdir}/firmware
make clean
make bluetooth_rxtx
cp bluetooth_rxtx.hex ${targetdir}/ubertooth-one-firmware-bin/
cp bluetooth_rxtx.bin ${targetdir}/ubertooth-one-firmware-bin/
cp bluetooth_rxtx.dfu ${targetdir}/ubertooth-one-firmware-bin/
cd ${targetdir}/firmware/bootloader
make bootloader
cp bootloader.hex ${targetdir}/ubertooth-one-firmware-bin/
cp bootloader.bin ${targetdir}/ubertooth-one-firmware-bin/
cd ${targetdir}/firmware/assembly_test
make assembly_test
cp assembly_test.hex ${targetdir}/ubertooth-one-firmware-bin/
cp assembly_test.bin ${targetdir}/ubertooth-one-firmware-bin/
make clean

export BOARD=UBERTOOTH_ZERO
mkdir ${targetdir}/ubertooth-zero-firmware-bin
cd ${targetdir}/firmware/bluetooth_rxtx
make bluetooth_rxtx
cp bluetooth_rxtx.hex ${targetdir}/ubertooth-zero-firmware-bin/
cp bluetooth_rxtx.bin ${targetdir}/ubertooth-zero-firmware-bin/
cp bluetooth_rxtx.dfu ${targetdir}/ubertooth-zero-firmware-bin/
cd ${targetdir}/firmware/bootloader
make bootloader
cp bootloader.hex ${targetdir}/ubertooth-zero-firmware-bin/
cp bootloader.bin ${targetdir}/ubertooth-zero-firmware-bin/
make clean

############################
# HArdware
############################
mkdir ${targetdir}/pogoprog-hardware
mkdir ${targetdir}/pogoprog-hardware/gerbers
cp ${originaldir}/hardware/pogoprog/pogoprog-schematic.pdf ${targetdir}/pogoprog-hardware
cp ${originaldir}/hardware/pogoprog/pogoprog-assembly.pdf ${targetdir}/pogoprog-hardware
cp ${originaldir}/hardware/pogoprog/pogoprog-*.g* ${tagetdir}/pogoprog-hardware/gerbers
cp ${originaldir}/hardware/pogoprog/pogoprog.drl ${targetdir}/pogoprog-hardware/gerbers
cp ${originaldir}/hardware/pogoprog/pogoprog.csv ${targetdir}/pogoprog-hardware/pogoprog-bom.csv
mkdir ${targetdir}/ubertooth-one-hardware
mkdir ${targetdir}/ubertooth-one-hardware/gerbers
cp ${originaldir}/hardware/ubertooth-one/ubertooth-one-schematic.pdf ${targetdir}/ubertooth-one-hardware
cp ${originaldir}/hardware/ubertooth-one/ubertooth-one-assembly.pdf ${targetdir}/ubertooth-one-hardware
cp ${originaldir}/hardware/ubertooth-one/ubertooth-one-*.g* ${targetdir}/ubertooth-one-hardware/gerbers
cp ${originaldir}/hardware/ubertooth-one/ubertooth-one.drl ${targetdir}/ubertooth-one-hardware/gerbers
cp ${originaldir}/hardware/ubertooth-one/ubertooth-one.csv ${targetdir}/ubertooth-one-hardware/ubertooth-one-bom.csv
mkdir ${targetdir}/tc13badge-hardware
mkdir ${targetdir}/tc13badge-hardware/gerbers
cp ${originaldir}/hardware/tc13badge/tc13badge-schematic.pdf ${targetdir}/tc13badge-hardware
cp ${originaldir}/hardware/tc13badge/tc13badge-assembly.pdf ${targetdir}/tc13badge-hardware
cp ${originaldir}/hardware/tc13badge/tc13badge-*.g* ${targetdir}/tc13badge-hardware/gerbers
cp ${originaldir}/hardware/tc13badge/tc13badge.drl ${targetdir}/tc13badge-hardware/gerbers
cp ${originaldir}/hardware/tc13badge/tc13badge.csv ${targetdir}/tc13badge-hardware/tc13badge-bom.csv

############################
# Clean up
############################
cp ${targetdir}/tools/RELEASENOTES ${targetdir}/
rm -rf ${targetdir}/tools
rm ${targetdir}/.gitignore

############################
# Archive
############################
cd ${top}
tar -zcf ${releasename}.tgz ${releasename}/
