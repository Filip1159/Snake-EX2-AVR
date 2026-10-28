#include "render/image.h"
#include "drivers/st7735.h"
#include <string.h>

/* Streams rows [row0, row0+rows) of the image; the RLE stream is skipped up to row0. */
static void img_stream(uint8_t x, uint8_t y, const img_t *img_P, uint8_t row0, uint8_t rows) {
    img_t img;
    memcpy_P(&img, img_P, sizeof(img_t));

    uint16_t skip = (uint16_t)row0 * img.w;
    uint16_t left = (uint16_t)rows * img.w;
    st7735_stream_begin(x, y, img.w, rows);
    const uint8_t *p = img.data;
    const uint8_t *end = img.data + img.len;
    while (p < end && left) {
        uint8_t t = pgm_read_byte(p++);
        uint16_t n;
        if (t & 0x80) {
            uint8_t idx = pgm_read_byte(p++);
            n = (t & 0x7F) + 1;
            uint16_t s = n < skip ? n : skip;
            skip -= s;
            n -= s;
            if (n > left) n = left;
            left -= n;
            uint16_t c = pgm_read_word(&img.pal[idx]);
            while (n) { uint8_t k = n > 255 ? 255 : (uint8_t)n; st7735_stream_pixels(c, k); n -= k; }
        } else {
            n = t + 1;
            while (n--) {
                uint8_t idx = pgm_read_byte(p++);
                if (skip) { skip--; continue; }
                if (!left) continue;
                left--;
                st7735_stream_pixels(pgm_read_word(&img.pal[idx]), 1);
            }
        }
    }
    st7735_stream_end();
}

void img_draw(uint8_t x, uint8_t y, const img_t *img_P) {
    img_t img;
    memcpy_P(&img, img_P, sizeof(img_t));
    img_stream(x, y, img_P, 0, img.h);
}

void img_draw_rows(uint8_t x, uint8_t y, const img_t *img_P, uint8_t row0, uint8_t rows) {
    img_stream(x, y, img_P, row0, rows);
}
