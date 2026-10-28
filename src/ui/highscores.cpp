/**
 * highscores.cpp — High-score table and saving (BACKLOG U3).
 */
#include "ui/highscores.h"
#include "render/text.h"
#include "render/image.h"
#include "render/hud.h"
#include "drivers/st7735.h"
#include "drivers/eeprom_store.h"
#include "drivers/buzzer.h"
#include <Arduino.h>

#define HS_W          128
#define HS_TITLE_H    12          /* table header; original rows start at y=15 */
#define HS_FRAME_X    33
#define HS_FRAME1_Y   15
#define HS_FRAME2_Y   90
#define HS_BADGE1_X   20
#define HS_BADGE2_X   95
#define HS_BADGE_Y    20
#define HS_SCORE_X    39
#define HS_SCORE1_Y   21
#define HS_SCORE2_Y   96
#define HS_SCORE_DIGITS 6
#define HS_DIGIT_PITCH  8
#define HS_FOOTER_Y   140
#define HS_FOOTER_H   20

#define HS_BG         0xFFFF
#define HS_INK        0x0000
#define HS_HEADER_BG  0x0000
#define HS_HEADER_INK 0xFFFF
#define HS_FOOTER_BG  0x0000
#define HS_FOOTER_INK 0xFFFF

static hs_mode_t s_mode;
static uint32_t s_scores[2];   /* two scores for the current mode */

static const char *const HS_MODE_NAMES[HS_MODE_COUNT] = {
    "CLASSIC",
    "EXTREME",
};

static uint8_t centered_x(const char *txt, uint8_t scale) {
    uint8_t w = text_width(txt, scale);
    return (w >= HS_W) ? 0 : (uint8_t)((HS_W - w) / 2);
}

static void load_scores(void) {
    uint8_t base = s_mode == HS_MODE_CLASSIC ? 0 : 2;
    s_scores[0] = eeprom_get_score(base);
    s_scores[1] = eeprom_get_score(base + 1);
}

/** Original six-digit score (SnakeCanvas.a: 8 px pitch, zero padded).  Empty
 *  slots keep the reconstructed table's dashes rather than six zeros. */
static void draw_score_value(uint8_t x, uint8_t y, uint32_t v) {
    if (v == 0) {
        for (uint8_t i = 0; i < HS_SCORE_DIGITS; i++)
            st7735_fill_rect((uint8_t)(x + i * HS_DIGIT_PITCH + 1),
                             (uint8_t)(y + 4), 6, 2, HS_INK);
    } else {
        hud_draw_number(x, y, v, HS_SCORE_DIGITS);
    }
}

void highscores_show(hs_mode_t mode) {
    s_mode = mode;
    load_scores();
    highscores_draw(mode);
}

bool highscores_qualifies(uint32_t score, bool extreme) {
    hs_mode_t mode = extreme ? HS_MODE_EXTREME : HS_MODE_CLASSIC;
    uint8_t base = mode == HS_MODE_CLASSIC ? 0 : 2;
    uint32_t s1 = eeprom_get_score(base);
    uint32_t s2 = eeprom_get_score(base + 1);
    return score > s1 || score > s2;
}

/** Stores `score` in the table (shifting the lower entry down when it takes first place). */
static void save_score(uint32_t score, hs_mode_t mode) {
    uint8_t base = mode == HS_MODE_CLASSIC ? 0 : 2;
    uint32_t s1 = eeprom_get_score(base);
    uint32_t s2 = eeprom_get_score(base + 1);

    if (score > s1) {
        eeprom_set_score(base + 1, s1);
        eeprom_set_score(base, score);
    } else if (score > s2) {
        eeprom_set_score(base + 1, score);
    }
}

bool highscores_submit(uint32_t score, bool extreme) {
    if (!highscores_qualifies(score, extreme)) return false;

    save_score(score, extreme ? HS_MODE_EXTREME : HS_MODE_CLASSIC);

    /* Snake.java:423 plays playSFX(11) when a score qualifies. */
    if (eeprom_get_settings()->sound_on) buzzer_play(SFX_HIGH_SCORE);
    return true;
}

void highscores_draw(hs_mode_t mode) {
    s_mode = mode;
    load_scores();

    st7735_fill_screen(HS_BG);

    /* Tier-B header; the original screen has no title and its first row starts
     * at y=15.  A 12 px bar leaves that original geometry intact. */
    st7735_fill_rect(0, 0, HS_W, HS_TITLE_H, HS_HEADER_BG);
    {
        char title[24];
        uint8_t p = 0;
        const char *pre = "HIGH SCORES ";
        while (*pre) title[p++] = *pre++;
        const char *m = HS_MODE_NAMES[mode];
        while (*m) title[p++] = *m++;
        title[p] = '\0';
        text_draw(centered_x(title, 1), 3, title, 1, HS_HEADER_INK);
    }

    /* Snake.java:710-719: two g medals beside the top score, two G frames, then
     * the six-digit scores at (39,21) and (39,96). */
    img_draw(HS_BADGE1_X, HS_BADGE_Y, &IMG_HS_BADGE);
    img_draw(HS_BADGE2_X, HS_BADGE_Y, &IMG_HS_BADGE);
    img_draw(HS_FRAME_X, HS_FRAME1_Y, &IMG_HS_FRAME);
    img_draw(HS_FRAME_X, HS_FRAME2_Y, &IMG_HS_FRAME);
    draw_score_value(HS_SCORE_X, HS_SCORE1_Y, s_scores[0]);
    draw_score_value(HS_SCORE_X, HS_SCORE2_Y, s_scores[1]);

    st7735_fill_rect(0, HS_FOOTER_Y, HS_W, HS_FOOTER_H, HS_FOOTER_BG);
    text_draw(5, 146, "PAUSE BACK", 1, HS_FOOTER_INK);
}