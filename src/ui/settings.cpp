/**
 * settings.cpp — Settings screen (BACKLOG U2).
 */
#include "ui/settings.h"
#include "ui/menu.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include "drivers/eeprom_store.h"
#include "assets/maze.h"
#include "config.h"
#include <Arduino.h>

#define SET_W           128
#define SET_H           160
#define SET_HEADER_H    20
#define SET_FIRST_Y     26
#define SET_ROW_H       27
#define SET_ROW_GAP     2
#define SET_ROW_X       4
#define SET_ROW_W       120
#define SET_LABEL_W     70
#define SET_VALUE_X     (SET_ROW_X + SET_LABEL_W + 4)
#define SET_FOOTER_Y    140
#define SET_FOOTER_H    20

#define SET_BG          0xFFFF
#define SET_INK         0x0000
#define SET_HEADER_BG   0x0000
#define SET_HEADER_INK  0xFFFF
#define SET_HL_BG       0x001F
#define SET_HL_INK      0xFFFF
#define SET_RULE        0xC618
#define SET_FOOTER_BG   0x0000
#define SET_FOOTER_INK  0xFFFF
#define SET_CONFIRM_BG  0x0000
#define SET_CONFIRM_INK 0xFFFF

static setting_item_t s_selected;

/* Shadow copy of the settings so we can edit before confirming. */
static game_mode_t s_mode;
static int8_t      s_maze;
static uint8_t     s_speed;
static bool        s_sound_on;

static const char *const SET_LABELS[SETTING_COUNT] = {
    "MODE",
    "MAZE",
    "SPEED",
    "SOUND",
};

static uint8_t centered_x(const char *txt, uint8_t scale) {
    uint8_t w = text_width(txt, scale);
    return (w >= SET_W) ? 0 : (uint8_t)((SET_W - w) / 2);
}

static void draw_row_bg(uint8_t index) {
    uint8_t y = (uint8_t)(SET_FIRST_Y + index * (SET_ROW_H + SET_ROW_GAP));
    bool selected = (index == s_selected);
    uint16_t bg = selected ? SET_HL_BG : SET_BG;
    st7735_fill_rect(SET_ROW_X, y, SET_ROW_W, SET_ROW_H, bg);
    if (index + 1 < SETTING_COUNT) {
        st7735_fill_rect(SET_ROW_X, (uint8_t)(y + SET_ROW_H), SET_ROW_W, 1, SET_RULE);
    }
}

static void draw_label(uint8_t index) {
    uint8_t y = (uint8_t)(SET_FIRST_Y + index * (SET_ROW_H + SET_ROW_GAP));
    bool selected = (index == s_selected);
    uint16_t ink = selected ? SET_HL_INK : SET_INK;
    text_draw(SET_ROW_X + 8, (uint8_t)(y + 10), SET_LABELS[index], 1, ink);
}

static const char *mode_text(game_mode_t m) {
    return m == MODE_CLASSIC ? "CLASSIC" : "EXTREME";
}

static const char *speed_text(uint8_t s) {
    static char buf[4];
    buf[0] = '0' + (s % 10);
    buf[1] = '\0';
    return buf;
}

static const char *bool_text(bool b) {
    return b ? "ON" : "OFF";
}

static void draw_value(uint8_t index) {
    uint8_t y = (uint8_t)(SET_FIRST_Y + index * (SET_ROW_H + SET_ROW_GAP));
    bool selected = (index == s_selected);
    uint16_t ink = selected ? SET_HL_INK : SET_INK;

    const char *val = "";
    switch (index) {
        case SETTING_MODE:       val = mode_text(s_mode); break;
        case SETTING_MAZE:       /* uses PROGMEM version in draw_value_p */ break;
        case SETTING_SPEED:      val = speed_text(s_speed); break;
        case SETTING_SOUND:      val = bool_text(s_sound_on); break;
    }
    text_draw(SET_VALUE_X, (uint8_t)(y + 10), val, 1, ink);
}

void settings_init(void) {
    const settings_t *st = eeprom_get_settings();
    s_mode       = st->mode;
    s_maze       = st->maze;
    s_speed      = st->speed;
    s_sound_on   = st->sound_on;
    s_selected   = SETTING_MODE;
}

void settings_move(int8_t delta) {
    int8_t next = (int8_t)s_selected + delta;
    while (next < 0) next += SETTING_COUNT;
    while (next >= SETTING_COUNT) next -= SETTING_COUNT;
    s_selected = (setting_item_t)next;
}

void settings_adjust(int8_t delta) {
    switch (s_selected) {
        case SETTING_MODE:
            s_mode = (s_mode == MODE_CLASSIC) ? MODE_EXTREME : MODE_CLASSIC;
            break;
        case SETTING_MAZE:
            s_maze += delta;
            if (s_maze > MAZE_TWISTED) s_maze = MAZE_NONE;
            if (s_maze < MAZE_NONE)    s_maze = MAZE_TWISTED;
            break;
        case SETTING_SPEED:
            if (delta > 0) {
                if (s_speed < SPEED_INDEX_MAX) s_speed++;
            } else {
                if (s_speed > 0) s_speed--;
            }
            break;
        case SETTING_SOUND:
            s_sound_on = !s_sound_on;
            break;
    }
}

setting_item_t settings_selected(void) {
    return s_selected;
}

static void flash_confirm(void) {
    /* Draw a centred "SAVED" box for ~500 ms, then restore the row. */
    const char *msg = "SAVED";
    uint8_t msg_w = text_width(msg, 2);
    uint8_t box_x = (uint8_t)((SET_W - msg_w - 20) / 2);
    uint8_t box_y = 60;
    uint8_t box_w = msg_w + 20;
    uint8_t box_h = 22;

    st7735_fill_rect(box_x, box_y, box_w, box_h, SET_CONFIRM_BG);
    text_draw(box_x + 10, box_y + 5, msg, 2, SET_CONFIRM_INK);

    uint32_t start = millis();
    while ((int32_t)(millis() - start) < 500) {
        /* busy-wait; short enough not to matter */
    }

    /* Restore the three rows that might have been overlapped. */
    for (uint8_t i = 0; i < SETTING_COUNT; i++) {
        uint8_t y = (uint8_t)(SET_FIRST_Y + i * (SET_ROW_H + SET_ROW_GAP));
        if (y + SET_ROW_H > box_y && y < box_y + box_h) {
            draw_row_bg(i);
            draw_label(i);
            if (i == SETTING_MAZE) {
                bool selected = (i == s_selected);
                uint16_t ink = selected ? SET_HL_INK : SET_INK;
                const char *val = maze_name(s_maze);
                if (val) text_draw_p(SET_VALUE_X, (uint8_t)(y + 10), val, 1, ink);
            } else {
                draw_value(i);
            }
        }
    }
}

void settings_save(void) {
    settings_t st = { s_mode, s_maze, s_speed, s_sound_on, true };
    eeprom_save_settings(&st);
    flash_confirm();
}

void settings_draw(void) {
    st7735_fill_screen(SET_BG);

    st7735_fill_rect(0, 0, SET_W, SET_HEADER_H, SET_HEADER_BG);
    {
        const char *title = "SETTINGS";
        text_draw(centered_x(title, 2), 3, title, 2, SET_HEADER_INK);
    }

    for (uint8_t i = 0; i < SETTING_COUNT; i++) {
        draw_row_bg(i);
        draw_label(i);
        if (i == SETTING_MAZE) {
            /* Maze names are in PROGMEM, use the PROGMEM-aware version. */
            uint8_t y = (uint8_t)(SET_FIRST_Y + i * (SET_ROW_H + SET_ROW_GAP));
            bool selected = (i == s_selected);
            uint16_t ink = selected ? SET_HL_INK : SET_INK;
            const char *val = maze_name(s_maze);
            if (val) text_draw_p(SET_VALUE_X, (uint8_t)(y + 10), val, 1, ink);
        } else {
            draw_value(i);
        }
    }

    st7735_fill_rect(0, SET_FOOTER_Y, SET_W, SET_FOOTER_H, SET_FOOTER_BG);
    text_draw(5, 146, "LR CHANGE", 1, SET_FOOTER_INK);
    {
        const char *back = "PAUSE BACK";
        uint8_t w = text_width(back, 1);
        text_draw((uint8_t)(SET_W - w - 5), 146, back, 1, SET_FOOTER_INK);
    }
}

void settings_draw_value(setting_item_t item) {
    if (item < 0 || item >= SETTING_COUNT) return;
    draw_row_bg((uint8_t)item);
    draw_label((uint8_t)item);
    if (item == SETTING_MAZE) {
        uint8_t y = (uint8_t)(SET_FIRST_Y + (uint8_t)item * (SET_ROW_H + SET_ROW_GAP));
        bool selected = (item == s_selected);
        uint16_t ink = selected ? SET_HL_INK : SET_INK;
        const char *val = maze_name(s_maze);
        if (val) text_draw_p(SET_VALUE_X, (uint8_t)(y + 10), val, 1, ink);
    } else {
        draw_value((uint8_t)item);
    }
}

