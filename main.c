/*  
    FUSE_L=0x6A (Clock divide fuse enabled = 8Mhz CPU frequency is actually 1MHz)
    FUSE_H=0xFF (0xFE (no bootloader) / 0xEE (bootloader) = RSTDISBL -> CATION: If enabled chip can only be programmed once)
    FUSE_H=0xFB (BODLEVEL 0xFD = 1.8V, 0xFB = 2.7V, 0xFF = BOD Disabled)

    Slowing down CPU to save power
    CPU @ 1.2Mhz / 8 = 150 Khz
*/
#ifndef F_CPU
    #define F_CPU (1200000UL)
#endif
/*
             [ATtiny13A]
              +------+
(LED)   PB5  1| O    |8  VCC
(TX)    PB3  2|      |7  PB2 (Solar)
(RX/A0) PB4  3|      |6  PB1 (Sensor)
        GND  4|      |5  PB0 (Pump)
              +------+
*/

/* HARDWARE CONFIGURATION */
/*------------------------*/
#define WS78L05 //LM2731
//#define EEPROM_ENABLED
#define UART_TX_ENABLED
//#define UART_RX_ENABLED
//#define SOLAR_ENABLED
//#define SENSORLESS_ENABLED
//#define LOG_ENABLED
/*------------------------*/

//#include <avr/pgmspace.h>
//#include <avr/eeprom.h>
//#include <stdio.h>
//#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
//#include <avr/wdt.h>
//#include <avr/power.h>

#ifndef versionID
    #define versionID          4
#endif
#ifndef sensorMoisture
    #define sensorMoisture          388
#endif
#ifndef potSize
 #define potSize                    32 //32 x 2 x 100 = 6400 miliseconds (6.4 seconds)
#endif

#define pumpPin                     PB0 //Output
#define sensorPin                   PB1 //Output
#define solarPin                    PB2 //Output
#define ledPin                      PB5 //Output
#define solarSensorPin              PB3 //Input/Output
#define moistureSensorPin           PB4 //Input
#define delayBetweenWaterings       40  //8seconds x 40 = 5.5 min
#define delayBetweenSolarDischarge  4   //8seconds x 5 = .5 min
#ifdef LOG_ENABLED
    #define delayBetweenLogReset        20   //8seconds x 40 x 20 = 1.5 hours
    #define delayBetweenRefillReset     1024 //8seconds x 1024 = 2 hours
#endif

#ifdef EEPROM_ENABLED
    static unsigned char EEPROM_read(unsigned char ucAddress);
    static void EEPROM_write(unsigned char ucAddress, unsigned char ucData);
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
static uint16_t sensorRead(uint8_t pin);
static uint16_t ReadADC(uint8_t pin);

/*
Tips and Tricks to Optimize Your C Code for 8-bit AVR Microcontrollers
https://ww1.microchip.com/downloads/en/AppNotes/doc8453.pdf
*/

#ifdef EEPROM_ENABLED
    unsigned char EEPROM_read(unsigned char ucAddress)
    {
        /* Wait for completion of previous write */
        while(EECR & (1<<EEPE))
        ;
        /* Set up address register */
        EEARL = ucAddress;
        /* Start eeprom read by writing EERE */
        EECR |= (1<<EERE);
        /* Return data from data register */
        return EEDR;
    }

    void EEPROM_write(unsigned char ucAddress, unsigned char ucData)
    {
        /* Wait for completion of previous write */
        while(EECR & (1<<EEPE))
        ;
        /* Set Programming mode */
        //EECR = (0<<EEPM1)|(0>>EEPM0);
        EECR = (0<<EEPM1)|(0<<EEPM0);
        /* Set up address and data registers */
        EEARL = ucAddress;
        EEDR = ucData;
        /* Write logical one to EEMPE */
        EECR |= (1<<EEMPE);
        /* Start eeprom write by setting EEPE */
        EECR |= (1<<EEPE);
    }
#endif

ISR(WDT_vect)
{
}

int main(void)
{
    uint16_t suitableMoisture = sensorMoisture; //Analog value with 10k pull-up resistor

    DDRB |= (1<<pumpPin); //Digital OUTPUT
    DDRB |= (1<<sensorPin); //Digital OUTPUT

    #ifdef SOLAR_ENABLED
        DDRB |= (1<<solarPin); //Digital OUTPUT
    #endif

    #ifdef ledPin
        DDRB |= (1<<ledPin); //Digital OUTPUT
    #endif

    DDRB &= ~(1<<moistureSensorPin); //Analog INPUT

    //================
    //EEPROM
    //================
    #ifdef EEPROM_ENABLED
        unsigned char suitableMoisture_ee = EEPROM_read(0x01);
        if (suitableMoisture_ee == 255) //EEPROM is blank
        {
            EEPROM_write(0x01, (suitableMoisture/10)); //max 255 we try to fit 10x
        }else{
            suitableMoisture = (suitableMoisture_ee * 10);
        }
        //EEPROM_write(0x01,_suitableMoisture);
    #endif
    //================
    //WATCHDOG
    //================
    //cli(); // disable all interrupts
    //wdt_reset();
    //----------------
    WDTCR = (1<<WDTIE);  // Enable watchdog timer interrupts
    //WDTCR = (1<<WDP3); // (1<<WDP2) | (1<<WDP0); //Set timer 4s (max)
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

    /*
    Note: Global variables cost a lot of flash size only use if needed.
    https://ww1.microchip.com/downloads/en/AppNotes/doc8453.pdf
    */
    uint8_t sleepLoop = 0;  //Track the passage of time
    /*
    #ifdef LOG_ENABLED
        uint8_t sleepLogReset = 0; //Reset logs once in a while
    #endif
    */
    #ifdef SENSORLESS_ENABLED
        uint16_t moistureLog[3] = {0,0,0}; //Collect past logs for empty detection
    #endif
    uint8_t overwaterProtection = 0;
    uint8_t emptyBottle = 0;
    uint16_t moisture = 0;

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

        //Chip Identifier for Web Interface
        #ifdef ATTINY45
            uart_putc('$');  //$=attiny45
        #else
            uart_putc('!'); //!=attiny13
        #endif
        uart_putu(versionID); 
        //uart_putc('\r');
        //uart_putc('\n');
    #endif
    blink(2,4);
    //================

    #ifndef SOLAR_ENABLED
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        //set_sleep_mode(SLEEP_MODE_IDLE);

        sleep_enable();
    #endif

    for (;;) {
        
        //power_all_disable(); // turn power off to ADC, TIMER 1 and 2, Serial Interface
        //power_adc_disable();
        //-------------
        //sleep_mode(); //makes a call to three routines:  sleep_enable(); sleep_cpu(); sleep_disable();
        //sleep_enable(); //sets the Sleep Enable bit in the MCUCR register
        sleep_cpu(); //issues the SLEEP command
        //sleep_disable(); //clears the SE bit.
        //-------------
        //power_all_enable(); // put everything on again
        //power_adc_enable();

        #ifdef SOLAR_ENABLED

            #ifdef UART_TX_ENABLED
                DDRB &= ~(1<<solarSensorPin); //Shared pin with UART, Set Analog INPUT
            #endif

            moisture = ReadADC(solarSensorPin); //Detect Solar intensity - 10k inline + 10k pullup

            #ifdef UART_TX_ENABLED
                //DDRB |= (1<<solarSensorPin); //Shared pin with UART, set Digital OUTPUT
                uart_putc(',');
                uart_putu(moisture);
            #endif

            /*
            Regulators

            WS78L05 (TO92) linear "low efficiency" - turn on with GND
            LM2731 (SOT23) switching "high efficiency" - turn off with 5V

            Note: with WS78L05 5V Solar ONLY, LM2731 5V-10V
            */

            //420(@4.2V) = start, over 510(@5V) = direct sunlight ...go to else and stay ON
            if(moisture > 420 && moisture < 510) //Regulator OFF (Charge capacitor)
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
        #endif

        if (sleepLoop > delayBetweenWaterings)
        {
            #ifdef LOG_ENABLED
                uint8_t resetLog = 0;
            #endif

            if(emptyBottle == 1) { //Low Water LED

                blink(9,200);

                #ifdef LOG_ENABLED
                    //Retry every 2 hours ...when someone refilled the bottle but did not cycle power.
                    if(sleepLoop > delayBetweenRefillReset)
                    {
                        resetLog = 1;
                    }
                #else
                    emptyBottle = 0;
                #endif

            }else{
                sleepLoop = 0;
                //======================
                //Prevents false-positive (empty detection)
                //Moisture sensor (too accurate) triggers exactly same value when dry
                //======================
                /*
                #ifdef LOG_ENABLED
                    if (sleepLogReset > delayBetweenLogReset) {
                        resetLog = 1;
                    }else{
                        sleepLogReset++;
                    }
                #endif
                */
                //======================
                //blink(4,2);

                moisture = sensorRead(sensorPin);

                if(moisture == 0) { //Sensor Not in Soil

                    #ifdef UART_TX_ENABLED
                        uart_putu(suitableMoisture);
                        //uart_putc('\r');
                        //uart_putc('\n');
                    #endif

                    blink(4,4);

                #ifdef EEPROM_ENABLED
                }else if(moisture >= 1021) { //Sensor Manual Calibrate (cross/short both sensor leads)
                    
                    moisture = 0;

                    #ifdef ledPin
                        PORTB |= (1<<ledPin); //ON
                    #endif
                    
                    for(uint8_t i = 0; i < 8; ++i) //Get ready to place into base-line soil
                    {
                        #ifdef UART_TX_ENABLED
                            uart_putc('.');
                        #endif
                        _delay_ms(800);
                    }
                    
                    #ifdef ledPin
                        PORTB &= ~(1<<ledPin); //OFF
                    #endif

                    for(uint8_t i = 0; i < 9; ++i)
                    {
                        blink(6,2);

                        uint16_t a  = sensorRead(sensorPin);
                        //=============
                        //Take Highest
                        //=============
                        if (a > moisture) {
                            moisture = a;
                        }
                    }
                    
                    suitableMoisture = moisture;

                    EEPROM_write(0x01, (suitableMoisture/10)); //max 255 we try to fit 10x
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
                            #ifdef LOG_ENABLED
                            }else{
                                resetLog = 1;
                            #endif
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
                    
                    moisture = sensorRead(pumpPin);

                    if (moisture > 2 && moisture < 98) { //avoid 0 detection if wire not connected
                        emptyBottle = 1;
                    }else{ //Bottle must be empty do not pump air
           
                        if(overwaterProtection < 4) { //Prevent flooding
                            #ifdef UART_TX_ENABLED
                                uart_putc('P');
                            #endif

                            PORTB |= (1<<PB0); //ON
                            //_delay_ms(6800); //6.8 seconds;
                            blink(potSize,2);
                            PORTB &= ~(1<<PB0); //OFF

                            overwaterProtection++; //When battery < 3V (without regulator) ADC readouts are unstable
                        }
                    
                        #ifdef UART_TX_ENABLED
                            uart_putc('E');
                        #endif
                    }
                }else{
                    overwaterProtection = 0;
                }
            }
            #ifdef LOG_ENABLED
                if(resetLog == 1)
                {
                    //sleepLogReset = 0;
                    //sleepLoop = 0;
                    emptyBottle = 0;
                    overwaterProtection = 0;
                    #ifdef SENSORLESS_ENABLED
                        moistureLog[0] = 0;
                        moistureLog[1] = 0;
                        moistureLog[2] = 0;
                    #endif
                }
            #endif
        }
        sleepLoop++;
    }
    return 0;
}

uint16_t sensorRead(uint8_t pin)
{
    PORTB |= (1<<pin); //ON
    uint16_t value = ReadADC(moistureSensorPin);
    PORTB &= ~(1<<pin); //OFF

    #ifdef UART_TX_ENABLED
        //uart_putc(',');
        uart_putu(value);
    #endif

    return value;
}

void blink(uint8_t time, uint8_t duration)
{
    #ifdef ledPin
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
    //ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Prescaler of 128
    //--------------
    ADCSRA |= (1 << ADEN); // Enables ADC

    #ifndef SOLAR_ENABLED
        _delay_ms(200); // Wait for Vref to settle
    #endif

    ADCSRA |= (1 << ADSC); // Start conversion by writing 1 to ADSC
    while(bit_is_set(ADCSRA, ADSC)); // Wait conversion is done

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
    //uint8_t sreg;
    //sreg = SREG;

    //cli(); // disable all interrupts
    PORTB |= (1 << UART_TX);
    DDRB |= (1 << UART_TX);
    __asm volatile(
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
    //SREG = sreg;
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
    //uint8_t sreg;
    //sreg = SREG;

    //cli(); // disable all interrupts
    PORTB &= ~(1 << UART_RX);
    DDRB &= ~(1 << UART_RX);
    __asm volatile(
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
    //SREG = sreg;
    return c;
}
#endif