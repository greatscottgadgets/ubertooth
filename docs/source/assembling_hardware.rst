===================
Assembling Hardware
===================

The quickest and easiest way to get Ubertooth hardware is to buy it. A list of resellers is available from the `Great Scott Gadgets website <http://greatscottgadgets.com/ubertoothone/>`__. If you choose to build your own hardware, the following steps should help.



Step 0: read these instructions
-------------------------------

Seriously. There are probably things in the later steps that you should know about before getting started.



Step 1: order a PCB
-------------------

You will need a four layer printed circuit board. We recommend using `OSH Park <https://oshpark.com/>`__. Take the Ubertooth One Gerber files from the most recent Project Ubertooth release package (or generate them with KiCad) and send them to your chosen PCB manufacturer. The board is 1.8 square inches, so it will cost ~$18 per set of three. That's one for yourself, one for a friend, and one to screw up! Pogoprog is a two layer board.

If you are building an Ubertooth One, you should also consider building a Pogoprog unless you already have a plan for how you will `program <https://github.com/greatscottgadgets/ubertooth/wiki/Programming>`__ your board.



Step 2: order a stencil
-----------------------

Surface mount soldering is fun and easy if you use a stencil to apply solder paste to your circuit board! Send the top paste Gerber file (ubertooth-one-SoldP_Front.gtp) to a stencil manufacturer such as `OHARARP <http://ohararp.com/stencils>`__, `OSH Stencils <https://www.oshstencils.com/>`__, or `Pololu <https://www.pololu.com/product/446>`__. Alternatively you might plan to use a syringe or a toothpick to apply solder paste, but this is not recommended. You might instead just use a soldering iron, but this is strongly discouraged unless you have successfully soldered QFNs with required ground pads before (and, if you have, you probably aren't reading these instructions anyway).



Step 3: order the parts
-----------------------

Take the bill of materials (bom) from the most recent Project Ubertooth release (or generate it with KiCad) and order the parts. The parts should all be available from one or more online electronics suppliers such as `Mouser <http://www.mouser.com/>`__ or `Digi-key <http://www.digikey.com/>`__. It is important to order some extra parts (especially the tiny ones which fortunately are cheap) in case you lose or damage any components.

You may want to order an antenna too. The `Pulse W1030 <http://www.pulseelectronics.com/products/old_antennas/products__solutions/antennas_for_wireless_devices/wd_antennas/w1030_external_24_ghz_high_gain_short>`__ is a nice size, but you can also find compatible antennas on many commercial Wi-Fi and Bluetooth products. Most any antenna intended for the 2.4 GHz band (such as 802.11b/g/n) is suitable as long as it has an RP-SMA connector, adapter, or pigtail. You could choose an SMA connector instead of RP-SMA; this might especially be convenient for interfacing with benchtop test gear. RP-SMA was selected as the default choice for Project Ubertooth because there are more RP-SMA than SMA antennas floating around on consumer Wi-Fi and Bluetooth gear.

You might prefer to select alternative parts, but be careful of the 1% resistors and all of the 0402 inductors and capacitors in the RF section which have been selected for their particular characteristics. Any LPC175x microcontroller will do, but it is recommended that you choose one with at least 128 kB RAM. And, really, if you're going through all this trouble, why not go with 512 kB?



Step 4: prepare your tools and materials
----------------------------------------

Essential:

    * an electric skillet, one that you don't intend to use for food ever again
    * solder paste (no-clean lead-based solder paste is recommended)
    * a small putty knife or razor blade
    * fine tipped tweezers
    * any soldering iron
    * solder

Strongly recommended:

    * good ventilation
    * a temperature controlled soldering iron: this is more than just having a knob; it should have a temperature sensor in the iron
    * an embossing tool or other high temperature heat gun (even better: a proper hot air rework station)
    * a multitester with LED/diode test mode
    * desoldering braid
    * brass sponge
    * helping hands
    * magnifying glass



Step 5: apply solder paste
--------------------------

Using your stencil and a putty knife, apply the solder paste as described in this `tutorial <https://www.sparkfun.com/tutorials/58>`__.



Step 6: place the parts
-----------------------

With fine tipped tweezers, carefully place the parts on the board. If you have to move a part, pick it up and place it again rather than sliding it around a lot. Otherwise the paste can get out of place. Most of the 0402 and 0603 parts can be placed in either direction, but the LEDs are exceptions. You must place them with ground in the direction of the arrow on the circuit board. You may have to look at the design in KiCad to see which way the arrow goes, and you'll probably have to test your LEDs with a multitester to find out which side is which. Don't populate USB connectors, RP-SMA connectors, or pogo pins (in the case of Pogoprog) at this time.



Step 7: reflow
--------------

Carefully place the board in the electric skillet, and turn the skillet on. It is best to warm up the board to a moderate temperature before turning the skillet up to full power. Then turn up the heat until you can see the solder flow. If you see parts moving around to incorrect positions, resist the temptation to correct them at this time! As soon as the solder everywhere on the board appears liquid, cut the power completely. You may want to lift the board out of the skillet with a spatula at this point to allow it to cool faster. There is a danger of overheating the components, but this is unlikely unless you left the skillet on longer than necessary or used lead-free solder paste.



Step 8: rework
--------------

Here is where the embossing tool, a good soldering iron, desoldering braid, and a magnifying glass come in very handy. If there is anything wrong with the assembly, you will have to correct individual part placement as needed.



Step 9: inspection
------------------

Once all the parts appear to be soldered in place correctly, look again, this time with a magnifying glass. You should also do some continuity tests with a multitester at this point. Watch out in particular for supply shorts; the easiest way to test for these is to verify a lack of continuity across bypass capacitors (all the caps that are close to the ICs). If there is a short that you can't see, it is probably under the pins of one of the ICs. Repeat steps 7 and 8 as necessary.



Step 10: hand soldering
-----------------------

There are a few parts that you should solder on by hand with an iron at this point. These are the USB and RP-SMA connectors on the Ubertooth boards and the pogo pins on Pogoprog.



Step 11: power-on test
----------------------

Power on the device by plugging in the USB connection. An Ubertooth One or Zero should illuminate the RST LED. If this doesn't happen, quickly unplug USB verify that the LED is oriented correctly, and go back to step 9. A Pogoprog should flash its TX and RX LEDs during USB enumeration. If this doesn't happen, quickly unplug USB, verify the LED orientations, check your driver situation, and go back to step 8.



Step 12: further testing
------------------------

If you are building a Pogoprog, you should make sure that an FTDI USB serial adapter has been detected by your host operating system. If so, you can try using it to `program <https://github.com/greatscottgadgets/ubertooth/wiki/Programming>`__ an Ubertooth board. If you are making an Ubertooth board, you should follow the procedure in firmware/assembly_test/README.



Step 13: boast
--------------

Tell us about your success on the Great Scott Gadgets `Discord <https://discord.gg/rsfMw3rsU8>`__. 