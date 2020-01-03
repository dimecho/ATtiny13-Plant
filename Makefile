MCU=attiny13a
FUSE_L=0x6A
FUSE_H=0xFF #0xFE
F_CPU=1200000
CC=avr-gcc
LD=avr-ld
OBJCOPY=avr-objcopy
SIZE=avr-size
AVRDUDE=avrdude
CFLAGS=-std=c99 -Wall -g -Os -mmcu=${MCU} -DF_CPU=${F_CPU} -I. -I..
TARGET=main
SRCS=main.c
SOIL = 280 388 460
POT = 12 32 64

all:
	${CC} ${CFLAGS} -o ${TARGET}.o ${SRCS}
	${LD} -o ${TARGET}.elf ${TARGET}.o
	${OBJCOPY} -j .text -j .data -O binary ${TARGET}.o ${TARGET}.bin
	${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.o ${TARGET}.hex
	${SIZE} -C --mcu=${MCU} ${TARGET}.elf

flash:
	${AVRDUDE} -p ${MCU} -c usbasp -U flash:w:${TARGET}.hex:i -F -P usb

fuse:
	$(AVRDUDE) -p ${MCU} -c usbasp -U hfuse:w:${FUSE_H}:m -U lfuse:w:${FUSE_L}:m

clean:
	rm -f *.c~ *.o *.elf *.hex

guibuild:
	#ATtiny13
	$(foreach m,$(SOIL), \
		$(foreach p,$(POT), \
			${CC} ${CFLAGS} -DsensorMoisture=$(m) -DpotSize=$(p) -o ${TARGET}.o ${SRCS}; \
			${LD} -o ${TARGET}.elf ${TARGET}.o; \
			${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.o gui/Web/firmware/attiny13/${TARGET}-$(m)-$(p).hex; \
			${SIZE} -C --mcu=${MCU} ${TARGET}.elf; \
		) \
	)
	#ATtiny45
	$(foreach m,$(SOIL), \
		$(foreach p,$(POT), \
			${CC} -std=c99 -Wall -g -Os -mmcu=attiny45 -DF_CPU=${F_CPU} -I. -I.. -DsensorMoisture=$(m) -DpotSize=$(p) -o ${TARGET}.o ${SRCS}; \
			${LD} -o ${TARGET}.elf ${TARGET}.o; \
			${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.o gui/Web/firmware/attiny45/${TARGET}-$(m)-$(p).hex; \
			${SIZE} -C --mcu=attiny45 ${TARGET}.elf; \
		) \
	)
	#ATtiny85
	$(foreach m,$(SOIL), \
		$(foreach p,$(POT), \
			${CC} -std=c99 -Wall -g -Os -mmcu=attiny85 -DF_CPU=${F_CPU} -I. -I.. -DsensorMoisture=$(m) -DpotSize=$(p) -o ${TARGET}.o ${SRCS}; \
			${LD} -o ${TARGET}.elf ${TARGET}.o; \
			${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.o gui/Web/firmware/attiny85/${TARGET}-$(m)-$(p).hex; \
			${SIZE} -C --mcu=attiny85 ${TARGET}.elf; \
		) \
	)