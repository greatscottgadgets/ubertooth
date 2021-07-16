==========================
Bluetooth Captures in PCAP
==========================

Overview
~~~~~~~~

Classic PCAP files store a sequence of packets of a single link type. Published link types are `here <http://www.tcpdump.org/linktypes.html>`__. The `pcapng <http://www.winpcap.org/ntar/draft/PCAP-DumpFileFormat.html>`__ format also uses these same link types and the per-packet formatting as PCAP.

Early versions of libbtbb and ubertooth saved PCAP files with the DLT_PPI format, which was expedient but is considered deprecated by the libpcap folks. Best practice is to allocate a DLT for a particular link-layer and define a pseudo-header for that DLT that precedes each packet in the file. Early versions of this article formed a place to collect such a proposal. Now that the DLTs are allocated, and this article serves to collect implementation alternatives and details.

It was possible, and somewhat consistent with DLT_BLUETOOTH_HCI_H4_WITH_PHDR, to allocate a single DLT_BLUETOOTH_LOW_LEVEL to indicate any Bluetooth capture (BR/EDR or LE). However, the preference was to have separate DLT's for BR/EDR baseband and LE link-layer (as per the terms used in the Bluetooth spec).

Since PCAP has general applicability to packet capture, it made sense to provide a vendor-neutral, generic view of Bluetooth capture. The allocated DLTs can be applied to current and future versions of both ubertooth and gr-bluetooth, and potentially other RF capture tools as well. 



Aspects of Bluetooth Capture
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Sometimes we want to capture and record malformed, truncated, or garbled packets along with ostensibly valid or pristine ones. We also want to capture packets when some link parameters are unknown. When operating promiscuously, the capture tool may accept noise bursts as candidate Bluetooth packets. In extreme cases, the capture tool may not even de-whiten the packets, preferring to do that as a post-processing step. It was therefore important to include some of the receiver metrics in the capture metadata so post-processing and display tools like Wireshark can easily filter out unwanted packets, or avoid redundantly checking packet integrity when it's already known.

Five areas of receiver metadata were contemplated to assist in packet classification:

    #. signal and noise strength

    #. flags and metrics on the validity of unprotected fields (BR sync word, LE access address)

    #. error-correction on BR/EDR when subject to FEC

    #. whether all or portions of the packet is de-whitened

    #. packet-level error-checking already performed at capture (CRC, HEC, MIC)

Since the current ubertooth can recover BR and LE packets, but not EDR payloads, there was also a need to indicate whether the EDR data is present in the packet capture. 



Capture Use Cases Summarized
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following use cases apply to both full-band and narrowband capture strategies. There are no particular restrictions imposed under full-band captures. However, for a narrowband captures (e.g. Ubertooth):

    * learning implies the tool transitions from receiving a static channel, or surveying channels, to following particular hop sequence(s), and

    * limited hop sequence(s) (piconets/access addresses) may be followed at a given time.

Hybrid captures are also supported by these use cases and capture formats. For example, one may configure several narrowband (e.g. Ubertooth) or partial-band (e.g. gr-bluetooth) captures, on static channels, and perform post-processing on the set of capture files (either PCAP or PCAPNG). A similar hybrid arrangement is possible to post-process several narrowband capture tools following different hop sequences.

Use cases involving encryption are TBD. 



BR/EDR
^^^^^^

.. list-table ::
  :header-rows: 1
  :widths: 1 1 1 1 1 1 1

  * - Name 	
    - Description 	
    - Comments 	
    - Packets De-whitened? 	
    - Reference LAP 	
    - Reference UAP 	
    - HEC/CRC checking
  * - Promiscuous capture without learning 	
    - All packets that meet the configured RF criteria (e.g. signal strength or SNR), and meet the configured criteria for access-code offenses (e.g. preamble, trailer must be valid), are decoded and stored in the capture file. 	
    - Either PCAP or PCAPNG is equally useful. Capture file may be very large. Post-processing necessary to recover UAP per LAP, and CLK per LAP/UAP, and filter packets of interest. 	
    - No 	
    - Invalid 	
    - Invalid 	
    - No
  * - Promiscuous capture with learning 	
    - Initially operates as above, but as capture tool recovers LAPs, and the associated UAP, and CLK parameters, it is able to perform more processing per packet. 	
    - PCAP or PCAPNG may be used, but the latter is more useful since it includes a record of the capture tool's learning process. Post-processing similar to above may back-annotate, de-whiten, and integrity-check packets captured before the parameters were learned. Essentially, the post-processing mentioned above is split between capture-time and post-capture-time. 	
    - Initially no, later yes 	
    - Initially invalid, later known 	
    - Initially invalid, later known 	
    - Initially no, later optionally performed
  * - Capture of targeted LAPs, with and without learning 	
    - Only captures packets with an access code that includes the targeted LAPs, with a configured tolerance for access code bit errors. The capture tool may or may not attempt to learn the UAP and CLK parameters per configured LAP. 	
    - This is simply a stricter version of the promiscuous captures, where the access code triggering capture is targeted, resulting in a much smaller capture file for the same traffic pattern. Whether the capture tool learns or not, post-processing is useful to back-annotate, modify, and filter packets. 	
    - Initially no, later yes with learning 	
    - Valid 	
    - Initially invalid, later known with learning 	
    - Initially no, later optionally performed with learning
  * - Capture of targeted LAP/UAPs, with and without learning 	
    - Operates as above, except the configured LAP/UAPs are used for all packet capture processing. The capture tool may or may not attempt to learn the CLK alignment per LAP/UAP. 	
    - This is an accelerated version of targeted LAPs, where the associated UAP is not learned. 	
    - Initially no, later yes with learning 	
    - Valid 	
    - Valid 	
    - Initially no, later optionally performed with learning
  * - Capture of known master devices 	
    - Operates as above, except the master devices' internal state (BD_ADDR, CLK) is known at capture time. 	
    - There is no need to passively learn any piconet parameters. 	
    - Yes 	
    - Valid 	
    - Valid 	
    - Optionally performed 



LE
^^^

.. list-table :: 
  :header-rows: 1
  :widths: 1 1 1 1 1 1 

  * - Name 	
    - Description 	
    - Comments 	
    - Packets De-whitened? 	
    - Reference AA 	
    - CRC checking
  * - Promiscuous capture without learning 	
    - All packets that meet the configured RF criteria (e.g. signal strength or SNR), and meet the configured criteria for access address offenses (e.g. preamble must be valid, access address must be well-formed), are stored in the capture file. The captured packets are optionally de-whitened. 	
    - Either PCAP or PCAPNG is equally useful. Capture file may be very large. Post-processing necessary to recover connection parameters, and filter packets of interest. 	
    - Optional 	
    - Valid for Advertising channels only 	
    - Optionally performed for Advertising channels only
  * - Promiscuous capture with learning 	
    - Initially operates as above, except access addresses are learned by the capture tool, e.g. by accumulating candidates on Data channels or processing CONNECT_REQ PDUs on Advertising channels. 	
    - Access addresses learned by profiling Data channels cannot be CRC-checked, because the CRCInit parameter is unknown. 	
    - Yes (but optional) 	
    - Initially valid for Advertising channels only, but later valid for Data channels as access addresses are learned 	
    - Optionally performed for Advertising channels or Data channels where CRCInit is known for the access address
  * - Targeted capture 	
    - Specific BD_ADDRs are selected, and the associated access addresses are whitelisted for capture when data from a CONNECT_REQ PDU involving those BD_ADDRs is processed 	
    - The contents of the CONNECT_REQ PDU may be found by capturing Advertising channels or configured into the capture tool directly. 	
    - Yes (but optional) 	
    - Valid 	
    - Optionally performed 



Session Meta Information (Proposed)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Often there is meta-information that is recovered during the Bluetooth air capture, during post-processing, or provided out-of-band. Here we enumerate `PCAPNG options <http://www.winpcap.org/ntar/draft/PCAP-DumpFileFormat.html#sectionopt>`__ for use within the `interface description block <http://www.winpcap.org/ntar/draft/PCAP-DumpFileFormat.html#sectionidb>`__ for Bluetooth captures.

Some of these session-oriented data have time-windows that bound their applicability. A timestamp pair is used to define such a window. Timestamps are stored in same precision as indicated in ts_resol field of the `capture interface <http://www.winpcap.org/ntar/draft/PCAP-DumpFileFormat.html#sectionidb>`__. When both timestamps are equal (e.g. both zero), the meta-datum applies for the entire capture session.

PCAPNG options with MSB set are available for local use. We simply state that the interface options enumerated below are local to the DLTs allocated for Bluetooth RF captures. The most-significant byte for all interface option codes below is 0xd3, which was selected as unlikely to conflict with other local interface options that might be in use in PCAPNG generally. The ranges of least-significant bytes allocated below to option codes are: general Bluetooth is 0x0-0x3f, BR/EDR is 0x40-0x7f, and LE is 0x80-0xbf, with 0xc0-0xff reserved. 



BREDR_BD_ADDR
^^^^^^^^^^^^^

This record provides Bluetooth device addresses (BD_ADDRs) that may be present in the packet capture. BD_ADDRs are useful in post-processing or display tools to provide unique identification of the devices involved in piconet communication.

Device addresses may be recovered by the capture tool or provided by the user as a parameter to the capture session. In either case, this BREDR_BD_ADDR record may appear in the PCAPNG capture file.

When recovered by the capture tool, the UAP may be partly recovered by determining the channel hop sequence. Only the 4 least-significant bits of the UAP are used in hop-sequence determination. UAPs are also used in the BR/EDR Header Error Check, and payload Cyclic Redundancy Check generation, and may be recovered by accumulating candidates from the captured Bluetooth packets. In these cases, the UAP recorded may be masked to indicate which bits are known with certainty.

The UAP and NAP are available in the clear as fields within the FHS Packet. When captured directly, or when provided as a capture session parameter, the UAP and NAP may be recorded with certainty (all mask bits set).

The capture tool may store multiple records for the same BD_ADDR, as long as subsequent records indicate more certainty in the known UAP bits or add a known NAP. This sort of situation might occur if a capture tool starts out without knowing any UAP bits, then determines some UAP bits from hop-sequence following, more UAP bits from HEC and CRC prediction, and finally the full BD_ADDR contents after capturing an applicable FHS packet. 



Option Structure
++++++++++++++++

::

	 0                   1                   2                   3
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|            0xd340             |               8               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    LAP                        |      UAP      |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|              NAP              |   UAP_mask    |   NAP_valid   |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+



Description
+++++++++++

The option code and length are expressed in the native endianness used by PCAPNG. All multi-octet fields defined below are expressed in little-endian format.

The **LAP** field is the Lower Address Part of the the Bluetooth device address, as per Bluetooth spec Volume 2, Part B, Section 1.2.

The **UAP** field is the Upper Address Part of the the Bluetooth device address, as per Bluetooth spec Volume 2, Part B, Section 1.2.

The **NAP** field is the Network Address Part of the the Bluetooth device address, as per Bluetooth spec Volume 2, Part B, Section 1.2.

The **UAP_mask** field has its bits set to indicate which bits of the UAP are known with certainty.

The **NAP_valid** field is a flag in the least-significant bit that indicates whether the NAP field is populated with valid data. All other bits of this field are reserved and must be zero. 



C Structure
+++++++++++

::

	typedef struct _brder_bdaddr {
	        uint8_t  LAP[3];
	        uint8_t  UAP;
	        uint16_t NAP;
	        uint8_t  UAP_mask;
	        uint8_t  NAP_valid;
	} bredr_bdaddr;



BREDR_CLK 
^^^^^^^^^

This record provides Bluetooth Clock alignment information. The alignment timestamp used in this record is the same precision as the PCAPNG interface header indicates.

Some capture tools estimate the master device clock by inspecting packets and building confidence in the estimate. This record provides a mask that has bits set for known master clock bits. This distinguishes known bits from unknown bits as the master clock estimate improves. Consequently, multiple BREDR_CLK records may appear in the PCAPNG capture file for the same LAP/UAP, provided that subsequent entries offer a better estimate of the device clock.

The information record may be formed as a result of capturing a Bluetooth FHS packet, in which case the CLK_mask should indicate all CLK bits are known. 



Option Structure
++++++++++++++++

::

	 0                   1                   2                   3
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|            0xd341             |              20               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	|                      alignment timestamp                      |
	|                                                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                    LAP                        |      UAP      |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                              CLK                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                            CLK_mask                           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+



Description
+++++++++++

The option code and length are expressed in the native endianness used by PCAPNG. All multi-octet fields defined below are expressed in little-endian format.

The **Alignment Timestamp** is a PCAPNG-resolution timestamp that serves as a reference point for **CLK** associated with the Bluetooth master device referenced by **LAP** and **UAP**.

The **LAP** field is the Lower Address Part of the the Bluetooth device address, as per Bluetooth spec Volume 2, Part B, Section 1.2.

The **UAP** field is the Upper Address Part of the the Bluetooth device address, as per Bluetooth spec Volume 2, Part B, Section 1.2.

The **CLK** field is the native clock of the Bluetooth device, with bits 0-1 and bits 28-31 always zero.

The **CLK_mask** field determines which bits of **CLK** are valid, with bits 0-1 and bits 28-31 always zero. 



C Structure
+++++++++++

::

	typedef struct _brder_bdaddr {
	        uint64_t ns;
	        uint8_t  LAP[3];
	        uint8_t  UAP;
	        uint32_t CLK;
	        uint32_t CLK_mask;
	} bredr_bdaddr;



BT_WIDEBAND_RF_INFO 
^^^^^^^^^^^^^^^^^^^
Some capture tools, e.g. gr-bluetooth, allow for intentional aliasing such that multiple Bluetooth channels appear as superimposed images within a relatively narrow baseband.

Here we define a generic wideband RF information structure so aliasing conditions may be recorded in the PCAPNG capture file.

A post-processing or display tool might use this information to indicate the set of possible RF channels ascribed to each captured packet. BT_WIDEBAND_RF_INFO only applies to packets captured under the applicable interface, where the packet's Flags field indicates the RF channel was subject to aliasing. 



Option Structure
++++++++++++++++

::

	 0                   1                   2                   3
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|            0xd300             |              16               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                   centre frequency in Hz                      |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                   analog bandwidth in Hz                      |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                 intermediate frequency in Hz                  |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                  sampling bandwidth in Hz                     |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+



Description
+++++++++++

The option code and length are expressed in the native endianness used by PCAPNG. All multi-octet fields defined below are expressed in little-endian format.

The **Centre Frequency** field determines the centre of the RF capture, in Hz.

The **Analog Bandwidth** field determines the passband width of the analog section employed in the capture tool. It is measured from band centre to the band edge, in Hz.

The **Intermediate Frequency** field determines the intermediate carrier frequency used in the analog receiver, relative to the **Centre Frequency**, in Hz.

The **Sampling Bandwidth** field determines the digital sampling bandwidth employed in the capture tool, in Hz. 



C Structure
+++++++++++

::

	typedef struct _bt_wideband_rf_info {
	        uint32_t centre_freq_hz;
	        uint32_t analog_bw_hz;
	        int32_t  intermediate_freq_hz;
	        uint32_t sampling_bw_hz;
	} bt_wideband_rf_info;



LE_LL_CONNECTION_INFO 
^^^^^^^^^^^^^^^^^^^^^

This record provides context for a BTLE connection so that a post-processor or display tool may perform a more in-depth packet analysis. The following fields may be applied:

    * InitA, the initiator's public or random device address, may be used to connect packets with a device.

    * AdvA, the advertiser's public or random device address, may be used to connect packets with a device.

    * AA, the access address, connects a given LE packet to the rest of the data in this record (since all LE packets contain an AA field).

    * CRCInit, the 24-bit LFSR initial value, may be used to verify per-packet CRC integrity.

    * WinSize, WinOffset, Interval, and Latency may be used to verify adherence to RF transmission rules.

    * Timeout may be used to infer connection loss when packets are absent.

    * ChM, the allowable RF channel map, and Hop, may be used to verify the RF hop sequence.

The format of this record matches the CONNECT_REQ PDU used in the LE link layer. It is anticipated records of this nature would accrue in the capture file as follows:

    #. when a CONNECT_REQ PDU is captured, a record is stored with the PDU contents, and the capture tool considers the values current for the indicated AA.

    #. when a LL_CONNECTION_UPDATE_REQ PDU is captured after a CONNECT_REQ PDU, for the same AA:

        #. a new record is created, updating the WinSize, WinOffset, Interval, Latency, and Timeout fields.

        #. the valid-from timestamp is determined by the Instant parameter of the LL_CONNECTION_UPDATE_REQ PDU.

        #. the other parameters in this record are populated with those values already considered current.

        #. at the indicated instant, capture tool considers the updated values current for the indicated AA.

    #. when a LL_CHANNEL_MAP_REQ PDU is captured after a CONNECT_REQ PDU, for the same AA:

        #. a new record is created, updating the ChM field.

        #. the valid-from timestamp is determined by the Instant parameter of the LL_CHANNEL_MAP_REQ PDU.

        #. the other parameters in this record are populated with those values already considered current.

        #. at the indicated instant, capture tool considers the updated ChM current for the indicated AA.

It is noted that an LE packet capture may contain all the information necessary to synthesize these records. Therefore, these records may be created during capture or afterwards, as a post-processing step. In the latter case, a classic PCAP file may be converted to PCAPNG. 



Option Structure
++++++++++++++++

::

	 0                   1                   2                   3
	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|            0xd380             |              42               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	|                    valid from timestamp                       |
	|                                                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	|            InitA              +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                               |                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+             AdvA              |
	|                                                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                              AA                               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                     CRCInit                   |    WinSize    |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|            WinOffset          |           Interval            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|             Latency           |           Timeout             |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                                                               |
	|    ChM        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|               |   Hop+SCA     |             pad               |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+



Description
+++++++++++

The definition of the fields are found in the Bluetooth specification Volume 6, Part B, Sections 2.3.3.1, 2.4.2.1, and 2.4.2.2. 



C Structure
+++++++++++

::

	typedef struct _le_ll_connection_info {
	        uint64_t valid_from_ts;
	        uint8_t  InitA[6];
	        uint8_t  AdvA[6];
	        uint32_t AA;
	        uint8_t  CRCInit[3];
	        uint8_t  WinSize;
	        uint16_t WinOffset;
	        uint16_t Interval;
	        uint16_t Latency;
	        uint16_t Timeout;
	        uint8_t  ChM[5];
	        uint8_t  HopAndSCA;
	} le_ll_connection_info;



Under Development
^^^^^^^^^^^^^^^^^

* BR/EDR

    * link-key info: link-key (16 bytes) + 2 BR_ADDR (12 bytes) + 2 timestamps (16 bytes)

    * E0 encryption info: BR_ADDR (6 bytes) + 2 timestamps (16 bytes) + keylen (1 bytes) + key (N bytes)

    * AES-CCM session info: session key (16 bytes) + nonce (13 bytes) + BR_ADDR (6 bytes) + 2 timestamps (16 bytes)

* LE

    * AES-CCM session info: session key (16 bytes) + nonce (13 bytes) + AA (4 bytes) + 2 timestamps (16 bytes)



Allocated DLTs
~~~~~~~~~~~~~~

Common to the following pseudoheaders:

    * mandatory fields:

        * a rf_channel field, although channels differ between BR/EDR and LE.

        * a flags field that indicates which optional fields are present, and other boolean metadata.

    * optional fields:

        * signal power and noise power, probably used by more sophisticated capture tools.

The remaining fields are specific to the BR/EDR and LE capture process, including the packet quality indicators mentioned above. 



LINKTYPE_BLUETOOTH_BREDR_BB 
^^^^^^^^^^^^^^^^^^^^^^^^^^^

* only covers BR/EDR baseband packets, Bluetooth spec Vol.2 Part B.

* each packet includes a packed pseudoheader described below, optionally followed by the decoded BR/EDR baseband PAYLOAD.

    * here decoded is used in the same sense as Bluetooth spec Vol 2 Part B Section 7.

    * BR/EDR PAYLOAD formats are described in Bluetooth spec Vol 2 Part B Section 6.1 and 6.6.

* the packet SYNC WORD (sec 6.3) is not stored, rather the LAP is recovered and stored in the pseudoheader.

* the packet HEADER (sec 6.4) is decoded and stored in the pseudoheader.

* further downstream receiver processing may or may not be performed on the PAYLOAD.

    * refer to figure 7.2 of Bluetooth spec Vol 2 Part B.

    * de-whitening, CRC checking, decryption, and MIC checking, are optional.

    * flags in the pseudo-header indicate which of these have been performed.

    * none of these optional processing steps affect the length of the stored packet PAYLOAD.



Packet Structure
++++++++++++++++

::

	+---------------------------+
	|         RF Channel        |
	|         (1 Octet)         |
	+---------------------------+
	|        Signal Power       |
	|         (1 Octet)         |
	+---------------------------+
	|        Noise Power        |
	|         (1 Octet)         |
	+---------------------------+
	|    Access Code Offenses   |
	|         (1 Octet)         |
	+---------------------------+
	|   Payload Transport Rate  |
	|         (1 Octet)         |
	+---------------------------+
	|   Corrected Header Bits   |
	|         (1 Octet)         |
	+---------------------------+
	|  Corrected Payload Bits   |
	|        (2 Octets)         |
	+---------------------------+
	|    Lower Address Part     |
	|        (4 Octets)         |
	+---------------------------+
	|       Reference LAP       |
	|        (3 Octets)         |
	+---------------------------+
	|       Reference UAP       |
	|         (1 Octet)         |
	+---------------------------+
	|     BT Packet Header      |
	|        (4 Octets)         |
	+---------------------------+
	|          Flags            |
	|        (2 Octets)         |
	+---------------------------+
	|    BR or EDR Payload      |
	.                           .
	.                           .
	.                           .



Description
+++++++++++

All multi-octet fields are expressed in little-endian format. Fields with a corresponding **Flags** bit are only considered valid when the bit is set.

The **RF Channel** field ranges 0 to 78. It reflects the value described in the Bluetooth specification Volume 2, Part A, Section 2.

The **Signal Power** and **Noise Power** fields are signed integers expressing values in dBm.

The **Access Code Offenses** field is an unsigned integer indicating the number of deviations from the valid access code that led to the packet capture. Access codes are interpreted as described in Bluetooth specification Volume 2, Part B, Section 6.3.

The **Payload Transport Rate** field represents a column of Bluetooth specification Volume 2, Part B, Section 6.5, Table 6.2, and is interpreted as two nibbles as follows.

    * 0x.0 indicates the BT payload was BR and captured with GFSK demodulation

    * 0x.1 indicates the BT payload was EDR and captured with PI/2-DQPSK demodulation

    * 0x.2 indicates the BT payload was EDR and captured with 8DPSK demodulation

    * 0x0. indicates the packet logical transport is any (link parameters unknown)

    * 0x1. indicates the packet logical transport is SCO

    * 0x2. indicates the packet logical transport is eSCO

    * 0x3. indicates the packet logical transport is ACL

    * 0x4. indicates the packet logical transport is CSB

    * 0xff indicates this is an ID packet so BT Packet Header is ignored and there is no payload

All other values of the **Payload Transport Rate** field are reserved.

The **Corrected Header Bits** field is an unsigned integer indicating the number of corrected bits in the 18-bit **BT Packet Header**. The valid range is 0 to 18.

The **Corrected Payload Bits** field is a signed integer indicating the number of errored and corrected bits in the captured BT payload. Interpretation of this field corresponds to the **Payload Transport Rate**. The value ranges from 0 to 80 when the BT payload was captured at R=1/3 as per Bluetooth specification Volume 2, Part B, Section 7.4. The value ranges from -360 to +180 when the BT payload was captured at R=2/3 as per Bluetooth specification Volume 2, Part B, Section 7.5. A negative number indicates the field absolute value is the sum of the number of corrected and uncorrectable bits.

The **Lower Address Part** field is the 24-bit value recovered from the captured SYNC WORD as defined in Bluetooth specification Volume 2, Part B, Section 6.3.3. The most significant byte of this field is reserved and must be zero.

The **Reference LAP** field corresponds to the **Lower Address Part** configured into the capture tool that led to the capture of this packet.

The **Reference UAP** field corresponds to the **Upper Address Part** configured into the capture tool and corresponds to the **Reference LAP**.

The **BT Packet Header** field is the 18-bit value recovered from the packet capture, and is defined in Bluetooth specification Volume 2, Part B, Section 6.4. The most significant 14 bits are reserved and must be zero.

The **Flags** field represents packed bits defined as follows.

    * 0x0001 indicates the **BT Packet Header** and **BR or EDR Payload** are de-whitened.

    * 0x0002 indicates the **Signal Power** field is valid.

    * 0x0004 indicates the **Noise Power** field is valid.

    * 0x0008 indicates the **BR or EDR Payload** is decrypted

    * 0x0010 indicates the **Reference LAP** is valid and led to this packet being captured

    * 0x0020 indicates the **BR or EDR Payload** is present and follows this field

    * 0x0040 indicates the **RF Channel** field is subject to aliasing

    * 0x0080 indicates the **Reference UAP** field is valid for HEC and CRC checking

    * 0x0100 indicates the HEC portion of the **BT Packet Header** was checked

    * 0x0200 indicates the HEC portion of the **BT Packet Header** passed its check

    * 0x0400 indicates the CRC portion of the **BR or EDR Payload** was checked

    * 0x0800 indicates the CRC portion of the **BR or EDR Payload** passed its check

    * 0x1000 indicates the MIC portion of the decrypted **BR or EDR Payload** was checked

    * 0x2000 indicates the MIC portion of the decrypted **BR or EDR Payload** passed its check

All other bit positions of the **Flags** field are reserved and must be zero.

The decoded **BR or EDR Payload** optionally follows the previous fields, and is formatted as detailed in Bluetooth specification Volume 2, Part B, Section 6. The packet is decoded per Bluetooth specification Volume 2, Part B, Section 7. All multi-octet values in the **BR or EDR Payload** are always expressed in little-endian format, as is the normal Bluetooth practice. 



C Structure
+++++++++++

::

	typedef struct _pcap_bluetooth_bredr_bb_header {
	        uint8_t rf_channel;
	        int8_t signal_power;
	        int8_t noise_power;
	        uint8_t access_code_offenses;
	        uint8_t payload_transport_rate;
	        uint8_t corrected_header_bits; 
	        int16_t corrected_payload_bits;
	        uint32_t lap;
	        uint32_t ref_lap_uap;
	        uint32_t bt_header;
	        uint16_t flags;
	        uint8_t  br_edr_payload[0];
	} pcap_bluetooth_bredr_bb_header;



LINKTYPE_BLUETOOTH_LE_LL_WITH_PHDR 
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* supplements DLT_BLUETOOTH_LE_LL which already exists but is not used for RF captures.

* only covers LE link layer packets, Bluetooth spec Vol.6 Part B.

* each packet includes a packed pseudoheader described below, followed by the LE link-layer packet consisting of ACCESS ADDRESS, PDU, and CRC, but excluding the PREAMBLE.

    * reference Bluetooth spec Vol.6 Part B sec 2 for formatting.

* not all receiver processing need be performed at capture time.

    * refer to figures 3.1 of Bluetooth spec Vol 6 Part B.

    * de-whitening, CRC checking, decryption, and MIC checking, are optional.

    * flags in the pseudo-header indicate which of these have been performed.

    * none of these optional processing steps affect the length of the stored packet data.



Packet Structure
++++++++++++++++

::

	+---------------------------+
	|         RF Channel        |
	|         (1 Octet)         |
	+---------------------------+
	|        Signal Power       |
	|         (1 Octet)         |
	+---------------------------+
	|        Noise Power        |
	|         (1 Octet)         |
	+---------------------------+
	|  Access Address Offenses  |
	|         (1 Octet)         |
	+---------------------------+
	| Reference Access Address  |
	|        (4 Octets)         |
	+---------------------------+
	|          Flags            |
	|        (2 Octets)         |
	+---------------------------+
	|  LE Packet (no preamble)  |
	.                           .
	.                           .
	.                           .


Description
+++++++++++

All multi-octet fields are expressed in little-endian format. Fields with a corresponding **Flags** bit are only considered valid when the bit is set.

The **RF Channel** field ranges 0 to 39. It reflects the value described in the Bluetooth specification Volume 6, Part A, Section 2.

The **Signal Power** and **Noise Power** fields are signed integers expressing values in dBm.

The **Access Address Offenses** field is an unsigned integer indicating the number of deviations from the valid access address that led to the packet capture. Access addresses are interpreted as described in Bluetooth specification Volume 6, Part B, Section 2.1.2.

The **Reference Access Address** field corresponds to the Access Address configured into the capture tool that led to the capture of this packet.

The **Flags** field represents packed bits defined as follows.

    * 0x0001 indicates the **LE Packet** is de-whitened.

    * 0x0002 indicates the **Signal Power** field is valid.

    * 0x0004 indicates the **Noise Power** field is valid.

    * 0x0008 indicates the **LE Packet** is decrypted.

    * 0x0010 indicates the **Reference Access Address** is valid and led to this packet being captured.

    * 0x0020 indicates the **Access Address Offenses** field contains valid data.

    * 0x0040 indicates the **RF Channel** field is subject to aliasing.

    * 0x0400 indicates the CRC portion of the **LE Packet** was checked.

    * 0x0800 indicates the CRC portion of the **LE Packet** passed its check.

    * 0x1000 indicates the MIC portion of the decrypted **LE Packet** was checked.

    * 0x2000 indicates the MIC portion of the decrypted **LE Packet** passed its check.

All other bit positions of the **Flags** field are reserved and must be zero.

The **LE Packet** follows the previous fields, and is formatted as detailed in Bluetooth specification Volume 6, Part B, Section 2, but does not include the preamble. All multi-octet values in the **LE Packet** are always expressed in little-endian format, as is the normal Bluetooth practice. 



C Structure
+++++++++++

::

	typedef struct _pcap_bluetooth_le_ll_header {
	        uint8_t rf_channel;
	        int8_t signal_power;
	        int8_t noise_power;
	        uint8_t access_address_offenses;
	        uint32_t ref_access_address;
	        uint16_t flags;
	        uint8_t le_packet[0];
	} pcap_bluetooth_le_ll_header;