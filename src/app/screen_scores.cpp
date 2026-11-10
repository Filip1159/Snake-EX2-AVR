/**
 * screen_scores.cpp — High scores: Classic/Extreme list, then the top-3 view of the mode.
 */
#include "app/screens.h"

/* ── Classic / Extreme list ─────────────────────────────────────────────── */

static void list_enter(void) { menu_show(); }

static void list_on_button(uint8_t btn, btn_event_t ev) {
    if (screen_menu_navigate(btn, ev)) return;
    if (ev != BTN_EVENT_PRESS) return;
    if (btn == BTN_FIRE)       screen_score_view_open((hs_mode_t)menu_selected_index());
    else if (btn == BTN_PAUSE) screen_menu_return(MENU_ITEM_HIGH_SCORES);
}

const screen_t screen_score_list = { list_enter, list_on_button, 0, true };

void screen_score_list_open(uint8_t selected) {
    menu_open(MENU_LIST_SCORES);
    menu_move((int8_t)selected);
    screen_goto(&screen_score_list);
}

/* ── Top-3 view ─────────────────────────────────────────────────────────── */

static hs_mode_t s_mode;

static void view_enter(void) { highscores_show(s_mode); }

static void view_on_button(uint8_t btn, btn_event_t ev) {
    if (ev == BTN_EVENT_PRESS && (btn == BTN_FIRE || btn == BTN_PAUSE))
        screen_score_list_open((uint8_t)s_mode);      /* back to the Classic/Extreme list */
}

const screen_t screen_score_view = { view_enter, view_on_button, 0, false };

void screen_score_view_open(hs_mode_t mode) {
    s_mode = mode;
    screen_goto(&screen_score_view);
}
