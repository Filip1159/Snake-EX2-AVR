/**
 * spi.cpp — Hardware SPI HAL for ATmega644P/PA
 */
#include "hal/spi.h"
#include <avr/io.h>

void spi_init(void) {
    DDRB |= (1 << PB5) | (1 << PB7);
    DDRB &= ~(1 << PB6);
    SPCR = (1 << SPE) | (1 << MSTR);
    SPSR |= (1 << SPI2X);
}

uint8_t spi_transfer(uint8_t data) {
    SPDR = data;
    while (!(SPSR & (1 << SPIF)));
    return SPDR;
}

void spi_write(const uint8_t *buf, uint16_t len) {
    while (len--) spi_transfer(*buf++);
}

void spi_write16(uint16_t data) {
    spi_transfer((uint8_t)(data >> 8));
    spi_transfer((uint8_t)(data & 0xFF));
}

void spi_write16_bulk(const uint16_t *buf, uint16_t count) {
    while (count--) {
        uint16_t val = *buf++;
        SPDR = (uint8_t)(val >> 8);
        while (!(SPSR & (1 << SPIF)));
        SPDR = (uint8_t)(val & 0xFF);
        while (!(SPSR & (1 << SPIF)));
    }
}

void spi_set_divider(uint8_t div) {
    SPSR &= ~(1 << SPI2X);
    SPCR &= ~((1 << SPR1) | (1 << SPR0));
    switch (div) {
        case 2:  SPSR |= (1 << SPI2X); break;
        case 8:  SPSR |= (1 << SPI2X); SPCR |= (1 << SPR0); break;
        case 16: SPCR |= (1 << SPR0); break;
        case 32: SPSR |= (1 << SPI2X); SPCR |= (1 << SPR1); break;
        case 64: SPCR |= (1 << SPR1); break;
        case 128: SPCR |= (1 << SPR1) | (1 << SPR0); break;
        default: break; // div 4
    }
}