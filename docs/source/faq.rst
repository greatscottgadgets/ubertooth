===
FAQ
===

**Question: What is the latest release?**

**Answer:** The latest release is 2018-08-R1: the DEFCON release, but you should check `this repo's releases <https://github.com/greatscottgadgets/ubertooth/releases>`__ for the most up to date information. See the `Build Guide <https://github.com/greatscottgadgets/ubertooth/wiki/Build-Guide>`__ for instructions on how to download and build the software.

----------

**Question: I run Windows or Mac OS X. What's the best way to use Ubertooth?**

**Answer:** The best way to use Ubertooth is from a native Linux install. If you don't normally run Linux, we recommend you boot Linux from USB using a distro such as `Kali <https://www.kali.org/>`__ or `Pentoo <http://pentoo.ch/>`__.

It **may** be possible to use Ubertooth from within a virtual machine. However, people have reported issues with USB throughput and requests timing out. If you're a newbie, booting directly into Linux is the recommended method.

----------

**Question: How do I update firmware?**

**Answer:** Refer to the wiki page `Firmware <https://github.com/greatscottgadgets/ubertooth/wiki/Firmware>`__.

----------

**Question: Can I use my Ubertooth with a VM?**

**Answer:** Yes. Many people have reported successfully using their Ubertooth with host tools installed in a virtual machine. There are, as always, some things to be aware of when attempting this.

    * The Ubertooth uses USB2.0, which requires the `VirtualBox extension pack <https://www.virtualbox.org/wiki/Downloads>`__ to be downloaded and installed.
    * The Ubertooth also uses different vendor and product identifiers when in normal operation and firmware upload mode, so rules for both of these will need to be added to the virtual machine for full operation.

Other virtual machine environments may differ and therefore present different issues. If you successfully use the Ubertooth with a different virtualisation system, please let us know so that we can help future users.

----------

**Question: What can the Ubertooth capture?**

**Answer:** The Ubertooth is able to capture and demodulate signals in the 2.4GHz ISM band with a bandwidth of 1MHz using a modulation scheme of Frequency Shift Keying or related methods.

This includes, but is not limited to:

    * Bluetooth Basic Rate packets

    * Bluetooth Low Energy (Bluetooth Smart)

The following may be possible:

    * 802.11 FHSS (1MBit)

    * Some proprietary 2.4GHz wireless devices

----------

**Question: Can I listen to my phone calls?**

**Answer:** Not yet, this will require full frequency hopping support and possibly require breaking encryption by sniffing the connection process. It's on the todo list, but there's a lot of hard work to go before we get there. If you're interested in making it happen, you can get involved in the project.