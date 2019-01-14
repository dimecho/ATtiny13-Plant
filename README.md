<p align="center"><img src="img/icon.png?raw=true"></p>

# ATtiny13 Plant

Monitor soil moisture and water plant. Portable and high efficiency with lithium-ion batteries.

![Photo](img/photo.jpg?raw=true)

## PCB

![Screenshot](img/pcb.png?raw=true)

## BOM (Bill of Materials)

| Part  | Value      | Package             | Function |
| ----- |:----------:| -------------------:|---------:|
| IC1   | ATTINY13A  | SOP-8 or DIP-8      | CPU      |
| C1    | 100nF      | 0805 or 0603 (104)  | CPU      |
| T1A/B | 2N4401 NPN | TO-92 or SOT-23     | Pump     |
| R1    | 510R       | 0805 or 0603 (511)  | Pump     |
| R2    | 10k        | 0805 or 0603 (103)  | Pump     |
| LED1  | Red        | 0805 or DIP-2       | LED      |
| R3    | 510R       | 0805 or 0603 (511)  | LED      |
| R4    | 10k        | 0805 or 0603 (103)  | LED      |
| T2A/B | 2N4401 NPN | TO-92 or SOT-23     | Solar    |
| REG1  | 7805L (5V) | TO-92               | Solar    |
| C2    | 1000uF     | 7x7 mm              | Solar    |
| R5    | 510R       | 0805 or 0603 (511)  | Solar    |
| R6    | 10k        | 0805 or 0603 (103)  | Solar    |
| R7    | 510R       | 0805 or 0603 (511)  | Sensor   |
| R8    | 10k        | 0805 or 0603 (103)  | Sensor   |
| -     | 3.7-4.2V   | Lithium Cell 18650  | Battery  |
| -     | TP4056     | Lithium Charger     | Charger  |
| -     | 10V+       | 2x 5V Solar Cells   | Solar    |

## Logic

![Screenshot](img/main.png?raw=true)

## Solar

Optional Pin7 (PB2) used to "burst" charge from solar panel.

## LED

Empty container detection **Caution:** LED uses RESET Pin1 (PB5) requires HFuse 0xFE, if enabled ATTiny13 can only be flashed once.
Future flashing requires "High-Voltage programmer" to clear the fuse.

## Compile

Install "avr-gcc" and run "make".
```
avr-gcc -std=c99 -Wall -g -Os -mmcu=attiny13a -DF_CPU=1200000 -I. -I.. -DUART_BAUDRATE=19200 main.c uart.c -o main.o
avr-objcopy -R .eeprom -O binary main.o main.bin
```

## Download

![ATTiny13](img/attiny.png?raw=true) [Download Binary](../../releases/download/1.0/ATTiny13.Plant.zip)


## Flash

Use Raspberry Pi (using linuxgpio) or other AVR programmer.
[avrdude](http://download.savannah.gnu.org/releases/avrdude/)
```
sudo avrdude -p t13 -c linuxgpio -Uhfuse:w:0xFF:m -Ulfuse:w:0x6A:m -U flash:w:main.bin
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