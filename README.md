<p align="center"><img src="img/icon.png?raw=true"></p>

# ATtiny13 Plant

Monitor soil moisture and water plant. Portable and high efficiency with lithium-ion batteries.

![Photo](img/photo.jpg?raw=true)

![Screenshot](img/main.png?raw=true)

## Solar

Optional Pin7 (PB2) can be used to "burst" charge solar **or** as LED to indicate empty bottle.

## Compile

Install "avr-gcc" and run "make".
```
avr-gcc -std=c99 -Wall -g -Os -mmcu=attiny13a -DF_CPU=1200000 -I. -I.. -DUART_BAUDRATE=19200 main.c uart.c -o main.o
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