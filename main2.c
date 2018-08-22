/*
* basic nRF24L01+ transmitter with ACK returned with ATtiny85 MCU control
* over janky two-wire SPI as defined by nerdralph
* http://nerdralph.blogspot.com/2015/05/nrf24l01-control-with-2-mcu-pins-using.html
* need to tie CE pin high and cut out all code references to it
* nRF MCU needs 3.3V. Nerdralph used a diode (20mA red LED) to cut 5V down to 3-ish.
* DS says every new command must be preceded by a H->L on CSN.  Used clock pin to pull
* CSN high/low and a cap between then to ground to filter out signals before they hit CSN
*/

#ifndef __AVR_ATtiny85__ // added bc IDEs don't get compiler flag for ATtiny85 and header files need that info
#define __AVR_ATtiny85__
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

static uint8_t buf[] = {0xde, 0xad, 0x01};

void tx_byte(uint8_t outbyte)
{
    uint8_t bits = 8;
    while (bits > 0)
    {
        //SPI_PORT &= ~_BV(SPI_SCK);  // set clock low
        if (outbyte & 0x80) SPI_PORT |= _BV(SPI_MOMI); // to data buffer, write a 1 (if true) or leave it 0
        SPI_PORT |= _BV(SPI_SCK);   // set clock high
        _delay_us(25);
        SPI_PORT &= ~_BV(SPI_SCK);  // set clock low, clocking out bit
        SPI_PORT &= ~_BV(SPI_MOMI); // clear data buffer
        outbyte <<= 1;
        bits--;
        _delay_us(79);
    };
};

int main()
{
    SPI_PORT &= ~(_BV(SPI_SCK) | _BV(SPI_MOMI | _BV(LED_PIN)));  // set SCK and MOMI and LED low so it starts low
    SPI_DDR = (_BV(SPI_SCK) | _BV(SPI_MOMI) | _BV(LED_PIN));    // set SCK and MOMI and LED to output, everything else to input
    _delay_us(500);

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