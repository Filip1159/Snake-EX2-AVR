/* Host test: moving between the wallpaper screens repaints only what differs. */
#include "ui/menu.h"
#include "ui/settings.h"
#include "ui/credits.h"
#include "ui/highscores.h"
#include "ui/chrome.h"
#include "drivers/eeprom_store.h"
#include "drivers/st7735.h"
#include <stdio.h>
#include <string.h>

uint8_t mock_eeprom[1024];
unsigned mock_eeprom_writes;
uint32_t mock_millis = 0;
extern "C" void buzzer_play(uint8_t id) { (void)id; }

static uint16_t fb[160][128];
static uint16_t ref[160][128];
static int writes[160][128];       /* pixel writes since the last reset */

extern "C" {
void st7735_fill_screen(uint16_t c) {
    for (int y = 0; y < 160; y++)
        for (int x = 0; x < 128; x++) { fb[y][x] = c; writes[y][x]++; }
}
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (x + i < 128 && y + j < 160) { fb[y + j][x + i] = c; writes[y + j][x + i]++; }
}
static int s_cx, s_cy, s_cw, s_px;
void st7735_stream_begin(uint8_t x, uint8_t y, uint8_t w, uint8_t h) { (void)h; s_cx = x; s_cy = y; s_cw = w; s_px = 0; }
void st7735_stream_pixels(uint16_t c, uint8_t n) {
    while (n--) {
        int y = s_cy + s_px / s_cw, x = s_cx + s_px % s_cw;
        fb[y][x] = c; writes[y][x]++; s_px++;
    }
}
void st7735_stream_end(void) {}
void st7735_backlight(bool on) { (void)on; }
}

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static void reset_writes(void) { memset(writes, 0, sizeof writes); }

static int written(int x, int y, int w, int h) {
    int n = 0;
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) n += writes[y + j][x + i] != 0;
    return n;
}

static int total_written(void) { return written(0, 0, 128, 160); }

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
