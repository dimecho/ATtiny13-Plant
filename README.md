<p align="center"><img src="img/icon.png?raw=true"></p>

# ATtiny13 Plant

Monitor soil moisture and water plant. Portable and high efficiency with lithium-ion batteries.

![Photo](img/photo.jpg?raw=true)

![GUI](gui/Interface.png?raw=true)

## Download

![MacOS](img/mac.png?raw=true) [MacOS](../../releases/download/1.0/ATTiny13.Plant.dmg)

![Windows](img/win.png?raw=true) [Windows](../../releases/download/1.0/ATTiny13.Plant.zip)

![Linux](img/linux.png?raw=true) [Linux](../../releases/download/1.0/ATTiny13.Plant.tgz)

![Firmware](img/chip.png?raw=true) [Firmware](../../releases/download/1.0/ATTiny13.Plant.Firmware.zip)

## BOM (Bill of Materials)

| Part  | Value      | Package             | Function |
| ----- |:----------:| -------------------:|---------:|
| IC1   | ATTINY13A  | SOP-8 or DIP-8      | CPU      |
| C1    | 100nF      | 0805 or 0603 (104)  | CPU      |
| T1A/B | 2N4401 NPN | TO-92 or SOT-23     | Pump     |
| R1    | 1k         | 0805 or 0603 (102)  | Pump     |
| R2    | 10k        | 0805 or 0603 (103)  | Pump     |
| R3    | 330R       | 0805 or 0603 (331)  | Solar    |
| T2A/B | 2N4401 NPN | TO-92 or SOT-23     | Solar    |
| REG1  | 78L05 (5V) | TO-92               | Solar    |
| C2    | 1000uF     | 7x7 mm              | Solar    |
| R5    | 10k        | 0805 or 0603 (103)  | Solar    |
| R6    | 10k        | 0805 or 0603 (103)  | Sensor   |
| LED1  | Red        | 0805 or DIP-2       | LED      |
| R7    | 1k         | 0805 or 0603 (102)  | LED      |
| -     | 3.7-4.2V   | Lithium Cell 18650  | Battery  |
| -     | TP4056     | Lithium Charger     | Charger  |
| -     | 10V+       | 2x 5V Solar Cells   | Solar    |

## PCB

![PCB](img/pcb.png?raw=true)

Designed with [EagleCAD](https://www.autodesk.com/products/eagle/free-download)

## Diagram

![Diagram](img/diagram.png?raw=true)

![Technical](img/technical.png?raw=true)

## Compile

Install "avr-gcc" and run "make".

> [MacOS](https://ww1.microchip.com/downloads/en/DeviceDoc/avr8-gnu-toolchain-osx-3.6.2.503-darwin.any.x86_64.tar.gz)
>
> [Windows](https://ww1.microchip.com/downloads/en/DeviceDoc/avr8-gnu-toolchain-3.6.2.1759-win32.any.x86.zip)

```
avr-gcc -std=gnu99 -Wall -Os -mmcu=attiny13a main.c -o main.o
avr-objcopy -O binary main.o main.bin
avr-objcopy -O ihex main.o main.hex
```

## Flash

### Option 1 - USBTiny (Recommended)
```
avrdude -p t13 -c usbtiny -Uhfuse:w:0xFF:m -Ulfuse:w:0x6A:m -U flash:w:main.hex:i
```

![USBTINY](img/attiny_programmer_usbtiny.png?raw=true)

### Option 2 - USBasp
```
avrdude -p t13 -c usbasp -Uhfuse:w:0xFF:m -Ulfuse:w:0x6A:m -U flash:w:main.hex:i
```

![USBASP](img/attiny_programmer_usbasp.png?raw=true)

### Option 3 - Raspberry Pi (Using linuxgpio)
```
avrdude -p t13 -c linuxgpio -Uhfuse:w:0xFF:m -Ulfuse:w:0x6A:m -U flash:w:main.hex:i
```

![PI](img/attiny_programmer_pi.png?raw=true)

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

## Licenses

> ATtiny13 Plant
>
> [![CC0](http://i.creativecommons.org/l/zero/1.0/88x31.png)](https://creativecommons.org/publicdomain/zero/1.0/)
>
> USBasp / USBtiny
>
> [![GNU](https://upload.wikimedia.org/wikipedia/commons/thumb/2/22/Heckert_GNU_white.svg/38px-Heckert_GNU_white.svg.png)](https://www.gnu.org/licenses/)