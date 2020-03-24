/* Name: osccal.c
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-10
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 * This Revision: $Id: osccal.c 762 2009-08-12 17:10:30Z cs $
 */

#include <avr/io.h>
#include <avr/eeprom.h>
#include "usbdrv.h"

#if USB_CFG_HAVE_MEASURE_FRAME_LENGTH

#ifndef uchar
#define uchar   unsigned char
#endif

/* ------------------------------------------------------------------------- */
/* ------------------------ Oscillator Calibration ------------------------- */
/* ------------------------------------------------------------------------- */

void    oscInit(void)
{
uchar   calibrationValue;

	calibrationValue	= eeprom_read_byte(0);
    if(calibrationValue != 0xff)
		OSCCAL	= calibrationValue;
	else {
#if USB_CFG_CLOCK_KHZ==12800
		OSCCAL	= 232;
#else
		OSCCAL	+= 4;		/*	8.00 -> 8.25MHz	*/
#endif
	}
}

/* Calibrate the RC oscillator. Our timing reference is the Start Of Frame
 * signal (a single SE0 bit) repeating every millisecond immediately after
 * a USB RESET. We first do a binary search for the OSCCAL value and then
 * optimize this value with a neighboorhod search.
 */
void    calibrateOscillator(void)
{
int         x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);
uchar		optimumValue;
#if USB_CFG_CLOCK_KHZ==12800
uchar       step = 32;
uchar       trialValue = 192;
#else
uchar       step = 64;
uchar       trialValue = 0;
#endif
uchar		org;
int			err;

	org	= OSCCAL;						/* keep the original value				*/
										/* keep the current error ...			*/
	err	= usbMeasureFrameLength() - targetValue;
    if(err < 0)
        err = -err;

#if USB_CFG_CLOCK_KHZ==16500
    OSCCAL	= 0x98;        				/* select a split range  - O.Tamura		*/
    x = usbMeasureFrameLength();
    if(x < targetValue)
        trialValue	= 128;
#endif

	/* do a binary search: */
    do{
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();    /* proportional to current real frequency */
        if(x < targetValue)             /* frequency still too low */
            trialValue += step;
        step >>= 1;
    }while(step > 0);

	/*
	This calibration may fail if the other low-speed device is connected
	to the same host controller (by downstream broadcast packet). - O.Tamura
	*/
    x -= targetValue;
    if(x < 0)
        x = -x;
	if( x>err ) {
		OSCCAL	= org;
		return;
	}

    /* We have a precision of +/- 2 for optimum OSCCAL here */
    /* now do a neighborhood search for optimum value */
    optimumValue = trialValue;
    optimumDev = x; /* this is certainly far away from optimum */
    for(OSCCAL = trialValue - 2; OSCCAL <= trialValue + 2; OSCCAL++){
        x = usbMeasureFrameLength() - targetValue;
        if(x < 0)
            x = -x;
        if(x < optimumDev){
            optimumDev = x;
            optimumValue = OSCCAL;
        }
    }
    OSCCAL = optimumValue;

	if( eeprom_read_byte(0)!=optimumValue )
		eeprom_write_byte(0, optimumValue);
}
/*
Note: This calibration algorithm may try OSCCAL values of up to 192 even if
the optimum value is far below 192. It may therefore exceed the allowed clock
frequency of the CPU in low voltage designs!
You may replace this search algorithm with any other algorithm you like if
you have additional constraints such as a maximum CPU clock.
For version 5.x RC oscillators (those with a split range of 2x128 steps, e.g.
ATTiny25, ATTiny45, ATTiny85), it may be useful to search for the optimum in
both regions.
*/

#endif
