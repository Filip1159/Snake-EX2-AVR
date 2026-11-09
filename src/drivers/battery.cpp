/**
 * battery.cpp — Cell voltage from the internal bandgap (see battery.h).
 */
#include "drivers/battery.h"
#include "config.h"

uint8_t battery_percent(uint16_t mv) {
    if (mv <= BATTERY_EMPTY_MV) return 0;
    if (mv >= BATTERY_FULL_MV) return 100;
    return (uint8_t)(((uint32_t)(mv - BATTERY_EMPTY_MV) * 100 + (BATTERY_FULL_MV - BATTERY_EMPTY_MV) / 2)
                     / (BATTERY_FULL_MV - BATTERY_EMPTY_MV));
}

#if defined(__AVR__)
#include <avr/io.h>
#include <util/delay.h>

#define BATTERY_SAMPLES 8

static uint16_t convert(void) {
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) {}
    return ADC;
}

uint16_t battery_read_mv(void) {
    ADMUX = (1 << REFS0) | 0x1E;                 /* AVcc reference, 1.1 V bandgap input */
    ADCSRA |= (1 << ADEN);                       /* Arduino's init() already sets the prescaler */
    _delay_ms(1);                                /* bandgap settling */
    convert();                                   /* first result after switching is unreliable */
    uint16_t sum = 0;
    for (uint8_t i = 0; i < BATTERY_SAMPLES; i++) sum += convert();
    uint16_t adc = sum / BATTERY_SAMPLES;
    return adc ? (uint16_t)((uint32_t)BATTERY_BANDGAP_MV * 1024 / adc) : 0;
}
#else
uint16_t battery_read_mv(void) { return BATTERY_FULL_MV; }
#endif
