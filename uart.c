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