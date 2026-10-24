#include "render/hud.h"
#include "render/image.h"
#include "drivers/st7735.h"
#include "config.h"

#define HUD_ICON_X  (POWERUP_ICON_X)
#define HUD_ICON_Y  (POWERUP_ICON_Y)
#define DIGIT_W     8

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

void title_draw(void) {
    st7735_fill_rect(0, 0, 128, TITLE_Y, TITLE_TOP_COLOR);
    st7735_fill_rect(0, TITLE_Y + TITLE_HEIGHT, 128, 160 - TITLE_Y - TITLE_HEIGHT, TITLE_BOTTOM_COLOR);
    img_draw(TITLE_X, TITLE_Y, &IMG_TITLE);
}
