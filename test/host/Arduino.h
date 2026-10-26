#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H
#include <stdint.h>
#ifndef F_CPU
#define F_CPU 8000000L
#endif
extern uint32_t mock_millis;
static inline uint32_t millis(void) { return mock_millis; }
static inline void delay(uint32_t ms) { mock_millis += ms; }
#endif
