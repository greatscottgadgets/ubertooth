#!/usr/bin/python
# -*- coding: utf-8 -*-
# Copyright 2008, 2009 Dominic Spill, Michael Ossmann
"""
Bluetooth monitoring utility.
Receives samples from USRP, file (as created by usrp_rx_cfile.py), or standard input.
If LAP is unspecified, LAP detection mode is enabled.
If LAP is specified without UAP, UAP detection mode is enabled.
If both LAP and UAP are specified, sniffing mode is enabled.
"""

from gnuradio import gr, eng_notation, blks2

from gnuradio import bluetooth
from gnuradio.eng_option import eng_option
from optparse import OptionParser

class my_top_block(gr.top_block):

	def __init__(self):
		gr.top_block.__init__(self)

		usage="%prog: [options]"
		parser = OptionParser(option_class=eng_option, usage=usage)
		parser.add_option("-N", "--nsamples", type="eng_float", default=None,
						help="number of samples to collect [default=+inf]")
		parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=(0, 0),
						help="select USRP Rx side A or B (default=A)")
		parser.add_option("-b", "--bitstream", action="store_true", default=None,
						help="input data from file is already demodulated\n(Not compatible with USRP use")
		parser.add_option("-c", "--ddc", type="string", default="0",
						help="comma separated list of ddc frequencies (default=0)")
		parser.add_option("-d", "--decim", type="int", default=32,
						help="set fgpa decimation rate to DECIM (default=32)") 
		parser.add_option("-e", "--interface", type="string", default="eth0",
						help="use specified Ethernet interface for USRP2 [default=%default]")
		parser.add_option("-f", "--freq", type="eng_float", default=0,
						help="set USRP frequency to FREQ", metavar="FREQ")
		parser.add_option("-g", "--gain", type="eng_float", default=None,
						help="set USRP gain in dB (default is midpoint)")
		parser.add_option("-i", "--input-file", type="string", default=None,
						help="use named input file instead of USRP")
		parser.add_option("-l", "--lap", type="string", default=None,
						help="LAP of the master device")
		parser.add_option("-m", "--mac-addr", type="string", default="",
						help="use USRP2 at specified MAC address [default=None]")
		parser.add_option("-n", "--channel", type="int", default=None,
						help="channel number for hop reversal (0-78) (default=None)") 
		parser.add_option("-r", "--sample-rate", type="eng_float", default=None,
						help="sample rate of input (default: use DECIM)")
		parser.add_option("-s", "--input-shorts", action="store_true", default=False,
						help="input interleaved shorts instead of complex floats")
		parser.add_option("-t", "--squelch", type="eng_float", default=None,
						help="power squelch threshold in dB (default=None)")
		parser.add_option("-u", "--uap", type="string", default=None,
						help="UAP of the master device")
		parser.add_option("-w","--wireshark", action="store_true", default=False,
						help="direct output to a tun interface")
		parser.add_option("-2","--usrp2", action="store_true", default=False,
						help="use USRP2 (or file originating from USRP2) instead of USRP")

		(options, args) = parser.parse_args ()
		if len(args) != 0:
			parser.print_help()
			raise SystemExit, 1

		if options.bitstream:
			if options.input_file is None:
				print "Error: bitstream processing requires file input\n"
				raise SystemExit, 1

		# Bluetooth operates at 1 million symbols per second
		symbol_rate = 1e6

		# the demodulator needs at least two samples per symbol
		min_samples_per_symbol = 2
		min_sample_rate = symbol_rate * min_samples_per_symbol

		# use options.sample_rate unless not provided by user
		if options.sample_rate is None:
			if options.usrp2:
				# original source is USRP2
				adc_rate = 100e6
			else:
				# assume original source is USRP
				adc_rate = 64e6
			options.sample_rate = adc_rate / options.decim

		# make sure we have a high enough sample rate
		if options.sample_rate < min_sample_rate:
			raise ValueError, "Sample rate (%d) below minimum (%d)\n" % (options.sample_rate, min_sample_rate)

		if options.input_shorts:
			input_size = gr.sizeof_short
		else:
			input_size = gr.sizeof_gr_complex

		if options.bitstream:
			input_size = gr.sizeof_char

		# select input source
		if options.input_file is None:
			# input from USRP or USRP2
			if options.usrp2:
				from gnuradio import usrp2
				src = usrp2.source_32fc(options.interface, options.mac_addr)
				print "Using RX board id 0x%04X" % (src.daughterboard_id(),)
				src.set_decim(options.decim)
				r = src.set_center_freq(options.freq)
				if r == None:
					raise SystemExit, "Failed to set USRP2 frequency"
				if options.gain is None:
					# if no gain was specified, use the mid-point in dB
					g = src.gain_range()
					options.gain = float(g[0]+g[1])/2
				src.set_gain(options.gain)
			else:
				from gnuradio import usrp
				src = usrp.source_c(decim_rate=options.decim)
				subdev = usrp.selected_subdev(src, options.rx_subdev_spec)
				print "Using RX board %s" % (subdev.side_and_name())
				r = src.tune(0, subdev, options.freq)
				if not r:
					raise SystemExit, "Failed to set USRP frequency"
				if options.gain is None:
					# if no gain was specified, use the mid-point in dB
					g = subdev.gain_range()
					options.gain = float(g[0]+g[1])/2
				subdev.set_gain(options.gain)
		elif options.input_file == '-':
			# input from stdin
			src = gr.file_descriptor_source(input_size, 0)
		else:
			# input from file
			src = gr.file_source(input_size, options.input_file)

		# stage 1: limit input to desired number of samples
		if options.nsamples is None:
			stage1 = src
		else:
			head = gr.head(input_size, int(options.nsamples))
			self.connect(src, head)
			stage1 = head
	
		# stage 2: convert input from shorts if necessary
		if options.input_shorts:
			s2c = gr.interleaved_short_to_complex()
			self.connect(stage1, s2c)
			stage2 = s2c
		else:
			stage2 = stage1

		# stage 3: power squelch
		if options.squelch is None:
			stage3 = stage2
		else:
			squelch = gr.pwr_squelch_cc(options.squelch, 0.01, 0, True)
			self.connect(stage2, squelch)
			stage3 = squelch

		# coefficients for filter to select single channel
		channel_filter = gr.firdes.low_pass(1.0, options.sample_rate, 500e3, 500e3, gr.firdes.WIN_HANN)

		# we will decimate by the largest integer that results in enough samples per symbol
		decimation_rate = int(options.sample_rate/(min_sample_rate))
		samples_per_symbol = (options.sample_rate/decimation_rate)/symbol_rate

		# look for packets on each channel specified by options.ddc
		# this works well for a small number of channels, but a more efficient
		# method should be possible for a large number of contiguous channels.
		channel = 0
		for ddc_option in options.ddc.split(","):
			if options.bitstream is None:
				ddc_freq = int(eng_notation.str_to_num(ddc_option))

				# digital downconverter
				# does three things:
				# 1. converts frequency so channel of interest is centered at 0 Hz
				# 2. filters out everything outside the channel
				# 3. downsamples to 2 Msps (2 samples per symbol) or so
				ddc = gr.freq_xlating_fir_filter_ccf(decimation_rate, channel_filter, -ddc_freq, options.sample_rate)

				# GMSK demodulate baseband to bits
				demod = blks2.gmsk_demod(mu=0.32, samples_per_symbol=samples_per_symbol)
				self.connect(stage3, ddc)
				self.connect(ddc, demod)
			else:
				# Skip demod if using a bitstream (it's already demodulated)
				demod = stage3
				ddc_freq = 1

			# bluetooth decoding
			if options.lap is None:
				if options.wireshark:
					dst = bluetooth.tun(ddc_freq, channel)
					channel = channel + 1
				else:
					# print out LAP for every frame detected
					dst = bluetooth.LAP(ddc_freq)
			else:
				if options.uap is None:
					if options.channel is None:
						# determine UAP from frames matching the user-specified LAP
						dst = bluetooth.UAP(int(options.lap, 16))
					else:
						# determine UAP and clock
						dst = bluetooth.hopper(int(options.lap, 16), options.channel)
				else:
					# sniffer mode
					dst = bluetooth.sniffer(int(options.lap, 16), int(options.uap, 16))
		
			# connect the blocks
			self.connect(demod, dst)

if __name__ == '__main__':
	try:
		my_top_block().run()
	except KeyboardInterrupt:
		pass
