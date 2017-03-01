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
	# Not sure this is really a good idea, better to make
	# sure that you're on the branch in the first place
	#git checkout ${2}
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
mkdir ${targetdir}/ubertooth-one-firmware-bin
cd ${targetdir}/firmware
make clean
make bluetooth_rxtx
cp bluetooth_rxtx/bluetooth_rxtx.hex ${targetdir}/ubertooth-one-firmware-bin/
cp bluetooth_rxtx/bluetooth_rxtx.bin ${targetdir}/ubertooth-one-firmware-bin/
cp bluetooth_rxtx/bluetooth_rxtx.dfu ${targetdir}/ubertooth-one-firmware-bin/
make bootloader
cp bootloader/bootloader.hex ${targetdir}/ubertooth-one-firmware-bin/
cp bootloader/bootloader.bin ${targetdir}/ubertooth-one-firmware-bin/
make assembly_test
cp assembly_test/assembly_test.hex ${targetdir}/ubertooth-one-firmware-bin/
cp assembly_test/assembly_test.bin ${targetdir}/ubertooth-one-firmware-bin/
make clean

# Receive Only firmware
DISABLE_TX=1 make bluetooth_rxtx
cp bluetooth_rxtx/bluetooth_rxtx.hex ${targetdir}/ubertooth-one-firmware-bin/bluetooth_rx_only.hex
cp bluetooth_rxtx/bluetooth_rxtx.bin ${targetdir}/ubertooth-one-firmware-bin/bluetooth_rx_only.bin
cp bluetooth_rxtx/bluetooth_rxtx.dfu ${targetdir}/ubertooth-one-firmware-bin/bluetooth_rx_only.dfu
make clean

############################
# Clean up
############################
cp ${targetdir}/tools/RELEASENOTES ${targetdir}/
rm -rf ${targetdir}/tools
rm ${targetdir}/.gitignore
rm ${targetdir}/.travis.yml

############################
# Archive
############################
cd ${top}
tar -cJf ${releasename}.tar.xz ${releasename}/

sha256sum ${releasename}.tar.xz
sha512sum ${releasename}.tar.xz

