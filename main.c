/*  
    FUSE_L=0x6A (Clock divide fuse enabled = 8Mhz CPU frequency is actually 1MHz)
    FUSE_H=0xFF (0xFF (no bootloader) / 0xEE (bootloader) = RSTDISBL + SELFPRGEN -> CATION: If enabled chip can only be programmed once)
    FUSE_H=0xFB (BODLEVEL 0xFD = 1.8V, 0xFB = 2.7V, 0xFF = BOD Disabled)

    ** The brown-out detector is designed to put the chip into reset when the voltage gets too low,
        and hold it in reset until the voltage returns back to a safe operating level.
*/
#ifndef F_CPU
    #if defined __AVR_ATtiny45__ || defined __AVR_ATtiny85__
        #define F_CPU 1000000 //CPU @ 8Mhz / 8 = 1 Mhz
    #else
        #define F_CPU 1200000 //CPU @ 9.6Mhz / 8 = 1.2Mhz
        //#define F_CPU 128000 //CPU @ 128Khz (no clock divide)

        //WARNING: @128Khz do not divide clock by 8 (LFUSE:6B) - ATtiny 13 will be bricked
        //Recovery: See "arduino_isp_boot-selectable_serial-config_slow_spi.cpp"
    #endif
#endif
/*
                             [ATtiny13A]
                              +------+
                [RESET] PB5  1| O    |8  VCC (2.7V - 4.8V)
(Sensor OUT / Solar IN) PB3  2|      |7  PB2 [CLK]  (LED)
(Sensor IN)             PB4  3|      |6  PB1 [MISO] (Pump)
                        GND  4|      |5  PB0 [MOSI] (Solar ON (WS78L05) / Sleep (TPL5110))
                              +------+
*/
#ifndef SERIAL
    #define SERIAL 0 //Serial # (identify multiple devices)
#endif
/* HARDWARE CONFIGURATION */
/*------------------------*/
#define WS78L05 1 //WS78L05 = cheap
//#define LM2731 2 //LM2731 = expensive
//#define TPL5110 3 //TPL5110 = most expensive
#define EEPROM_ENABLED
//#define UART_TX_ENABLED
//#define UART_RX_ENABLED
#define SOLAR_ENABLED
#define SENSORLESS_ENABLED
/*------------------------*/

//#include <avr/pgmspace.h>
//#include <stdio.h>
//#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>

/*
Enable the watchdog and wait in an infinite loop for software reset
https://www.microchip.com/webdoc/AVRLibcReferenceManual/FAQ_1faq_softreset.html
*/
#define soft_reset()        \
do                          \
{                           \
    /*cli();*/              \
    WDTCR |= (0<<WDP3) | (0<<WDP2) | (0<<WDP1) | (0<<WDP0); \
    for(;;)                 \
    {                       \
    }                       \
} while(0)

#ifndef versionID
    #define versionID               10 //1.0
#endif
#ifndef sensorMoisture
    #define sensorMoisture          484 //ADC value
#endif
#ifndef potSize
 #define potSizeTimer               20 //20x2x100 = 4000 miliseconds = 4 seconds
#endif

#define pumpPin                     PB1 //Output
#define sensorPin                   PB3 //Output
#define solarPin                    PB0 //Output
#define ledPin                      PB2 //Output
#define solarSensorPin              PB3 //Input/Output
#define moistureSensorPin           PB4 //Input
#define delayWatering               50  //8 seconds x 50 = 6.5 min
#define delayBetweenRefillReset     20  //8 seconds x 40 x 20 = 2 hours
#define delayBetweenOverfloodReset  100 //8 seconds x 40 x 100 = 8 hours
#define delayBetweenSolarDischarge  4   //8 seconds x 5 = .5 min

#ifdef EEPROM_ENABLED
    //#include <avr/eeprom.h>
    static uint8_t EEPROM_read(uint8_t ucAddress);
    static void EEPROM_write(uint8_t ucAddress, uint8_t ucValue);
    static void EEPROM_save(uint8_t ucAddress, uint16_t ucValue, uint8_t ee);
#endif

#define UART_BAUDRATE   (9600)

#ifdef UART_TX_ENABLED
    #define TXDELAY         (uint8_t)(((F_CPU/UART_BAUDRATE)-7 +1.5)/3)
    #define UART_TX         PB3 // Use PB3 as TX pin
    void uart_putc(char c);
    void uart_putu(uint16_t x);
    void uart_puts(const char *s);
#endif

#ifdef UART_RX_ENABLED
    #define RXDELAY          (int)(((F_CPU/UART_BAUDRATE)-5 +1.5)/3)
    #define RXDELAY2         (int)((RXDELAY*1.5)-2.5)
    #define UART_RX          PB4 // Use PB4 as RX pin
    /*
    #define RXROUNDED         (((F_CPU/UART_BAUDRATE)-5 +2)/3)
    #if RXROUNDED > 127
    #error Low baud rates are not supported - use higher, UART_BAUDRATE
    #endif
    */
#endif

static void blink(uint8_t time, uint8_t duration);
static uint16_t sensorRead(uint8_t enablePin,uint8_t readPin);
static uint16_t ReadADC(uint8_t pin, uint8_t vref);

static uint16_t div3(uint16_t n);
//static uint16_t div10(uint16_t n);
/*
Tips and Tricks to Optimize Your C Code for 8-bit AVR Microcontrollers
https://ww1.microchip.com/downloads/en/AppNotes/doc8453.pdf
*/

#ifdef EEPROM_ENABLED
    uint8_t EEPROM_read(uint8_t ucAddress)
    {
        // Wait for completion of previous write
        while(EECR & (1<<EEPE))
        ;
        // Set up address register
        EEARL = ucAddress;
        // Start eeprom read by writing EERE
        EECR |= (1<<EERE);
        // Return data from data register
        return EEDR;
    }

    void EEPROM_write(uint8_t ucAddress, uint8_t ucValue)
    {
        // Wait for completion of previous write
        while(EECR & (1<<EEPE))
        ;
        // Set Programming mode
        //EECR = (0<<EEPM1)|(0>>EEPM0);
        EECR = (0<<EEPM1)|(0<<EEPM0);
        // Set up address and data registers
        EEARL = ucAddress;
        EEDR = ucValue;
        // Write logical one to EEMPE
        EECR |= (1<<EEMPE);
        // Start eeprom write by setting EEPE
        EECR |= (1<<EEPE);
    }

    void EEPROM_save(uint8_t ucAddress, uint16_t ucValue, uint8_t ee)
    {
    	//if (ee < 0xFF)
        if (ee == 0xEE || ee == 0xEF) //EEPROM wear reduction
        {
            if(ucValue > 255) //split into two epprom fields -> 388 = 38 + 8
            {
                uint8_t lo_hi[] = { (uint8_t)ucValue, (uint8_t)(ucValue >> 8) }; //0xAAFF = { 0xFF, 0xAA }
                EEPROM_write(ucAddress, lo_hi[0]);
                EEPROM_write((ucAddress + 1), lo_hi[1]);
            }else{
                EEPROM_write(ucAddress, ucValue);
                EEPROM_write((ucAddress + 1), 255);
                //eeprom_write_word((uint16_t*)ucAddress, ucValue);
            }
        }
    }
#endif

ISR(WDT_vect)
{
}

int main(void)
{
    //clock_prescale_set(3); //clock_div_8 = 3

    //=============
    //VARIABLE
    //=============
    uint16_t suitableMoisture = sensorMoisture; //Analog value with 10k pull-up resistor
    uint8_t deepSleep = delayWatering;
    uint8_t potSize = potSizeTimer;
    uint8_t runSolar = 0;
    uint8_t ee = 0xFF;
    uint16_t errorCode = 0;

    blink(10,4); //Delay before setting IO pins

    //=============
    //WATCHDOG
    //=============
    //wdt_reset();
    //MCUSR = 0; //Clear reset register
    //----------------
    #if defined __AVR_ATtiny45__ || defined __AVR_ATtiny85__
    	WDTCR = (1<<WDIE);  // Enable watchdog timer interrupts
    #else
        WDTCR = (1<<WDTIE);  // Enable watchdog timer interrupts 
    #endif

    //=================================
    //AVR I/O pins are input by default
    //=================================
    /*
    DDRB |= (1<<pumpPin); //Digital OUTPUT
    DDRB |= (1<<solarPin); //Digital OUTPUT
    DDRB |= (1<<sensorPin); //Digital OUTPUT
    DDRB |= (1<<ledPin); //Digital OUTPUT
    */
    DDRB = 0xFF; //All Pins as OUTPUT
    PORTB = 0x00; //All Pins OFF

    //=============
    //EEPROM
    //=============
    //eeprom_busy_wait();
    #ifdef EEPROM_ENABLED
        ee = EEPROM_read(0x02);
        //uint8_t ee = eeprom_read_byte((uint8_t*)0x02);
        if (ee == 255) //EEPROM is blank
        {
            EEPROM_save(0x00,versionID,0xEE);
            EEPROM_save(0x02,sensorMoisture,0xEE);
            EEPROM_save(0x04,potSize,0xEE);
            EEPROM_save(0x06,runSolar,0xEE);
            EEPROM_save(0x08,deepSleep,0xEE);

            #ifdef WS78L05
            	EEPROM_save(0xB,WS78L05,0xEE);
            #endif
            #ifdef LM2731
            	EEPROM_save(0xB,LM2731,0xEE);
            #endif
            #ifdef TPL5110
				EEPROM_save(0xB,TPL5110,0xEE);
            #endif

            EEPROM_save(0xF,SERIAL,0xEE); //Serial # - not UART

        }else{
            suitableMoisture = ee | (uint8_t)EEPROM_read(0x03) << 8;
            //suitableMoisture = eeprom_read_word((uint16_t*)0x02);

            potSize = EEPROM_read(0x04);
            //potSize = eeprom_read_byte((uint8_t*)0x04);

            runSolar = EEPROM_read(0x06);
            //runSolar = eeprom_read_byte((uint8_t*)0x06);
        }
        
        deepSleep = EEPROM_read(0x08);
        //deepSleep = eeprom_read_byte((uint8_t*)0x08);
        //EEPROM_save(0x08,delayWatering,0xEE); //reset for next time

        ee = EEPROM_read(0xA);
        //ee = eeprom_read_byte((uint8_t*)0xA);
        //EEPROM_save(0x01,0xFF,0xEE); //reset for next time
    #endif

    /*
    Note: Global variables cost a lot of flash size only use if needed.
    https://ww1.microchip.com/downloads/en/AppNotes/doc8453.pdf
    */
    uint8_t sleepLoop = 0;  //Track the passage of time
    uint8_t emptyBottle = 0;

    #ifdef UART_TX_ENABLED
        //================
        //ALIVE TEST
        //================
        //BODCR |= (1<<BODS)|(1<<BODSE); //Disable Brown Out Detector Control Register
        /*
        PORTB |= (1<<PB0); //ON
        _delay_ms(900);
        PORTB &= ~(1<<PB0); //OFF
        */
        uart_putc('.');
        uart_putu(versionID);
        //uart_putc('\r');
        //uart_putc('\n');
    #endif
    //================

    #ifdef SENSORLESS_ENABLED
        uint16_t moistureLog = 0;
    #endif

    /* 
    !!! IMPORTANT NOTE !!!

    "EEPROM Debug" avrdude will reset ATtiny on read loosing all variables.
    This means that variables like emptyBottle or moistureLog are difficult to debug.

    Solution: Write/Read to EEPROM while debugging.
    */

    if(runSolar == 1 || ee == 0xEE) {

    	emptyBottle = EEPROM_read(0x38);
        moistureLog = EEPROM_read(0x3A);

        //deepSleep = 0;

        set_sleep_mode(SLEEP_MODE_IDLE);
        WDTCR |= (0<<WDP3) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0); //Set timer 2s

    }else{

        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        //Table 8-2 page 43
        WDTCR |= (1<<WDP3) | (0<<WDP2) | (0<<WDP1) | (1<<WDP0); //Set timer 8s (max)
        //----------------
        //wdt_enable(WDTO_8S); // 8s
        // Valid delays:
        //  WDTO_15MS
        //  WDTO_30MS
        //  WDTO_60MS
        //  WDTO_120MS
        //  WDTO_250MS
        //  WDTO_500MS
        //  WDTO_1S
        //  WDTO_2S
        //  WDTO_4S
        //  WDTO_8S
    }
    //sleep_enable();

    sei(); // enable all interrupts
 
    for (;;) {
        
       /*
        uint8_t resetAVR = ReadADC(ledPin);
        if(resetAVR > 250) {
            //EEPROM_save(0x2A,resetAVR,ee); //DEBUG
            //https://forum.arduino.cc/index.php?topic=149235.0
            /
            asm ("cli");                    // interrupts off
            asm volatile ("eor r1,r1");     // make sure zero-register is zero
            asm volatile ("ldi r16,0xFF");  // end of RAM (0xFF)
            asm volatile ("sts 0x5E,r16");  // SPH
            asm volatile ("sts 0x5D,r16");  // SPL
            asm volatile ("eor r31,r31");   // Clear Z register
            asm volatile ("eor r30,r30");
            asm volatile ("ijmp");          // jump to (Z)
            /
            soft_reset();
        }
        */
        
        //power_all_disable(); // turn power off to ADC, TIMER 1 and 2, Serial Interface
        //power_adc_disable();
        //-------------
        sleep_mode(); //makes a call to three routines:  sleep_enable(); sleep_cpu(); sleep_disable();
        //sleep_enable(); //sets the Sleep Enable bit in the MCUCR register
        //sleep_cpu(); //issues the SLEEP command
        //sleep_disable(); //clears the SE bit
        //-------------
        //power_all_enable(); // put everything on again
        //power_adc_enable();

        if (sleepLoop > deepSleep)
        {
            if(emptyBottle >= 3) { //Low Water LED

                blink(9,254);

                //Retry every 2 hours ...when someone refilled the bottle but did not cycle power.
                if(ee == 0xFF)
                {
                    if((emptyBottle > 10 && sleepLoop > delayBetweenRefillReset) || (emptyBottle < 10 && sleepLoop > delayBetweenOverfloodReset))
                    {
                        soft_reset();
                    }
                }else{
                    emptyBottle = 0;
                    moistureLog = 0;
                }
                
            }else{
                sleepLoop = 0;
                //======================
                // Low Voltage Warning
                //======================
                uint16_t moisture = ReadADC(PB5, 5); //Self-VCC @ 5Vref
                /*
                694 = 2.8V
                673 = 3.0V
                655 = 3.2V
                600 = 4.0V
                555 = 5.0V
                */
                //Just before Brown-Out @ 2.7V
                if(moisture > 688) {
                	blink(255,22); //Warn to charge battery with LED
                }
                //======================
                //Prevents false-positive (empty detection)
                //Moisture sensor (too accurate) triggers exactly same value when dry
                //======================
                /*
                if (runSolar == 1 && sleepLogReset > delayBetweenLogReset) {
                    soft_reset();
                }else{
                    sleepLogReset++;
                }
                */
                //======================
                //Get Sensor Moisture
                //======================
                moisture = sensorRead(sensorPin,moistureSensorPin);

                #ifdef EEPROM_ENABLED
                    EEPROM_save(0x1A,moisture,ee);
                #endif
                if(ee == 0xEA) { //LED Monitor enabled
                    /*
                    int number = 1234;
                    (number) % 10 => 4
                    (number / 10) % 10 => 3
                    (number / 100) % 10 => 2
                    (number / 1000) % 10 => 1
                    */
                    //=======================
                    //DEBUG (LED MORSE CODE)
                    //=======================
                    //for (uint16_t i = 1000 ; i >= 1; i=div10(i)) {
                    for (uint16_t i = 100 ; i >= 1; i /= 10) {
                        uint8_t d = (moisture/i) % 10;
                        d = d + d;
                        blink(4,d); //blink a zero with a quick pulse
                        _delay_ms(1200);
                    }
                    //=======================
                }else {
                    //avoid this during the science project (data gathering)
                	if(ee == 0xFF && (moisture - suitableMoisture) > 300) { //soil is too wet for set threshold, wait longer before checking again
                		deepSleep = 255; //8 seconds x 255 = 35 min
                	}
                    blink(3,2);
                }

                if(moisture < 4) { //Sensor Not in Soil

                    #ifdef UART_TX_ENABLED
                        uart_putu(suitableMoisture);
                        //uart_putc('\r');
                        //uart_putc('\n');
                    #endif

                    _delay_ms(3000);
                    blink(1,18);

                #ifdef EEPROM_ENABLED
                }else if(moisture >= 1021) { //Sensor Manual Calibrate (cross/short both sensor leads)

                    #ifdef UART_TX_ENABLED
                        #ifdef ledPin
                            PORTB |= (1<<ledPin); //ON
                        #endif

                        for(uint8_t i = 0; i < 8; i++) //Get ready to place into base-line soil
                        {
                            uart_putc('.');
                            _delay_ms(800);
                        }
                        
                        #ifdef ledPin
                            PORTB &= ~(1<<ledPin); //OFF
                        #endif
                    #else
                        blink(9,9);
                    #endif

                    suitableMoisture = sensorRead(sensorPin,moistureSensorPin);

                    EEPROM_save(0x02,suitableMoisture,0xEE);
                #endif
                
                }else if (moisture < suitableMoisture) { //Water Plant

                    //===================
                    #ifdef SENSORLESS_ENABLED
                        //Detect Empty Bottle (Sensor-less)

                        //uint8_t m = moisture / 10;
                        moistureLog = moistureLog + moisture;

                        uint16_t mm = div3(moistureLog); //Average 3
                        //uint16_t mm = (moistureLog >> 2); //Average 4

                        if (mm > (moisture - 10) && mm < (moisture + 10)) {
                            emptyBottle = 11; //Pump ran but no change in moisture
                            //continue; //force next wait loop
                        }

                        errorCode = mm;
                    #endif

                    //===================
                    //Detect Empty Bottle (Sensored)
                    //===================
                    /*
                    loopback wire from water jug to Pin3 (PB4)
                    given power from Pin5 (PB0) while turning on NPN transistor
                    more accurate than sensor-less detection
                    */
                    /*
                    moisture = sensorRead(pumpPin,moistureSensorPin);

                    #ifdef EEPROM_ENABLED
                        EEPROM_save(0x1C,moisture,ee);
                    #endif

                    if (moisture > 10 && moisture < 255) { //avoid 0 detection if wire not connected
                        emptyBottle = 12;

                        #ifdef UART_TX_ENABLED
                            uart_putc('E');
                        #endif
                    }
                    */
                    if(emptyBottle < 3) { //Prevent flooding

                        #ifdef UART_TX_ENABLED
                            uart_putc('P');
                        #endif

                        if(ee != 0xEE) { //prevent actual water pumping during debug
                            PORTB |= (1<<pumpPin); //ON
                        }
                        //_delay_ms(6800); //6.8 seconds;
                        blink(potSize,2);
                        PORTB &= ~(1<<pumpPin); //OFF

                        //When battery < 3V (without regulator) ADC readouts are unstable
                        emptyBottle++;
                    }
                }else{
                    emptyBottle = 0;
                    moistureLog = 0;
                    errorCode = 0;
                }
            }
            #ifdef EEPROM_ENABLED
                EEPROM_save(0x38,emptyBottle,ee);
                EEPROM_save(0x3A,moistureLog,ee);
                EEPROM_save(0x2A,errorCode,ee);
            #endif
        }

        #ifdef SOLAR_ENABLED
            if(runSolar == 1) {
                /*
                (Optional TPL5110 = extreme efficiency 50nA! (nano-amp) sleep. Bypasses ATtiny ligic. Solar pin = DONE pin = Turnoff ATtiny)
                Note: Install both TPL5110 & LM2731
                */
                #ifdef TPL5110
                    DDRB |= (1<<solarPin); //Digital
                    PORTB |= (1<<solarPin); //High (5V)
                #endif
                /*
                (Optional TPL5110: If we are not off at this point TPL5110 was not installed,
                Do solar with ATtiny (Highly NOT recommened ...TPL5110 is the way to go)
                */

                //DDRB &= ~(1<<solarSensorPin); //Shared pin with Sensor, Set Analog INPUT
                uint16_t solarVoltage = ReadADC(solarSensorPin, 1); //Detect Solar intensity - 300R inline + 10k pullup
                //DDRB |= (1<<solarSensorPin); //Shared pin with Sensor, set Digital OUTPUT
                
                #ifdef UART_TX_ENABLED
                    uart_putc(',');
                    uart_putu(solarVoltage);
                #endif

                #ifdef EEPROM_ENABLED
                    EEPROM_save(0xC,solarVoltage,ee);
                #endif

                /*
                Regulators

                WS78L05 (TO92) linear "low efficiency" - turn on with GND
                LM2731 (SOT23) switching "high efficiency" - turn off with 5V

                Note: with WS78L05 5V Solar ONLY, LM2731 5V-10V
                */

                //420(@4.2V) = start, over 510(@5V) = direct sunlight ...go to else and stay ON
                if(solarVoltage > 420 && solarVoltage < 510) //Regulator OFF (Charge capacitor)
                {
                    //when sleep mode this creates 8 second pulse
                    #ifdef WS78L05
                        DDRB &= ~(1<<solarPin); //Analog (Off)
                    #else
                        DDRB |= (1<<solarPin); //Digital
                        PORTB |= (1<<solarPin); //High (5V)
                    #endif
                }else{ //Regulator ON (Discharge capacitor)
                    DDRB |= (1<<solarPin); //Digital
                    PORTB &= ~(1<<solarPin); //Low (GND)
                }
            }
        #endif
            
        sleepLoop++;
    }
    return 0;
}

uint16_t div3(uint16_t n) {
    uint16_t q = 0;
    while(1){
        if(!(n >>= 1)) return q;
        q += n;
        if(!(n >>= 1)) return q;
        q -= n;
    }
}

/*
uint16_t div10(uint16_t n) {
    uint16_t q = (n >> 1) + (n >> 2);
    q = q + (q >> 4);
    q = q + (q >> 8); // not needed for 8 bit version
    q = q >> 3;
    uint16_t r = n - (((q << 2) + q) << 1); // r = n - q*10; (mod)
    return q + (r > 9); //div
    //if (r > 9) mod = r - 10;
    //else mod = r;
}
*/
uint16_t sensorRead(uint8_t enablePin, uint8_t readPin)
{
    DDRB |= (1<<enablePin); //Digital OUTPUT
    DDRB &= ~(1<<readPin); //Analog INPUT

    PORTB |= (1<<enablePin); //ON
    uint16_t value = ReadADC(readPin, 1);
    PORTB &= ~(1<<enablePin); //OFF

    DDRB |= (1<<readPin);   //Digital OUTPUT
    PORTB &= ~(1<<readPin); //OFF

    #ifdef UART_TX_ENABLED
        uart_putc(',');
        uart_putu(value);
    #endif

    return value;
}

void blink(uint8_t time, uint8_t duration)
{
    //#if defined __AVR_ATtiny45__ || defined __AVR_ATtiny85__ //@8Mhz
    //    time /= 16;
    //#endif

    if(duration == 0) {
        duration = 2;
        time = 1;
    }

    #ifdef ledPin
        //DDRB |= (1<<ledPin); //Digital OUTPUT
        do {
            PORTB ^= (1<<ledPin); //Toggle ON/OFF
            uint8_t i = time;
            do {
                _delay_ms(100);
                i--;
            } while (i);
            duration--;
        } while (duration);
    #endif
}

uint16_t ReadADC(uint8_t pin, uint8_t vref)
{
	/*
	Set VCC as internal reference voltage
    http://maxembedded.com/2011/06/the-adc-of-the-avr/
	*/
    #if defined __AVR_ATtiny45__ || defined __AVR_ATtiny85__
    	ADMUX = ((0 << REFS2) | (1 << REFS1) | (0 << REFS0)); // 1.1V
       //ADMUX = ((1 << REFS2) | (1 << REFS1) | (0 << REFS0)); // 2.56V
       if(vref == 5)
       		ADMUX = ((0 << REFS2) | (0 << REFS1) | (0 << REFS0)); // 5.0V
    #else
       	ADMUX = (1 << REFS0);  // 1.1V
    	if(vref == 5)
        	ADMUX = (0 << REFS0);  // 5.0V
    #endif

    /*
    if(pin == PB5) { // ADC0
        ADMUX |= (0 << MUX0) | (0 << MUX1); //ADC0 PB5 as analog input channel
    }else if(pin == PB2) { // ADC1
        ADMUX |= (1 << MUX0) | (0 << MUX1); //ADC1 PB2 as analog input channel
    }*/

    if(pin == PB3) { // ADC3
        ADMUX = (1 << MUX0) | (1 << MUX1); //ADC3 PB3 as analog input channel
    }else if (pin == PB4){ // ADC2
        ADMUX = (0 << MUX0) | (1 << MUX1); //ADC2 PB4 as analog input channel
    }

    //--------------
    //ADMUX |= (0 << ADLAR);  //Right adjust for 10-bit resolution
    //ADMUX |= (1 << ADLAR);  //Left adjusts for 8-bit resolution
    //--------------
    // See ATtiny13 datasheet, Table 14.4.
    // Predefined division factors – 2, 4, 8, 16, 32, 64, and 128. For example, a prescaler of 64 implies F_ADC = F_CPU/64.
    // For F_CPU = 16MHz, F_ADC = 16M/64 = 250kHz. Greater the frequency, lesser the accuracy.
    //ADCSRA |= (1 << ADPS1) | (1 << ADPS0);                // Prescaler of 8
    //ADCSRA |= (1 << ADPS2) | (1 << ADPS1);                // Prescaler of 64
    //ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Prescaler of 128
    //--------------
    ADCSRA |= (1 << ADEN); // Enables ADC
    //_delay_ms(200); // Wait for Vref to settle

    uint16_t result = 0;
    /*
    Undocumented bug on selecting the reference as an input.
	The reference has a very high impedance, so it need several conversions to charge the sample&hold stage to the final value.
	You need at least throw away 7 conversions and trust the last.
	*/
    //for(uint8_t i = 0; i < 8; i++) {
    for(uint8_t i = 0; i < 16; i++) {
    
    	ADCSRA |= (1 << ADSC); // Start conversion by writing 1 to ADSC
    	
        //uint16_t a = ReadADC(pin);
        //=============
        //Take Highest
        //=============
        //if (a > m) {
        //    m = a;
        //}
        //=============
        //Take Lowest
        //=============
        //if (a < moisture) {
        //    m = a;
        //}
        //=============
        //Average
        //=============

        // Wait conversion is done
        while ((ADCSRA & (1 << ADSC)) != 0);

        // Read values
        if(i < 8) { //throw away first 8 conversions
        	ADC;
    	}else{ //average the other 8 conversions
    		result += ADC; // For 10-bit resolution (includes ADCL + ADCH)
    	}
        //------------------
        //https://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage
        //------------------
        //uint8_t low = ADCL;
        //uint8_t high = ADCH; //(for 8-bit only)
        //result = (high << 8) | low; // Combine two bytes
        //result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
        //------------------
        //Vcc_value = (0x400 * 1.1 ) / (ADC * 0x100); // Calculate Vcc
    }

    ADCSRA &= ~ (1 << ADEN); // Disables ADC

    return (result >> 3); //Average 8
    //return (result >> 4); //Average 16
    //return (result);
}

/**
 * Copyright (c) 2017, Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 * Software UART for ATtiny13
 */

#ifdef UART_TX_ENABLED
void uart_putc(char c)
{
    uint8_t sreg;
    sreg = SREG;

    cli(); // disable all interrupts
    PORTB |= (1 << UART_TX);
    DDRB |= (1 << UART_TX);
    asm volatile(
        " cbi %[uart_port], %[uart_pin] \n\t" // start bit
        " in r0, %[uart_port] \n\t"
        " ldi r30, 3 \n\t" // stop bit + idle state
        " ldi r28, %[txdelay] \n\t"
        "TxLoop: \n\t"
        // 8 cycle loop + delay - total = 7 + 3*r22
        " mov r29, r28 \n\t"
        "TxDelay: \n\t"
        // delay (3 cycle * delayCount) - 1
        " dec r29 \n\t"
        " brne TxDelay \n\t"
        " bst %[ch], 0 \n\t"
        " bld r0, %[uart_pin] \n\t"
        " lsr r30 \n\t"
        " ror %[ch] \n\t"
        " out %[uart_port], r0 \n\t"
        " brne TxLoop \n\t"
        :
        : [uart_port] "I" (_SFR_IO_ADDR(PORTB)),
        [uart_pin] "I" (UART_TX),
        [txdelay] "I" (TXDELAY),
        [ch] "r" (c)
        : "r0","r28","r29","r30"
    );
    SREG = sreg;
}

void uart_putu(uint16_t x)
{
    char buff[8] = {0};
    char *p = buff+6;
    do { *(p--) = (x % 10) + '0'; x /= 10; } while(x);
    uart_puts((const char *)(p+1));
}

void uart_puts(const char *s)
{
    while (*s) uart_putc(*(s++));
}
#endif

#ifdef UART_RX_ENABLED

char uart_getc(void)
{
    char c;
    uint8_t sreg;
    sreg = SREG;

    cli(); // disable all interrupts
    PORTB &= ~(1 << UART_RX);
    DDRB &= ~(1 << UART_RX);
    asm volatile(
        " ldi r18, %[rxdelay2] \n\t" // 1.5 bit delay
        " ldi %0, 0x80 \n\t" // bit shift counter
        "WaitStart: \n\t"
        " sbic %[uart_port]-2, %[uart_pin] \n\t" // wait for start edge
        " rjmp WaitStart \n\t"
        "RxBit: \n\t"
        // 6 cycle loop + delay - total = 5 + 3*r22
        // delay (3 cycle * r18) -1 and clear carry with subi
        " subi r18, 1 \n\t"
        " brne RxBit \n\t"
        " ldi r18, %[rxdelay] \n\t"
        " sbic %[uart_port]-2, %[uart_pin] \n\t" // check UART PIN
        " sec \n\t"
        " ror %0 \n\t"
        " brcc RxBit \n\t"
        "StopBit: \n\t"
        " dec r18 \n\t"
        " brne StopBit \n\t"
        : "=r" (c)
        : [uart_port] "I" (_SFR_IO_ADDR(PORTB)),
        [uart_pin] "I" (UART_RX),
        [rxdelay] "I" (RXDELAY),
        [rxdelay2] "I" (RXDELAY2)
        : "r0","r18","r19"
    );
    SREG = sreg;
    return c;
}
#endif