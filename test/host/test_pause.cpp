/* Host test: pause overlay. */
#include "ui/pause.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include "drivers/buttons.h"
#include "Arduino.h"
#include <stdio.h>
#include <string.h>

/* Mock button functions. */
extern "C" uint8_t buttons_poll(void) { return 0; }
extern "C" btn_event_t buttons_get_event(uint8_t *idx) { (void)idx; return BTN_EVENT_NONE; }
extern "C" bool buttons_is_pressed(uint8_t btn) { (void)btn; return false; }

/* Mock buzzer: the blocking UI loops must keep a playing effect advancing. */
static int mock_buzzer_updates;
extern "C" bool buzzer_is_playing(void) { return true; }
extern "C" void buzzer_update(void) { mock_buzzer_updates++; }

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
}

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main() {
    /* Initialize framebuffer to white (so we can see the black grid) */
    for (int y = 0; y < 160; y++)
        for (int x = 0; x < 128; x++) fb[y][x] = 0xFFFF;

    /* Pause overlay draws a grid pattern, not a full-screen fill */
    pause_draw();
    CHECK(fill_screen_calls == 0);
    /* Check that grid pattern was drawn (every other pixel black) */
    CHECK(fb[0][0] == 0x0000);
    CHECK(fb[0][1] == 0xFFFF);

    /* The blocking pause loop services the buzzer. */
    mock_buzzer_updates = 0;
    pause_handle_input();
    CHECK(mock_buzzer_updates == 1);

    printf(fails ? "pause: %d FAILED\n" : "pause: all passed\n", fails);
    return fails != 0;
}