/**
 * screen_menu.cpp — Main menu: [Continue] / New game / High scores / Settings / Instructions.
 */
#include "app/screens.h"
#include "drivers/eeprom_store.h"
#include "game/savegame.h"

static void enter(void) { menu_show(); }

bool screen_menu_navigate(uint8_t btn, btn_event_t ev) {
    if (ev != BTN_EVENT_PRESS && ev != BTN_EVENT_REPEAT) return false;
    int8_t delta;
    if (btn == BTN_UP || btn == BTN_LEFT)        delta = -1;
    else if (btn == BTN_DOWN || btn == BTN_RIGHT) delta = 1;
    else return false;

    menu_item_t previous = menu_selected();
    menu_move(delta);
    menu_item_t current = menu_selected();
    if (current != previous) {
        /* Only the row that lost the highlight and the one that gained it are repainted;
         * header, footer and the other rows stay untouched. */
        menu_draw_item(previous);
        menu_draw_item(current);
    }
    return true;
}

static void on_button(uint8_t btn, btn_event_t ev) {
    if (screen_menu_navigate(btn, ev)) return;
    if (ev != BTN_EVENT_PRESS) return;

    if (btn == BTN_PAUSE) {
        screen_title_open();
    } else if (btn == BTN_FIRE) {
        menu_item_t item = menu_item_at(menu_selected_index());
        if (item == MENU_ITEM_CONTINUE) {
            if (!screen_game_continue()) screen_menu_open();   /* the save was unusable */
        }
        else if (item == MENU_ITEM_NEW_GAME) screen_game_new();
        else if (item == MENU_ITEM_SETTINGS) screen_goto(&screen_settings);
        else if (item == MENU_ITEM_HIGH_SCORES)
            screen_score_list_open(eeprom_get_settings()->mode == MODE_EXTREME ? HS_MODE_EXTREME : HS_MODE_CLASSIC);
        else if (item == MENU_ITEM_INSTRUCTIONS) screen_goto(&screen_credits);
    }
}

const screen_t screen_menu = { enter, on_button, 0, true };

void screen_menu_open(void) {
    menu_set_continue(savegame_exists());
    menu_init();
    screen_goto(&screen_menu);
}

void screen_menu_return(menu_item_t item) {
    menu_set_continue(savegame_exists());
    menu_init();
    menu_move((int8_t)menu_index_of(item));
    screen_goto(&screen_menu);
}
