/**
 * menu.h — Title-screen main menu state and renderer (BACKLOG U1).
 *
 * The menu is a reconstruction of the original Series 40 list UI: a small
 * status bar, a “Snake EX2” title row, four full-width rows with dotted
 * leaders, and Select/Exit soft keys. Up/left moves to the previous item,
 * down/right to the next, Fire selects, Pause goes back.
 */
#ifndef UI_MENU_H
#define UI_MENU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MENU_ITEM_NEW_GAME = 0,
    MENU_ITEM_HIGH_SCORES,
    MENU_ITEM_SETTINGS,
    MENU_ITEM_INSTRUCTIONS,
    MENU_ITEM_COUNT
} menu_item_t;

/** Reset the selection to New game. */
void menu_init(void);

/** Move the selection by delta, wrapping at both ends. */
void menu_move(int8_t delta);

/** Currently highlighted item. */
menu_item_t menu_selected(void);

/** Text label for an item (never NULL for a valid item). */
const char *menu_item_label(menu_item_t item);

/** Repaint the complete 128×160 main-menu screen. */
void menu_draw(void);

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
