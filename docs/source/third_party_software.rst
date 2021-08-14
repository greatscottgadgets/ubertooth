====================
Third Party Software
====================

The following are third party applications that support Ubertooth One, either natively or with plugins. Unless otherwise stated, the Ubertooth project does not provide support these applications.



Kismet
~~~~~~

The version of kismet provided by Debian/Ubuntu is 2008-05-R1, which is too old to support the Ubertooth plugin. In order to use Ubertooth with Kismet it is necessary to compile Kismet from source. First make sure that you have completed the instruction in the `build guide <https://github.com/greatscottgadgets/ubertooth/wiki/Build-Guide>`__ and then use the following instruction to build Kismet:

	.. code-block:: sh

		sudo apt-get install libpcap0.8-dev libcap-dev pkg-config build-essential libnl-3-dev libncurses-dev libpcre3-dev libpcap-dev libcap-dev libnl-genl-3-dev
		wget https://kismetwireless.net/code/kismet-2013-03-R1b.tar.xz
		tar xf kismet-2013-03-R1b.tar.xz
		cd kismet-2013-03-R1b
		ln -s ../ubertooth-2015-10-R1/host/kismet/plugin-ubertooth .
		./configure
		make && make plugins
		sudo make suidinstall
		sudo make plugins-install
		Add "pcapbtbb" to the "logtypes=..." line in kismet.conf

Support for the Kismet is provided by the Kismet project, but the plugins are part of the Ubertooth software releases. For queries about the Ubertooth plugins please see the `getting help <https://github.com/greatscottgadgets/ubertooth/wiki/Getting-Help>`__ page.



Flying Squirrel
~~~~~~~~~~~~~~~

`Flying Squirrel <http://www.nrl.navy.mil/itd/chacs/5545/flying-squirrel>`__ has built in support for Ubertooth. Unfortunately Flying Squirrel is not available to the general public. "Flying Squirrel is only available to DOD and federal agencies." this was the reply when one tried to request the download to fsadmin@nrl.navy.mil.



Spectools
~~~~~~~~~

`Spectools <https://www.kismetwireless.net/spectools>`__ is a very useful 2.4GHz spectrum monitor, showing multiple views of spectrum usage. Spectools supports Ubertooth if built from git.