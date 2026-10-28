/**
 * menu.h — Title-screen main menu state and renderer (BACKLOG U1).
 *
 * The menu is a reconstruction of the original Series 40 list UI: a small
 * status bar, a title row, full-width rows
 * and Select/Exit soft keys. Up/left moves to the previous item,
 * down/right to the next, Fire selects, Pause goes back.
 */
#ifndef UI_MENU_H
#define UI_MENU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MENU_ITEM_NEW_GAME = 0,
    MENU_ITEM_HIGH_SCORES,
    MENU_ITEM_SETTINGS,
    MENU_ITEM_INSTRUCTIONS,
    MENU_ITEM_COUNT,
    MENU_ITEM_CONTINUE          /* extra first row, only while a saved game exists */
} menu_item_t;

/** Which list the menu shows: the main menu or the Classic/Extreme choice for the
 *  high-score view. */
typedef enum {
    MENU_LIST_MAIN = 0,
    MENU_LIST_SCORES,
} menu_list_t;

#define MENU_SCORES_COUNT 2    /* Classic, Extreme */

/** Make `list` the active list with its first row selected (draw it with menu_draw()). */
void menu_open(menu_list_t list);

/** Show a "Continue" row at the top of the main menu from the next menu_open()/menu_init(). */
void menu_set_continue(bool available);

/** Item shown at row `index` of the main menu / row index of an item (honours "Continue"). */
menu_item_t menu_item_at(uint8_t index);
uint8_t menu_index_of(menu_item_t item);

/** Open the main menu with its first row (Continue, else New game) selected. */
void menu_init(void);

/** Move the selection by delta, wrapping at both ends. */
void menu_move(int8_t delta);

/** Currently highlighted item (main-menu enum; for the score list use the index). */
menu_item_t menu_selected(void);

/** Row index of the highlighted item in the active list. */
uint8_t menu_selected_index(void);

/** Text label for an item (never NULL for a valid item). */
const char *menu_item_label(menu_item_t item);

/** Repaint the complete 128×160 main-menu screen. */
void menu_draw(void);

/** Show the open list coming from another wallpaper screen (settings, credits, the other
 *  list): the status bar, wallpaper and unchanged labels stay; only what differs is painted.
 *  Same as menu_draw() when the last screen was not a wallpaper one. */
void menu_show(void);

/**
 * Repaint one menu row (including its highlight/arrow) without clearing the
 * screen. Use this for the old and new selection after menu_move() so normal
 * navigation does not trigger a full-screen refresh.
 */
void menu_draw_item(menu_item_t item);

#ifdef __cplusplus
}
#endif

#endif
