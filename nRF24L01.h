/*
    Copyright (c) 2007 Stefan Engelke <mbox@stefanengelke.de>

    Permission is hereby granted, free of charge, to any person 
    obtaining a copy of this software and associated documentation 
    files (the "Software"), to deal in the Software without 
    restriction, including without limitation the rights to use, copy, 
    modify, merge, publish, distribute, sublicense, and/or sell copies 
    of the Software, and to permit persons to whom the Software is 
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be 
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
    DEALINGS IN THE SOFTWARE.

    $Id$
*/

#ifndef NRF24_H
#define NRF24_H
#endif

#ifndef _AVR_IO_H_
#define __AVR_ATtiny85__
#include <avr/io.h>
#endif

#ifndef _UTIL_DELAY_H_
#include <util/delay.h>
#endif

#ifndef MICRO_SPI_H
#include "halfduplexspi.h"
#endif

#define txAddress = 0xE7E7E7E7E7
#define mirf_PAYLOAD 3 // number of bytes in static-sized payload

/* Memory Map */
#define CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define STATUS      0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17
#define FEATURE     0x1D
#define DYNPD       0x1C

/* Bit Mnemonics */
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0
#define AW          0
#define ARD         4
#define ARC         0
#define PLL_LOCK    4
#define RF_DR       3
#define RF_DR_LOW   5
#define RF_PWR      1
#define LNA_HCURR   0        
#define RX_DR       6
#define TX_DS       5
#define MAX_RT      4
#define RX_P_NO     1
#define TX_FULL     0
#define PLOS_CNT    4
#define ARC_CNT     0
#define TX_REUSE    6
#define FIFO_FULL   5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0
#define DPL_P0      0
#define DPL_P1      1
#define DPL_P2      2
#define DPL_P3      3
#define DPL_P4      4
#define DPL_P5      5
#define EN_DPL      2
#define EN_ACK_PAY  1
#define EN_DYN_ACK  0


/* Instruction Mnemonics */
#define R_REGISTER    0x00
#define W_REGISTER    0x20
#define REGISTER_MASK 0x1F
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
#define NOP           0xFF

void mirf_CSN_hi()
{
    SPI_PORT |= _BV(SPI_SCK);
    _delay_us(64);
};

void mirf_CSN_lo()
{
    SPI_PORT &= ~_BV(SPI_SCK);
    _delay_us(8);
};

void mirf_config_register(uint8_t reg, uint8_t value)
// Clocks only one byte into the given MiRF register
{
    mirf_CSN_hi();
    _delay_us(1);
    mirf_CSN_lo();
    spi_out(W_REGISTER | (REGISTER_MASK & reg));
    spi_out(value);
    mirf_CSN_hi();
    _delay_us(1);
};


// per Nordic DS: "to tx, need PWR_UP = 1, PRIM_RX = 0, payload in TX FIFO, and high pulse >10uS on CE" (we're tying CE high)
// needs worst case maximum of 1.5ms to come out of power down mode
void setupRF24()
{
    // initializing pins
    SPI_DDR |= _BV(SPI_SCK);
    mirf_CSN_hi();

    // initializing registers
    //mirf_config_register(CONFIG, _BV(PWR_UP)); // enable power up
    mirf_config_register(SETUP_AW, (_BV(AW)| _BV(AW+1)));  // reduced address width to 3 bytes
    mirf_config_register(RF_SETUP, //turn data rate low on, and turn power all the way up
        (_BV(RF_DR_LOW)|
        _BV(RF_PWR)|
        _BV(RF_PWR+1)));
    // to disable enhanced shockburst, turn off auto-ack and auto-retry
    mirf_config_register(EN_AA, 0); //disable auto-ack
    mirf_config_register(SETUP_RETR, 0); //disable auto-retry
    //mirf_config_register(DYNPD, (_BV(DPL_P0)|_BV(DPL_P1)|_BV(DPL_P2)));  //enable dynamic payloads of pipes 0 - 2
    //mirf_config_register(FEATURE, (_BV(EN_ACK_PAY)|_BV(EN_DPL))); //enable dynamic payload length and payload with ACK in FEATURE register
    //mirf_config_register(RX_PW_P0, 3); // set received payload width in pipe 0 to 3 bytes; i don't think i need this, going to try DPL
    // default P0 RX address = P1 TX address are fine.  No need to mess with those registers.
    //_delay_ms(2);
};

void RF24powerup()
{
    mirf_CSN_hi();
    mirf_config_register(CONFIG, _BV(PWR_UP)); // this is lazy, you should really read the register, mask off PWR_UP, and write that back.
    _delay_ms(2);
};

void RF24powerdown()
{
    mirf_CSN_hi();
    mirf_config_register(CONFIG, 0); // this is lazy, you should really read the register, mask off PWR_UP, and write that back.
}