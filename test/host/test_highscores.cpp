/* Host test: high scores table and entry logic. */
#include "ui/highscores.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include "drivers/buttons.h"
#include "drivers/buzzer.h"
#include "drivers/eeprom_store.h"
#include "Arduino.h"
#include <stdio.h>
#include <string.h>

/* Mock the EEPROM storage (referenced by the avr/eeprom.h mock). */
uint8_t mock_eeprom[1024];
unsigned mock_eeprom_writes;
uint32_t mock_millis = 0;

/* Mock buzzer functions called by highscores_maybe_enter. */
static uint8_t mock_play_id;
static int     mock_play_count;
extern "C" void buzzer_init(void) {}
extern "C" void buzzer_play(uint8_t id) { mock_play_id = id; mock_play_count++; }
extern "C" void buzzer_stop(void) {}
extern "C" bool buzzer_is_playing(void) { return false; }
extern "C" void buzzer_update(void) { }

/* Mock button functions called by highscores_maybe_enter.  A pending FIRE
 * press lets the entry loop save and return immediately. */
static int mock_fire_pending;
extern "C" uint8_t buttons_poll(void) { return 0; }
extern "C" btn_event_t buttons_get_event(uint8_t *idx) {
    if (mock_fire_pending) {
        mock_fire_pending = 0;
        if (idx) *idx = BTN_FIRE;
        return BTN_EVENT_PRESS;
    }
    return BTN_EVENT_NONE;
}
extern "C" bool buttons_is_pressed(uint8_t btn) { (void)btn; return false; }

static uint16_t fb[160][128];
static int fill_screen_calls;
static uint8_t st_x, st_y, st_w, st_h, st_px;

extern "C" {
void st7735_fill_screen(uint16_t c) {
    fill_screen_calls++;
    for (int y = 0; y < 160; y++)
        for (int x = 0; x < 128; x++) fb[y][x] = c;
}
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (x + i < 128 && y + j < 160) fb[y + j][x + i] = c;
}
void st7735_stream_begin(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    st_x = x; st_y = y; st_w = w; st_h = h; st_px = 0;
}
void st7735_stream_pixels(uint16_t c, uint8_t n) {
    while (n--) {
        fb[st_y + st_px / st_w][st_x + st_px % st_w] = c;
        st_px++;
    }
}
void st7735_stream_end(void) { (void)st_h; }
void st7735_backlight(bool on) { (void)on; }
}

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static int non_bg(int x, int y, int w, int h, uint16_t bg) {
    int n = 0;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (fb[y + j][x + i] != bg) n++;
    return n;
}

static int count_color(int x, int y, int w, int h, uint16_t color) {
    int n = 0;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (fb[y + j][x + i] == color) n++;
    return n;
}

int main() {
    /* Qualify check with empty scores */
    CHECK(highscores_qualifies(100, false));  /* Classic: both 0, 100 qualifies */
    CHECK(highscores_qualifies(100, true));   /* Extreme: both 0, 100 qualifies */

    /* Set some scores manually via mock EEPROM */
    mock_eeprom_writes = 0;
    memset(mock_eeprom, 0xFF, sizeof mock_eeprom);
    eeprom_store_init();  /* writes defaults */
    eeprom_set_score(0, 5000);  /* Classic #1 */
    eeprom_set_score(1, 3000);  /* Classic #2 */
    eeprom_set_score(2, 7000);  /* Extreme #1 */
    eeprom_set_score(3, 4000);  /* Extreme #2 */

    CHECK(!highscores_qualifies(2000, false));  /* Classic: 2000 < 3000 */
    CHECK(highscores_qualifies(4000, false));   /* Classic: 4000 > 3000, < 5000 */
    CHECK(highscores_qualifies(6000, false));   /* Classic: 6000 > 5000 */
    CHECK(!highscores_qualifies(3000, true));   /* Extreme: 3000 < 4000 */
    CHECK(highscores_qualifies(5000, true));    /* Extreme: 5000 > 4000, < 7000 */
    CHECK(highscores_qualifies(8000, true));    /* Extreme: 8000 > 7000 */

    /* Full draw test */
    fill_screen_calls = 0;
    highscores_draw(HS_MODE_CLASSIC);
    CHECK(fill_screen_calls == 1);
    CHECK(fb[0][0] == 0x0000);                   /* table header */
    CHECK(fb[159][0] == 0x0000);                 /* footer */
    /* Original decorations: two g medals at y=20 and two G frames at y=15/90. */
    CHECK(non_bg(20, 20, 11, 11, 0xFFFF) > 10);
    CHECK(non_bg(95, 20, 11, 11, 0xFFFF) > 10);
    CHECK(non_bg(33, 15, 60, 21, 0xFFFF) > 20);
    CHECK(non_bg(33, 90, 60, 21, 0xFFFF) > 20);
    /* Scores 5000/3000 use the original 8x9 digit glyphs (white pixels). */
    CHECK(count_color(39, 21, 48, 9, 0xFFFF) > 0);
    CHECK(count_color(39, 96, 48, 9, 0xFFFF) > 0);

    /* Extreme mode draw */
    fill_screen_calls = 0;
    highscores_draw(HS_MODE_EXTREME);
    CHECK(fill_screen_calls == 1);
    CHECK(non_bg(20, 20, 11, 11, 0xFFFF) > 10);
    CHECK(non_bg(33, 15, 60, 21, 0xFFFF) > 20);
    CHECK(non_bg(33, 90, 60, 21, 0xFFFF) > 20);

    /* Score formatting: check that zero scores show dashes */
    mock_eeprom_writes = 0;
    memset(mock_eeprom, 0xFF, sizeof mock_eeprom);
    eeprom_store_init();
    fill_screen_calls = 0;
    highscores_draw(HS_MODE_CLASSIC);
    CHECK(fill_screen_calls == 1);
    CHECK(count_color(39, 21, 48, 9, 0x0000) > 20);  /* six dashes, row 1 */
    CHECK(count_color(39, 96, 48, 9, 0x0000) > 20);  /* six dashes, row 2 */

    /* High-score entry requests SFX 11, gated by the sound setting. */
    mock_fire_pending = 1;
    mock_play_count = 0;
    CHECK(highscores_maybe_enter(100, false));
    CHECK(mock_play_count == 1);
    CHECK(mock_play_id == SFX_HIGH_SCORE);

    settings_t st = *eeprom_get_settings();
    st.sound_on = false;
    eeprom_save_settings(&st);
    mock_fire_pending = 1;
    mock_play_count = 0;
    CHECK(highscores_maybe_enter(100, false));
    CHECK(mock_play_count == 0);
    st.sound_on = true;
    eeprom_save_settings(&st);

    printf(fails ? "highscores: %d FAILED\n" : "highscores: all passed\n", fails);
    return fails != 0;
}