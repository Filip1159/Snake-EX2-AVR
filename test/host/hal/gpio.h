#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H
#include <stdint.h>
extern uint8_t mock_buttons;
static inline uint8_t gpio_read_buttons(void) { return mock_buttons; }
#endif
