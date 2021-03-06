/* Name: main.c
  
  created by chris chung, 2010 April

  based on the works found in

  v-usb framework http://www.obdev.at/vusb/
	 Project: Thermostat based on AVR USB driver
	 Author: Christian Starkjohann
    
  usbtiny isp http://www.xs4all.nl/~dicks/avr/usbtiny/
  	Dick Streefland
  
  please observe licensing term from the above two projects

	Copyright (C) 2010  chris chung

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


  **** fuse setting, 
  **** this will blow reset fuse, u will need to use HV programmer to recover if u mess up
  avrdude -c usbtiny -p t45 -V -U lfuse:w:0xe1:m -U hfuse:w:0x5d:m -U efuse:w:0xff:m 
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>

#include "usbdrv.h"
//#include "oddebug.h"

enum
{
	// Generic requests
	USBTINY_ECHO = 0,		// echo test
	USBTINY_READ = 1,		// read byte
	USBTINY_WRITE = 2,		// write byte
	USBTINY_CLR = 3,		// clear bit 
	USBTINY_SET = 4,		// set bit
	// Programming requests
	USBTINY_POWERUP = 5,	// apply power (wValue:SCK-period, wIndex:RESET)
	USBTINY_POWERDOWN = 6,	// remove power from chip
	USBTINY_SPI = 7,		// issue SPI command (wValue:c1c0, wIndex:c3c2)
	USBTINY_POLL_BYTES = 8,	// set poll bytes for write (wValue:p1p2)
	USBTINY_FLASH_READ = 9,	// read flash (wIndex:address)
	USBTINY_FLASH_WRITE = 10,	// write flash (wIndex:address, wValue:timeout)
	USBTINY_EEPROM_READ = 11,	// read eeprom (wIndex:address)
	USBTINY_EEPROM_WRITE = 12,	// write eeprom (wIndex:address, wValue:timeout)
	USBTINY_DDRWRITE = 13,	// set port direction
	//USBTINY_SPI1 = 14		// a single SPI command
};

#define	PORT	PORTB
#define	DDR		DDRB
#define	PIN		PINB

//
// to reduce pin count so that this can fit in a 8 pin tiny
// . no power nor ground pins to target, they are to be connected always
// . no reset control pin to target, target reset always grounded
//   * this had caused problem and there are two solutions
//     1. provide a toggle switch to off-on-off target reset to ground
//     2. introduce reset control and use reset pin as io
//
#define	POWER_MASK	0x00
#define	GND_MASK	0x00

#define	RESET_MASK	(1 << 5)
#define	SCK_MASK	(1 << 2)
#define	MISO_MASK	(1 << 1)
#define	MOSI_MASK	(1 << 0)

// ----------------------------------------------------------------------
// Programmer input pins:
//	MISO	PD3	(ACK)
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Local data
// ----------------------------------------------------------------------
static	uchar		sck_period = 10;	// SCK period in microseconds (1..250)
static	uchar		poll1;		// first poll byte for write
static	uchar		poll2;		// second poll byte for write
static	unsigned		address;	// read/write address
static	unsigned		timeout;	// write timeout in usec
static	uchar		cmd0;		// current read/write command byte
static	uchar		cmd[4];		// SPI command buffer
static	uchar		res[4];		// SPI result buffer

// ----------------------------------------------------------------------
// Delay exactly <sck_period> times 0.5 microseconds (6 cycles).
// ----------------------------------------------------------------------
__attribute__((always_inline))
static inline void delay ( void )
{
	asm volatile(
		"	mov	__tmp_reg__,%0	\n"
		"0:	rjmp	1f		\n"
		"1:	nop			\n"
		"	dec	__tmp_reg__	\n"
		"	brne	0b		\n"
		: : "r" (sck_period) );
}

// ----------------------------------------------------------------------
// Issue one SPI command.
// ----------------------------------------------------------------------
static	void	spi ( uchar* cmd, uchar* res, uchar n )
{
	uchar	c;
	uchar	r;
	uchar	mask;

	while	( n != 0 )
	{
		n--;
		c = *cmd++;
		r = 0;
		for	( mask = 0x80; mask; mask >>= 1 )
		{
			if	( c & mask )
			{
				PORT |= MOSI_MASK;
			}
			delay();
			PORT |= SCK_MASK;
			delay();
			r <<= 1;
			if	( PIN & MISO_MASK )
			{
				r++;
			}
			PORT &= ~MOSI_MASK;
			PORT &= ~SCK_MASK;
		}
		*res++ = r;
	}
}

// ----------------------------------------------------------------------
// Create and issue a read or write SPI command.
// ----------------------------------------------------------------------
static	void	spi_rw ( void )
{
	unsigned	a;

	a = address++;
	if	( cmd0 & 0x80 )
	{	// eeprom
		a <<= 1;
	}
	cmd[0] = cmd0;
	if	( a & 1 )
	{
		cmd[0] |= 0x08;
	}
	cmd[1] = a >> 9;
	cmd[2] = a >> 1;
	spi( cmd, res, 4 );
}

// ----------------------------------------------------------------------
// Handle an IN packet.
// ----------------------------------------------------------------------
uchar usbFunctionRead(uchar *data, uchar len)
{
	uchar	i;

	for	( i = 0; i < len; i++ )
	{
		spi_rw();
		data[i] = res[3];
	}
	return len;
}

// ----------------------------------------------------------------------
// Handle an OUT packet.
// ----------------------------------------------------------------------
uchar usbFunctionWrite(uchar *data, uchar len)
{
	uchar	i;
	unsigned	usec;
	uchar	r;
	//uchar	last = (len != 8);

	for	( i = 0; i < len; i++ )
	{
		cmd[3] = data[i];
		spi_rw();
		cmd[0] ^= 0x60;	// turn write into read
		//
		for	( usec = 0; usec < timeout; usec += 32 * sck_period )
		{	// when timeout > 0, poll until byte is written
			spi( cmd, res, 4 );
			r = res[3];
			if	( r == cmd[3] && r != poll1 && r != poll2 )
			{
				break;
			}
		}
		//
	}
	//return last;
	return 1;
}

/* ------------------------------------------------------------------------- */
/* ------------------------ interface to USB driver ------------------------ */
/* ------------------------------------------------------------------------- */

uchar	usbFunctionSetup(uchar data[8])
{
// ----------------------------------------------------------------------
// Handle a non-standard SETUP packet.
// ----------------------------------------------------------------------
	uchar	bit;
	uchar	mask;
	uchar*	addr;
	uchar	req;

	// Generic requests
	req = data[1];
	if	( req == USBTINY_ECHO )
	{
		usbMsgPtr = data;
		return 8;
	}
	addr = (uchar*) (int) data[4];
	if	( req == USBTINY_READ )
	{
		data[0] = *addr;
		usbMsgPtr = data;
		return 1;
	}
	if	( req == USBTINY_WRITE )
	{
		*addr = data[2];
		return 0;
	}
	bit = data[2] & 7;
	mask = 1 << bit;
	if	( req == USBTINY_CLR )
	{
		*addr &= ~ mask;
		return 0;
	}
	if	( req == USBTINY_SET )
	{
		*addr |= mask;
		return 0;
	}
	if	( req == USBTINY_DDRWRITE )
	{
		DDR = data[2];
	}

	// Programming requests
	if	( req == USBTINY_POWERUP )
	{
		sck_period = data[2];
		mask = POWER_MASK;
		if	( data[4] )
		{
			mask |= RESET_MASK;
		}
		DDR  &= ~MISO_MASK;
		DDR  |= (RESET_MASK|SCK_MASK|MOSI_MASK);
		PORT &= ~(RESET_MASK|SCK_MASK|MOSI_MASK|MISO_MASK);
		return 0;
	}
	if	( req == USBTINY_POWERDOWN )
	{
		//PORT |= RESET_MASK;
		//DDR  &= ~(SCK_MASK|MOSI_MASK);
		DDRB  = RESET_MASK;
		PORTB = RESET_MASK;
		return 0;
	}
	/* have to remove the following check as we strip a lot of io
	if	( ! PORT )
	{
		return 0;
	}
	*/
	if	( req == USBTINY_SPI )
	{
		spi( data + 2, data + 0, 4 );
		usbMsgPtr = data;
		return 4;
	}
	// There are no single SPI transactions in the ISP protocol
	/*if	( req == USBTINY_SPI1 )
	{
		spi( data + 2, data + 0, 1 );
		usbMsgPtr = data;
		return 1;
	}*/
	if	( req == USBTINY_POLL_BYTES )
	{
		poll1 = data[2];
		poll2 = data[3];
		return 0;
	}
	address = * (unsigned*) & data[4];
	if	( req == USBTINY_FLASH_READ )
	{
		cmd0 = 0x20;
		return USB_NO_MSG;;	// usbFunctionRead() will be called to get the data
	}
	if	( req == USBTINY_EEPROM_READ )
	{
		cmd0 = 0xa0;
		return USB_NO_MSG;;	// usbFunctionRead() will be called to get the data
	}
	timeout = * (unsigned*) & data[2];
	if	( req == USBTINY_FLASH_WRITE )
	{
		cmd0 = 0x40;
		return USB_NO_MSG;	// data will be received by usbFunctionWrite()
	}
	if	( req == USBTINY_EEPROM_WRITE )
	{
		cmd0 = 0xc0;
		return USB_NO_MSG;	// data will be received by usbFunctionWrite()
	}
	return 0; // ignore all unknown requests
}

/* ------------------------------------------------------------------------- */
/* --------------------------------- main ---------------------------------- */
/* ------------------------------------------------------------------------- */
/*
static void hardwareInit(void)
{
    // activate pull-ups except on USB lines
    USB_CFG_IOPORT   = (uchar)~((1<<USB_CFG_DMINUS_BIT)|(1<<USB_CFG_DPLUS_BIT));
    //all pins input except USB (-> USB reset)
	#ifdef USB_CFG_PULLUP_IOPORT    // use usbDeviceConnect()/usbDeviceDisconnect() if available
	    USBDDR    = 0;    //we do RESET by deactivating pullup
	    usbDeviceDisconnect();
	#else
	    USBDDR    = (1<<USB_CFG_DMINUS_BIT)|(1<<USB_CFG_DPLUS_BIT);
	#endif

	uchar i;
    for(i = 0; i<250; i++) { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }

	#ifdef USB_CFG_PULLUP_IOPORT
	    usbDeviceConnect();
	#else
	    USBDDR    = 0;      //  remove USB reset condition
	#endif
}
*/
int main(void)
{
	#if USB_CFG_HAVE_MEASURE_FRAME_LENGTH
		//oscInit();
		calibrateOscillatorASM();
	#endif

	//DDRB  = (RESET_MASK|SCK_MASK|MOSI_MASK);
	DDRB  = RESET_MASK;
	PORTB = RESET_MASK;
    /* RESET status: all port bits are inputs without pull-up.
    * That's the way we need D+ and D-. Therefore we don't need any
    * additional hardware initialization.
    */
    //odDebugInit();
    
    wdt_enable(WDTO_1S);
    /* If you don't use the watchdog, replace the call above with a wdt_disable().
    * On newer devices, the status of the watchdog (on/off, period) is PRESERVED
    * OVER RESET!
    */
    //hardwareInit();
    usbDeviceDisconnect();
    uchar i;
    for(i = 0; i<250; i++) { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }
    usbDeviceConnect();

    usbInit();
    sei();
    for(;;){    // main event loop
        wdt_reset(); // keep the watchdog happy
        usbPoll();
    }
    return 0;
}