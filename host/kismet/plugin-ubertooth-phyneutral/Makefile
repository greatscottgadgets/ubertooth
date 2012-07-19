KIS_SRC_DIR ?= /usr/src/kismet
KIS_INC_DIR ?= $(KIS_SRC_DIR)
RXTX_SRC_DIR ?= ../../bluetooth_rxtx
RXTX_INC_DIR ?= $(RXTX_SRC_DIR)
LIBDIR ?= /lib

include $(KIS_SRC_DIR)/Makefile.inc

BLDHOME	= .
top_builddir = $(BLDHOME)

PLUGINLDFLAGS ?= $(LDFLAGS)
PLUGINLDFLAGS += -shared -rdynamic
LIBS	+= -lstdc++ -lusb-1.0 -lpthread -lbtbb -lubertooth
CFLAGS	?= $(CFLAGS) $(CPPFLAGS)
CFLAGS	+= -I$(KIS_INC_DIR) -I$(RXTX_INC_DIR) -g -fPIC

SRVOBJS = packetsource_ubertooth.o packet_btbb_types.o \
			phy_btbb.o kismet_ubertooth.o \
			$(RXTX_SRC_DIR)/ubertooth.o
SRVOUT	= ubertooth.so

all:	$(SRVOUT)

$(SRVOUT):	$(SRVOBJS)
	$(CC) $(PLUGINLDFLAGS) $(CFLAGS) $(SRVOBJS) -o $(SRVOUT) $(LIBS)

install:	$(SRVOUT)
	mkdir -p $(DESTDIR)$(prefix)/$(LIBDIR)/kismet/
	$(INSTALL) -o $(INSTUSR) -g $(INSTGRP) -m 644 $(SRVOUT) $(DESTDIR)$(prefix)/$(LIBDIR)/kismet/$(SRVOUT)

userinstall:	$(SRVOUT)
	mkdir -p ${HOME}/.kismet/plugins/
	$(INSTALL) -v $(SRVOUT) ${HOME}/.kismet/plugins/$(SRVOUT)

clean:
	@-rm -f *.o
	@-rm -f *.so

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $@ 

.cc.o:
	$(CXX) $(CFLAGS) -c $*.cc -o $@ 

.SUFFIXES: .c .cc .o
