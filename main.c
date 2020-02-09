/*  
    FUSE_L=0x6A (Clock divide fuse enabled = 8Mhz CPU frequency is actually 1MHz)
    FUSE_H=0xFF (0xFE (no bootloader) / 0xEE (bootloader) = RSTDISBL -> CATION: If enabled chip can only be programmed once)
    FUSE_H=0xFB (BODLEVEL 0xFD = 1.8V, 0xFB = 2.7V, 0xFF = BOD Disabled)

    Slowing down CPU to save power
    CPU @ 1.2Mhz / 8 = 150 Khz
*/
#ifndef F_CPU
    #define F_CPU 1200000 //CPU @ 1.2Mhz / 8 = 150 Khz
    //#define F_CPU 128000 //CPU @ 128Khz / 8 = 16 Khz
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
#define LOG_ENABLED
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
    #define sensorMoisture          388 //ADC value
    #define sensorMoistureOffset    20  //ADC calibration offset
#endif
#ifndef potSize
 #define potSizeTimer               60 //(6 seconds)
#endif

#define pumpPin                     PB1 //Output
#define sensorPin                   PB3 //Output
#define solarPin                    PB0 //Output
#define ledPin                      PB2 //Output
#define solarSensorPin              PB3 //Input/Output
#define moistureSensorPin           PB4 //Input
#define delayWatering               40  //8seconds x 40 = 5.5 min
#define delayBetweenSolarDischarge  4   //8seconds x 5 = .5 min
#ifdef LOG_ENABLED
    #define delayBetweenLogReset        20   //8seconds x 40 x 20 = 1.5 hours
    #define delayBetweenRefillReset     1024 //8seconds x 1024 = 2 hours
#endif

#ifdef EEPROM_ENABLED
    //#include <avr/eeprom.h>
    static uint8_t EEPROM_read(uint8_t ucAddress);
    static void EEPROM_write(uint8_t ucAddress, uint8_t ucData);
    static void EEPROM_save(uint16_t ucCode, uint16_t ucValue, uint8_t ee);
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
static uint16_t sensorRead(uint8_t enablePin,uint8_t readPin, uint8_t loop);
static uint16_t ReadADCHighest(uint8_t pin, uint8_t loop, uint16_t moisture);
static uint16_t ReadADC(uint8_t pin);

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

    void EEPROM_write(uint8_t ucAddress, uint8_t ucData)
    {
        // Wait for completion of previous write
        while(EECR & (1<<EEPE))
        ;
        // Set Programming mode
        //EECR = (0<<EEPM1)|(0>>EEPM0);
        EECR = (0<<EEPM1)|(0<<EEPM0);
        // Set up address and data registers
        EEARL = ucAddress;
        EEDR = ucData;
        // Write logical one to EEMPE
        EECR |= (1<<EEMPE);
        // Start eeprom write by setting EEPE
        EECR |= (1<<EEPE);
    }

    void EEPROM_save(uint16_t ucCode, uint16_t ucValue, uint8_t ee)
    {
        if (ee == 0xEE) //EEPROM wear reduction
        {
            if(ucValue > 255) //split into two epprom fields -> 388 = 38 + 8
            {
                //uint8_t hi_lo[] = { (uint8_t)(ucValue >> 8), (uint8_t)ucValue }; //0xAAFF = { 0xAA, 0xFF }
                //EEPROM_write(ucCode, hi_lo[0]);
                //EEPROM_write((ucCode + 1), hi_lo[1]);

                uint8_t lo_hi[] = { (uint8_t)ucValue, (uint8_t)(ucValue >> 8) }; //0xAAFF = { 0xFF, 0xAA }
                EEPROM_write(ucCode, lo_hi[0]);
                EEPROM_write((ucCode + 1), lo_hi[1]);

            }else{
                EEPROM_write(ucCode, ucValue);
                //eeprom_write_word((uint16_t*)ucCode, ucValue);
            }
        }
    }
#endif

ISR(WDT_vect)
{
}

int main(void)
{
    //=============
    //128Khz ONLY
    //=============
    clock_prescale_set(3); //clock_div_8 = 3

    //=============
    //VARIABLE
    //=============
    uint16_t suitableMoisture = sensorMoisture; //Analog value with 10k pull-up resistor
    uint8_t deepSleep = delayWatering;
    uint8_t potSize = potSizeTimer;
    uint8_t runSolar = 1;
    uint8_t ee = 0xFF;

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
    //WDTCR |= (1<<WDP3); // (1<<WDP2) | (1<<WDP0); //Set timer 4s (max)
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
    sei(); // enable all interrupts

    //=============
    //I/O
    //=============
    //DDRB |= (1<<pumpPin); //Digital OUTPUT
    //DDRB |= (1<<sensorPin); //Digital OUTPUT

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
            	EEPROM_save(0xA,WS78L05,0xEE);
            #endif
            #ifdef LM2731
            	EEPROM_save(0xA,LM2731,0xEE);
            #endif
            #ifdef TPL5110
				EEPROM_save(0xA,TPL5110,0xEE);
            #endif

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

        ee = EEPROM_read(0x01);
        //ee = eeprom_read_byte((uint8_t*)0x01);
        //EEPROM_save(0x01,0xFF,0xEE); //reset for next time
    #endif

    /*
    Note: Global variables cost a lot of flash size only use if needed.
    https://ww1.microchip.com/downloads/en/AppNotes/doc8453.pdf
    */
    uint8_t sleepLoop = 0;  //Track the passage of time
    
    #ifdef LOG_ENABLED
        uint8_t sleepLogReset = 0; //Reset logs once in a while
    #endif
    
    #ifdef SENSORLESS_ENABLED
        uint16_t moistureLog[3] = {0,0,0}; //Collect past logs for empty detection
    #endif
    //uint8_t overwaterProtection = 0;
    uint8_t emptyBottle = 8;
    //uint16_t moisture = 0;

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
        
    blink(2,4);
    //================

    #ifdef SOLAR_ENABLED
        if(runSolar == 1) {
            DDRB |= (1<<solarPin); //Digital OUTPUT
            suitableMoisture += sensorMoistureOffset;
        }else{
            if(ee == 0xFF) {
                set_sleep_mode(SLEEP_MODE_PWR_DOWN);
                //set_sleep_mode(SLEEP_MODE_IDLE);
                sleep_enable();
            }
        }
    #endif

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
        //sleep_mode(); //makes a call to three routines:  sleep_enable(); sleep_cpu(); sleep_disable();
        //sleep_enable(); //sets the Sleep Enable bit in the MCUCR register
        sleep_cpu(); //issues the SLEEP command
        //sleep_disable(); //clears the SE bit
        //-------------
        //power_all_enable(); // put everything on again
        //power_adc_enable();

        if (sleepLoop > deepSleep)
        {
            if(emptyBottle < 8) { //Low Water LED

                blink(9,200);
                /*
                //Retry every 2 hours ...when someone refilled the bottle but did not cycle power.
                if(sleepLoop > delayBetweenRefillReset)
                {
                    soft_reset();
                }
                */
                emptyBottle = 8;

            }else{
                sleepLoop = 0;
                //======================
                //Prevents false-positive (empty detection)
                //Moisture sensor (too accurate) triggers exactly same value when dry
                //======================
                if (runSolar == 1 && sleepLogReset > delayBetweenLogReset) {
                    soft_reset();
                }else{
                    sleepLogReset++;
                }
                //======================
                //blink(4,2);

                uint16_t moisture = sensorRead(sensorPin,moistureSensorPin,10);

                #ifdef EEPROM_ENABLED
                    EEPROM_save(0x1A,moisture,ee);
                #endif

                if(moisture <= 4) { //Sensor Not in Soil

                    #ifdef UART_TX_ENABLED
                        uart_putu(suitableMoisture);
                        //uart_putc('\r');
                        //uart_putc('\n');
                    #endif

                    blink(4,4);

                #ifdef EEPROM_ENABLED
                }else if(moisture >= 1021) { //Sensor Manual Calibrate (cross/short both sensor leads)

                    #ifdef ledPin
                        PORTB |= (1<<ledPin); //ON
                    #endif

                    #ifdef UART_TX_ENABLED
                        for(uint8_t i = 0; i < 8; ++i) //Get ready to place into base-line soil
                        {
                            uart_putc('.');
                            _delay_ms(800);
                        }
                    #endif

                    #ifdef ledPin
                        PORTB &= ~(1<<ledPin); //OFF
                    #endif

                    suitableMoisture = sensorRead(sensorPin,moistureSensorPin,8);

                    EEPROM_save(0x02,suitableMoisture,0xEE);
                #endif
                    
                }else if (moisture < suitableMoisture) { //Water Plant

                    //===================
                    #ifdef SENSORLESS_ENABLED
                        //Detect Empty Bottle (Sensor-less)
                        if(moistureLog[0] == 0) {
                            moistureLog[0] = moisture;
                        }else if(moistureLog[1] == 0) {
                            moistureLog[1] = moisture;
                        }else if(moistureLog[2] == 0) {
                            moistureLog[2] = moisture;
                        }else{
                            uint16_t m = (moistureLog[0] + moistureLog[1] + moistureLog[2]) / 3; //Average
                            //uart_send_line("M",m);

                            if (m >= (moisture - 2) && m <= (moisture + 2)) {
                                emptyBottle = 1; //Pump ran but no change in moisture
                                //continue; //force next wait loop
                            }else{
                                #ifdef EEPROM_ENABLED
                                    EEPROM_save(0x2A,200,ee);
                                #endif
                                soft_reset();
                            }
                        }
                    #endif

                    //===================
                    //Detect Empty Bottle (Sensored)
                    //===================
                    /*
                    loopback wire from water jug to Pin3 (PB4)
                    given power from Pin5 (PB0) while turning on NPN transistor
                    more accurate than sensor-less detection
                    */
                    
                    moisture = sensorRead(pumpPin,moistureSensorPin,1);

                    #ifdef EEPROM_ENABLED
                        EEPROM_save(0x1C,moisture,ee);
                    #endif

                    if (moisture > 50 && moisture < 254) { //avoid 0 detection if wire not connected
                        emptyBottle = 2;

                        #ifdef UART_TX_ENABLED
                            uart_putc('E');
                        #endif
                    }else{ //Bottle must be empty do not pump air
           
                        if(emptyBottle < 11) { //Prevent flooding
                            #ifdef UART_TX_ENABLED
                                uart_putc('P');
                            #endif

                            PORTB |= (1<<PB0); //ON
                            //_delay_ms(6800); //6.8 seconds;
                            blink(potSize,2);
                            PORTB &= ~(1<<PB0); //OFF

                            //When battery < 3V (without regulator) ADC readouts are unstable
                            emptyBottle++; //overwaterProtection++; 
                        }
                    }
                }else{
                    emptyBottle = 8; //overwaterProtection = 0;
                }
                #ifdef EEPROM_ENABLED
                    EEPROM_save(0x2A,emptyBottle,ee);
                    //EEPROM_save(0x1E,overwaterProtection,ee);
                #endif
            }
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
                uint16_t solarVoltage = ReadADCHighest(solarSensorPin,4,0); //Detect Solar intensity - 1k inline + 10k pullup
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

uint16_t sensorRead(uint8_t enablePin, uint8_t readPin, uint8_t loop)
{
    DDRB |= (1<<enablePin); //Digital OUTPUT
    DDRB &= ~(1<<readPin); //Analog INPUT

    PORTB |= (1<<enablePin); //ON
    //uint16_t value = ReadADC(readPin);
    uint16_t value = ReadADCHighest(readPin,loop,0);
    PORTB &= ~(1<<enablePin); //OFF

    DDRB |= (1<<readPin); //Digital OUTPUT
    PORTB &= ~(1<<readPin); //OFF

    #ifdef UART_TX_ENABLED
        uart_putc(',');
        uart_putu(value);
    #endif

    return value;
}

void blink(uint8_t time, uint8_t duration)
{
    #ifdef ledPin
        DDRB |= (1<<ledPin); //Digital OUTPUT
        do {
            PORTB ^= (1<<ledPin); //Toggle ON/OFF
            uint8_t i = time;
            do {
                _delay_ms(100);
                i--;
            } while (i);
            duration--;
        } while (duration);
        DDRB &= ~(1<<ledPin); //Analog INPUT (Acts as RESET sense)
    #endif
}

uint16_t ReadADCHighest(uint8_t pin, uint8_t loop, uint16_t moisture)
{
    for(uint8_t i = 0; i < loop; ++i)
    {
        //uint16_t a = ReadADC(pin);
        //=============
        //Take Highest
        //=============
        //if (a > moisture) {
        //    moisture = a;
        //}
        //=============
        //Take Lowest
        //=============
        //if (a < moisture) {
        //    moisture = a;
        //}
        //=============
        //Average
        //=============
        moisture += ReadADC(pin);
    }

    return (moisture / loop);
    //return moisture;
}

uint16_t ReadADC(uint8_t pin) {

    //http://maxembedded.com/2011/06/the-adc-of-the-avr/

    ADMUX = (0 << REFS0);     //Set VCC as reference voltage (5V)
    //ADMUX |= (1 << REFS0);  //Set VCC as reference voltage (Internal 1.1V)

    /*
    if(pin == PB5) { // ADC0
        ADMUX |= (0 << MUX0) | (0 << MUX1); //ADC0 PB5 as analog input channel
    }else if(pin == PB2) { // ADC1
        ADMUX |= (1 << MUX0) | (0 << MUX1); //ADC1 PB2 as analog input channel
    }*/
    if(pin == PB3) { // ADC3
        ADMUX |= (1 << MUX0) | (1 << MUX1); //ADC3 PB3 as analog input channel
    }else if (pin == PB4){ // ADC2
        ADMUX |= (0 << MUX0) | (1 << MUX1); //ADC2 PB4 as analog input channel
    }
    
    //--------------
    //ADMUX |= (1 << ADLAR);  //Left adjusts for 8-bit resolution
    //--------------
    // See ATtiny13 datasheet, Table 14.4.
    // Predefined division factors – 2, 4, 8, 16, 32, 64, and 128. For example, a prescaler of 64 implies F_ADC = F_CPU/64.
    // For F_CPU = 16MHz, F_ADC = 16M/64 = 250kHz. Greater the frequency, lesser the accuracy.

    //ADCSRA |= (1 << ADPS1) | (1 << ADPS0);                // Prescaler of 8
    //ADCSRA |= (1 << ADPS2) | (1 << ADPS1);                // Prescaler of 64
    ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Prescaler of 128
    //--------------
    ADCSRA |= (1 << ADEN); // Enables ADC

    //_delay_ms(200); // Wait for Vref to settle
    
    ADCSRA |= (1 << ADSC); // Start conversion by writing 1 to ADSC
    while (ADCSRA & (1 << ADSC));
    //while(bit_is_set(ADCSRA, ADSC)); // Wait conversion is done

    // Read values
    uint16_t result = ADC; // For 10-bit resolution (includes ADCL + ADCH)
    //uint8_t low = ADCL;
    //uint8_t high = ADCH;
    //uint16_t result = (high << 8) | low; // Combine two bytes
    //result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000

    ADCSRA &= ~ (1 << ADEN); // Disables ADC

    return result;
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