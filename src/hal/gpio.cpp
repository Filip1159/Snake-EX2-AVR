/**
 * gpio.cpp — GPIO HAL for ATmega644P/PA
 */
#include "hal/gpio.h"

void gpio_init(void) {
    // LCD control: outputs, high (CS/RST active low → high = inactive)
    DDRB  |= (1 << PB4) | (1 << PB3) | (1 << PB2);
    lcd_cs_high();
    lcd_dc_high();
    lcd_rst_high();

    // SD CS: output, high

    // Buzzer: output, low
    DDRD |= (1 << PD6);
    buzzer_off();

    // Backlight: output, on
    DDRC |= (1 << PC1);
    lcd_bl_on();

    // Buttons PA0–PA5: inputs with pull-ups
    DDRA &= ~((1 << PA0) | (1 << PA1) | (1 << PA2) | (1 << PA3) | (1 << PA4) | (1 << PA5));
    PORTA |= (1 << PA0) | (1 << PA1) | (1 << PA2) | (1 << PA3) | (1 << PA4) | (1 << PA5);
}

uint8_t gpio_read_buttons(void) {
    uint8_t raw = PINA;
    uint8_t result = 0;
    if (!(raw & (1 << PA0))) result |= (1 << 0);  // UP
    if (!(raw & (1 << PA1))) result |= (1 << 1);  // DOWN
    if (!(raw & (1 << PA2))) result |= (1 << 2);  // LEFT
    if (!(raw & (1 << PA3))) result |= (1 << 3);  // RIGHT
    if (!(raw & (1 << PA4))) result |= (1 << 4);  // FIRE
    if (!(raw & (1 << PA5))) result |= (1 << 5);  // PAUSE
    return result;
}