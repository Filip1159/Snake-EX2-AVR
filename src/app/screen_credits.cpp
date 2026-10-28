/**
 * screen_credits.cpp — Credits ("Instructions" row of the main menu).
 */
#include "app/screens.h"
#include "ui/credits.h"

static void enter(void) { credits_show(); }

static void on_button(uint8_t btn, btn_event_t ev) {
    if (ev == BTN_EVENT_PRESS && (btn == BTN_FIRE || btn == BTN_PAUSE)) screen_goto(&screen_menu);
}

const screen_t screen_credits = { enter, on_button, 0, true };
