Capturing BLE in Wireshark
~~~~~~~~~~~~~~~~~~~~~~~~~~

You can capture BLE in Wireshark with standard Wireshark builds. This guide assumes Linux.

    #. Run the command: ``mkfifo /tmp/pipe``
    #. Open Wireshark
    #. Click Capture -> Options
    #. Click "Manage Interfaces" button on the right side of the window
    #. Click the "New" button
    #. In the "Pipe" text box, type "/tmp/pipe"
    #. Click Save, then click Close
    #. Click "Start"

In a terminal, run ``ubertooth-btle``:

	.. code-block:: sh

		ubertooth-btle -f -c /tmp/pipe

In the Wireshark window you should see packets scrolling by.

Note: If you get `User encapsulation not handled: DLT=147, check your Preferences->Protocols->DLT_USER <https://github.com/greatscottgadgets/ubertooth/issues/61>`__ the steps you want are:

    #. Click Edit -> Preferences
    #. Click Protocols -> DLT_USER
    #. Click Edit (Encapsulations Table)
    #. Click New
    #. Under DLT, select "User 0 (DLT=147)" (adjust this selection as appropriate if the error message showed a different DLT number than 147)
    #. Under Payload Protocol, enter: btle
    #. Click OK
    #. Click OK



Capturing BLE in scapy
^^^^^^^^^^^^^^^^^^^^^^

#. Do not use mkfifo for the filename, it will cause scapy to slow dramatically.

#. In a terminal, run ubertooth-btle:

	.. code-block:: sh

		ubertooth-btle -f -q /tmp/pipe

#. Open python and run:

	.. code-block:: sh

		from scapy.all import *
		p = sniff(offline='/tmp/pipe')

p is now a list of the packets captured!



Sniffing connection data
^^^^^^^^^^^^^^^^^^^^^^^^

With recent Ubertooth firmware, only advertisements are captured by default. Once you have identified the device address of the target device you would like to sniff, run:

	.. code-block:: sh

		ubertooth-btle -t aa:bb:cc:dd:ee:ff

The Ubertooth will follow connections involving this target until -t none is passed or the device is reset.

You may need to attempt connecting several times until Ubertooth is able to follow the connection successfully.



Capturing from a remote host
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can use `sshdump <https://www.wireshark.org/docs/man-pages/sshdump.html>`__ to remotely capture packets from a Ubertooth attached to another host or virtual machine. In the Wireshark UI, this may show up as an interface named "SSH remote capture: ``sshdump``" which needs to be configured first with the following "remote capture command":

	.. code-block:: sh

		killall ubertooth-btle; unlink /tmp/btlepipe; mkfifo /tmp/btlepipe; ubertooth-btle -f -c /tmp/btlepipe &>/dev/null & cat /tmp/btlepipe

The "remote interface" option is ignored and can be set to any value.



Useful display filters
^^^^^^^^^^^^^^^^^^^^^^

Only connection requests and non-zero data packets:

	.. code-block:: sh

		btle.data_header.length > 0 || btle.advertising_header.pdu_type == 0x05

Only attribute read responses, write requests, and notifications:

	.. code-block:: sh

		btatt.opcode in { 0x0b 0x12 0x1b }
