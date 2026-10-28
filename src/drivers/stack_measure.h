/**
 * stack_measure.h — Q5 stack high-water instrumentation.
 *
 * At boot the AVR implementation fills the unused SRAM between `__bss_end`
 * and just below the current stack pointer with a marker byte.  The stack
 * grows downward, so a later scan from the bottom finds the deepest byte the
 * stack has touched.  The pause screen can display the free
 * margin in a debug build (`STACK_MEASURE=1`).
 */
#ifndef DRIVERS_STACK_MEASURE_H
#define DRIVERS_STACK_MEASURE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Marker written to unused SRAM. */
#define STACK_MEASURE_PATTERN 0xA5

/** Pure scan helper: count untouched marker bytes in [lo, hi).
 *
 * Returns the number of bytes from `lo` up to the first non-marker byte.
 * If every byte is the marker, returns `hi - lo` (a lower bound on the free
 * margin).  This is separated from the AVR code so the host tests can drive it.
 */
uint16_t stack_measure_scan(const uint8_t *lo, const uint8_t *hi);

/** Paint unused SRAM once at boot.  No-op on non-AVR builds. */
void stack_measure_begin(void);

/** Scan the painted region and return the smallest free margin seen. */
uint16_t stack_measure_update(void);

/** Last measured free margin in bytes (0 when measurement is unavailable). */
uint16_t stack_measure_free(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_STACK_MEASURE_H */
