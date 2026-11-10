/**
 * buttons.cpp — Button Driver
 *
 * Time-based debounce (DEBOUNCE_MS): a raw level change is
 * accepted only after it has persisted for DEBOUNCE_MS. Auto-repeat after
 * REPEAT_DELAY_MS of hold, then every REPEAT_RATE_MS.
 *
 * Call buttons_poll() at least every ~10 ms for accurate timing.
 */
#include "drivers/buttons.h"
#include "hal/gpio.h"
#include <Arduino.h>
#include <stddef.h>

#define DEBOUNCE_MS      20
#define REPEAT_DELAY_MS  400
#define REPEAT_RATE_MS   100

static struct {
    bool     stable;        // debounced state (true = pressed)
    uint16_t change_ms;     // how long raw has differed from stable
    uint16_t hold_ms;       // time since press / last repeat
    bool     repeating;
} s_buttons[BTN_COUNT];

static uint8_t s_stable_mask = 0;

#define EVENT_QUEUE_SIZE 8
static struct {
    btn_event_t type;
    uint8_t     index;
} s_event_queue[EVENT_QUEUE_SIZE];
static uint8_t s_queue_head = 0;
static uint8_t s_queue_tail = 0;

static uint32_t s_last_scan_ms = 0;
static bool     s_have_last = false;

static void queue_event(uint8_t idx, btn_event_t type) {
    uint8_t next = (s_queue_head + 1) % EVENT_QUEUE_SIZE;
    if (next == s_queue_tail) return;  // full, drop
    s_event_queue[s_queue_head].type  = type;
    s_event_queue[s_queue_head].index = idx;
    s_queue_head = next;
}

void buttons_init(void) {
    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        s_buttons[i].stable = false;
        s_buttons[i].change_ms = 0;
        s_buttons[i].hold_ms = 0;
        s_buttons[i].repeating = false;
    }
    s_stable_mask = 0;
    s_queue_head = s_queue_tail = 0;
    s_have_last = false;
}

uint8_t buttons_poll(void) {
    uint32_t now = millis();
    uint16_t elapsed = 0;
    if (s_have_last) {
        uint32_t d = now - s_last_scan_ms;   // wrap-safe
        elapsed = d > 1000 ? 1000 : (uint16_t)d;
    }
    s_last_scan_ms = now;
    s_have_last = true;

    uint8_t raw = gpio_read_buttons();

    for (uint8_t i = 0; i < BTN_COUNT; i++) {
        bool level = (raw >> i) & 1;

        if (level != s_buttons[i].stable) {
            s_buttons[i].change_ms += elapsed;
            if (s_buttons[i].change_ms >= DEBOUNCE_MS) {
                s_buttons[i].stable = level;
                s_buttons[i].change_ms = 0;
                s_buttons[i].hold_ms = 0;
                s_buttons[i].repeating = false;
                if (level) {
                    s_stable_mask |= (uint8_t)(1 << i);
                    queue_event(i, BTN_EVENT_PRESS);
                } else {
                    s_stable_mask &= (uint8_t)~(1 << i);
                    queue_event(i, BTN_EVENT_RELEASE);
                }
            }
            continue;
        }

        s_buttons[i].change_ms = 0;   // glitch ended: restart timing

        if (s_buttons[i].stable) {
            s_buttons[i].hold_ms += elapsed;
            uint16_t limit = s_buttons[i].repeating ? REPEAT_RATE_MS : REPEAT_DELAY_MS;
            if (s_buttons[i].hold_ms >= limit) {
                s_buttons[i].hold_ms = 0;
                s_buttons[i].repeating = true;
                queue_event(i, BTN_EVENT_REPEAT);
            }
        }
    }

    return s_stable_mask;
}

btn_event_t buttons_get_event(uint8_t *button_index) {
    if (s_queue_tail == s_queue_head) return BTN_EVENT_NONE;

    btn_event_t type = s_event_queue[s_queue_tail].type;
    *button_index = s_event_queue[s_queue_tail].index;
    s_queue_tail = (s_queue_tail + 1) % EVENT_QUEUE_SIZE;
    return type;
}

bool buttons_is_pressed(uint8_t button) {
    if (button >= BTN_COUNT) return false;
    return s_buttons[button].stable;
}

uint8_t buttons_state(void) {
    return s_stable_mask;
}
