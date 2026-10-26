#ifndef MOCK_PGMSPACE_H
#define MOCK_PGMSPACE_H
#include <stdint.h>
#define PROGMEM
#define PSTR(s) (s)
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_word(p) (*(const uint16_t *)(p))
#include <string.h>
#define memcpy_P memcpy
#endif
