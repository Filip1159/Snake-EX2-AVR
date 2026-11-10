/* Host test: high scores table and saving logic. */
#include "ui/highscores.h"
#include "config.h"
#include "render/text.h"
#include "mock_display.h"
#include "drivers/buzzer.h"
#include "drivers/eeprom_store.h"
#include "Arduino.h"
#include <stdio.h>
#include <string.h>

/* Mock the EEPROM storage (referenced by the avr/eeprom.h mock). */
uint8_t mock_eeprom[1024];
unsigned mock_eeprom_writes;
uint32_t mock_millis = 0;

/* Mock buzzer functions called by highscores_submit. */
static uint8_t mock_play_id;
static int     mock_play_count;
extern "C" void buzzer_init(void) {}
extern "C" void buzzer_play(uint8_t id) { mock_play_id = id; mock_play_count++; }
extern "C" void buzzer_stop(void) {}
extern "C" bool buzzer_is_playing(void) { return false; }
extern "C" void buzzer_update(void) { }

int main() {
    /* Qualify check with empty scores */
    CHECK(highscores_qualifies(100, false));
    CHECK(highscores_qualifies(100, true));
    CHECK(!highscores_qualifies(0, false));

    /* Classic 5000/3000/1000, Extreme 7000/4000/2000 */
    mock_eeprom_writes = 0;
    memset(mock_eeprom, 0xFF, sizeof mock_eeprom);
    eeprom_store_init();
    eeprom_set_score(0, 5000); eeprom_set_score(1, 3000); eeprom_set_score(2, 1000);
    eeprom_set_score(3, 7000); eeprom_set_score(4, 4000); eeprom_set_score(5, 2000);

    CHECK(!highscores_qualifies(1000, false));  /* ties the 3rd */
    CHECK(highscores_qualifies(1001, false));   /* beats the 3rd */
    CHECK(!highscores_qualifies(2000, true));
    CHECK(highscores_qualifies(2001, true));

    highscores_draw(HS_MODE_CLASSIC);
    CHECK(fb[100][5] == COLOR_FIELD_BG && fb[16][100] == COLOR_FIELD_BG && fb[155][5] == COLOR_FIELD_BG);

    /* Draw: no top bar, title or soft key - only the frames and medals on the flat background */
    highscores_draw(HS_MODE_CLASSIC);
    CHECK(non_bg(0, 0, 128, 38, COLOR_FIELD_BG) == 0);       /* nothing above the first frame */
    CHECK(non_bg(0, 130, 128, 30, COLOR_FIELD_BG) == 0);     /* nothing below the last one */
    for (int rank = 0; rank < 3; rank++)
        CHECK(non_bg(33, 38 + rank * 34, 60, 21, 0xFFFF) > 20);   /* score frame */
    CHECK(non_bg(20, 43, 11, 11, 0xFFFF) > 10);              /* medals beside the best score only */
    CHECK(non_bg(95, 43, 11, 11, 0xFFFF) > 10);
    CHECK(non_bg(20, 77, 11, 11, 0xFFFF) > 0 && count_color(20, 77, 11, 11, 0xFFFF) < 10);
    for (int rank = 0; rank < 3; rank++)
        CHECK(count_color(39, 44 + rank * 34, 48, 9, 0xFFFF) > 0);  /* digit glyphs (white) */
    uint16_t classic_row3[48], extreme_row3[48];
    for (int i = 0; i < 48; i++) classic_row3[i] = fb[44 + 68 + 4][39 + i];
    highscores_draw(HS_MODE_EXTREME);                        /* a different table */
    for (int i = 0; i < 48; i++) extreme_row3[i] = fb[44 + 68 + 4][39 + i];
    CHECK(memcmp(classic_row3, extreme_row3, sizeof classic_row3) != 0);

    /* Empty slots show dashes instead of digits */
    memset(mock_eeprom, 0xFF, sizeof mock_eeprom);
    eeprom_store_init();
    highscores_draw(HS_MODE_CLASSIC);
    for (int rank = 0; rank < 3; rank++)
        CHECK(count_color(39, 44 + rank * 34, 48, 9, 0x0000) > 20);

    /* Saving: new scores are inserted by rank and push the others down; top-3 only */
    CHECK(highscores_submit(500, false));
    CHECK(eeprom_get_score(0) == 500 && eeprom_get_score(1) == 0 && eeprom_get_score(2) == 0);
    CHECK(highscores_submit(900, false));
    CHECK(highscores_submit(700, false));
    CHECK(eeprom_get_score(0) == 900 && eeprom_get_score(1) == 700 && eeprom_get_score(2) == 500);
    CHECK(highscores_submit(800, false));                       /* middle insert */
    CHECK(eeprom_get_score(0) == 900 && eeprom_get_score(1) == 800 && eeprom_get_score(2) == 700);
    CHECK(!highscores_submit(600, false));                      /* below the 3rd: not stored */
    CHECK(eeprom_get_score(2) == 700);
    CHECK(highscores_submit(1000, false));                      /* new best */
    CHECK(eeprom_get_score(0) == 1000 && eeprom_get_score(1) == 900 && eeprom_get_score(2) == 800);
    CHECK(eeprom_get_score(3) == 0 && eeprom_get_score(5) == 0); /* Extreme untouched */
    CHECK(highscores_submit(42, true));
    CHECK(eeprom_get_score(3) == 42 && eeprom_get_score(0) == 1000);

    /* Saving a qualifying score requests SFX 11, gated by the sound setting. */
    mock_play_count = 0;
    CHECK(highscores_submit(1100, false));
    CHECK(mock_play_count == 1);
    CHECK(mock_play_id == SFX_HIGH_SCORE);

    settings_t st = *eeprom_get_settings();
    st.sound_on = false;
    eeprom_save_settings(&st);
    mock_play_count = 0;
    CHECK(highscores_submit(1200, false));
    CHECK(mock_play_count == 0);
    st.sound_on = true;
    eeprom_save_settings(&st);

    printf(fails ? "highscores: %d FAILED\n" : "highscores: all passed\n", fails);
    return fails != 0;
}
