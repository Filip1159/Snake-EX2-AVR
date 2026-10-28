#include "render/image.h"
#include "drivers/st7735.h"
#include <string.h>

/* Streams the sub-rectangle [rx,rx+rw) x [ry,ry+rh) of the image, with the image's
 * top-left at (x,y).  Runs outside the rectangle are skipped without touching the display. */
static void img_stream(uint8_t x, uint8_t y, const img_t *img_P,
                       uint8_t rx, uint8_t ry, uint8_t rw, uint8_t rh) {
    img_t img;
    memcpy_P(&img, img_P, sizeof(img_t));

    st7735_stream_begin((uint8_t)(x + rx), (uint8_t)(y + ry), rw, rh);
    const uint8_t *p = img.data;
    const uint8_t *end = img.data + img.len;
    uint8_t row = 0, col = 0;
    const uint8_t row_end = (uint8_t)(ry + rh);
    const uint8_t col_end = (uint8_t)(rx + rw);

    while (p < end && row < row_end) {
        uint8_t t = pgm_read_byte(p++);
        bool is_run = (t & 0x80) != 0;
        uint8_t n = is_run ? (uint8_t)((t & 0x7F) + 1) : (uint8_t)(t + 1);
        uint16_t color = 0;
        if (is_run) color = pgm_read_word(&img.pal[pgm_read_byte(p++)]);

        while (n && row < row_end) {
            if (!is_run) color = pgm_read_word(&img.pal[pgm_read_byte(p++)]);
            uint8_t take = (uint8_t)(img.w - col);        /* to the end of this image row */
            if (take > n) take = n;
            if (is_run == false) take = 1;

            if (row >= ry && col < col_end && (uint8_t)(col + take) > rx) {
                uint8_t a = col > rx ? col : rx;
                uint8_t b = (uint8_t)(col + take) < col_end ? (uint8_t)(col + take) : col_end;
                st7735_stream_pixels(color, (uint8_t)(b - a));
            }
            col = (uint8_t)(col + take);
            n = (uint8_t)(n - take);
            if (col >= img.w) { col = 0; row++; }
        }
    }
    st7735_stream_end();
}

void img_draw(uint8_t x, uint8_t y, const img_t *img_P) {
    img_t img;
    memcpy_P(&img, img_P, sizeof(img_t));
    img_stream(x, y, img_P, 0, 0, img.w, img.h);
}

void img_draw_rect(uint8_t x, uint8_t y, const img_t *img_P,
                   uint8_t rx, uint8_t ry, uint8_t rw, uint8_t rh) {
    img_stream(x, y, img_P, rx, ry, rw, rh);
}
