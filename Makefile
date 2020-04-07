MCU=attiny13a
FUSE_H=0xFF #0xFE
FUSE_L=0x6A
F_CPU=1200000
#FUSE_L=0x7B
#F_CPU=128000
CC=avr-gcc
LD=avr-ld
OBJCOPY=avr-objcopy
SIZE=avr-size
AVRDUDE=avrdude
CFLAGS=-std=gnu99 -Wall -g -Os -mmcu=${MCU} -DF_CPU=${F_CPU} -I. -I..
TARGET=main
SRCS=main.c
SERIAL := $(shell bash -c 'echo $$RANDOM')

all:
	${CC} ${CFLAGS} -DSERIAL=${SERIAL} -o ${TARGET}.o ${SRCS}
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
	#Blinky
	${CC} ${CFLAGS} -o blinky.o blinky.c;
	${OBJCOPY} -O ihex blinky.o gui/Web/firmware/blinky.hex;

	#ATtiny13
	${CC} ${CFLAGS} -DSERIAL=${SERIAL} -o ${TARGET}.o ${SRCS};
	${LD} -o ${TARGET}.elf ${TARGET}.o;
	${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.o gui/Web/firmware/attiny13.hex;
	${SIZE} -C --mcu=${MCU} ${TARGET}.elf;

	#ATtiny45
	${CC} -std=gnu99 -Wall -g -Os -mmcu=attiny45 -DF_CPU=1000000 -I. -I.. -DSERIAL=${SERIAL} -o ${TARGET}.o ${SRCS};
	${LD} -o ${TARGET}.elf ${TARGET}.o;
	${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.o gui/Web/firmware/attiny45.hex;
	${SIZE} -C --mcu=attiny45 ${TARGET}.elf;

	#ATtiny85
	${CC} -std=gnu99 -Wall -g -Os -mmcu=attiny85 -DF_CPU=1000000 -I. -I.. -DSERIAL=${SERIAL} -o ${TARGET}.o ${SRCS};
	${LD} -o ${TARGET}.elf ${TARGET}.o;
	${OBJCOPY} -j .text -j .data -O ihex ${TARGET}.o gui/Web/firmware/attiny85.hex;
	${SIZE} -C --mcu=attiny85 ${TARGET}.elf;