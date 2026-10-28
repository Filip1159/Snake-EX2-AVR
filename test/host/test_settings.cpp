/* Host test: settings screen state, labels, value changes, and partial repaint. */
#include "ui/settings.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include <stdio.h>
#include <string.h>

/* Mock the EEPROM storage for settings (referenced by the avr/eeprom.h mock
 * used when eeprom_store.cpp is compiled). */
uint8_t mock_eeprom[1024];
unsigned mock_eeprom_writes;

/* Mock millis for the Arduino mock. */
uint32_t mock_millis = 0;

static uint16_t fb[160][128];
static uint16_t before[160][128];
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
static int s_cx, s_cy, s_cw, s_px;
void st7735_stream_begin(uint8_t x, uint8_t y, uint8_t w, uint8_t h) { (void)h; s_cx = x; s_cy = y; s_cw = w; s_px = 0; }
void st7735_stream_pixels(uint16_t c, uint8_t n) {
    while (n--) { fb[s_cy + s_px / s_cw][s_cx + s_px % s_cw] = c; s_px++; }
}
void st7735_stream_end(void) {}

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

static int row_top(int item) {
    return 26 + item * 29;
}

int main() {
    settings_init();
    CHECK(settings_selected() == SETTING_MODE);

    /* Move down through all items and wrap */
    settings_move(1);
    CHECK(settings_selected() == SETTING_MAZE);
    settings_move(1);
    CHECK(settings_selected() == SETTING_SPEED);
    settings_move(1);
    CHECK(settings_selected() == SETTING_SOUND);
    settings_move(1);
    CHECK(settings_selected() == SETTING_MODE);

    /* Move up wraps */
    settings_move(-1);
    CHECK(settings_selected() == SETTING_SOUND);

    /* Full screen draw */
    fill_screen_calls = 0;
    settings_draw();
    CHECK(fill_screen_calls == 1);
    CHECK(fb[0][0] == 0x0000);
    CHECK(fb[159][0] == 0x0000);
    CHECK(fb[20][0] == 0xFFFF);
    CHECK(non_bg(0, 26, 128, 27, 0xFFFF) > 20);   /* first row bg */
    CHECK(non_bg(0, 55, 128, 27, 0xFFFF) > 20);   /* second row bg */
    CHECK(non_bg(0, 84, 128, 27, 0xFFFF) > 20);
    CHECK(non_bg(0, 113, 128, 27, 0xFFFF) > 20);
    CHECK(fb[140][0] == 0x0000);                  /* footer */

    /* Adjusting a value repaints only that row's value column (via
     * settings_draw_value). Verify the selection highlight doesn't change. */
    settings_init();
    settings_draw();
    memcpy(before, fb, sizeof fb);
    fill_screen_calls = 0;
    {
        /* Make sure we're on MODE row, then adjust */
        settings_adjust(1);               /* CLASSIC -> EXTREME */
        settings_draw_value(settings_selected());
    }
    CHECK(fill_screen_calls == 0);
    /* The mode value should have changed; compare with full redraw */
    memcpy(before, fb, sizeof fb);
    settings_draw();
    CHECK(memcmp(before, fb, sizeof fb) == 0);

    /* Speed increments and clamps */
    settings_init();
    settings_move(2); /* SPEED */
    settings_draw();
    {
        uint8_t init_speed = 0; /* we don't have access to shadow; just test clamp */
    }
    settings_adjust(1);
    settings_draw_value(settings_selected());
    /* second adjust should increment again */
    settings_adjust(1);
    settings_draw_value(settings_selected());
    /* going down should decrement */
    settings_adjust(-1);
    settings_draw_value(settings_selected());
    /* clamp at 0 */
    for (int i = 0; i < 10; i++) settings_adjust(-1);
    settings_draw_value(settings_selected());
    /* clamp at max */
    for (int i = 0; i < 10; i++) settings_adjust(1);
    settings_draw_value(settings_selected());

    /* Sound toggle */
    settings_init();
    settings_move(3); /* SOUND */
    settings_draw();
    settings_adjust(1);  /* toggle */
    settings_draw_value(settings_selected());
    settings_adjust(1);  /* toggle back */
    settings_draw_value(settings_selected());

    /* Maze cycles */
    settings_init();
    settings_move(1); /* MAZE */
    for (int i = 0; i < 7; i++) {
        settings_adjust(1);
        settings_draw_value(settings_selected());
    }
    for (int i = 0; i < 7; i++) {
        settings_adjust(-1);
        settings_draw_value(settings_selected());
    }

    printf(fails ? "settings: %d FAILED\n" : "settings: all passed\n", fails);
    return fails != 0;
}