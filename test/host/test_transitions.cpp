/* Host test: moving between the wallpaper screens repaints only what differs. */
#include "ui/menu.h"
#include "ui/settings.h"
#include "ui/credits.h"
#include "ui/highscores.h"
#include "ui/chrome.h"
#include "drivers/eeprom_store.h"
#include "mock_display.h"
#include <stdio.h>
#include <string.h>

uint8_t mock_eeprom[1024];
unsigned mock_eeprom_writes;
uint32_t mock_millis = 0;
extern "C" void buzzer_play(uint8_t id) { (void)id; }

static uint16_t ref[160][128];

/* Screen as drawn from scratch by `full`, compared with what the incremental path left. */
static bool matches_full(void (*full)(void)) {
    static uint16_t got[160][128];
    memcpy(got, fb, sizeof fb);
    full();
    bool same = memcmp(got, fb, sizeof fb) == 0;
    return same;
}

static void full_menu(void) { menu_draw(); }
static void full_settings(void) { settings_draw(); }
static void full_credits(void) { credits_draw(); }
static void full_scores(void) { menu_open(MENU_LIST_SCORES); menu_draw(); }

int main() {
    eeprom_store_init();
    chrome_set_silent(false);
    (void)ref;

    /* main menu -> settings -> main menu */
    menu_init(); menu_draw();
    reset_writes();
    settings_init(); settings_show();
    CHECK(written(0, 0, 128, 14) == 0);              /* status bar untouched */
    CHECK(written(122, 30, 6, 111) == 0);            /* same 4-row list, thumb unchanged */
    CHECK(written(0, 112, 128, 28) == 0);            /* blank rows below the text are not repainted */
    CHECK(total_written() < 128 * 160 * 2 / 5);   /* text boxes and the band, not the screen */
    CHECK(matches_full(full_settings));

    reset_writes();
    menu_init(); menu_show();
    CHECK(written(0, 0, 128, 14) == 0);
    CHECK(written(122, 30, 6, 111) == 0);
    CHECK(total_written() < 128 * 160 * 2 / 5);   /* text boxes and the band, not the screen */
    CHECK(matches_full(full_menu));

    /* main menu -> credits -> main menu */
    menu_init(); menu_draw();
    reset_writes();
    credits_show();
    CHECK(written(0, 0, 128, 14) == 0);
    CHECK(written(0, 14, 100, 16) == 0);             /* same "Snake EX2" title: not redrawn */
    CHECK(total_written() < 128 * 160 * 2 / 5);   /* text boxes and the band, not the screen */
    CHECK(matches_full(full_credits));

    reset_writes();
    menu_init(); menu_show();
    CHECK(written(0, 0, 128, 14) == 0);
    CHECK(written(0, 14, 100, 16) == 0);
    CHECK(total_written() < 128 * 160 * 2 / 5);   /* text boxes and the band, not the screen */
    CHECK(matches_full(full_menu));

    /* main menu -> Classic/Extreme list -> main menu, on the thumb: only its rows change */
    menu_init(); menu_draw();
    reset_writes();
    menu_open(MENU_LIST_SCORES); menu_show();
    CHECK(written(0, 0, 128, 14) == 0);
    CHECK(written(0, 120, 100, 20) == 0);
    CHECK(total_written() < 128 * 160 * 2 / 5);   /* text boxes and the band, not the screen */
    CHECK(matches_full(full_scores));

    reset_writes();
    menu_init(); menu_show();
    CHECK(written(0, 0, 128, 14) == 0);
    CHECK(total_written() < 128 * 160 * 2 / 5);   /* text boxes and the band, not the screen */
    CHECK(matches_full(full_menu));

    /* settings: "Silent" toggles in the status bar only */
    reset_writes();
    chrome_set_silent(true);
    CHECK(written(0, 14, 128, 146) == 0 && written(49, 1, 36, 12) > 0);
    chrome_set_silent(false);

    /* high-score view (flat) -> list: full redraw, identical to a fresh one */
    menu_open(MENU_LIST_SCORES); menu_draw();
    highscores_draw(HS_MODE_CLASSIC);
    menu_open(MENU_LIST_SCORES); menu_show();
    CHECK(matches_full(full_scores));

    printf(fails ? "transitions: %d FAILED\n" : "transitions: all passed\n", fails);
    return fails != 0;
}
