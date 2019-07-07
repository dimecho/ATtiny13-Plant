# 
# Makefile for Peter Dannegger's bootloader, to be used with the GNU
# toolchain (as opposed to Atmel's Assembler).
#


####### user presets ##########################
# adjust the following definitions to match your target:
##############################################

# MCU name
MCU = attiny13a

# fuse-bits
# -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xfe:m 

# Name of the Atmel defs file for the actual MCU.
ATMEL_INC= tn13Adef.inc

# Processor frequency.  The value is not critical:
#F_CPU = 14745600
F_CPU = 8000000

# Boot dealy. How many cycles after boot to wait for bootload request
# In seconds: Boot_Delay/F_CPU
Boot_Delay = 2000000

#     AVR Studio 4.10 requires dwarf-2.
#     gdb runs better with stabs
#DEBUG = dwarf-2
DEBUG = stabs+

# Define the Tx and Rx lines here.  Set both groups to the same for
# one wire mode:
STX_PORT = PORTB
STX = PB3

SRX_PORT = PORTB
SRX = PB3

####### End user presets ######################

SHELL=/bin/bash

CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) 
CFLAGS += -I . -I ./added -I ./converted -I/usr/local/share/avr8-gnu-toolchain-darwin_x86_64/avr/include
CFLAGS += -ffreestanding
CFLAGS += -g$(DEBUG)
CFLAGS += -L,-g$(DEBUG)
CFLAGS += -DRAM_START=$(SRAM_START) -DSRAM_SIZE=$(SRAM_SIZE)
CFLAGS += -DSTX_PORT=$(STX_PORT) -DSTX=$(STX)
CFLAGS += -DSRX_PORT=$(SRX_PORT) -DSRX=$(SRX)
CFLAGS += -DBootDelay=$(Boot_Delay) -DBOOTDELAY=$(Boot_Delay)

# The following files were imported by a gawk script without user
# intervention (in order to ease keeping up with future releases of
# the original bootloader):
AUTO_CONVERTED_FILES = \
  converted/progtiny.inc \
  converted/uart.inc \
  converted/password.inc \
  converted/progmega.inc \
  converted/watchdog.inc \
  converted/bootload.asm \
  converted/abaud.inc \
  converted/command.inc \
  converted/protocol.h \
  converted/apicall.inc \
  converted/verify.inc \
  converted/message.inc

# The following files must be worked on manually:
MANUALLY_ADDED_FILES = \
  added/fastload.inc \
  added/fastload.h \
  added/mangled_case.h \
  added/bootload.S \
  added/compat.h \
  added/fastload.h

ADDITIONAL_DEPENDENCIES = atmel_def.h

ASMSRC = $(AUTO_CONVERTED_FILES) $(MANUALLY_ADDED_FILES)

include atmel_def.mak

ifdef BOOTRST
STUB_OFFSET = 510 
LOADER_START = ( $(FLASHEND) * 2 ) - 510
endif

all: bootload.hex

bootload.hex: bootload.elf

bootload.elf : bootload.o stub.o
ifndef BOOTRST
	vars="$$(./get_text_addrs.sh $(FLASHEND))"; \
	arch="$$(./get_avr_arch.sh -mmcu=$(MCU) bootload.o)"; \
	echo "arch=$$arch";\
	echo "$$vars"; \
	eval "$$vars"; \
	sed -e "s/@LOADER_START@/$$LOADER_START/g" \
	    -e s"/@ARCH@/$$arch/" \
	    -e s'/@RAM_START@/$(SRAM_START)/g' \
	    -e s'/@RAM_SIZE@/$(SRAM_SIZE)/g' \
	    -e "s/@STUB_OFFSET@/$$STUB_OFFSET/g" \
	    bootload.template > bootload.x; \
	avr-ld -N -E -T bootload.x -Map=$(patsubst %.elf,%,$@).map \
	  --cref $+ -o $@ --defsym Application="$$LOADER_START-2"
else
	vars="$$(./get_bootsection_addrs.sh $(FLASHEND) $(FIRSTBOOTSTART) \
                $(SECONDBOOTSTART) $(THIRDBOOTSTART) $(FORTHBOOTSTART))"; \
	arch="$$(./get_avr_arch.sh -mmcu=$(MCU) bootload.o)"; \
	echo "arch=$$arch";\
	echo "$$vars"; \
	eval "$$vars"; \
	sed -e "s/@LOADER_START@/$$LOADER_START/g" \
	    -e s"/@ARCH@/$$arch/" \
	    -e s'/@RAM_START@/$(SRAM_START)/g' \
	    -e s'/@RAM_SIZE@/$(SRAM_SIZE)/g' \
	    -e "s/@STUB_OFFSET@/$$STUB_OFFSET/g" \
	    bootload.template > bootload.x; \
	avr-ld -N -E -T bootload.x -Map=$(patsubst %.elf,%,$@).map \
	  --cref $+ -o $@ --defsym Application=0
endif

atmel_def.h: $(ATMEL_INC) Makefile
#        We use gawk instead of egrep here due to problems with
#        WinAVR's egrep (which I didn't dive into):
	./conv.awk $< | gawk '/PAGESIZE|SIGNATURE_|SRAM_|FLASHEND|BOOT/' > $@

atmel_def.mak: atmel_def.h
	gawk '{ printf "%s = %s\n", $$2, $$3 }' $< > $@


bootload.o: $(ASMSRC) $(ADDITIONAL_DEPENDENCIES)
	avr-gcc -c -Wa,-adhlns=bootload.lst $(CFLAGS) added/bootload.S -o $@

stub.o: added/stub.S
	avr-gcc -c -Wa,-adhlns=stub.lst $(CFLAGS) $< -o $@

%.hex: %.elf
	avr-objcopy -O ihex $< $@

.PHONY: clean dbg

clean: 
	rm -f atmel_def.h bootload.x *.defs *.o *.gas *.mak *.lst *.02x *.map

###
# generate a dump of the definitions available to the assembler
# (bootload.defs, (sorted) bootload.sdefs) and the result of
# preprocessing the asm files (bootload.gas) for debugging:
dbg:
	avr-cpp $(CFLAGS) -dD -E added/bootload.S > bootload.defs
	sort bootload.defs | gawk '/^#define/' > bootload.sdefs
	avr-gcc -E $(CFLAGS) added/bootload.S -o bootload.gas
