/**
 * screen_title.cpp — Title screen: timed at boot, or waiting for a key with the last score.
 */
#include <Arduino.h>
#include "app/screens.h"
#include "render/hud.h"

#define TITLE_MS  3000      /* auto-advance from the title screen */

static bool     s_with_score;
static uint32_t s_score;
static bool     s_new_best;
static uint32_t s_deadline;

static void enter(void) {
    if (s_with_score) title_draw_score(s_score, s_new_best);
    else              title_draw();
    s_deadline = millis() + TITLE_MS;
}

static void on_button(uint8_t btn, btn_event_t ev) {
    (void)btn;
    if (ev == BTN_EVENT_PRESS) screen_menu_open();
}

static void tick(uint32_t now, uint8_t raw) {
    (void)raw;
    if (!s_with_score && (int32_t)(now - s_deadline) >= 0) screen_menu_open();
}

const screen_t screen_title = { enter, on_button, tick, false };

void screen_title_open(void) {
    s_with_score = false;
    screen_goto(&screen_title);
}

void screen_title_open_score(uint32_t score, bool new_best) {
    s_with_score = true;
    s_score = score;
    s_new_best = new_best;
    screen_goto(&screen_title);
}
