/**
 * highscores.cpp — High-score screen and entry (BACKLOG U3).
 */
#include "ui/highscores.h"
#include "render/text.h"
#include "render/image.h"
#include "render/hud.h"
#include "drivers/st7735.h"
#include "drivers/buttons.h"
#include "drivers/eeprom_store.h"
#include "drivers/buzzer.h"
#include <Arduino.h>

#define HS_W          128
#define HS_HEADER_H   20          /* entry screen header */
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
#define HS_HL_BG      0x001F
#define HS_HL_INK     0xFFFF
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

/* Entry screen state */
static uint32_t s_entry_score;
static hs_mode_t s_entry_mode;
static char s_initials[4];
static uint8_t s_char_index;

static void entry_draw(void) {
    st7735_fill_screen(HS_BG);

    st7735_fill_rect(0, 0, HS_W, HS_HEADER_H, HS_HEADER_BG);
    {
        const char *title = "NEW HIGH SCORE";
        text_draw(centered_x(title, 2), 3, title, 2, HS_HEADER_INK);
    }

    /* Score */
    char buf[12];
    uint32_t v = s_entry_score;
    uint8_t pos = 0;
    char rev[10];
    do {
        rev[pos++] = '0' + (v % 10);
        v /= 10;
    } while (v && pos < 10);
    for (uint8_t i = 0; i < pos; i++) buf[i] = rev[pos - 1 - i];
    buf[pos] = '\0';
    text_draw(centered_x(buf, 2), 30, buf, 2, HS_INK);

    /* Initials */
    text_draw(centered_x("INITIALS", 1), 60, "INITIALS", 1, HS_INK);
    text_draw(centered_x(s_initials, 3), 74, s_initials, 3, HS_HL_INK);

    /* Cursor */
    uint8_t cx = centered_x(s_initials, 3) + s_char_index * 18;
    st7735_fill_rect(cx, 90, 14, 2, HS_HL_INK);

    st7735_fill_rect(0, HS_FOOTER_Y, HS_W, HS_FOOTER_H, HS_FOOTER_BG);
    text_draw(5, 146, "LR CHANGE", 1, HS_FOOTER_INK);
    {
        const char *back = "FIRE OK";
        uint8_t w = text_width(back, 1);
        text_draw((uint8_t)(HS_W - w - 5), 146, back, 1, HS_FOOTER_INK);
    }
}

static void entry_save(void) {
    uint8_t base = s_entry_mode == HS_MODE_CLASSIC ? 0 : 2;
    uint32_t s1 = eeprom_get_score(base);
    uint32_t s2 = eeprom_get_score(base + 1);

    if (s_entry_score > s1) {
        /* Shift down and insert at top */
        eeprom_set_score(base + 1, s1);
        eeprom_set_score(base, s_entry_score);
    } else if (s_entry_score > s2) {
        eeprom_set_score(base + 1, s_entry_score);
    }
    /* Note: initials are not stored in this implementation (original had none). */
}

bool highscores_maybe_enter(uint32_t score, bool extreme) {
    if (!highscores_qualifies(score, extreme)) return false;

    s_entry_score = score;
    s_entry_mode = extreme ? HS_MODE_EXTREME : HS_MODE_CLASSIC;
    s_initials[0] = 'A';
    s_initials[1] = 'A';
    s_initials[2] = 'A';
    s_initials[3] = '\0';
    s_char_index = 0;

    entry_draw();

    /* Snake.java:423 plays playSFX(11) when a score qualifies. */
    if (eeprom_get_settings()->sound_on) buzzer_play(SFX_HIGH_SCORE);

    while (true) {
        (void)buttons_poll();   /* updates the event queue */
        uint8_t btn;
        btn_event_t ev;
        while ((ev = buttons_get_event(&btn)) != BTN_EVENT_NONE) {
            if (ev != BTN_EVENT_PRESS && ev != BTN_EVENT_REPEAT) continue;

            if (btn == BTN_LEFT || btn == BTN_RIGHT) {
                int8_t delta = (btn == BTN_LEFT) ? -1 : 1;
                char c = s_initials[s_char_index];
                if (delta > 0) {
                    c = (c == 'Z') ? 'A' : c + 1;
                } else {
                    c = (c == 'A') ? 'Z' : c - 1;
                }
                s_initials[s_char_index] = c;
                entry_draw();
            } else if (btn == BTN_UP || btn == BTN_DOWN) {
                if (s_char_index < 2) s_char_index++;
                entry_draw();
            } else if (ev == BTN_EVENT_PRESS && btn == BTN_FIRE) {
                entry_save();
                return true;
            } else if (ev == BTN_EVENT_PRESS && btn == BTN_PAUSE) {
                /* Allow cancel? Original didn't have cancel; just save anyway. */
                entry_save();
                return true;
            }
        }
        if (buzzer_is_playing()) buzzer_update();
        delay(2);
    }
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