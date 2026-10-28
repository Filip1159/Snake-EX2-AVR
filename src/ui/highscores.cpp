/**
 * highscores.cpp — High-score table and saving (BACKLOG U3).
 */
#include "ui/highscores.h"
#include "ui/chrome.h"
#include "config.h"
#include "render/text.h"
#include "render/image.h"
#include "render/hud.h"
#include "drivers/st7735.h"
#include "drivers/eeprom_store.h"
#include "drivers/buzzer.h"

#define HS_FRAME_X    33
#define HS_FRAME_Y0   38                 /* first of three 21 px frames, 13 px apart */
#define HS_FRAME_PITCH 34
#define HS_BADGE1_X   20                 /* medals beside the best score */
#define HS_BADGE2_X   95
#define HS_BADGE_DY   5
#define HS_SCORE_DX   6                  /* digits inside the frame (Snake.java: 39,21 for 33,15) */
#define HS_SCORE_DY   6
#define HS_SCORE_DIGITS 6
#define HS_DIGIT_PITCH  8

/** Original six-digit score (SnakeCanvas.a: 8 px pitch, zero padded).  Empty
 *  slots keep the reconstructed table's dashes rather than six zeros. */
static void draw_score_value(uint8_t x, uint8_t y, uint32_t v) {
    if (v == 0) {
        for (uint8_t i = 0; i < HS_SCORE_DIGITS; i++)
            st7735_fill_rect((uint8_t)(x + i * HS_DIGIT_PITCH + 1),
                             (uint8_t)(y + 4), 6, 2, CHROME_INK);
    } else {
        hud_draw_number(x, y, v, HS_SCORE_DIGITS);
    }
}

void highscores_show(hs_mode_t mode) {
    highscores_draw(mode);
}

bool highscores_qualifies(uint32_t score, bool extreme) {
    uint8_t base = extreme ? SCORES_PER_MODE : 0;
    return score > eeprom_get_score((uint8_t)(base + SCORES_PER_MODE - 1));   /* beats the 3rd */
}

/** Stores `score` in the mode's table, pushing the lower entries down one rank. */
static void save_score(uint32_t score, hs_mode_t mode) {
    uint8_t base = mode == HS_MODE_CLASSIC ? 0 : SCORES_PER_MODE;
    uint8_t pos = 0;
    while (pos < SCORES_PER_MODE && score <= eeprom_get_score((uint8_t)(base + pos))) pos++;
    if (pos == SCORES_PER_MODE) return;
    for (uint8_t i = SCORES_PER_MODE - 1; i > pos; i--)
        eeprom_set_score((uint8_t)(base + i), eeprom_get_score((uint8_t)(base + i - 1)));
    eeprom_set_score((uint8_t)(base + pos), score);
}

bool highscores_submit(uint32_t score, bool extreme) {
    if (!highscores_qualifies(score, extreme)) return false;

    save_score(score, extreme ? HS_MODE_EXTREME : HS_MODE_CLASSIC);

    /* Snake.java:423 plays playSFX(11) when a score qualifies. */
    if (eeprom_get_settings()->sound_on) buzzer_play(SFX_HIGH_SCORE);
    return true;
}

void highscores_draw(hs_mode_t mode) {
    chrome_use_flat(COLOR_FIELD_BG);          /* same background as the game board */
    chrome_draw_wallpaper();

    /* Snake.java:710-719: two g medals beside the top score, a G frame per score,
     * then the six-digit scores inside the frames. */
    uint8_t base = mode == HS_MODE_CLASSIC ? 0 : SCORES_PER_MODE;
    for (uint8_t rank = 0; rank < SCORES_PER_MODE; rank++) {
        uint8_t y = (uint8_t)(HS_FRAME_Y0 + rank * HS_FRAME_PITCH);
        img_draw(HS_FRAME_X, y, &IMG_HS_FRAME);
        draw_score_value((uint8_t)(HS_FRAME_X + HS_SCORE_DX), (uint8_t)(y + HS_SCORE_DY),
                         eeprom_get_score((uint8_t)(base + rank)));
        if (rank == 0) {
            img_draw(HS_BADGE1_X, (uint8_t)(y + HS_BADGE_DY), &IMG_HS_BADGE);
            img_draw(HS_BADGE2_X, (uint8_t)(y + HS_BADGE_DY), &IMG_HS_BADGE);
        }
    }
}
