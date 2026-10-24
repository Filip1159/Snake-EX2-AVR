#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H
#include <stdint.h>
extern uint32_t mock_millis;
static inline uint32_t millis(void) { return mock_millis; }
static inline void delay(uint32_t ms) { mock_millis += ms; }
#endif
