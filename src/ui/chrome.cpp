/**
 * chrome.cpp — Shared Series 40 screen chrome (see chrome.h).
 */
#include "ui/chrome.h"
#include "render/text.h"
#include "render/image.h"
#include "assets/wallpaper.h"
#include "drivers/st7735.h"

#define CHROME_STATUS_INK 0x0010
#define CHROME_BATT_FILL  0x07E0
#define CHROME_RIGHT_EDGE 126          /* right-aligned title text ends here */

#define CHROME_STATUS_TIME "22:22"

static bool     s_flat;
static uint16_t s_flat_color;

void chrome_use_wallpaper(void) { s_flat = false; }
void chrome_use_flat(uint16_t color) { s_flat = true; s_flat_color = color; }

void chrome_draw_wallpaper(void) {
    if (s_flat) st7735_fill_rect(0, 0, CHROME_W, 160, s_flat_color);
    else        img_draw(0, 0, &IMG_WALLPAPER);
}

void chrome_restore(uint8_t x, uint8_t w, uint8_t y, uint8_t h) {
    if (s_flat) st7735_fill_rect(x, y, w, h, s_flat_color);
    else        img_draw_rect(0, 0, &IMG_WALLPAPER, x, y, w, h);
}

void chrome_dotted_leader(uint8_t x0, uint8_t x1, uint8_t y, uint16_t color) {
    if (x0 >= x1) return;
    for (uint8_t x = x0; x < x1; x = (uint8_t)(x + 4)) {
        st7735_fill_rect(x, y, 1, 1, color);
    }
}

void chrome_draw_status_bar(void) {
    /* Signal-strength bars at the left. */
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t h = (uint8_t)(2 + i * 2);
        st7735_fill_rect((uint8_t)(3 + i * 3), (uint8_t)(11 - h), 2, h,
                         CHROME_STATUS_INK);
    }
    chrome_dotted_leader(18, 38, 7, CHROME_DOT);

    /* Battery outline, fill and terminal nub. */
    st7735_fill_rect(42, 3, 11, 7, CHROME_STATUS_INK);
    st7735_fill_rect(43, 4, 9, 5, 0xFFFF);
    st7735_fill_rect(44, 5, 6, 3, CHROME_BATT_FILL);
    st7735_fill_rect(53, 5, 2, 3, CHROME_STATUS_INK);
    chrome_dotted_leader(57, 94, 7, CHROME_DOT);

    {
        uint8_t w = text_width(CHROME_STATUS_TIME, 1);
        text_draw((uint8_t)(CHROME_W - w - 2), 3, CHROME_STATUS_TIME, 1,
                  CHROME_STATUS_INK);
    }
}

void chrome_draw_title(const char *title, const char *right) {
    chrome_restore(0, CHROME_W, CHROME_TITLE_Y, CHROME_TITLE_H);
    text_draw(4, 18, title, 1, CHROME_INK);

    uint8_t leader_end = 118;
    if (right) {
        uint8_t rx = (uint8_t)(CHROME_RIGHT_EDGE + 1 - text_width(right, 1));
        text_draw(rx, 18, right, 1, CHROME_INK);
        leader_end = (uint8_t)(rx - 3);
    }
    chrome_dotted_leader((uint8_t)(4 + text_width(title, 1) + 3), leader_end, 21, CHROME_DOT);
}

uint8_t chrome_draw_softkey_right(const char *label) {
    uint8_t rx = (uint8_t)(CHROME_W - text_width(label, 1) - 4);
    text_draw(rx, 148, label, 1, CHROME_SOFT_INK);
    return rx;
}
