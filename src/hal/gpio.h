/**
 * gpio.h — GPIO HAL for ATmega644P/PA
 */
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>
#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
#endif

void gpio_init(void);
uint8_t gpio_read_buttons(void);

/* LCD control pins (PB2–PB4) */
static inline void lcd_cs_low(void)   { PORTB &= ~(1 << PB4); }
static inline void lcd_cs_high(void)  { PORTB |=  (1 << PB4); }
static inline void lcd_dc_low(void)   { PORTB &= ~(1 << PB3); }
static inline void lcd_dc_high(void)  { PORTB |=  (1 << PB3); }
static inline void lcd_rst_low(void)  { PORTB &= ~(1 << PB2); }
static inline void lcd_rst_high(void) { PORTB |=  (1 << PB2); }
static inline void buzzer_on(void)    { PORTB |=  (1 << PB0); }
static inline void buzzer_off(void)   { PORTB &= ~(1 << PB0); }
static inline void lcd_bl_on(void)    { PORTC |=  (1 << PC1); }
static inline void lcd_bl_off(void)   { PORTC &= ~(1 << PC1); }

#ifdef __cplusplus
}
#endif

#endif