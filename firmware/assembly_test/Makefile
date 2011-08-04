# Hey Emacs, this is a -*- makefile -*-

# Target file name (without extension).
TARGET = assembly_test

# List C source files here. (C dependencies are automatically generated.)
SRC = $(TARGET).c \
	$(LIBS_PATH)/usb_serial.c \
	$(LIBS_PATH)/serial_fifo.c \
	$(LIBS_PATH)/LPC17xx_Startup.c \
	$(LIBS_PATH)/LPC17xx_Interrupts.c \
	$(LIBS_PATH)/ubertooth.c \
	$(LPCUSB_PATH)/usbcontrol.c \
	$(LPCUSB_PATH)/usbinit.c \
	$(LPCUSB_PATH)/usbhw_lpc.c \
	$(LPCUSB_PATH)/usbstdreq.c

# The assembly_test firmware should be installed via ISP or JTAG without a
# bootloader present.  After the test succeeds, install the bootloader with ISP
# or JTAG.
LINKER_SCRIPT = LPC17xx_Linker_Script_without_bootloader.ld

include ../common.mk
