Wire-up ATTiny chip into Arduino that needs its fuses fixed (see schematics for details) and load ATTinyFuseReset.ino sketch. In the Arduino IDE open Serial Monitor and select 19200 baud as the serial rate. Then send a character to the Arduino using the Serial Monitor's "send" button to trigger the fuse reset cycle. The code will print out the device signature and before and after values of the fuses.

You should now have a functional ATTiny!

Source: https://sites.google.com/site/wayneholder/attiny-fuse-reset