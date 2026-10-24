#include "render/image.h"
#include "drivers/st7735.h"
#include <string.h>

void img_draw(uint8_t x, uint8_t y, const img_t *img_P) {
    img_t img;
    memcpy_P(&img, img_P, sizeof(img_t));

    st7735_stream_begin(x, y, img.w, img.h);
    const uint8_t *p = img.data;
    const uint8_t *end = img.data + img.len;
    while (p < end) {
        uint8_t t = pgm_read_byte(p++);
        if (t & 0x80) {
            uint8_t idx = pgm_read_byte(p++);
            st7735_stream_pixels(pgm_read_word(&img.pal[idx]), (t & 0x7F) + 1);
        } else {
            uint8_t n = t + 1;
            while (n--) {
                uint8_t idx = pgm_read_byte(p++);
                st7735_stream_pixels(pgm_read_word(&img.pal[idx]), 1);
            }
        }
    }
    st7735_stream_end();
}
