#include "drivers/buttons.h"
#include <stdio.h>
uint32_t mock_millis;
uint8_t mock_buttons;
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static void tick(uint8_t raw, int ms) { mock_buttons = raw; mock_millis += ms; buttons_poll(); }
static int count(btn_event_t t, uint8_t idx) {
    int n = 0; uint8_t b; btn_event_t e;
    while ((e = buttons_get_event(&b)) != BTN_EVENT_NONE) if (e == t && b == idx) n++;
    return n;
}

int main() {
    buttons_init(); mock_millis = 1000; tick(0, 0);

    /* bounce on press: 1-0-1-0-1 with 3 ms steps, then stable */
    for (int i = 0; i < 5; i++) tick(i & 1 ? 0 : 1, 3);
    CHECK(buttons_state() == 0);
    for (int i = 0; i < 4; i++) tick(1, 10);
    CHECK(buttons_state() == 1);
    CHECK(count(BTN_EVENT_PRESS, 0) == 1);

    /* glitch while held does not release */
    tick(0, 5); tick(1, 5); tick(1, 10);
    CHECK(buttons_is_pressed(0));
    CHECK(count(BTN_EVENT_RELEASE, 0) == 0);

    /* auto-repeat: ~400 ms then every 100 ms (held 10 ms steps, ~1000 ms) */
    int reps = 0;
    for (int i = 0; i < 100; i++) { tick(1, 10); uint8_t b; btn_event_t e;
        while ((e = buttons_get_event(&b)) != BTN_EVENT_NONE) if (e == BTN_EVENT_REPEAT) reps++; }
    CHECK(reps >= 5 && reps <= 7);

    /* clean release */
    for (int i = 0; i < 4; i++) tick(0, 10);
    CHECK(buttons_state() == 0);
    CHECK(count(BTN_EVENT_RELEASE, 0) == 1);

    /* two buttons independent, millis wraparound */
    mock_millis = 0xFFFFFFF0u;
    for (int i = 0; i < 6; i++) tick(0x28, 10);
    CHECK(buttons_state() == 0x28);
    CHECK(count(BTN_EVENT_PRESS, 3) == 1);

    printf(fails ? "buttons: %d FAILED\n" : "buttons: all passed\n", fails);
    return fails != 0;
}
