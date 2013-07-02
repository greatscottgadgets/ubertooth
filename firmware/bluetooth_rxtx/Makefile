# Hey Emacs, this is a -*- makefile -*-

# Target file name (without extension).
TARGET = bluetooth_rxtx

# List C source files here. (C dependencies are automatically generated.)
SRC = $(TARGET).c \
	bluetooth.c \
	bluetooth_le.c \
	ubertooth_usb.c \
	cc2400_rangetest.c \
	$(LIBS_PATH)/usb_serial.c \
	$(LIBS_PATH)/serial_fifo.c \
	$(LIBS_PATH)/LPC17xx_Startup.c \
	$(LIBS_PATH)/LPC17xx_Interrupts.c \
	$(LIBS_PATH)/ubertooth.c \
	$(LPCUSB_PATH)/usbcontrol.c \
	$(LPCUSB_PATH)/usbinit.c \
	$(LPCUSB_PATH)/usbhw_lpc.c \
	$(LPCUSB_PATH)/usbstdreq.c

include ../common.mk
