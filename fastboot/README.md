### Bootloader

Bootloader is usefull for updating the firmware over UART without desoldering the chip. It also frees up RESET pin as IO (see Caution note).

Compile
```
cd ./bootloader
make
```
One-Time Flash
```
avrdude -p t13 -c linuxgpio -Uhfuse:w:0xEE:m -Ulfuse:w:0x6A:m -e -U flash:w:bootload.hex:i
```
Update (MacOS)
```
./fastboot/osx/bootloader -d /dev/cu.usbserial -b 9600 -p main.hex
```
Update (Windows)
```
./fastboot/win/FBOOT.EXE -C2 -B9600 -Pmain.hex
```

**Caution:** RESET Pin1 (PB5) if fuses set HFuse 0xFE (or 0xEE), bootloader can only be flashed once. Future flashing requires "High-Voltage programmer" to clear the fuse.

## License

[![GNU](https://upload.wikimedia.org/wikipedia/commons/thumb/2/22/Heckert_GNU_white.svg/38px-Heckert_GNU_white.svg.png)](https://www.gnu.org/licenses/)