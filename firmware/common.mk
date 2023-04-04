# Hey Emacs, this is a -*- makefile -*-
#----------------------------------------------------------------------------
# WinAVR Makefile Template written by Eric B. Weddington, Jörg Wunsch, et al.
#  >> Modified for use with the LUFA project. <<
#
# Released to the Public Domain
#
# Altered for ARM Cortex-M3 by Opendous Inc. - 2010-01-05
# Altered for Project Ubertooth by Michael Ossmann 2010
#
# Additional material for this makefile was written by:
# Peter Fleury
# Tim Henigan
# Colin O'Flynn
# Reiner Patommel
# Markus Pfaff
# Sander Pool
# Frederik Rouleau
# Carlos Lamas
# Dean Camera
# Denver Gingerich
# Opendous Inc.
# Michael Ossmann
#
#----------------------------------------------------------------------------
# On command line:
#
# make all = Make software.
#
# make clean = Clean out built project files.
#
# make program = Download the hex file to the device
#
# make doxygen = Generate DoxyGen documentation for the project (must have
#                DoxyGen installed)
#
# make filename.s = Just compile filename.c into the assembler code only.
#
# make filename.i = Create a preprocessed source file for use in submitting
#                   bug reports to the GCC project.
#
# To rebuild project do "make clean" then "make all".
#----------------------------------------------------------------------------

# set to UBERTOOTH_ZERO or UBERTOOTH_ONE
BOARD  ?= UBERTOOTH_ONE

UBERTOOTH_OPTS = -D$(BOARD)

ifeq ($(DISABLE_TX), )
	# comment to disable RF transmission
	UBERTOOTH_OPTS += -DTX_ENABLE
endif

DIRTY = $(shell git status -s --untracked-files=no)
ifneq ($(DIRTY), )
	DIRTY_FLAG = *
endif

# automatic git version when working out of git
GIT_REVISION ?= -D'GIT_REVISION="git-$(shell git log --pretty=format:'%h' -n 1)$(DIRTY_FLAG)"'

# compile info
COMPILE_BY ?= -D'COMPILE_BY="$(shell whoami)"'
COMPILE_HOST ?= -D'COMPILE_HOST="$(shell hostname -s)"'
TIMESTAMP ?= -D'TIMESTAMP="$(shell date)"'

# CPU architecture
CPU = cortex-m3

# Instruction Set to use (also known as CPU Mode)
CPU_MODE = mthumb

# Additional CPU flags
CPU_FLAGS = -mapcs-frame
CPU_FLAGS_ASM = -mthumb-interwork

# Target definition for lpcusb library
LPCUSB_TARGET = LPC17xx

# Output format. (can be srec, ihex, binary)
FORMAT = ihex

# Output Directory
OUTDIR = .

# Library paths
LIBS_PATH = ../common
LPCUSB_PATH = $(LIBS_PATH)/lpcusb/target

# List Assembler source files here.
#     Make them always end in a capital .S.  Files ending in a lowercase .s
#     will not be considered source files but generated files (assembler
#     output from the compiler), and will be deleted upon "make clean"!
#     Even though the DOS/Win* filesystem matches both .s and .S the same,
#     it will preserve the spelling of the filenames, and gcc itself does
#     care about how the name is spelled on its command-line.
ASRC =

# Object files directory
#     To put object files in current directory, use a dot (.), do NOT make
#     this an empty or blank macro!
OBJDIR = .

# Optimization level, can be [0, 1, 2, 3, s].
#     0 = turn off optimization. s = optimize for size.
#     (Note: 3 is not always the best optimization level. See libc FAQ.)
ifeq ($(OPT),)
OPT = s
endif

# Debugging format.
DEBUG = dwarf-2 -g3

# Linker Script for the current MCU
LINKER_SCRIPT ?= LPC17xx_Linker_Script_with_bootloader.ld

# List any extra directories to look for include files here.
#     Each directory must be seperated by a space.
#     Use forward slashes for directory separators.
#     For a directory that has spaces, enclose it in quotes.
EXTRAINCDIRS = $(LIBS_PATH) $(LPCUSB_PATH) ../../host/libubertooth/src

# Compiler flag to set the C Standard level.
#     c89   = "ANSI" C
#     gnu89 = c89 plus GCC extensions
#     c99   = ISO C99 standard (not yet fully implemented)
#     gnu99 = c99 plus GCC extensions
CSTANDARD = -std=gnu99

# Place -D or -U options here for C sources
CDEFS  = -D$(LPCUSB_TARGET) $(UBERTOOTH_OPTS) $(COMPILE_OPTS) -Wa,-a,-ad

# Place -D or -U options here for ASM sources
ADEFS =

# Place -D or -U options here for C++ sources
CPPDEFS = -D$(BOARD) -D$(LPCUSB_TARGET) $(COMPILE_OPTS)

#---------------- Compiler Options C ----------------
#  -g:          generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -alhms...: create assembler listing
CFLAGS = -g
CFLAGS += $(CDEFS)
CFLAGS += -O$(OPT)
CFLAGS += -Wall
CFLAGS += -Wno-unused
CFLAGS += -Wno-comments
CFLAGS += -fmessage-length=0
CFLAGS += -fno-builtin
CFLAGS += -ffunction-sections
CFLAGS += -Wextra
CFLAGS += -D__thumb2__=1
CFLAGS += -msoft-float
CFLAGS += -mno-sched-prolog
CFLAGS += -fno-hosted
CFLAGS += -mtune=cortex-m3
CFLAGS += -march=armv7-m
CFLAGS += -mfix-cortex-m3-ldrd
#CFLAGS += -Wundef
#CFLAGS += -funsigned-char
#CFLAGS += -funsigned-bitfields
#CFLAGS += -fno-inline-small-functions
#CFLAGS += -fpack-struct
#CFLAGS += -fshort-enums
#CFLAGS += -Wstrict-prototypes
#CFLAGS += -fno-unit-at-a-time
#CFLAGS += -Wunreachable-code
#CFLAGS += -Wsign-compare
CFLAGS += -Wa,-alhms=$(<:%.c=$(OBJDIR)/%.lst)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
CFLAGS += $(CSTANDARD)
CFLAGS += $(GIT_REVISION)
CFLAGS += $(COMPILE_BY)
CFLAGS += $(COMPILE_HOST)
CFLAGS += $(TIMESTAMP)

#---------------- Compiler Options C++ ----------------
#  -g*:          generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -alhms...: create assembler listing
CPPFLAGS = -g$(DEBUG)
CPPFLAGS += $(CPPDEFS)
CPPFLAGS += -O$(OPT)
#CPPFLAGS += -funsigned-char
#CPPFLAGS += -funsigned-bitfields
#CPPFLAGS += -fpack-struct
#CPPFLAGS += -fshort-enums
#CPPFLAGS += -fno-exceptions
CPPFLAGS += -Wall
#CFLAGS += -Wundef
#CPPFLAGS += -mshort-calls
#CPPFLAGS += -fno-unit-at-a-time
#CPPFLAGS += -Wstrict-prototypes
#CPPFLAGS += -Wunreachable-code
#CPPFLAGS += -Wsign-compare
CPPFLAGS += -Wa,-alhms=$(<:%.cpp=$(OBJDIR)/%.lst)
CPPFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))
#CPPFLAGS += $(CSTANDARD)
CPPFLAGS += -fno-exceptions
CPPFLAGS += -fno-rtti
CPPFLAGS += -fno-threadsafe-statics
CPPFLAGS += -Wextra
CPPFLAGS += -Weffc++

#---------------- Assembler Options ----------------
#  -Wa,...:   tell GCC to pass this to the assembler.
#  -alhms:   create listing
#  -gstabs:   have the assembler create line number information
#  -listing-cont-lines: Sets the maximum number of continuation lines of hex
#       dump that will be displayed for a given single line of source input.
ASFLAGS = -g$(DEBUG) $(ADEFS) -I.  -alhms=$(<:%.S=$(OBJDIR)/%.lst)

#---------------- Library Options ----------------

# List any extra directories to look for libraries here.
#     Each directory must be seperated by a space.
#     Use forward slashes for directory separators.
#     For a directory that has spaces, enclose it in quotes.
EXTRALIBDIRS = $(LIBS_PATH)

#---------------- Linker Options ----------------
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LDFLAGS = -Wl,-Map=$(TARGET).map $(LINKER_OPTS)
#LDFLAGS += -Wl,--relax
LDFLAGS += -Wl,--gc-sections
LDFLAGS += $(patsubst %,-L%,$(EXTRALIBDIRS))
LDFLAGS += -static
LDFLAGS += -Wl,--start-group
#LDFLAGS += -L$(THUMB2GNULIB) -L$(THUMB2GNULIB2)
LDFLAGS += -lc -lg
LDFLAGS += -lgcc -lm
LDFLAGS += -Wl,--end-group

#---------------- Programming Options ----------------
LPCISP  ?= $(shell which lpc21isp)
PROGDEV ?= /dev/ttyUSB0

#============================================================================

# Define programs and commands.
CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc -T
AS = $(CROSS_COMPILE)as
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
READELF  = $(CROSS_COMPILE)readelf
SIZE = $(CROSS_COMPILE)size
AR = $(CROSS_COMPILE)ar -r
NM = $(CROSS_COMPILE)nm
REMOVE = rm -f

# DFU tool should be ubertooth-dfu
DFU_TOOL ?= ubertooth-dfu

# Define Messages
# English
MSG_BEGIN = -------- begin --------
MSG_END = --------  end  --------
MSG_SIZE_BEFORE = Size before:
MSG_SIZE_AFTER = Size after:
MSG_FLASH = Creating load file for Flash:
MSG_DFU = Creating DFU firmware file:
MSG_EEPROM = Creating load file for EEPROM:
MSG_EXTENDED_LISTING = Creating Extended Listing:
MSG_SYMBOL_TABLE = Creating Symbol Table:
MSG_LINKING = Linking:
MSG_COMPILING = Compiling C:
MSG_COMPILING_CPP = Compiling C++:
MSG_ASSEMBLING = Assembling:
MSG_CLEANING = Cleaning project:
MSG_CREATING_LIBRARY = Creating library:

# Define all object files.
OBJ = $(SRC:%.c=$(OBJDIR)/%.o) $(CPPSRC:%.cpp=$(OBJDIR)/%.o) $(ASRC:%.S=$(OBJDIR)/%.o)

# Define all listing files.
LST = $(SRC:%.c=$(OBJDIR)/%.lst) $(CPPSRC:%.cpp=$(OBJDIR)/%.lst) $(ASRC:%.S=$(OBJDIR)/%.lst)

# Compiler flags to generate dependency files.
GENDEPFLAGS = -MMD -MP -MD

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS = -mcpu=$(CPU) -$(CPU_MODE) $(CPU_FLAGS) -I. $(CFLAGS) $(GENDEPFLAGS)
ALL_CPPFLAGS = -mcpu=$(CPU) -$(CPU_MODE) $(CPU_FLAGS) -I. -x c++ $(CPPFLAGS) $(GENDEPFLAGS)
ALL_ASFLAGS = -mcpu=$(CPU) -$(CPU_MODE) $(CPU_FLAGS_ASM) $(ASFLAGS)
# only difference between Linker flags and CFLAGS is CPU_FLAGS_ASM as -mapcs-frame is not needed
ALL_CFLAGS_LINKER = -mcpu=$(CPU) -$(CPU_MODE) $(CPU_FLAGS_ASM)

# Default target.
all: begin gccversion sizebefore build showtarget sizeafter end

# Change the build target to build a HEX file or a library.
build: elf hex bin srec lss sym dfu
#build: lib

elf: $(TARGET).elf
hex: $(TARGET).hex
bin: $(TARGET).bin
srec: $(TARGET).srec
eep: $(TARGET).eep
lss: $(TARGET).lss
sym: $(TARGET).sym
dfu: $(TARGET).dfu
LIBNAME=lib$(TARGET).a
lib: $(LIBNAME)

# Eye candy.
begin:
	@echo
	@echo $(MSG_BEGIN)

end:
	@echo $(MSG_END)
	@echo

# Display size of file.
ELFSIZE = $(SIZE) $(FORMAT_FLAG) $(TARGET).elf

sizebefore:
	@if test -f $(TARGET).elf; then echo; echo $(MSG_SIZE_BEFORE); $(ELFSIZE); \
	2>/dev/null; echo; fi

sizeafter:
	@if test -f $(TARGET).elf; then echo; echo $(MSG_SIZE_AFTER); $(ELFSIZE); \
	2>/dev/null; echo; fi

showtarget:
	@echo
	@echo --------- Target Information ---------
	@echo ARM Model: $(CPU)
	@echo Board:     $(BOARD)
	@echo --------------------------------------

# Display compiler version information.
gccversion:
	@$(CC) --version

program: $(TARGET).hex
	$(LPCISP) -control $(TARGET).hex $(PROGDEV)  230400 4000

# Create final output files (.hex, .eep) from ELF output file.
%.hex: %.elf
	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O $(FORMAT) $< $@

%.bin: %.elf
	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O binary $< $(TARGET).bin

%.srec: %.elf
	@echo
	@echo $(MSG_FLASH) $@
	$(OBJCOPY) -O srec $< $(TARGET).srec

%.eep: %.elf
	@echo
	@echo $(MSG_EEPROM) $@
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 --no-change-warnings -O $(FORMAT) $< $@ || exit 0

# Create extended listing file from ELF output file.
%.lss: %.elf
	@echo
	@echo $(MSG_EXTENDED_LISTING) $@
	$(OBJDUMP) -h -z -S $< > $@

# Create a symbol table from ELF output file.
%.sym: %.elf
	@echo
	@echo $(MSG_SYMBOL_TABLE) $@
	$(NM) -n $< > $@

# Create a DFU file with checksum and 16 byte suffix
%.dfu: %.bin
	@echo
	@echo $(MSG_DFU) $@
	$(DFU_TOOL) -s $(TARGET).bin

# Create library from object files.
.SECONDARY : $(TARGET).a
.PRECIOUS : $(OBJ)
%.a: $(OBJ)
	@echo
	@echo $(MSG_CREATING_LIBRARY) $@
	$(AR) $@ $(OBJ)

# Link: create ELF output file from object files.
.SECONDARY : $(TARGET).elf
.PRECIOUS : $(OBJ)
%.elf: $(OBJ)
	@echo
	@echo $(MSG_LINKING) $@
	$(LD) $(LINKER_SCRIPT) $(ALL_CFLAGS_LINKER) $(LDFLAGS) -o $@ $^

# Compile: create object files from C source files.
$(OBJDIR)/%.o : %.c
	@echo
	@echo $(MSG_COMPILING) $<
	$(CC) -c $(ALL_CFLAGS) $< -o $@

# Compile: create object files from C++ source files.
$(OBJDIR)/%.o : %.cpp
	@echo
	@echo $(MSG_COMPILING_CPP) $<
	$(CC) -c $(ALL_CPPFLAGS) $< -o $@

# Compile: create assembler files from C source files.
%.s : %.c
	$(AS) -S $(ALL_CFLAGS) $< -o $@

# Compile: create assembler files from C++ source files.
%.s : %.cpp
	$(CC) -S $(ALL_CPPFLAGS) $< -o $@

# Assemble: create object files from assembler source files.
$(OBJDIR)/%.o : %.S
	@echo
	@echo $(MSG_ASSEMBLING) $<
	$(AS) $< $(ALL_ASFLAGS) --MD $(OBJDIR)/$(@F).d -o $@

# Create preprocessed source for use in sending a bug report.
%.i : %.c
	$(CC) -E -mmcu=$(CPU) -I. $(CFLAGS) $< -o $@

# Target: clean project.
clean: begin clean_list clean_binary end

clean_binary:
	$(REMOVE) $(TARGET).hex

clean_list:
	@echo $(MSG_CLEANING)
	$(REMOVE) $(TARGET).eep
	$(REMOVE) $(TARGET)eep.hex
	$(REMOVE) $(TARGET).cof
	$(REMOVE) $(TARGET).elf
	$(REMOVE) $(TARGET).map
	$(REMOVE) $(TARGET).sym
	$(REMOVE) $(TARGET).lss
	$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.o)
	$(REMOVE) $(CPPSRC:%.cpp=$(OBJDIR)/%.o)
	$(REMOVE) $(SRC:%.c=$(OBJDIR)/%.lst)
	$(REMOVE) $(CPPSRC:%.cpp=$(OBJDIR)/%.lst)
	$(REMOVE) $(ASRC:%.S=$(OBJDIR)/%.lst)
	$(REMOVE) $(ASRC:%.S=$(OBJDIR)/%.o)
	$(REMOVE) $(ASRC:%.S=$(OBJDIR)/%.o.d)
	$(REMOVE) *.o.d
	$(REMOVE) $(SRC:.c=.s)
	$(REMOVE) $(SRC:.c=.d)
	$(REMOVE) $(CPPSRC:.cpp=.d)
	$(REMOVE) $(SRC:.c=.i)
	$(REMOVE) InvalidEvents.tmp
	$(REMOVE) $(TARGET).bin
	$(REMOVE) $(TARGET).dfu
	$(REMOVE) $(TARGET).srec

doxygen:
	@echo Generating Project Documentation...
	@doxygen Doxygen.conf
	@echo Documentation Generation Complete.

clean_doxygen:
	rm -rf Documentation

# Create object files directory
$(shell mkdir $(OBJDIR)/ 2>/dev/null)

# Listing of phony targets.
.PHONY : all showtarget begin end sizebefore sizeafter \
gccversion build elf hex eep lss sym program clean \
clean_list clean_binary doxygen

include ../ctags.mk
