=================
Release Procedure
=================



An Ubertooth release is not as simple as a standard GitHub release, there are a few extra tasks, as listed below.

    * Tag both libbtbb and ubertooth with the release version. Use an annotated tag. e.g.

    * git tag -a yyyy-mm-Rx

    * Create a release directory using git archive

    * Update the following files to set RELEASE to yyyy-mm-Rx:

    	* ubertooth/libubertooth/src/CMakeLists.txt

    	* libbtbb/lib/src/CMakeLists.txt

    * Build binary firmware images for all board revs and all common firmware images.

    * At time of writing, these are only bluetooth_rxtx and bootloader for Ubertooth One.

    * Adjust the GIT_REVISION variable in firmware/common.mk to match the release number

    * Build the bluetooth_rxtx firmware and rename bluetooth_rxtx.dfu to ubertooth-one-bin-firmware.dfu

    * Copy built firmare hex/bin/dfu files to /ubertooth-one-firmware-bin

    * Export gerbers from KiCad (or copy from previous release if unchanged)

    * Write release notes, save to top level of archive, add to the wiki and send to the mailing list

    * Write/update the build instructions on the wiki as needed
    
    * Perhaps we should branch and do some of this on the branch before tagging, this seems like good practice.

