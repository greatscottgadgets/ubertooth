======================
Ubertooth Two Wishlist
======================

At some point the supply of CC2400 ICs will dry up and we will need to design and build a replacement for Ubertooth. The design goals are the same as Ubertooth One, but it would be nice to add some additional features if we can. This page is for a wishlist of features.

MCU candidates:

    * STM32F2

    * LPCxxxx (Which? 18xx? 43xx? 17xx?)

RF candidates:

    * ADF7242

    * CC2541

    * TRC104 - Fixed FSK deviation - unsuitable for BLE

    * Amiccom A7137 - Not easy to source



Wishlist
~~~~~~~~

* Improved MCU

	* USB peripheral with 512 byte bulk endpoints

    * AES peripheral - ITAR may be an issue

Note: LPC185x, LPC183x, and LPC182x MCUs have a high speed USB peripheral capable of 512 byte bulk endpoints. Reference: LPC18xx User Manual (`UM10430 (PDF) <http://www.nxp.com/documents/user_manual/UM10430.pdf>`__) Chapter 22 Section 22.4.5, PDF page 520.

Room for an SD expansion - preferably using an MCU with SDIO.

Lots of randomly blinking LEDs.

* a more sophisticated transceiver capable of handling EDR payloads
