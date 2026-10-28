/* Host test: credits screen layout. */
#include "ui/credits.h"
#include "ui/chrome.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include <stdio.h>

static uint16_t fb[160][128];

extern "C" {
void st7735_fill_screen(uint16_t c) {
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
}

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static int count_color(int x, int y, int w, int h, uint16_t color) {
    int n = 0;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (fb[y + j][x + i] == color) n++;
    return n;
}

/* Pixels in the box that are dark navy ink (RGB565 red < 6, green < 24). */
static int dark_count(int x, int y, int w, int h) {
    int n = 0;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            uint16_t c = fb[y + j][x + i];
            if ((c >> 11) < 6 && ((c >> 5) & 63) < 24) n++;
        }
    return n;
}

int main() {
    credits_draw();
    CHECK(dark_count(96, 1, 32, 12) > 25);          /* status bar: the clock digits */
    CHECK(count_color(4, 18, 54, 7, 0x0000) > 20);           /* title */
    CHECK(count_color(100, 148, 24, 7, 0x0010) > 10);        /* "Back" bottom-right */

    /* The text block is centred: ink only between rows 30..145, symmetric about y=80,
     * and every line is horizontally centred (its ink is symmetric about x=64 +-3). */
    int top = -1, bot = -1;
    for (int y = 30; y < 146; y++)
        if (count_color(0, y, 128, 1, 0x0000) > 0) { if (top < 0) top = y; bot = y; }
    CHECK(top > 0);
    CHECK((top + bot) / 2 >= 78 && (top + bot) / 2 <= 82);
    int lines = 0;
    for (int y = top; y <= bot; ) {                          /* group ink rows into text lines */
        if (count_color(0, y, 128, 1, 0x0000) == 0) { y++; continue; }
        int l = 127, r = 0, y0 = y;
        for (; y <= bot && count_color(0, y, 128, 1, 0x0000) > 0; y++)
            for (int x = 0; x < 128; x++)
                if (fb[y][x] == 0x0000) { if (x < l) l = x; if (x > r) r = x; }
        lines++;
        CHECK(l + r >= 124 && l + r <= 132);                 /* centred on x=64 */
        (void)y0;
    }
    CHECK(lines == 5);

    /* "Silent" and the red muted note only show with chrome_set_silent(true). */
    auto reds = [](int x, int y, int w, int h) {
        int n = 0;
        for (int j = 0; j < h; j++)
            for (int i = 0; i < w; i++) {
                uint16_t c = fb[y + j][x + i];
                if ((c >> 11) > 20 && ((c >> 5) & 63) < 28) n++;
            }
        return n;
    };
    chrome_set_silent(false);
    credits_draw();
    CHECK(reds(48, 1, 14, 12) == 0 && dark_count(62, 1, 30, 12) == 0);
    chrome_set_silent(true);
    credits_draw();
    CHECK(reds(48, 1, 14, 12) > 5 && dark_count(62, 1, 24, 12) > 20);
    CHECK(dark_count(96, 1, 32, 12) > 25);               /* the clock stays */
    chrome_set_silent(false);

    printf(fails ? "credits: %d FAILED\n" : "credits: all passed\n", fails);
    return fails != 0;
}
