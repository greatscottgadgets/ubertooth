# UBERTOOTH-AFH 1 "March 2017" "Project Ubertooth" "User Commands"

## NAME

ubertooth-afh(1) - passive detection of AFH channel map

## SYNOPSIS

    ubertooth-afh -u <uap> -l <lap> [-r] [-m <seconds>]

## DESCRIPTION

Classic Bluetooth piconets commonly make use of Adaptive Frequency
Hopping (AFH) to avoid interference with other signals in 2.4 GHz. This
tool attempts to determine the AFH map being used for a given piconet
identified by UAP and LAP.

At a minimum you must provide UAP and LAP. See ubertooth-rx(1) for a
method for sniffing these over the air and details on where else to find
them. By default the tool will only print the AFH map when it is
updated. By providing the `-r` flag, the AFH map will be printed out in
binary form once per second.

## OPTIONS

 - `-l <LAP>` :
   LAP of target piconet (3 bytes / 6 hex digits)
 - `-u <UAP>` :
   UAP of target piconet (1 byte / 2 hex digits)
 - `-m <int>` :
   threshold for channel removal (default: 5)
 - `-r` :
   print AFH channel map once every second (default: print on update)

Other options:

 - `-t <seconds>` :
   timeout for initial AFH map detection (not required)
 - `-e <0-4>` :
   maximum access code errors (default: 2, range: 0-4)
 - `-V` :
   print version information
 - `-U <0-7>` :
   set ubertooth device to use

## SEE ALSO

ubertooth-rx(1): a tool for finding UAP and LAP

ubertooth(7): overview of Project Ubertooth

## AUTHOR

This manual page was written by Mike Ryan.

## COPYRIGHT

ubertooth-afh(1) is Copyright (c) 2015-2017. This tool is released under the
GPLv2. Refer to `COPYING` for further details.
