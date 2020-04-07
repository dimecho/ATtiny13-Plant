#include <avr/io.h>
#include <util/delay.h>

#define LED_PIN PB2                     // PB2 as a LED pin

int main(void)
{
        /* setup */
        DDRB = 0b00000001;              // set LED pin as OUTPUT 
        PORTB = 0b00000000;             // set all pins to LOW

        /* loop */
        while (1) {
                PORTB ^= _BV(LED_PIN);  // toggle LED pin
                _delay_ms(500);
        }
}
