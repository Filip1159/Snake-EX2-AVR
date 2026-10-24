/**
 * settings.h — Settings screen (BACKLOG U2).
 *
 * Edits the five persisted settings: Mode, Maze, Speed, Sound, Backlight.
 * Up/Down selects item; Left/Right changes value; Fire saves and shows
 * brief confirmation; Pause returns to the main menu.
 */
#ifndef UI_SETTINGS_H
#define UI_SETTINGS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SETTING_MODE = 0,
    SETTING_MAZE,
    SETTING_SPEED,
    SETTING_SOUND,
    SETTING_COUNT
} setting_item_t;

/** Initialise the settings screen with current EEPROM values. */
void settings_init(void);

/** Move selection by delta (wraps). */
void settings_move(int8_t delta);

/** Change the currently selected setting by delta (+1 / -1). */
void settings_adjust(int8_t delta);

/** Currently highlighted item. */
setting_item_t settings_selected(void);

/** Save current values to EEPROM and show a momentary confirmation. */
void settings_save(void);

/** Repaint the complete 128×160 settings screen. */
void settings_draw(void);

/** Repaint only the value column of one row (for incremental updates). */
void settings_draw_value(setting_item_t item);

#ifdef __cplusplus
}
#endif

#endif