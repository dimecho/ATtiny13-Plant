/*  
    FUSE_L=0x6A (Clock divide fuse enabled = 8Mhz CPU frequency is actually 1MHz)
    FUSE_H=0xFF (0xFE = RSTDISBL -> CATION: If enabled chip can only be programmed once)
    F_CPU=8000000 (8Mhz)

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

//#include <avr/pgmspace.h>
//#include <avr/eeprom.h>
//#include <avr/wdt.h>
//#include <stdio.h>
//#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "uart.h"

#define pumpPin                     PB0 //Output
#define sensorPin                   PB1 //Output
//#define solarPin                    PB2 //Output
#define ledPin                      PB2 //  PB5 //Output
#define moistureSensorPin           PB4 //Input
#define delayBetweenWaterings       8   //8seconds x 12 = 1.5 min
#define delayBetweenSolarDischarge  4   //8seconds x 4 = .5 min
#define delayBetweenLogReset        60  //8seconds x 12 x 60 = 1.5 hours
#define delayBetweenRefillReset     450 //8seconds x 12 x 450 = 12 hours

//static void powerSave(void);
//static void powerWakeup(void);
static unsigned char EEPROM_read(unsigned char ucAddress);
static void EEPROM_write(unsigned char ucAddress, unsigned char ucData);
static void blink(uint8_t time, uint8_t duration);
static uint8_t checkEmptyBottle();
static void resetLog(uint8_t *sleepLogReset, uint16_t *sleepOneDay, uint8_t *emptyBottle, uint16_t (*moistureLog)[3]);
static uint16_t ReadADC(uint8_t channel);
static void uart_send_line(char *s, uint16_t i);

//Global variable, set to 'volatile' if used with ISR

//Brown Out Detector Control Register
/*
#define BODCR _SFR_IO8(0x30)
#define BODSE 0
#define BODS 1
*/

ISR(WDT_vect)
{
}

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

int main(void)
{
    uint16_t suitableMoisture = 380; //Analog value with 10k pull-up resistor

    //================
    //TRANSISTORS
    //================
    //DDRB |= (1<<DDB0);   // Pin 5 Digital OUTPUT
    //DDRB |= (1<<DDB1);   // Pin 6 Digital OUTPUT
    DDRB |= _BV(pumpPin);
    DDRB |= _BV(sensorPin);

    #ifdef solarPin
        //DDRB |= (1<<DDB2);   // Pin 7 Digital OUTPUT
        DDRB |= _BV(solarPin);
    #endif
    #ifdef ledPin
        //DDRB |= (1<<DDB5);   // Pin 1 (RESET) Pin as Digital OUTPUT
        DDRB |= _BV(ledPin);
    #endif
    //================
    //ANALOG SENSOR
    //================
    DDRB &= ~_BV(moistureSensorPin); // Pin 3 as Analog INPUT
    //DDRB &= ~(1 << 4); // Pin 3 as Analog INPUT
    //PORTB &= ~(1 << 4); // Pin 3 Shutdown Digital OUTPUT
    
    //ADMUX = (0 << REFS0);     //Set VCC as reference voltage (5V)
    //ADMUX = (1 << REFS0);  //Set VCC as reference voltage (Internal 1.1V)
    //--------------
    //ADMUX |= (1 << MUX0) |= (1 << MUX1); //ADC3 PB3 as analog input channel
    //ADMUX |= (1 << MUX1) | (0 << MUX0); // ADC2 PB4 as analog input channel
    //--------------
    //ADMUX |= (1 << ADLAR);  //Left adjusts for 8-bit resolution
    //--------------
    // Set the prescaler to clock/128 & enable ADC See ATtiny13 datasheet, Table 14.4.
    //ADCSRA |= (1 << ADPS1) | (1 << ADPS0);  //Set division factor-8 for 125kHz ADC clock
    //--------------
    //ADCSRA |= (1 << ADEN); //Enables the ADC

    //================
    //EEPROM
    //================
    unsigned char suitableMoisture_ee = EEPROM_read(0x01);
    if (suitableMoisture_ee == 255) //EEPROM is blank
    {
        EEPROM_write(0x01, (suitableMoisture/10)); //max 255 we try to fit 10x
    }else{
        suitableMoisture = (suitableMoisture_ee * 10);
    }
    //EEPROM_write(0x01,_suitableMoisture);
    //================
    //WATCHDOG
    //================
    cli(); // disable all interrupts
    //wdt_reset();
    //----------------
    //WDTCR |= (1<<WDP3); // (1<<WDP2) | (1<<WDP0); //Set timer 4s (max)
    WDTCR |= (1<<WDP3 )|(0<<WDP2 )|(0<<WDP1)|(1<<WDP0); //Set timer 8s (max)
    WDTCR |= (1<<WDTIE);  // Enable watchdog timer interrupts
    //set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    //set_sleep_mode(SLEEP_MODE_IDLE);
    //sei(); // Enable global interrupts
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
    uint8_t sleepLogReset = 0; //Reset logs once in a while
    uint16_t sleepOneDay = 0; //24 hour auto-check for refill

    //Collect past logs for empty detection
    uint16_t moistureLog[3] = {0,0,0};
    uint8_t emptyBottle = 0;
    uint16_t moisture;

    //================
    //ALIVE TEST
    //================
    //BODCR |= (1<<BODS)|(1<<BODSE); //Disable Brown Out Detector Control Register
    /*
    PORTB |= (1<<PB0); //ON
    _delay_ms(900);
    PORTB &= ~(1<<PB0); //OFF
    */
    blink(2,2);
    uart_send_line(">",0);
    //================

    for (;;) {
        
        //-------------
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_mode(); //makes a call to three routines:  sleep_enable(); sleep_cpu(); sleep_disable();
        //sleep_enable(); //sets the Sleep Enable bit in the MCUCR register
        //sleep_cpu(); //issues the SLEEP command
        //sleep_disable(); //clears the SE bit.
        //-------------

        #ifdef solarPin
            if (sleepLoop > delayBetweenSolarDischarge)
            {
                uart_send_line("SLR",0);
                
                PORTB |= (1<<solarPin); //ON
                //-------------------
                //Depends on Capacitor
                //_delay_ms(1800);
                //-------------------
                //Sense with voltage feedback wire from regulator
                uint8_t timeout = 40;
                uint16_t voltage;
                while(timeout-- && voltage > 100) {
                    _delay_ms(100);
                    voltage = ReadADC(moistureSensorPin);
                    uart_send_line(",",voltage);
                }
                //-------------------
                PORTB &= ~(1<<solarPin); //OFF
            }
        #endif

        //uart_puts("EE=");
        //uart_putu(suitableMoisture);
        //uart_puts("\r\n");

        if (sleepLoop > delayBetweenWaterings)
        {
            sleepLoop = 0;

            if(emptyBottle == 1) { //Low Water LED

                blink(9,99);

                emptyBottle = checkEmptyBottle();

                //Retry every 24 hours ...when someone refilled the bottle but did not cycle power.
                if(sleepOneDay > delayBetweenRefillReset)
                {
                   resetLog(&sleepLogReset, &sleepOneDay, &emptyBottle, &moistureLog);
                }else{
                    sleepOneDay++;
                }
            }else{
                //======================
                //Prevents false-positive (empty detection)
                //Moisture sensor (too accurate) triggers exactly same value when dry
                //======================
                if (sleepLogReset > delayBetweenLogReset) {
                    resetLog(&sleepLogReset, &sleepOneDay, &emptyBottle, &moistureLog);
                }else{
                    sleepLogReset++;
                }
                //======================
                PORTB |= (1<<sensorPin); //ON
                //_delay_ms(10);
                moisture = ReadADC(moistureSensorPin);
                //=============
                PORTB &= ~(1<<sensorPin); //OFF

                uart_send_line("S=",moisture);

                if(moisture == 0) { //Sensor Not in Soil

                    uart_send_line("I=",suitableMoisture);

                    blink(4,2);

                    //Read from UART and save to EEPROM
                    /*
                    char c, *p, buff[3];
                    p = buff;
                    while((c = uart_getc()) != '\n' && (p - buff) < 3) {
                        *(p++) = c;
                    }
                    *p = 0;
                    _delay_ms(10);
                    suitableMoisture = (int)buff;

                    uart_putu(suitableMoisture);
                    uart_puts("\r\n");
                    */
                }else if(moisture >= 1022) { //Sensor Manual Calibrate (cross/short both sensor leads)

                    moisture = 0;

                    #ifdef ledPin
                        PORTB |= (1<<ledPin); //ON
                    #endif
                    for(uint8_t i = 0; i < 8; ++i) //Get ready to place into base-line soil
                    {
                        #ifdef UART_TX_ENABLED
                            uart_puts(".");
                        #endif
                        _delay_ms(500);
                    }
                    #ifdef ledPin
                        PORTB &= ~(1<<ledPin); //OFF
                    #endif

                    for(uint8_t i = 0; i < 9; ++i)
                    {
                        blink(4,1);

                        PORTB |= (1<<sensorPin); //ON
                        //_delay_ms(10);
                        //=============
                        //Take Highest
                        //=============
                        uint16_t a = ReadADC(moistureSensorPin);
                        if (a > moisture) {
                            moisture = a;
                        }
                        PORTB &= ~(1<<sensorPin); //OFF

                        uart_send_line("",moisture);
                    }
                    suitableMoisture = moisture;
                    EEPROM_write(0x01, (suitableMoisture/10)); //max 255 we try to fit 10x
                
                }else if (moisture < suitableMoisture) { //Water Plant

                    emptyBottle = checkEmptyBottle(); //Detect Empty Bottle (Sensored)

                    if (emptyBottle == 0) //Detect Empty Bottle (Sensor-less)
                    {
                        if(moistureLog[0] == 0) {
                            moistureLog[0] = moisture;
                        }else if(moistureLog[1] == 0) {
                            moistureLog[1] = moisture;
                        }else if(moistureLog[2] == 0) {
                            moistureLog[2] = moisture;
                        }else{
                            uint16_t m = (moistureLog[0] + moistureLog[1] + moistureLog[2]) / 3; //Average
                            uart_send_line("M=",m);

                            if (m >= (moisture - 2) && m <= (moisture + 2)) {
                                emptyBottle = 1; //Pump ran but no change in moisture
                            }else{
                                resetLog(&sleepLogReset, &sleepOneDay, &emptyBottle, &moistureLog);
                            }
                        }
                    }
                   
                    if (emptyBottle == 1) //Bottle must be empty do not pump air
                    {
                        uart_send_line("E",0);
                    }else{
                        uart_send_line("P",0);

                        PORTB |= (1<<PB0); //ON
                        _delay_ms(6800);
                        PORTB &= ~(1<<PB0); //OFF
                    }
                }
            }
        }else{
            sleepLoop++;
        }
    }
}

void uart_send_line(char *s, uint16_t u)
{
    #ifdef UART_TX_ENABLED
        uart_puts(s);
        if(u > 0) {
            uart_putu(u);
        }
        uart_puts("\r\n");
    #endif
}

uint8_t checkEmptyBottle()
{
    //===================
    //Detect Empty Bottle (Sensored)
    //===================
    /*
    loopback wire from water jug to Pin3 (PB4)
    given power from Pin5 (PB0) while turning on NPN transistor
    more accurate than sensor-less detection
    */

    PORTB |= (1<<PB0); //ON
    uint16_t water = ReadADC(moistureSensorPin);
    PORTB &= ~(1<<PB0); //OFF

    uart_send_line("J=",water);

    if (water > 0 && water < 100) { //avoid 0 detection if wire not connected
        return 1;
    }
    return 0;
}

void resetLog(uint8_t *sleepLogReset, uint16_t *sleepOneDay, uint8_t *emptyBottle, uint16_t (*moistureLog)[3])
{
    *sleepLogReset = 0;
    *sleepOneDay = 0;
	*emptyBottle = 0;
    *moistureLog[0] = 0;
    *moistureLog[1] = 0;
    *moistureLog[2] = 0;
}

void blink(uint8_t time, uint8_t duration)
{
    #ifdef ledPin
        do {
            PORTB |= (1<<ledPin); //ON
            for(uint8_t i = 0; i < time; ++i) {
                _delay_ms(100);
            }
            PORTB &= ~(1<<ledPin); //OFF
            for(uint8_t i = 0; i < time; ++i) {
                _delay_ms(100);
            }
            duration--;
        } while (duration);
    #endif
}

uint16_t ReadADC(uint8_t pin) {

    switch(pin) {
    case PB2: ADMUX = _BV(MUX0); break; // ADC1
        case PB4: ADMUX = _BV(MUX1); break; // ADC2
        case PB3: ADMUX = _BV(MUX0)|_BV(MUX1); break; // ADC3
        case PB5: // ADC0
    default: ADMUX = 0; break;
    }

    ADMUX &= ~_BV(REFS0); // Explicit set VCC as reference voltage (5V)
    ADCSRA |= _BV(ADEN);  // Enable ADC
    ADCSRA |= _BV(ADSC);  // Run single conversion
    while(bit_is_set(ADCSRA, ADSC)); // Wait conversion is done

    // Read values
    uint8_t low = ADCL;
    uint8_t high = ADCH;

    return (high << 8) | low; // Combine two bytes
}

/*
uint16_t ReadADC(uint8_t channel)
{
    ADMUX = (0 << REFS0) | channel;  // set reference and channel

    //ADMUX |= (1 << MUX0) |= (1 << MUX1); //ADC3 PB3
    ADMUX |= (1 << MUX1) | (0 << MUX0); // ADC2 PB4

    ADCSRA |= (1 << ADSC); // Start Conversion
    while(ADCSRA & (1<<ADSC)){}  // Wait for conversion complete  

    return ADC; // For 10-bit resolution
}
*/

/*
void powerSave()
{
    ADCSRA &= ~(1<<ADEN); // Disable ADC converter
    ACSR = (1<<ACD); // Disable the analog comparator

    //DIDR0 |= (1<< AIN1D)|(1 << AIN0D); // Disable analog input buffers
    DIDR0 = 0x3F; //Disable digital input buffers on all ADC0-ADC5 pins
}

void powerWakeup()
{
    ADCSRA |= (1 << ADEN);   //Enables the ADC
    while (ADCSRA & (1 << ADSC)); //Wait for completion
}
*/