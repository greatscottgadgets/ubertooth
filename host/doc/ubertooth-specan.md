# UBERTOOTH-SPECAN 1 "March 2017" "Project Ubertooth" "User Commands"

## NAME

ubertooth-specan(1) - a spectrum analyzer for Ubertooth

## SYNOPSIS

ubertooth-specan [-l <freq>] [-u <freq>] [-g|G]

## DESCRIPTION

ubertooth-specan(1) is a tool for using Ubertooth as a spectrum analyzer
in the 2.4GHz band. It is often used as a helper for GUI tools, such as
ubertooth-specan-ui or Spectools. It can also produce output suitable for
user with the feedgnuplot tool.

Other options allow upper and lower bounds to be set on the frequencies
monitored. Refer to the [OPTIONS][] section for full details.

## OPTIONS

Options:

 - `-l` :
   lower frequency (default 2402)
 - `-u` :
   upper frequency (default 2480)
 - `-g` :
   format output for feedgnuplot
 - `-G` :
   format output for 3D feedgnuplot
 - `-d<filename>` :
   output to file <filename>
 - `-v` :
   print verbose output to stderr
 - `-U<0-7>` :
   set ubertooth device to use

##EXAMPLES

To monitor the 2.4GHz band and produce a file suitable for feedgnuplot:

   ubertooth-specan -g -d output.dat

To monitor the upper half of the 2.4GHz band, use:

   ubertooth-specan -l 2440

## SEE ALSO

ubertooth(7): overview of Project Ubertooth

## AUTHOR

This manual page was written by Dominic Spill.

## COPYRIGHT

ubertooth-specan(1) is Copyright (c) 2010-2017. This tool is released under the
GPLv2. Refer to `COPYING` for further details.
