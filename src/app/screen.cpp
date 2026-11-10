/**
 * screen.cpp — Screen dispatcher (see screen.h).
 */
#include "app/screen.h"
#include "ui/chrome.h"

static const screen_t *s_current;

void screen_goto(const screen_t *next) {
    if (!next->wallpaper) chrome_invalidate();      /* it overwrites the shared chrome */
    s_current = next;
    next->enter();
}

const screen_t *screen_current(void) { return s_current; }

void screen_on_button(uint8_t btn, btn_event_t ev) {
    if (s_current) s_current->on_button(btn, ev);
}

void screen_tick(uint32_t now, uint8_t raw) {
    if (s_current && s_current->tick) s_current->tick(now, raw);
}
