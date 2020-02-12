http://www.simpleavr.com/avr/vusbtiny


Description from http://www.xs4all.nl/~dicks/avr/usbtiny/
-------

USBtiny is a software implementation of the USB low-speed protocol for the Atmel ATtiny microcontrollers. Of course, it will also work on the ATmega series. The software is written for an AVR clocked at 12 MHz. At this frequency, each bit on the USB bus takes 8 clock cycles, and with a lot of trickery, it is possible to decode and encode the USB waveforms by software. The USB driver needs approximately 1250 to 1350 bytes of flash space (excluding the optional identification strings), depending on the configuration and compiler version, and 46 bytes RAM (excluding stack space). The C interface consists of 3 to 5 functions, depending on the configuration.


vUSB
-------

description from http://www.obdev.at/products/vusb/

V-USB is a software-only implementation of a low-speed USB device for Atmel's AVR microcontrollers, making it possible to build USB hardware with almost any AVR microcontroller, not requiring any additional chip.


Features
-------

* Programming logic from usbtiny isp, mature avr-dude support
* Small foot-print
* Minimal components
* Powers target device


note that the io lines to the target mcus are not protected. you can add 1k-2k resistors to SCK and MOSI and protect against possible wrong connections


References
-------

based on the works found at

v-usb from framework http://www.obdev.at/vusb/
usbtiny isp http://www.xs4all.nl/~dicks/avr/usbtiny/


Parts List
-------

* 1x attiny45/85
* 1x 100nf capacitor (104)
* 2x 3.6v zener diodes (1n747,BZX79)
* 2x 68ohm resistor
* 1x 1.5k (1.7k) resistor

* (optional) 1k/2k resistors for io lines protection


Tools Required
-------
* A working AVR programmer (yes, it's a catch22, we need one to make one)
* Working AVR programming environment


Application Notes
-------

* vusbtiny works with avrdude and is viewed by avrdude as a usbtiny programmer
* vusbtiny always power target chip via USB
* Jumper J2 connects programmer to target chip
* When connected and NOT in programming
* Target chip w/ target RESET line high
* Other target pins (MISO, MOSI, CLK) in hi-Z state
* During programming
* Turns target RESET line LOW
* MISO becomes input
* MOSI, CLK becomes output


Schematic
-------


Building and Flashing
-------

* change into /firmware working directory
```
make
```
* attach your favorite ISP programmer
* modify makefile and change your avrdude parameters if needed. the stock one assumes USBTiny programmer. i.e. AVRDUDE_PROGRAMMERID=usbtiny
```
make install
```
After flashing firmware, we need to properly set the fuse, we are using pin 1 reset as IO in this project

* ppl clock used as required by v-usb layer for usb timing
* reset pin disabled as we need to use it as IO
```
avrdude -c usbtiny -p t45 -V -U lfuse:w:0xe1:m -U hfuse:w:0x5d:m -U efuse:w:0xff:m
```

This setting disables further programming via 5V SPI as we need the RESET pin (pin1) for IO. you will need access to a HVSP programmer to recover the fuse.


Troubleshooting
-------

* Cannot flash the firmware? check your original programmer, might need to adjust timing via -B flag in avrdude. try to read chip 1st, may be a bad fuse, may be your chip need an external clock signal. you may need to fix your chip back to default 1st.
    check connections
* If use different io pins, check code and connections
* You may substitute zener diodes with 500mw, 400mw types
* You may try reduce R3 value to 1.2K or less
* You are more likely to encounter avrdude timing problems, try -B flag of avrdude, have a shorter USB cable all helps


License
-------

GPL2