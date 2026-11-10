/**
 * buttons.h — Button Driver
 *
 * 6 buttons on PA0–PA5, active low, internal pull-ups.
 * Scan at ~100 Hz, debounce 20 ms.
 * Provides press/release events and auto-repeat for menus.
 */
#ifndef DRIVERS_BUTTONS_H
#define DRIVERS_BUTTONS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Button indices ─────────────────────────────────────────────────────── */

#define BTN_UP     0
#define BTN_DOWN   1
#define BTN_LEFT   2
#define BTN_RIGHT  3
#define BTN_FIRE   4   // tongue / confirm
#define BTN_PAUSE  5   // pause / back
#define BTN_COUNT  6

/* ── Event types ────────────────────────────────────────────────────────── */

typedef enum {
    BTN_EVENT_NONE = 0,
    BTN_EVENT_PRESS,
    BTN_EVENT_RELEASE,
    BTN_EVENT_REPEAT,   // auto-repeat (menus)
} btn_event_t;

/* ── API ────────────────────────────────────────────────────────────────── */

/** Initialize button scanning. */
void buttons_init(void);

/**
 * Poll all buttons. Call once per frame (15 Hz = game tick) or
 * per ~10 ms (100 Hz) for menus.
 * Returns the bitmask of currently pressed buttons.
 * Side effect: fills the event queue (see buttons_get_event).
 */
uint8_t buttons_poll(void);

/**
 * Get the next button event from the queue.
 * Returns BTN_EVENT_NONE if the queue is empty.
 * event_type: PRESS, RELEASE, or REPEAT.
 * button_index: 0..5.
 */
btn_event_t buttons_get_event(uint8_t *button_index);

/**
 * Check if a specific button is currently pressed.
 * button: 0..5.
 */
bool buttons_is_pressed(uint8_t button);

/**
 * Get raw button bitmask (same as gpio_read_buttons but debounced).
 * Bit 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT, 4=FIRE, 5=PAUSE.
 */
uint8_t buttons_state(void);

#ifdef __cplusplus
}
#endif

#endif