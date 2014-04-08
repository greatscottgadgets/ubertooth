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

# Ubertooth Zero is no longer supported

#export BOARD=UBERTOOTH_ZERO
#mkdir ${targetdir}/ubertooth-zero-firmware-bin
#cd ${targetdir}/firmware/
#make bluetooth_rxtx
#cp bluetooth_rxtx/bluetooth_rxtx.hex ${targetdir}/ubertooth-zero-firmware-bin/
#cp bluetooth_rxtx/bluetooth_rxtx.bin ${targetdir}/ubertooth-zero-firmware-bin/
#cp bluetooth_rxtx/bluetooth_rxtx.dfu ${targetdir}/ubertooth-zero-firmware-bin/
#make bootloader
#cp bootloader/bootloader.hex ${targetdir}/ubertooth-zero-firmware-bin/
#cp bootloader/bootloader.bin ${targetdir}/ubertooth-zero-firmware-bin/
#make clean

############################
# Hardware
############################
# Do this part by hand if there have been hardware changes since the last release
cp ${originaldir}/hardware/pogoprog/pogoprog-schematic.pdf ${targetdir}/hardware/pogoprog
cp ${originaldir}/hardware/pogoprog/pogoprog-assembly.pdf ${targetdir}/hardware/pogoprog
cp ${originaldir}/hardware/pogoprog/gerbers ${tagetdir}/hardware/pogoprog/
cp ${originaldir}/hardware/pogoprog/pogoprog-bom.csv ${targetdir}/hardware/pogoprog/pogoprog-bom.csv

cp ${originaldir}/hardware/ubertooth-one/ubertooth-one-schematic.pdf ${targetdir}/hardware/ubertooth-one
cp ${originaldir}/hardware/ubertooth-one/ubertooth-one-assembly.pdf ${targetdir}/hardware/ubertooth-one
cp ${originaldir}/hardware/ubertooth-one/gergers ${targetdir}/hardware/ubertooth-one/
cp ${originaldir}/hardware/ubertooth-one/ubertooth-one-bom.csv ${targetdir}/hardware/ubertooth-one/ubertooth-one-bom.csv

cp ${originaldir}/hardware/tc13badge/tc13badge-schematic.pdf ${targetdir}/hardware/tc13badge/
cp ${originaldir}/hardware/tc13badge/tc13badge-assembly.pdf ${targetdir}/hardware/tc13badge/
cp ${originaldir}/hardware/tc13badge/gerbers ${targetdir}/hardware/tc13badge/
cp ${originaldir}/hardware/tc13badge/tc13badge-bom.csv ${targetdir}/hardware/tc13badge/tc13badge-bom.csv

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
tar -cJf ${releasename}.tar.xz ${releasename}/
