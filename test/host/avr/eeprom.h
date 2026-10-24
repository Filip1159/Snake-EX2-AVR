/* Host mock of avr/eeprom.h backed by a RAM array. */
#ifndef MOCK_AVR_EEPROM_H
#define MOCK_AVR_EEPROM_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
extern uint8_t mock_eeprom[1024];
extern unsigned mock_eeprom_writes;   /* bytes actually changed */
static inline void eeprom_read_block(void *d, const void *a, size_t n) { memcpy(d, mock_eeprom + (uintptr_t)a, n); }
static inline uint8_t eeprom_read_byte(const uint8_t *a) { return mock_eeprom[(uintptr_t)a]; }
static inline void eeprom_update_block(const void *s, void *a, size_t n) {
    for (size_t i = 0; i < n; i++) {
        uint8_t v = ((const uint8_t *)s)[i], *p = mock_eeprom + (uintptr_t)a + i;
        if (*p != v) { *p = v; mock_eeprom_writes++; }
    }
}
static inline void eeprom_update_byte(uint8_t *a, uint8_t v) { eeprom_update_block(&v, a, 1); }
#endif
