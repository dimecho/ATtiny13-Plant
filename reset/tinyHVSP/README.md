http://www.simpleavr.com/avr/hvsp-fuse-resetter

Description
-------

Revert RESET fuse to factory default fuse settings.


Features
-------

* Reads device signature and hi-low fuses for hi-voltage serial programmable attinys
* Reset hi-low fuses to factory default on target devices
* Layout to drop-on attiny13, attiny25/45/85 8 pin devices targets
* Attiny24/44/84 targets needs additional breadboard and jumper wires
* Standalone operations, fuses values show on 7 segment display
* Cannot reset fuse for attiny2313 and atmega devices as they requires hi-voltage parallel programming


Parts List
-------

* attiny2313
* 4x7 segment LED display
* 1k resistor x 2
* 2n2222 NPN transistor or equivalent
* 78L05
* +12V battery source


Schematic
-------

![Schematic](reset/tinyHVSP/tinyHVSP.png?raw=true)


Project FUSE Setting
-------

```
avrdude -c usbtiny -p t2313 -V -U lfuse:w:0xe4:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
```