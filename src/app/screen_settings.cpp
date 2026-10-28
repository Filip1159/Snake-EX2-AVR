/**
 * screen_settings.cpp — Settings: Up/Down select, Left/Right change, Fire saves, Pause discards.
 */
#include "app/screens.h"
#include "ui/settings.h"
#include "ui/chrome.h"
#include "drivers/eeprom_store.h"

void screen_sync_silent(void) {
    chrome_set_silent(!eeprom_get_settings()->sound_on);
}

static void enter(void) {
    settings_init();
    settings_show();
}

static void on_button(uint8_t btn, btn_event_t ev) {
    if (ev != BTN_EVENT_PRESS && ev != BTN_EVENT_REPEAT) return;

    if (btn == BTN_UP || btn == BTN_DOWN) {
        setting_item_t prev = settings_selected();
        settings_move(btn == BTN_UP ? -1 : 1);
        settings_draw_row(prev);
        settings_draw_row(settings_selected());
    } else if (btn == BTN_LEFT || btn == BTN_RIGHT) {
        settings_adjust(btn == BTN_LEFT ? -1 : 1);
        settings_draw_value(settings_selected());
    } else if (ev == BTN_EVENT_PRESS && btn == BTN_FIRE) {
        settings_save();                  /* "Save": store everything, back to the menu */
        screen_sync_silent();
        screen_goto(&screen_menu);
    } else if (ev == BTN_EVENT_PRESS && btn == BTN_PAUSE) {
        screen_goto(&screen_menu);        /* "Back": discard the edits */
    }
}

const screen_t screen_settings = { enter, on_button, 0, true };
