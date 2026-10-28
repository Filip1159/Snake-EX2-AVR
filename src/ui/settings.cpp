/**
 * settings.cpp — Settings screen (BACKLOG U2), laid out like the main menu.
 */
#include "ui/settings.h"
#include "ui/chrome.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include "drivers/eeprom_store.h"
#include "assets/maze.h"
#include "config.h"
#include <avr/pgmspace.h>

#define SET_FIRST_Y     30
#define SET_LABEL_X     4
#define SET_VALUE_END   116          /* values are right-aligned to this x */
#define SET_LEADER_END  (SET_VALUE_END + 2)

static setting_item_t s_selected;
static uint8_t s_value_x[SETTING_COUNT];   /* left edge of each drawn value */

/* Shadow copy of the settings so we can edit before saving. */
static game_mode_t s_mode;
static int8_t      s_maze;
static uint8_t     s_speed;
static bool        s_sound_on;

static const char *const SET_LABELS[SETTING_COUNT] = {
    "Mode",
    "Maze",
    "Speed",
    "Sound",
};

/** Text of the item's current value; maze names live in PROGMEM, so copy to `buf`. */
static const char *value_text(uint8_t item, char *buf) {
    switch (item) {
        case SETTING_MODE:  return s_mode == MODE_CLASSIC ? "Classic" : "Extreme";
        case SETTING_MAZE: {
            const char *p = maze_name(s_maze);
            uint8_t i = 0;
            if (p) while (i < 8 && (buf[i] = (char)pgm_read_byte(p + i)) != '\0') i++;
            buf[i] = '\0';
            return buf;
        }
        case SETTING_SPEED:
            buf[0] = (char)('0' + (s_speed % 10));
            buf[1] = '\0';
            return buf;
        default:            return s_sound_on ? "On" : "Off";
    }
}

static uint8_t row_y(uint8_t index) {
    return (uint8_t)(SET_FIRST_Y + index * CHROME_ROW_H);
}

static uint8_t value_x(const char *value) {
    return (uint8_t)(SET_VALUE_END - text_width_big(value));
}

static void draw_title(void) {
    char idx[2] = { (char)('1' + s_selected), '\0' };
    chrome_draw_title("Settings", idx);
}

static void draw_row(uint8_t index) {
    uint8_t y = row_y(index);
    bool selected = (index == s_selected);
    uint16_t ink = selected ? CHROME_HL_INK : CHROME_INK;
    char buf[9];
    const char *value = value_text(index, buf);
    const char *label = SET_LABELS[index];

    if (selected) chrome_draw_band(y);
    else          chrome_restore(0, CHROME_SCROLL_X, y, CHROME_ROW_H);   /* the bar is never touched */
    text_draw_big(SET_LABEL_X, (uint8_t)(y + 4), label, ink);

    uint8_t vx = value_x(value);
    text_draw_big(vx, (uint8_t)(y + 4), value, ink);
    s_value_x[index] = vx;

    if (!selected) {
        uint8_t from = (uint8_t)(SET_LABEL_X + text_width_big(label) + 4);
        chrome_dotted_leader(from, (uint8_t)(vx - 4), (uint8_t)(y + 13), CHROME_DOT);
    }
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
            if (delta > 0) s_speed = (s_speed < SPEED_INDEX_MAX) ? s_speed + 1 : 0;
            else           s_speed = (s_speed > 0) ? s_speed - 1 : SPEED_INDEX_MAX;
            break;
        case SETTING_SOUND:
            s_sound_on = !s_sound_on;
            break;
        default:
            break;
    }
}

setting_item_t settings_selected(void) {
    return s_selected;
}

void settings_save(void) {
    /* Keep the stored backlight flag: it has no row on this screen. */
    settings_t st = { s_mode, s_maze, s_speed, s_sound_on, eeprom_get_settings()->backlight };
    eeprom_save_settings(&st);
}

void settings_draw(void) {
    chrome_use_wallpaper();
    chrome_draw_wallpaper();
    chrome_draw_status_bar();
    draw_title();
    for (uint8_t i = 0; i < SETTING_COUNT; i++) draw_row(i);
    chrome_scrollbar_draw(SETTING_COUNT, s_selected);
    chrome_draw_softkeys("Save", "Back");
}

void settings_draw_row(setting_item_t item) {
    if (item < 0 || item >= SETTING_COUNT) return;
    draw_row((uint8_t)item);
    if (item == s_selected) {
        draw_title();                                       /* shows the item index */
        chrome_scrollbar_move(SETTING_COUNT, s_selected);
    }
}

void settings_draw_value(setting_item_t item) {
    if (item < 0 || item >= SETTING_COUNT) return;
    if (item != s_selected) { draw_row((uint8_t)item); return; }

    /* Only the value changes: repaint the band behind the wider of the old and new text
     * and draw the new text. The label and the rest of the row stay untouched. */
    uint8_t y = row_y((uint8_t)item);
    char buf[9];
    const char *value = value_text((uint8_t)item, buf);
    uint8_t vx = value_x(value);
    uint8_t x0 = vx < s_value_x[item] ? vx : s_value_x[item];
    chrome_draw_band_part(y, x0, (uint8_t)(SET_VALUE_END - x0));
    text_draw_big(vx, (uint8_t)(y + 4), value, CHROME_HL_INK);
    s_value_x[item] = vx;
}
