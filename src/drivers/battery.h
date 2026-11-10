/**
 * battery.h — Cell voltage from the ATmega's internal 1.1 V bandgap.
 *
 * The MCU runs straight off one Li-ion cell, so VCC is the battery voltage. With AVcc as the
 * ADC reference, converting the bandgap gives 1.1 V * 1024 / ADC = VCC.
 */
#ifndef DRIVERS_BATTERY_H
#define DRIVERS_BATTERY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Measured supply voltage in millivolts (averaged over a few conversions). */
uint16_t battery_read_mv(void);

/** Charge estimate 0..100 % from a cell voltage (linear, BATTERY_EMPTY_MV..BATTERY_FULL_MV). */
uint8_t battery_percent(uint16_t mv);

#ifdef __cplusplus
}
#endif

#endif
