=========
ToDo List
=========

PCAP
~~~~

* Obtain DLT/linktype from libPCAP devs for BTBB, see `here <https://github.com/greatscottgadgets/ubertooth/wiki/Bluetooth-Captures-in-PCAP>`__.

* Define format for including meta-data (using PPI), see `here <https://github.com/greatscottgadgets/ubertooth/wiki/Bluetooth-Captures-in-PCAP>`__.

* Move Wireshark plugins to Wireshark repo.

* Add pcap support to ubertooth tools (currently only in kismet) by moving it to libbtbb.



Basic rate / libbtbb
~~~~~~~~~~~~~~~~~~~~

* Better handling of AFH maps when trying clock values. The code currently makes the assumption that AFH is enabled but all channels are in use.

* Detect syncword on Ubertooth for known piconets

* Make logging configurable - possibly combine text based logging, file dumps and pcap writing in to one logging system.



Releases
~~~~~~~~

* Binary packages for both libbtbb and ubertooth - rpm, deb, others?

* More frequent releases - requires better testing of code in git and separation of half-implemented features. This is more about process than an actionable todo item.

* DONE: Add uninstall to Makefile.

* Add WICD headers to support windows users (requires switch to libusbx).



GR-Bluetooth
~~~~~~~~~~~~

* Finish migration to libbtbb.

* Add BTLE support.

* General performance improvements (there must be some!).



Bigger / more vague projects
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Transmit basic rate packets - inquiry scan to start with.

* Implement full BTLE device in firmware.

* ANT/ANT+ sniffing.

    * `http://www.thisisant.com/developer/resources/downloads/#documents_tab <http://www.thisisant.com/developer/resources/downloads/#documents_tab>`__

    * `http://www.thisisant.com/developer/resources/tech-faq/ <http://www.thisisant.com/developer/resources/tech-faq/>`__

* Communicate between Ubertooth devices using SPI on the expansion port.

* Communicate with other devices using SPI on the expansion port.

* Write to SD card using SPI on the expansion port.
