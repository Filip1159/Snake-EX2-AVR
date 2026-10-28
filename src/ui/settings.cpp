/**
 * settings.cpp — Settings screen (BACKLOG U2), laid out like the main menu.
 */
#include "ui/settings.h"
#include "ui/list.h"
#include "drivers/eeprom_store.h"
#include "assets/maze.h"
#include "config.h"
#include <avr/pgmspace.h>


static setting_item_t s_selected;

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

static const list_t LIST = {
    "Settings", SET_LABELS, SETTING_COUNT, value_text, "Save", "Back",
};

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

void settings_show(void) { list_show(&LIST, s_selected); }

void settings_draw(void) { list_draw(&LIST, s_selected); }

void settings_draw_row(setting_item_t item) { list_draw_row(&LIST, (uint8_t)item, s_selected); }

void settings_draw_value(setting_item_t item) { list_draw_value(&LIST, (uint8_t)item, s_selected); }
