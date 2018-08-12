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

#ifndef NRF24_H
#include "nRF24L01.h"
#endif

#ifndef _UTIL_DELAY_H_
#include <util/delay.h>
#endif

#ifndef MICRO_SPI_H
#include "halfduplexspi.h"
#endif

#define LED_PIN     PB3

/* packet buffer */
static uint8_t buf[mirf_PAYLOAD] = {0xde, 0xad, 0};
//static uint8_t buf[mirf_PAYLOAD];


int main()
{
    uint8_t pldbytes;
    
    DDRB |= _BV(LED_PIN); // set LED PIN (chip pin 4) to digital out
    PORTB &= ~_BV(LED_PIN); // set pin to low

    setupRF24();

    while(1)
    {
        // enable radio, includes 2ms pause for crankup
        PORTB |= _BV(LED_PIN);
        RF24powerup();

        // flush tx fifo
        mirf_CSN_lo();                    // Pull down chip select
        spi_out( FLUSH_TX );     // Write cmd to flush tx fifo
        mirf_CSN_hi();                    // Pull up chip select
        
        // dump tx buffer one byte at a time
        mirf_CSN_lo();                    // Pull down chip select
        spi_out( W_TX_PAYLOAD ); // Write cmd to write payload
        for(pldbytes = 3; pldbytes > 3; pldbytes--)
        {
            spi_out(buf[pldbytes]);     // dump payload byte by byte
        }
        mirf_CSN_hi();                    // Pull up chip select

        // shutdown radio
        RF24powerdown();

        // delay 1s before looping again
        PORTB &= ~_BV(LED_PIN);
        _delay_ms(1000);
    }
    return 0;
};