#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H
#include <stdint.h>
extern uint8_t mock_buttons;
extern uint8_t mock_buzzer_pin;
static inline uint8_t gpio_read_buttons(void) { return mock_buttons; }
static inline void buzzer_on(void)  { mock_buzzer_pin = 1; }
static inline void buzzer_off(void) { mock_buzzer_pin = 0; }
#endif
