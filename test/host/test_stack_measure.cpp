/* Host test for the Q5 stack scan boundary logic. */
#include "drivers/stack_measure.h"
#include <stdio.h>
#include <string.h>

static int failures;

static void expect(uint16_t got, uint16_t want, const char *what) {
    if (got != want) {
        printf("FAIL stack_measure: %s: got %u, want %u\n", what, got, want);
        failures++;
    }
}

int main(void) {
    uint8_t buf[64];

    memset(buf, STACK_MEASURE_PATTERN, sizeof buf);
    expect(stack_measure_scan(buf, buf + sizeof buf), 64, "untouched");
    expect(stack_measure_scan(buf, buf), 0, "empty range");

    /* Stack touched the byte at offset 40; bytes below it remain painted. */
    buf[40] = 0x00;
    expect(stack_measure_scan(buf, buf + sizeof buf), 40, "partial");

    /* Deepest possible growth reaches the bottom of the painted range. */
    buf[0] = 0x00;
    expect(stack_measure_scan(buf, buf + sizeof buf), 0, "bottom");

    /* Marker bytes above the first non-marker do not affect the boundary. */
    memset(buf, STACK_MEASURE_PATTERN, sizeof buf);
    buf[10] = 0x00;
    buf[20] = 0x00;
    expect(stack_measure_scan(buf, buf + sizeof buf), 10, "first boundary");

    if (failures) {
        printf("stack_measure: %d failure(s)\n", failures);
        return 1;
    }
    printf("stack_measure: all passed\n");
    return 0;
}
