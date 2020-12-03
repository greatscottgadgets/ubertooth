# Hey Emacs, this is a -*- makefile -*-

# Target file name (without extension).
TARGET = btbr

COMPILE_OPTS=-Iinclude
LINKER_OPTS=--specs=nano.specs

# List C source files here. (C dependencies are automatically generated.)
SRC = $(shell find src -name '*.c') \
	$(LIBS_PATH)/usb_serial.c \
	$(LIBS_PATH)/serial_fifo.c \
	$(LIBS_PATH)/LPC17xx_Startup.c \
	$(LIBS_PATH)/LPC17xx_Interrupts.c \
	$(LIBS_PATH)/ubertooth.c \
	$(LPCUSB_PATH)/usbcontrol.c \
	$(LPCUSB_PATH)/usbinit.c \
	$(LPCUSB_PATH)/usbhw_lpc.c \
	$(LPCUSB_PATH)/usbstdreq.c

OPT = 3

include ../common.mk
