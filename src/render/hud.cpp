#include "render/hud.h"
#include "render/image.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include "config.h"

#define HUD_ICON_X  (POWERUP_ICON_X)
#define HUD_ICON_Y  (POWERUP_ICON_Y)
#define DIGIT_W     8

#define TITLE_CHROME_INK   0x0010
#define TITLE_TROPHY_FILL  0xD6BA
#define TITLE_TROPHY_HI    0xFFFF
#define TITLE_BUTTON_EDGE  0x8410

static void draw_digits(uint8_t x, uint8_t y, uint32_t v, uint8_t n) {
    while (n--) {
        uint8_t d = (uint8_t)(v % 10);
        v /= 10;
        img_draw(x + DIGIT_W * n, y, &IMG_DIGITS[d]);
    }
}

void hud_draw_bar(void) {
    img_draw(HUD_X, HUD_Y, &IMG_HUD);
}

void hud_draw_number(uint8_t x, uint8_t y, uint32_t value, uint8_t digits) {
    draw_digits(x, y, value, digits);
}

void hud_draw_score(uint32_t score) {
    draw_digits(SCORE_X, SCORE_Y, score, SCORE_DIGITS);
}

void hud_draw_bonus(uint16_t value) {
    draw_digits(BONUS_X, BONUS_Y, value, BONUS_DIGITS);
}

void hud_draw_powerup(uint8_t id) {
    if (id >= 1 && id <= POWERUP_TYPE_COUNT) {
        img_draw(HUD_ICON_X, HUD_ICON_Y, &IMG_POWERUPS[id - 1]);
    } else {
        img_draw(HUD_ICON_X, HUD_ICON_Y, &IMG_HUD_ICON_BG);
    }
}

/* 14x14 silver trophy for the title screen's bottom band. */
static void draw_trophy(uint8_t x, uint8_t y) {
    /* Handles. */
    st7735_fill_rect(x, y + 3, 3, 1, TITLE_CHROME_INK);
    st7735_fill_rect(x, y + 4, 1, 2, TITLE_CHROME_INK);
    st7735_fill_rect(x + 1, y + 6, 2, 1, TITLE_CHROME_INK);
    st7735_fill_rect(x + 11, y + 3, 3, 1, TITLE_CHROME_INK);
    st7735_fill_rect(x + 13, y + 4, 1, 2, TITLE_CHROME_INK);
    st7735_fill_rect(x + 11, y + 6, 2, 1, TITLE_CHROME_INK);

    /* Cup outline and fill. */
    st7735_fill_rect(x + 3, y + 1, 8, 1, TITLE_CHROME_INK);
    st7735_fill_rect(x + 3, y + 2, 1, 5, TITLE_CHROME_INK);
    st7735_fill_rect(x + 10, y + 2, 1, 5, TITLE_CHROME_INK);
    st7735_fill_rect(x + 3, y + 7, 8, 1, TITLE_CHROME_INK);
    st7735_fill_rect(x + 4, y + 2, 6, 5, TITLE_TROPHY_FILL);
    st7735_fill_rect(x + 5, y + 2, 1, 4, TITLE_TROPHY_HI);

    /* Stem and base. */
    st7735_fill_rect(x + 6, y + 8, 2, 2, TITLE_CHROME_INK);
    st7735_fill_rect(x + 7, y + 8, 1, 2, TITLE_TROPHY_FILL);
    st7735_fill_rect(x + 4, y + 10, 6, 1, TITLE_CHROME_INK);
    st7735_fill_rect(x + 3, y + 11, 8, 1, TITLE_CHROME_INK);
    st7735_fill_rect(x + 5, y + 10, 4, 1, TITLE_TROPHY_FILL);
    st7735_fill_rect(x + 4, y + 11, 6, 1, TITLE_TROPHY_FILL);
    st7735_fill_rect(x + 3, y + 13, 8, 1, TITLE_CHROME_INK);
}

static void draw_options_button(void) {
    const uint8_t x = 36;
    const uint8_t y = 146;
    const uint8_t w = 56;
    const uint8_t h = 10;
    const char *label = "OPTIONS";

    st7735_fill_rect(x, y, w, h, 0xFFFF);
    st7735_fill_rect(x, y, w, 1, TITLE_BUTTON_EDGE);
    st7735_fill_rect(x, (uint8_t)(y + h - 1), w, 1, TITLE_BUTTON_EDGE);
    st7735_fill_rect(x, y, 1, h, TITLE_BUTTON_EDGE);
    st7735_fill_rect((uint8_t)(x + w - 1), y, 1, h, TITLE_BUTTON_EDGE);

    {
        uint8_t tw = text_width(label, 1);
        text_draw((uint8_t)(x + (w - tw) / 2), (uint8_t)(y + 2), label, 1,
                  TITLE_CHROME_INK);
    }
}

void title_draw(void) {
    st7735_fill_rect(0, 0, 128, TITLE_Y, TITLE_BG_COLOR);
    st7735_fill_rect(0, TITLE_Y + TITLE_HEIGHT, 128,
                     160 - TITLE_Y - TITLE_HEIGHT, TITLE_BG_COLOR);
    img_draw(TITLE_X, TITLE_Y, &IMG_TITLE);

    /* The reference title screen places two trophies and an Options button
     * in the band below the original 128x128 art. */
    draw_trophy(6, 145);
    draw_trophy(108, 145);
    draw_options_button();
}
