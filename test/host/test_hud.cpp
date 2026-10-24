/* Verifies flash RLE images (title, HUD, digits, power-up icons) against converted RGB565 files. */
#include "render/hud.h"
#include "render/image.h"
#include "drivers/st7735.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include <cstdlib>

static uint16_t fb[160][128];
static int cx, cy, cw, ch, px;
extern "C" {
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c) {
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) fb[y + j][x + i] = c;
}
void st7735_stream_begin(uint8_t x, uint8_t y, uint8_t w, uint8_t h) { cx = x; cy = y; cw = w; ch = h; px = 0; }
void st7735_stream_pixels(uint16_t c, uint8_t n) {
    while (n--) { fb[cy + px / cw][cx + px % cw] = c; px++; }
}
void st7735_stream_end(void) { if (px != cw * ch) { printf("stream wrote %d of %d px\n", px, cw * ch); exit(3); } }
}

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static std::vector<uint8_t> slurp(const char *p) {
    FILE *f = fopen(p, "rb");
    if (!f) { printf("cannot open %s\n", p); exit(2); }
    std::vector<uint8_t> v; int c; while ((c = fgetc(f)) != EOF) v.push_back((uint8_t)c); fclose(f); return v;
}
static uint16_t at(const std::vector<uint8_t> &v, int w, int x, int y) { int o = (y * w + x) * 2; return (v[o] << 8) | v[o + 1]; }

/* count mismatching pixels of a w*h region at (x,y) against src (src_w wide) at (sx,sy) */
static int diff(const std::vector<uint8_t> &src, int sw, int sx, int sy, int x, int y, int w, int h) {
    int bad = 0;
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++)
        if (fb[y + j][x + i] != at(src, sw, sx + i, sy + j)) bad++;
    return bad;
}

int main() {
    auto title = slurp("build/assets/rgb565/title.rgb565");
    auto hud = slurp("build/assets/rgb565/hud.rgb565");
    auto dig = slurp("build/assets/rgb565/digits.rgb565");
    auto pu = slurp("build/assets/rgb565/powerup_strip.rgb565");

    title_draw();
    CHECK(diff(title, 128, 0, 0, TITLE_X, TITLE_Y, 128, 128) == 0);
    CHECK(fb[0][0] == TITLE_TOP_COLOR && fb[15][127] == TITLE_TOP_COLOR);
    CHECK(fb[144][0] == TITLE_BOTTOM_COLOR && fb[159][127] == TITLE_BOTTOM_COLOR);
    CHECK(at(title, 128, 0, 0) == TITLE_TOP_COLOR && at(title, 128, 5, 127) == TITLE_BOTTOM_COLOR);

    memset(fb, 0, sizeof fb);
    hud_draw_bar();
    CHECK(diff(hud, 128, 0, 0, HUD_X, HUD_Y, 128, 20) == 0);

    /* digits: every value 0..9 at every score cell, incl. padding and wrap */
    uint32_t tests[] = {0, 7, 123456, 999999, 1000000 + 42, 906090};
    for (uint32_t v : tests) {
        hud_draw_score(v);
        uint32_t t = v;
        for (int n = SCORE_DIGITS - 1; n >= 0; n--) {
            int d = t % 10; t /= 10;
            CHECK(diff(dig, 77, 7 * d, 0, SCORE_X + 8 * n, SCORE_Y, 8, 9) == 0);
        }
    }
    hud_draw_bonus(12345);   /* shows 2345 */
    { uint16_t t = 12345 % 10000; for (int n = 3; n >= 0; n--) { int d = t % 10; t /= 10;
        CHECK(diff(dig, 77, 7 * d, 0, BONUS_X + 8 * n, BONUS_Y, 8, 9) == 0); } }

    for (int id = 1; id <= 10; id++) {
        hud_draw_powerup(id);
        CHECK(diff(pu, 130, 13 * (id - 1), 0, POWERUP_ICON_X, POWERUP_ICON_Y, 13, 13) == 0);
    }
    hud_draw_powerup(0);
    CHECK(diff(hud, 128, POWERUP_ICON_X, POWERUP_ICON_Y - HUD_Y, POWERUP_ICON_X, POWERUP_ICON_Y, 13, 13) == 0);

    printf(fails ? "hud/title: %d FAILED\n" : "hud/title: all passed\n", fails);
    return fails != 0;
}
