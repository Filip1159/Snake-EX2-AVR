/**
 * stack_measure.cpp — Q5 stack high-water instrumentation.
 *
 * The AVR build uses the linker's `__bss_end` symbol (the first free SRAM
 * byte) and the live stack pointer.  The host build only provides the pure
 * scan helper so `test_stack_measure` can exercise the boundary logic.
 */
#include "drivers/stack_measure.h"
#include "config.h"

uint16_t stack_measure_scan(const uint8_t *lo, const uint8_t *hi) {
    const uint8_t *p = lo;
    while (p < hi && *p == (uint8_t)STACK_MEASURE_PATTERN) {
        p++;
    }
    return (uint16_t)(p - lo);
}

#if defined(__AVR__)
#include <avr/io.h>

extern "C" {
extern uint8_t __bss_end;   /* linker symbol: first byte after .bss/.data */
}

/* Leave room for this function's own frame and the caller's frame. */
#define STACK_MEASURE_GUARD 32

static uint8_t *s_lo;
static uint8_t *s_hi;
static uint16_t s_free;
static bool s_ready;

static uint8_t *stack_pointer(void) {
    return (uint8_t *)(uint16_t)SP;
}

void stack_measure_begin(void) {
    uint8_t *lo = &__bss_end;
    uint8_t *hi = stack_pointer() - STACK_MEASURE_GUARD;
    if (hi <= lo) {
        s_ready = false;
        return;
    }

    s_lo = lo;
    s_hi = hi;
    s_free = (uint16_t)(hi - lo);
    s_ready = true;

    volatile uint8_t *p = lo;
    while (p < hi) {
        *p++ = (uint8_t)STACK_MEASURE_PATTERN;
    }
}

uint16_t stack_measure_update(void) {
    if (!s_ready) {
        return 0;
    }
    uint16_t free_now = stack_measure_scan(s_lo, s_hi);
    if (free_now < s_free) {
        s_free = free_now;
    }
    return s_free;
}

uint16_t stack_measure_free(void) {
    return s_ready ? s_free : 0;
}

#else

void stack_measure_begin(void) {}
uint16_t stack_measure_update(void) { return 0; }
uint16_t stack_measure_free(void) { return 0; }

#endif
