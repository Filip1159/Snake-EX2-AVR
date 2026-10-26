/**
 * pause.cpp — Pause overlay and game-over screen (BACKLOG U4).
 */
#include "ui/pause.h"
#include "config.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include "drivers/buttons.h"
#include "drivers/eeprom_store.h"
#include "drivers/buzzer.h"
#include "drivers/stack_measure.h"
#include <Arduino.h>

#define P_W           128
#define P_H           160

#define P_BG          0x0000
#define P_INK         0xFFFF
#define P_HL_BG       0x001F
#define P_HL_INK      0xFFFF

static uint8_t s_pause_selected;
static uint8_t s_go_selected;

#if STACK_MEASURE
/* Q5 debug build only: show the free SRAM below the deepest stack use. */
static void draw_stack_debug(uint8_t y) {
    uint16_t free_b = stack_measure_update();
    char num[6];
    uint8_t n = 0;
    do {
        num[n++] = (char)('0' + (free_b % 10));
        free_b /= 10;
    } while (free_b && n < 5);
    for (uint8_t i = 0; i < n / 2; i++) {
        char t = num[i];
        num[i] = num[n - 1 - i];
        num[n - 1 - i] = t;
    }
    num[n] = '\0';

    st7735_fill_rect(0, (uint8_t)(y - 2), 128, 12, 0x0000);
    text_draw(4, y, "STACK FREE:", 1, 0xFFFF);
    text_draw(4 + 11 * 6, y, num, 1, 0xFFFF);
}
#endif

/* ── Pause overlay ───────────────────────────────────────────────────────── */

void pause_draw(void) {
    /* Semi-transparent dark overlay: draw a grid pattern over the game. */
    for (uint8_t y = 0; y < P_H; y += 2) {
        for (uint8_t x = 0; x < P_W; x += 2) {
            st7735_fill_rect(x, y, 1, 1, 0x0000);
        }
    }

    /* Centered box */
    const char *title = "PAUSED";
    uint8_t title_w = text_width(title, 2);
    uint8_t box_x = (uint8_t)((P_W - title_w - 20) / 2);
    uint8_t box_y = 30;
    uint8_t box_w = title_w + 20;
    uint8_t box_h = 80;

    st7735_fill_rect(box_x, box_y, box_w, box_h, P_HL_BG);
    text_draw(box_x + 10, box_y + 8, title, 2, P_HL_INK);

    const char *items[3] = { "RESUME", "RESTART", "MENU" };
    for (uint8_t i = 0; i < 3; i++) {
        uint8_t y = box_y + 30 + i * 18;
        bool sel = (i == s_pause_selected);
        text_draw(box_x + 10, y, items[i], 1, sel ? P_HL_INK : P_INK);
        if (sel) {
            st7735_fill_rect(box_x + 5, y + 2, 3, 3, P_HL_INK);
        }
    }

#if STACK_MEASURE
    draw_stack_debug(140);
#endif
}

pause_action_t pause_handle_input(void) {
    /* This loop blocks the main loop, so keep a long effect advancing. */
    if (buzzer_is_playing()) buzzer_update();

    (void)buttons_poll();   /* updates the event queue */
    uint8_t btn;
    btn_event_t ev;
    while ((ev = buttons_get_event(&btn)) != BTN_EVENT_NONE) {
        if (ev == BTN_EVENT_PRESS || ev == BTN_EVENT_REPEAT) {
            if (btn == BTN_UP) {
                if (s_pause_selected > 0) s_pause_selected--;
                pause_draw();
            } else if (btn == BTN_DOWN) {
                if (s_pause_selected < 2) s_pause_selected++;
                pause_draw();
            } else if (ev == BTN_EVENT_PRESS && btn == BTN_FIRE) {
                return (pause_action_t)(PAUSE_ACTION_RESUME + s_pause_selected);
            } else if (ev == BTN_EVENT_PRESS && btn == BTN_PAUSE) {
                return PAUSE_ACTION_RESUME;
            }
        }
    }
    return PAUSE_ACTION_NONE;
}

/* ── Game-over screen ────────────────────────────────────────────────────── */

void gameover_show(uint32_t score, bool extreme) {
    st7735_fill_screen(0x0000);

    const char *title = "GAME OVER";
    uint8_t title_w = text_width(title, 2);
    text_draw((uint8_t)((P_W - title_w) / 2), 20, title, 2, 0xFFFF);

    char buf[12];
    uint32_t v = score;
    uint8_t pos = 0;
    char rev[10];
    do {
        rev[pos++] = '0' + (v % 10);
        v /= 10;
    } while (v && pos < 10);
    for (uint8_t i = 0; i < pos; i++) buf[i] = rev[pos - 1 - i];
    buf[pos] = '\0';
    uint8_t score_w = text_width(buf, 2);
    text_draw((uint8_t)((P_W - score_w) / 2), 50, buf, 2, 0xFFE0);

    const char *items[2] = { "RESTART", "MENU" };
    s_go_selected = 0;
    for (uint8_t i = 0; i < 2; i++) {
        uint8_t y = 90 + i * 22;
        text_draw((uint8_t)((P_W - text_width(items[i], 1)) / 2), y, items[i], 1, 0xFFFF);
    }
    st7735_fill_rect((uint8_t)((P_W - text_width(items[0], 1)) / 2) - 8, 92, 3, 3, 0xFFFF);

#if STACK_MEASURE
    draw_stack_debug(140);
#endif
}

go_action_t gameover_handle_input(void) {
    /* This loop blocks the main loop, so keep a long effect advancing. */
    if (buzzer_is_playing()) buzzer_update();

    (void)buttons_poll();   /* updates the event queue */
    uint8_t btn;
    btn_event_t ev;
    while ((ev = buttons_get_event(&btn)) != BTN_EVENT_NONE) {
        if (ev == BTN_EVENT_PRESS || ev == BTN_EVENT_REPEAT) {
            if (btn == BTN_UP) {
                if (s_go_selected > 0) s_go_selected--;
                /* redraw selection */
                const char *items[2] = { "RESTART", "MENU" };
                for (uint8_t i = 0; i < 2; i++) {
                    uint8_t y = 90 + i * 22;
                    text_draw((uint8_t)((P_W - text_width(items[i], 1)) / 2), y, items[i], 1,
                              i == s_go_selected ? 0xFFFF : 0xFFFF);
                }
                st7735_fill_rect((uint8_t)((P_W - text_width(items[0], 1)) / 2) - 8, 92, 3, 3, 0xFFFF);
                if (s_go_selected == 1) {
                    st7735_fill_rect((uint8_t)((P_W - text_width(items[0], 1)) / 2) - 8, 92, 3, 3, 0x0000);
                    st7735_fill_rect((uint8_t)((P_W - text_width(items[1], 1)) / 2) - 8, 114, 3, 3, 0xFFFF);
                }
            } else if (btn == BTN_DOWN) {
                if (s_go_selected < 1) s_go_selected++;
                const char *items[2] = { "RESTART", "MENU" };
                for (uint8_t i = 0; i < 2; i++) {
                    uint8_t y = 90 + i * 22;
                    text_draw((uint8_t)((P_W - text_width(items[i], 1)) / 2), y, items[i], 1,
                              i == s_go_selected ? 0xFFFF : 0xFFFF);
                }
                st7735_fill_rect((uint8_t)((P_W - text_width(items[0], 1)) / 2) - 8, 114, 3, 3, 0xFFFF);
                if (s_go_selected == 0) {
                    st7735_fill_rect((uint8_t)((P_W - text_width(items[0], 1)) / 2) - 8, 92, 3, 3, 0xFFFF);
                    st7735_fill_rect((uint8_t)((P_W - text_width(items[1], 1)) / 2) - 8, 114, 3, 3, 0x0000);
                }
            } else if (ev == BTN_EVENT_PRESS && btn == BTN_FIRE) {
                return (go_action_t)(GO_ACTION_RESTART + s_go_selected);
            } else if (ev == BTN_EVENT_PRESS && btn == BTN_PAUSE) {
                return GO_ACTION_MENU;
            }
        }
    }
    return GO_ACTION_NONE;
}