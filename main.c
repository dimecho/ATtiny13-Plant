/*  
    FUSE_L=0x6A (Clock divide fuse enabled = 8Mhz CPU frequency is actually 1MHz)
    FUSE_H=0xFF (0xFE = RSTDISBL -> CATION: If enabled chip can only be programmed once)
    F_CPU=8000000 (8Mhz)

    Slowing down CPU to save power
    CPU @ 1.2Mhz / 8 = 150 Khz
*/
#define F_CPU 1200000L
/*
             [ATtiny13A]
              +------+
(LED)   PB5  1| O    |8  VCC
(TX)    PB3  2|      |7  PB2 (Solar)
(RX/A0) PB4  3|      |6  PB1 (Sensor)
        GND  4|      |5  PB0 (Pump)
              +------+
*/

//#include <avr/wdt.h>
//#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include "uart.h"

#define pumpPin                     PB0 //Output
#define sensorPin                   PB1 //Output
#define solarPin                    PB2 //Output
//#define ledPin                      PB5 //Output
#define moistureSensorPin           PB4 //Input
#define delayBetweenWaterings       12  //8seconds x 12 = 1.5 min
#define delayBetweenSolarDischarge  4   //8seconds x 4 = .5 min
#define delayBetweenLogReset        60  //8seconds x 12 x 60 = 1.5 hours
#define delayBetweenRefillReset     900 //8seconds x 12 x 900 = 24 hours

static void powerSave();
static void powerWakeup();
static void process();
static void resetLog();
static int ReadADC();

//================
//Global variable, set to volatile if used with ISR
volatile unsigned int sleepLoop = 0;  //Track the passage of time
volatile unsigned int sleepLogReset = 0; //Reset logs once in a while
volatile unsigned int sleepOneDay = 0; //24 hour auto-check for refill

//Collect past logs for empty detection
volatile unsigned int moistureLog1 = 0;
volatile unsigned int moistureLog2 = 0;
volatile unsigned int moistureLog3 = 0;

//EEPROM for adjusting with UART
//uint16_t EEMEM NonVolatileInt;

/*
Moisture analog sensor:
  - Pull-up Resistor 0(dry)-1023(wet)
  - MH Sensor Series 0(wet)-1023(dry)
  Note: Change line: 253/254
*/
int suitableMoisture = 380; //700
//================

ISR(WDT_vect)
{
    sleepLoop++; 
}

int main(void)
{
    //================
    //TRANSISTORS
    //================
    DDRB |= (1<<DDB0);   // Pin 5 Digital OUTPUT
    DDRB |= (1<<DDB1);   // Pin 6 Digital OUTPUT
    #ifdef solarPin
        DDRB |= (1<<DDB2);   // Pin 7 Digital OUTPUT
    #endif
    #ifdef ledPin
        DDRB |= (1<<DDB5);   // Pin 1 RESET Pin as Digital OUTPUT
    #endif
    //================
    //ANALOG SENSOR
    //================
    DDRB &= ~(1 << 4); // Pin 3 no Digital OUTPUT
    PORTB &= ~(1 << 4); // Pin 3 Shutdown Digital OUTPUT
    ADMUX &= ~_BV(REFS0);     //Set VCC as reference voltage (5V)
    //ADMUX |= (1 << REFS0);  //Set VCC as reference voltage (Internal 1.1V)
    //ADMUX |= (1 << MUX0) |= (1 << MUX1); //ADC3 PB3 as analog input channel
    ADMUX |= (1 << MUX1) | (0 << MUX0); // ADC2 PB4 as analog input channel
    //ADMUX |= (1 << ADLAR);  //Left adjusts for 8-bit resolution
    ADCSRA |= (1 << ADEN);   //Enables the ADC
    //ADCSRA |= (1 << ADPS1) | (1 << ADPS0);  //Set division factor-8 for 125kHz ADC clock
    
    //================
    //SLEEP
    //================
    //wdt_disable();
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

    //================
    //EEPROM
    //================
    /*
    suitableMoisture = eeprom_read_word(&NonVolatileInt);
    if (suitableMoisture == 65535) { //0xFFFF
        suitableMoisture = 780;
    }
    */
    //================

    for (;;) {

        //  SLEEP_MODE_PWR_DOWN: Disable every clock domain and timer except for the watchdog timer.
        //  Only way to wake up the device is the WDT and the external interrupt pins.
        //  The millis() timers will be disabled during sleep, which is why we track seconds in the
        //  watchdog timer routine.

        //  Shut down your external sensors & hardware here
        powerSave();
            //-------------
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);

            //sleep_mode(); //makes a call to three routines:  sleep_enable(); sleep_cpu(); sleep_disable();
            /*
                sleep_enable() sets the Sleep Enable bit in the MCUCR register
                sleep_cpu() issues the SLEEP command
                sleep_disable() clears the SE bit.
            */
            sleep_enable();
            sei();
            sleep_cpu();
            sleep_disable();
            //-------------
        powerWakeup();

        //_delay_ms(2000);
        process();
    }
}

void resetLog()
{
    sleepLogReset = 0;
    sleepOneDay = 0;
    moistureLog1 = 0;
    moistureLog2 = 0;
    moistureLog3 = 0;
}

void process()
{
    /*
    uart_putu(sleepLoop);
    uart_puts("\r\n");
    */

    #ifdef solarPin
        if (sleepLoop == delayBetweenSolarDischarge)
        {
            uart_puts("solar\r\n");
            PORTB |= (1<<solarPin); //ON
            _delay_ms(1000);
            PORTB &= ~(1<<solarPin); //OFF
        }
    #endif

    if (sleepLoop == delayBetweenWaterings) {

        if(suitableMoisture == 1023) { //Low Water LED
            #ifdef ledPin
                for(int i = 0; i < 100; i++) {
                    PORTB |= (1<<ledPin); //ON
                    _delay_ms(800);
                    PORTB &= ~(1<<ledPin); //OFF
                    _delay_ms(800);
                }
            #endif
            //Retry every 24 hours ...when someone refilled the bottle but did not cycle power.
            if(sleepOneDay > delayBetweenRefillReset)
            {
                resetLog();
            }else{
                sleepOneDay++;
            }
        }else{
            //======================
            //Prevents false-pisitive (empty detection)
            //Moisture sensor (too accurate) triggers exactly same value when dry
            //======================
            if (sleepLogReset > delayBetweenLogReset) {
                resetLog();
            }else{
                sleepLogReset++;
            }
            //======================
            PORTB |= (1<<sensorPin); //ON
            _delay_ms(10);
            int moisture = ReadADC();
            //=============
            //Take Highest
            //=============
            /*
            for(int i = 0; i < 3; i++) {
                int a = ReadADC();
                _delay_ms(10);
                if (a > moisture) {
                    moisture = a;
                }
            }
            */
            //=============
            PORTB &= ~(1<<sensorPin); //OFF

            uart_puts("sensor=");
            uart_putu(moisture);
            uart_puts("\r\n");

            if(moisture == 0 || moisture == 1023 ) { //Sensor Not in Soil

                uart_puts("soil=");
                uart_putu(suitableMoisture);
                uart_puts("\r\n");
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
                eeprom_write_word(&NonVolatileInt,suitableMoisture);

                uart_putu(suitableMoisture);
                uart_puts("\r\n");
                */
            }else if (moisture < suitableMoisture) { //Water Plant (Pull-up Resistor)
            //}else if (moisture > suitableMoisture) { //Water Plant (MH Sensor Series)
                //===================
                //Detect empty Bottle
                //===================
                int emptyBottle = 0;

                if(moistureLog1 == 0) {
                    moistureLog1 = moisture;
                }else if(moistureLog2 == 0) {
                    moistureLog2 = moisture;
                }else if(moistureLog3 == 0) {
                    moistureLog3 = moisture;
                }else{
                    int m = (moistureLog1 + moistureLog2 + moistureLog3) / 3; //Average
                    if (m >= (moisture - 2) && m <= (moisture + 2)) {
                        emptyBottle = 1; //Pump ran but no change in moisture
                    }else{
                        resetLog();
                    }
                }
                //===================

                //Bottle must be empty do not pump air
                if (emptyBottle == 1) {
                    uart_puts("empty\r\n");
                    suitableMoisture = 1023; //Lock until manually reset
                }else{
                    uart_puts("pump\r\n");
                    PORTB |= (1<<PB0); //ON
                    _delay_ms(6800);
                    PORTB &= ~(1<<PB0); //OFF
                }
            }
        }
        sleepLoop = 0;
    }
}

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

int ReadADC()
{
    ADMUX |= (0 << REFS0); // VCC as Reference
    //ADMUX |= (1 << MUX0) |= (1 << MUX1); //ADC3 PB3
    ADMUX |= (1 << MUX1) | (0 << MUX0); // ADC2 PB4
    ADCSRA |= (1 << ADSC); // Start Converstion

    while((ADCSRA & 0x40) !=0){}; //Wait for Converstion Complete

    return ADC;
}