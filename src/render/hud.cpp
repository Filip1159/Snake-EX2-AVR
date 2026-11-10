#include "render/hud.h"
#include "render/image.h"
#include "render/sprite.h"
#include "assets/trophy.h"
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

void title_draw(void) {
    /* The art's first and last rows are flat colours, so the 16 rows above and
     * below it extend them seamlessly to the full 128x160 screen. */
    st7735_fill_rect(0, 0, 128, TITLE_Y, TITLE_TOP_COLOR);
    st7735_fill_rect(0, TITLE_Y + TITLE_HEIGHT, 128,
                     160 - TITLE_Y - TITLE_HEIGHT, TITLE_BOTTOM_COLOR);
    img_draw(TITLE_X, TITLE_Y, &IMG_TITLE);
}

void title_draw_score(uint32_t score, bool new_best) {
    title_draw();
    img_draw(TITLE_SCORE_X, TITLE_SCORE_Y, &IMG_HS_FRAME);
    hud_draw_number(TITLE_SCORE_X + 6, TITLE_SCORE_Y + 6, score, SCORE_DIGITS);
    if (new_best) {
        sprite_draw_p(TITLE_TROPHY_L_X, TITLE_SCORE_Y, &SPR_TROPHY);
        sprite_draw_p(TITLE_TROPHY_R_X, TITLE_SCORE_Y, &SPR_TROPHY);
    }
}
