/*
*abandoning that dumpster fire for a modification of the UART method on becomingmaker.com
*/

#ifndef __AVR_ATtiny85__ // added bc IDEs don't get compiler flag for ATtiny85 and header files need that info
#define __AVR_ATtiny85__
#endif

#ifndef F_CPU
#define F_CPU       8000000
#endif

#ifndef _AVR_IO_H_
#include <avr/io.h>
#endif

#include <util/delay.h>

#define SPI_PORT    PORTB   // output register
#define SPI_DDR     DDRB
#define SPI_MOMI    PB1     // merged MOSI/MISO pin
#define SPI_SCK     PB2
#define SPI_PIN     PINB    // input register
#define LED_PIN     PB3

#define BAUDRATE        9600
#define CYCLES_PER_BIT  ( F_CPU/BAUDRATE )
#if (CYCLES_PER_BIT > 255)
#define DIVISOR         8
#define PRESCALE        2
#else
#define DIVISOR         1
#define PRESCALE        1
#endif
#define FULL_BIT_TICKS  ( CYCLES_PER_BIT/DIVISOR )

#define BUF_REST        0
#define BUF_FIRST       1
#define BUF_SECOND      2

static volatile uint8_t buf_status = BUF_REST;
static volatile uint8_t buf[] = {0xde, 0xad, 0x01};
static volatile uint8_t tx_data = 0;

static uint8_t reverse_byte (uint8_t x)
{
    x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
    x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
    x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
    return x;
};

void send_byte(uint8_t data)
{
    while (buf_status != BUF_REST) {};
    buf_status = BUF_FIRST;
    tx_data = reverse_byte(data);
    //configure Timer0
    TCCR0A = 2 << WGM00; // set CTC mode by setting WGM0[0:2] in TCCR0A to b010
    TCCR0B = PRESCALE; // set prescaler to clk or clk/8
    GTCCR |= _BV(PSR0); // writing 1 to prescalar reset 
    OCR0A = FULL_BIT_TICKS; // whole register is 8bit value to which the timer counter TCNT0 is compared against.
    TCNT0 = 0;

    //configure USI to send data
    USIDR = tx_data;  // dump data to USI data register
    USICR = ((1<<USIOIE)|   // enable usi counter ovf interrupt
            (0<<USIWM1)|
            (1<<USIWM0)|    // set 3wire mode
            (0<<USICS1)|
            (1<<USICS0)|
            (0<<USICLK));   // set Timer0 Compare Match as USI clock source
    SPI_DDR |= _BV(SPI_MOMI); // set MOMI as out
    USISR = _BV(USIOIF) | 0x08; // set USI counter to 8bits.
}; // need a way to generate a clock or figure out if the USI is generating a clock.

int main()
{
    SPI_PORT &= ~_BV(LED_PIN);  // setLED low so it starts low
    SPI_DDR = (_BV(SPI_SCK) | _BV(SPI_MOMI) | _BV(LED_PIN));    // set SCK and MOMI and LED to output, everything else to input
    
    while(1)
    {// set clock low + write to PORT, set clock high, wait, repeat
        uint8_t dataout, buflen;
        uint16_t buf_checksum = 0;
        buflen = sizeof(buf);

        SPI_PORT |= _BV(LED_PIN);   //turn on LED

        for(; buflen > 0; buflen--)
        {
            dataout = buf[buflen - 1];
            buf_checksum += dataout;
            tx_byte(dataout);
        };
        tx_byte(buf_checksum >> 8);
        tx_byte(buf_checksum & 0xFF);

        SPI_PORT &= ~_BV(LED_PIN);
        //_delay_ms(1000);
    };
    return 0;
};