/**
 * settings.h — Settings screen (BACKLOG U2).
 *
 * Edits the persisted settings (Mode, Maze, Speed, Sound) in the main menu's Series 40
 * style.  Up/Down selects an item, Left/Right changes its value (only the on-screen
 * copy); the "Save" soft key (Fire) writes all of them to EEPROM, "Back" (Pause) leaves
 * without saving.
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

/** Save all current values to EEPROM. */
void settings_save(void);

/** Repaint the complete 128×160 settings screen. */
void settings_draw(void);

/** Same screen coming from another wallpaper screen: repaints only what differs. */
void settings_show(void);

/** Repaint one row (selection moved: the row that lost / gained the highlight), the
 *  title-row index and the scrollbar thumb. */
void settings_draw_row(setting_item_t item);

/** Repaint only the value text of the selected row (a value changed). */
void settings_draw_value(setting_item_t item);

#ifdef __cplusplus
}
#endif

#endif