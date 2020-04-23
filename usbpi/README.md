## Flash ATtiny13 with Raspberry Pi (linuxgpio)
```
avrdude -p t13 -c linuxgpio -Uhfuse:w:0xFB:m -Ulfuse:w:0x6A:m -U flash:w:main.hex:i
```

![PI](avr_programmer_pi.png?raw=true)

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
