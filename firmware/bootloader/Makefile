# Hey Emacs, this is a -*- makefile -*-

# Target file name (without extension).
TARGET = bootloader

# List C source files here. (C dependencies are automatically generated.)
SRC = $(LIBS_PATH)/LPC17xx_Startup.c \
      $(LIBS_PATH)/LPC17xx_Interrupts.c \
      $(LIBS_PATH)/ubertooth.c \
      $(LPCUSB_PATH)/usbcontrol.c \
      $(LPCUSB_PATH)/usbinit.c \
      $(LPCUSB_PATH)/usbhw_lpc.c \
      $(LPCUSB_PATH)/usbstdreq.c

# List C++ source files here. (C dependencies are automatically generated.)
CPPSRC = $(TARGET).cpp \
         dfu.cpp \
         iap.cpp

include ../common.mk

CPPSRC = $(TARGET).cpp \
		dfu.cpp \
		iap.cpp

# Linker Script for the current MCU
LINKER_SCRIPT = LPC17xx_Linker_Script_bootloader.ld
