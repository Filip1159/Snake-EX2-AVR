/* Host test: navigating the UI screens through the dispatcher with button events (the
 * firmware's main loop minus the game): which screen each key leads to and what is repainted. */
#include "app/screens.h"
#include "ui/chrome.h"
#include "drivers/eeprom_store.h"
#include "game/savegame.h"
#include "mock_display.h"

uint8_t mock_eeprom[1024];
unsigned mock_eeprom_writes;
uint32_t mock_millis = 0;
extern "C" void buzzer_play(uint8_t id) { (void)id; }

/* Screens that need the game or the title art are replaced by recorders. */
static int  game_new_calls, game_continue_calls, title_calls;
static bool continue_ok = true;
static bool has_save;
void screen_game_new(void) { game_new_calls++; }
bool screen_game_continue(void) { game_continue_calls++; return continue_ok; }
void screen_title_open(void) { title_calls++; }
void screen_title_open_score(uint32_t, bool) {}
extern "C" {
bool savegame_exists(void) { return has_save; }
void savegame_save(const game_t *) {}
bool savegame_load(game_t *) { return false; }
void savegame_clear(void) {}
}

static void press(uint8_t btn) { screen_on_button(btn, BTN_EVENT_PRESS); }
static bool on(const screen_t &s) { return screen_current() == &s; }

int main() {
    eeprom_store_init();
    screen_sync_silent();

    /* ── main menu ── */
    screen_menu_open();
    CHECK(on(screen_menu) && menu_selected() == MENU_ITEM_NEW_GAME);
    press(BTN_DOWN);
    CHECK(menu_selected() == MENU_ITEM_HIGH_SCORES);
    screen_on_button(BTN_UP, BTN_EVENT_RELEASE);                 /* releases do nothing */
    CHECK(menu_selected() == MENU_ITEM_HIGH_SCORES);
    screen_on_button(BTN_DOWN, BTN_EVENT_REPEAT);                /* auto-repeat moves */
    CHECK(menu_selected() == MENU_ITEM_SETTINGS);
    press(BTN_RIGHT); press(BTN_RIGHT);                          /* wraps */
    CHECK(menu_selected() == MENU_ITEM_NEW_GAME);
    press(BTN_FIRE);
    CHECK(game_new_calls == 1);

    /* ── settings: edit, save; the status bar is not repainted going there and back ── */
    press(BTN_DOWN); press(BTN_DOWN);
    reset_writes();
    press(BTN_FIRE);
    CHECK(on(screen_settings));
    CHECK(written(0, 0, 128, 14) == 0);
    press(BTN_RIGHT);                                            /* Mode: Classic -> Extreme */
    press(BTN_PAUSE);                                            /* Back: discards */
    CHECK(on(screen_menu) && menu_selected() == MENU_ITEM_SETTINGS);
    CHECK(eeprom_get_settings()->mode == MODE_CLASSIC);
    CHECK(written(0, 0, 128, 14) == 0);

    press(BTN_FIRE);
    press(BTN_RIGHT);
    press(BTN_DOWN); press(BTN_DOWN); press(BTN_DOWN);           /* Sound */
    press(BTN_LEFT);                                             /* -> off */
    reset_writes();
    press(BTN_FIRE);                                             /* Save */
    CHECK(on(screen_menu));
    CHECK(eeprom_get_settings()->mode == MODE_EXTREME && !eeprom_get_settings()->sound_on);
    CHECK(written(49, 1, 36, 12) > 0);                           /* "Silent" appears in the bar */
    CHECK(written(0, 0, 49, 14) == 0 && written(85, 0, 43, 14) == 0);

    /* ── credits ── */
    press(BTN_DOWN);
    CHECK(menu_selected() == MENU_ITEM_INSTRUCTIONS);
    press(BTN_FIRE);
    CHECK(on(screen_credits));
    press(BTN_FIRE);
    CHECK(on(screen_menu) && menu_selected() == MENU_ITEM_INSTRUCTIONS);

    /* ── high scores: list -> view -> list -> menu ── */
    press(BTN_UP); press(BTN_UP);
    CHECK(menu_selected() == MENU_ITEM_HIGH_SCORES);
    press(BTN_FIRE);
    CHECK(on(screen_score_list) && menu_selected_index() == 1);  /* Extreme: the saved mode */
    press(BTN_UP);
    CHECK(menu_selected_index() == 0);
    press(BTN_FIRE);
    CHECK(on(screen_score_view));
    CHECK(count_color(0, 0, 128, 160, 0x03DB) == 0);             /* no menu band on the view */
    press(BTN_PAUSE);
    CHECK(on(screen_score_list) && menu_selected_index() == 0);
    press(BTN_PAUSE);
    CHECK(on(screen_menu) && menu_selected() == MENU_ITEM_HIGH_SCORES);

    /* ── Continue ── */
    has_save = true;
    screen_menu_open();
    CHECK(menu_item_at(menu_selected_index()) == MENU_ITEM_CONTINUE);
    press(BTN_FIRE);
    CHECK(game_continue_calls == 1 && on(screen_menu));
    continue_ok = false;                                         /* unusable save: stay on the menu */
    press(BTN_FIRE);
    CHECK(game_continue_calls == 2 && on(screen_menu));

    /* ── Pause on the menu goes back to the title ── */
    press(BTN_PAUSE);
    CHECK(title_calls == 1);

    printf(fails ? "screens: %d FAILED\n" : "screens: all passed\n", fails);
    return fails != 0;
}
