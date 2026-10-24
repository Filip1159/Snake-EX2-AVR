/**
 * spi.h — Hardware SPI HAL for ATmega644P/PA
 *
 * Uses hardware SPI (PB5=MOSI, PB6=MISO, PB7=SCK).
 * Mode 0 (CPOL=0, CPHA=0), MSB first.
 * BACKLOG.MD §2.3, §2.5.
 */
#ifndef HAL_SPI_H
#define HAL_SPI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void spi_init(void);
uint8_t spi_transfer(uint8_t data);
void spi_write(const uint8_t *buf, uint16_t len);
void spi_write16(uint16_t data);
void spi_write16_bulk(const uint16_t *buf, uint16_t count);
void spi_set_divider(uint8_t div);

#ifdef __cplusplus
}
#endif

#endif