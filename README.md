<p align="center"><img src="img/icon.png?raw=true"></p>
<link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/font-awesome/4.7.0/css/font-awesome.min.css">

# ATtiny13 Plant

Monitor soil moisture and water plant. Portable and high efficiency with lithium-ion batteries.

![Photo](img/photo.jpg?raw=true)

## PCB

![Screenshot](img/pcb.png?raw=true)

Designed with [EagleCAD](https://www.autodesk.com/products/eagle/free-download)

## BOM (Bill of Materials)

| Part  | Value      | Package             | Function |
| ----- |:----------:| -------------------:|---------:|
| IC1   | ATTINY13A  | SOP-8 or DIP-8      | CPU      |
| C1    | 100nF      | 0805 or 0603 (104)  | CPU      |
| T1A/B | 2N4401 NPN | TO-92 or SOT-23     | Pump     |
| R1    | 510R       | 0805 or 0603 (511)  | Pump     |
| R2    | 10k        | 0805 or 0603 (103)  | Pump     |
| LED1  | Red        | 0805 or DIP-2       | LED      |
| R3    | 120R       | 0805 or 0603 (121)  | LED      |
| T2A/B | 2N4401 NPN | TO-92 or SOT-23     | Solar    |
| REG1  | 7805L (5V) | TO-92               | Solar    |
| C2    | 1000uF     | 7x7 mm              | Solar    |
| R4    | 510R       | 0805 or 0603 (511)  | Solar    |
| R5    | 10k        | 0805 or 0603 (103)  | Solar    |
| R6    | 10k        | 0805 or 0603 (103)  | Sensor   |
| -     | 3.7-4.2V   | Lithium Cell 18650  | Battery  |
| -     | TP4056     | Lithium Charger     | Charger  |
| -     | 10V+       | 2x 5V Solar Cells   | Solar    |

## Diagram

![Screenshot](img/diagram.png?raw=true)

## Calibrate Sensor (Without Firmware Flashing)

Short sensor wires. After long LED blink quickly put into moist soil. The value will be stored in memmory as base-line.

## Solar

Optional Pin7 (PB2) used to "burst" charge from solar panel.

## LED

Empty container detection **Caution:** LED uses RESET Pin1 (PB5) requires HFuse 0xFE, if enabled ATTiny13 can only be flashed once.
Future flashing requires "High-Voltage programmer" to clear the fuse.

## Compile

Install "avr-gcc" and run "make".
<i class="fa fa-windows" aria-hidden="true"></i> [Windows](https://ww1.microchip.com/downloads/en/DeviceDoc/avr8-gnu-toolchain-3.6.2.1759-win32.any.x86.zip)
<i class="fa fa-apple" aria-hidden="true"></i> [Mac](https://ww1.microchip.com/downloads/en/DeviceDoc/avr8-gnu-toolchain-osx-3.6.2.503-darwin.any.x86_64.tar.gz)
```
avr-gcc -std=gnu99 -Wall -Os -mmcu=attiny13a -DF_CPU=1200000 -DUART_BAUDRATE=19200 main.c uart.c -o main.o
avr-objcopy -O binary main.o main.bin
avr-objcopy -O ihex main.o main.hex
```

## Download

<i class="fa fa-microchip" aria-hidden="true"></i> [Download Binary](../../releases/download/1.0/ATTiny13.Plant.zip)


## Flash

Use Raspberry Pi (using linuxgpio).
```
sudo avrdude -p t13 -c linuxgpio -Uhfuse:w:0xFF:m -Ulfuse:w:0x6A:m -U flash:w:main.bin
```

![AVR](img/attiny_programmer_pi.png?raw=true)

Install AVRDude.
```
sudo apt-get install avrdude
```
Open AVRDude configuration file for editing.
```
sudo nano /etc/avrdude.conf
```
In Nano, use ctrl-w to search for linuxgpio. This is the section that controls the GPIO pins used for programming. The section needs to be uncommented. Set the MOSI, MISO and SCK entries to the GPIO pins on the Pi.
```
programmer
  id    = "linuxgpio";
  desc  = "Use the Linux sysfs interface to bitbang GPIO lines";
  type  = "linuxgpio";
  reset = 12;
  sck   = 11;
  mosi  = 10;
  miso  = 9;
```

## Debug

Use Serial to USB on Pin2 (PB3) + GND.

**Note:** Serial speed is 19200

## Licenses

> ATtiny13 Plant
>
> [![CC0](http://i.creativecommons.org/l/zero/1.0/88x31.png)](https://creativecommons.org/publicdomain/zero/1.0/)
>
> Software UART for ATtiny13
>
> [![BSD](https://upload.wikimedia.org/wikipedia/commons/thumb/b/bf/License_icon-bsd.svg/38px-License_icon-bsd.svg.png)](https://opensource.org/licenses/BSD-2-Clause)