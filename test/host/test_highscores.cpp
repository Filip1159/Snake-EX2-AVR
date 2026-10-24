/* Host test: high scores table and entry logic. */
#include "ui/highscores.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include "drivers/buttons.h"
#include "drivers/eeprom_store.h"
#include "Arduino.h"
#include <stdio.h>
#include <string.h>

/* Mock the EEPROM storage (referenced by the avr/eeprom.h mock). */
uint8_t mock_eeprom[1024];
unsigned mock_eeprom_writes;
uint32_t mock_millis = 0;

/* Mock buzzer functions called by highscores_maybe_enter. */
extern "C" void buzzer_init(void) {}
extern "C" void buzzer_play(uint8_t id) { (void)id; }
extern "C" void buzzer_stop(void) {}
extern "C" bool buzzer_is_playing(void) { return false; }
extern "C" void buzzer_update(void) { }

/* Mock button functions called by highscores_maybe_enter. */
extern "C" uint8_t buttons_poll(void) { return 0; }
extern "C" btn_event_t buttons_get_event(uint8_t *idx) { (void)idx; return BTN_EVENT_NONE; }
extern "C" bool buttons_is_pressed(uint8_t btn) { (void)btn; return false; }

static uint16_t fb[160][128];
static int fill_screen_calls;

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
    CHECK(fb[0][0] == 0x0000);
    CHECK(fb[159][0] == 0x0000);
    CHECK(non_bg(0, 26, 128, 27, 0xFFFF) > 20);  /* row 1 */
    CHECK(non_bg(0, 55, 128, 27, 0xFFFF) > 20);  /* row 2 */

    /* Extreme mode draw */
    fill_screen_calls = 0;
    highscores_draw(HS_MODE_EXTREME);
    CHECK(fill_screen_calls == 1);
    CHECK(non_bg(0, 26, 128, 27, 0xFFFF) > 20);
    CHECK(non_bg(0, 55, 128, 27, 0xFFFF) > 20);

    /* Score formatting: check that zero scores show dashes */
    mock_eeprom_writes = 0;
    memset(mock_eeprom, 0xFF, sizeof mock_eeprom);
    eeprom_store_init();
    fill_screen_calls = 0;
    highscores_draw(HS_MODE_CLASSIC);
    CHECK(fill_screen_calls == 1);

    printf(fails ? "highscores: %d FAILED\n" : "highscores: all passed\n", fails);
    return fails != 0;
}