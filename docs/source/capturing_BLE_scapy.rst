
# Capturing BLE in scapy
Use ubertooth-btle with -q and stream to a file.

[Workaround](https://github.com/secdev/scapy/issues/2764#issuecomment-674387163) the dilemma of reaching EOF due to scapy defaults in a low traffic area.

## Rough concept
In a terminal, run ubertooth-btle with -f, -n or -p.  Use -q for the pcap output:
```
ubertooth-btle -f -q /tmp/pipe
```
In another terminal, open python and run:
```
from scapy.all import *

class Reader(PcapReader):
    def read_packet(self, size = MTU):
        try:
            return super(Reader, self).read_packet(size)
        except EOFError:
            return None


p = sniff(opened_socket=Reader('/tmp/pipe'), prn = lambda x: x.summary())

```

## Takeaways from the concept
- p is now a list of the packets captured.  This object is usable if you crtl+c
  within a Python IDE such as [ipython](https://ipython.org/)
- Save RAM by using prn to do something other than print to stdout; set store = 0
- ubertooth-btle is blocking when invoked via os.system()
- "mount -t tmpfs -o size=10M tmpfs /tmp/foo" <~~~ might just be a friend
