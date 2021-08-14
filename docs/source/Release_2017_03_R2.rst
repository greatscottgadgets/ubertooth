==================
Release 2017-03-R2
==================

`for Release 2015-10-R1 see here <https://github.com/greatscottgadgets/ubertooth/wiki/Release-2015-10-R1>`__



Prerequisites
~~~~~~~~~~~~~

There are some prerequisites that need to be installed before building libbtbb and the Ubertooth tools. Many of these are available from your operating system's package repositories, for example:

Debian / Ubuntu

	.. code-block:: sh

		sudo apt-get install cmake libusb-1.0-0-dev make gcc g++ libbluetooth-dev \
		pkg-config libpcap-dev python-numpy python-pyside python-qt4

Fedora / Red Hat

	.. code-block:: sh

		su -c "yum install libusb1-devel make gcc wget tar bluez-libs-devel"

Mac OS X users can use either MacPorts or Homebrew to install the required packages:

	.. code-block:: sh

		sudo port install libusb wget cmake python27 py27-numpy py27-pyside
		or
		brew install libusb wget cmake pkg-config libpcap

FreeBSD users can install the host tools and library directly from the ports and package system:

	.. code-block:: sh

		sudo pkg install ubertooth



libbtbb
~~~~~~~

Next the Bluetooth baseband library (libbtbb) needs to be built for the Ubertooth tools to decode Bluetooth packets:

	.. code-block:: sh

		wget https://github.com/greatscottgadgets/libbtbb/archive/2017-03-R2.tar.gz -O libbtbb-2017-03-R2.tar.gz
		tar xf libbtbb-2017-03-R2.tar.gz
		cd libbtbb-2017-03-R2
		mkdir build
		cd build
		cmake ..
		make
		sudo make install

Linux users: if you are installing for the first time, or you receive errors about finding the library, you should run:	

	.. code-block:: sh

		sudo ldconfig



Ubertooth tools
~~~~~~~~~~~~~~~

The Ubertooth repository contains host code for sniffing Bluetooth packets, configuring the Ubertooth and updating firmware. All three are built and installed by default using the following method:

	.. code-block:: sh

		wget https://github.com/greatscottgadgets/ubertooth/releases/download/2017-03-R2/ubertooth-2017-03-R2.tar.xz -O ubertooth-2017-03-R2.tar.xz
		tar xf ubertooth-2017-03-R2.tar.xz
		cd ubertooth-2017-03-R2/host
		mkdir build
		cd build
		cmake ..
		make
		sudo make install

Linux users: if you are installing for the first time, or you receive errors about finding the library, you should run:

	.. code-block:: sh

		sudo ldconfig



Wireshark
~~~~~~~~~

Wireshark version 1.12 and newer includes the Ubertooth BLE plugin by default. It is also possible to `capture BLE from Ubertooth directly into Wireshark <https://github.com/greatscottgadgets/ubertooth/wiki/Capturing-BLE-in-Wireshark>`__ with a little work.

The Wireshark BTBB and BR/EDR plugins allow Bluetooth baseband traffic that has been captured using Kismet to be analysed and disected within the Wireshark GUI. They are built separately from the rest of the Ubertooth and libbtbb software. The directory passed to cmake as ``MAKE_INSTALL_LIBDIR`` varies from system to system, but it should be the location of existing Wireshark plugins, such as ``asn1.so`` and ``ethercat.so``.

	.. code-block:: sh

		sudo apt-get install wireshark wireshark-dev libwireshark-dev cmake
		cd libbtbb-2017-03-R2/wireshark/plugins/btbb
		mkdir build
		cd build
		cmake -DCMAKE_INSTALL_LIBDIR=/usr/lib/x86_64-linux-gnu/wireshark/libwireshark3/plugins ..
		make
		sudo make install

Then repeat for the BT BR/EDR plugin:

	.. code-block:: sh

		sudo apt-get install wireshark wireshark-dev libwireshark-dev cmake
		cd libbtbb-2017-03-R2/wireshark/plugins/btbredr
		mkdir build
		cd build
		cmake -DCMAKE_INSTALL_LIBDIR=/usr/lib/x86_64-linux-gnu/wireshark/libwireshark3/plugins ..
		make
		sudo make install



Third Party Software
~~~~~~~~~~~~~~~~~~~~

There are a number of pieces of `third party software <https://github.com/greatscottgadgets/ubertooth/wiki/Third-Party-Software>`__ that support the Ubertooth. Some support Ubertooth out of the box, while others require plugins to be built.



Firmware
^^^^^^^^

This completes the install of the Ubertooth tools, the next step is to look at the getting started guide. You should always `update the firmware <https://github.com/greatscottgadgets/ubertooth/wiki/Firmware>`__ on the Ubertooth device to match the software release version that you are using.