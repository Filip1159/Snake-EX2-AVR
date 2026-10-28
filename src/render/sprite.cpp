#include "render/sprite.h"
#include "drivers/st7735.h"
#include <string.h>

#define SCREEN_W 128
#define SCREEN_H 160

static int16_t clip_x0 = 0, clip_y0 = 0, clip_x1 = SCREEN_W, clip_y1 = SCREEN_H;

void sprite_set_clip(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    clip_x0 = x0; clip_y0 = y0; clip_x1 = x1; clip_y1 = y1;
}

void sprite_reset_clip(void) {
    clip_x0 = 0; clip_y0 = 0; clip_x1 = SCREEN_W; clip_y1 = SCREEN_H;
}

void sprite_draw(int16_t x, int16_t y, uint8_t id) {
    if (id >= SPRITE_COUNT) return;
    sprite_draw_p(x, y, &SPRITES[id]);
}

void sprite_draw_p(int16_t x, int16_t y, const spr_t *spr_P) {
    spr_t s;
    memcpy_P(&s, spr_P, sizeof(spr_t));

    for (uint8_t row = 0; row < s.h; row++) {
        int16_t sy = y + row;
        if (sy < clip_y0 || sy >= clip_y1) continue;
        uint8_t col = 0;
        while (col < s.w) {
            uint16_t k;
            uint8_t idx;
            /* skip transparent / off-screen pixels */
            for (;;) {
                if (col >= s.w) break;
                k = (uint16_t)row * s.w + col;
                uint8_t b = pgm_read_byte(&s.pix[k >> 1]);
                idx = (k & 1) ? (b & 0x0F) : (b >> 4);
                int16_t sx = x + col;
                if (idx != s.transparent && sx >= clip_x0 && sx < clip_x1) break;
                col++;
            }
            if (col >= s.w) break;
            uint8_t start = col;
            /* collect the opaque run */
            uint16_t run_colors[11];
            uint8_t n = 0;
            while (col < s.w && n < 11) {
                k = (uint16_t)row * s.w + col;
                uint8_t b = pgm_read_byte(&s.pix[k >> 1]);
                idx = (k & 1) ? (b & 0x0F) : (b >> 4);
                int16_t sx = x + col;
                if (idx == s.transparent || sx < clip_x0 || sx >= clip_x1) break;
                run_colors[n++] = pgm_read_word(&s.pal[idx]);
                col++;
            }
            st7735_stream_begin((uint8_t)(x + start), (uint8_t)sy, n, 1);
            for (uint8_t i = 0; i < n; i++) st7735_stream_pixels(run_colors[i], 1);
            st7735_stream_end();
        }
    }
}
