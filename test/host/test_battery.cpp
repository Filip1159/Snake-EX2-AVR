/* Host test for the battery voltage to percent map. */
#include "drivers/battery.h"
#include "config.h"
#include <stdio.h>

static int failures;

static void expect(unsigned got, unsigned want, const char *what) {
    if (got != want) { printf("FAIL battery: %s: got %u, want %u\n", what, got, want); failures++; }
}

int main(void) {
    expect(battery_percent(BATTERY_FULL_MV + 100), 100, "above full");
    expect(battery_percent(BATTERY_FULL_MV), 100, "full");
    expect(battery_percent(3750), 50, "half");
    expect(battery_percent(BATTERY_EMPTY_MV), 0, "empty");
    expect(battery_percent(2800), 0, "below empty");
    expect(battery_percent(3470) < BATTERY_LOW_PERCENT, 1, "just below low");
    expect(battery_percent(3480) < BATTERY_LOW_PERCENT, 0, "low threshold");
    printf(failures ? "battery: %d FAILED\n" : "battery: all passed\n", failures);
    return failures != 0;
}
