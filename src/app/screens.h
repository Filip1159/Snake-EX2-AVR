/**
 * screens.h — The application's screens and the ways to open the ones other screens lead to.
 *
 *   title ─ menu ─┬─ game (new / continue; Back saves it and returns to the menu)
 *                 ├─ settings
 *                 ├─ credits
 *                 └─ score list ─ score view
 */
#ifndef APP_SCREENS_H
#define APP_SCREENS_H

#include <stdint.h>
#include <stdbool.h>
#include "app/screen.h"
#include "ui/menu.h"
#include "ui/highscores.h"

extern const screen_t screen_title;
extern const screen_t screen_menu;
extern const screen_t screen_settings;
extern const screen_t screen_credits;
extern const screen_t screen_score_list;
extern const screen_t screen_score_view;
extern const screen_t screen_game;

/** Title art for 3 s (or until a key), then the menu. */
void screen_title_open(void);

/** Title after a game: the final score (and trophies for a new best); waits for a key. */
void screen_title_open_score(uint32_t score, bool new_best);

/** The main menu from scratch (first row selected; "Continue" if a game is saved). */
void screen_menu_open(void);

/** The main menu again with `item` highlighted (coming back from a sub-list). */
void screen_menu_return(menu_item_t item);

/** Up/Down/Left/Right move the selection of the open menu list; true if the event was one. */
bool screen_menu_navigate(uint8_t btn, btn_event_t ev);

/** Classic/Extreme list with row `selected` highlighted; the top-3 view of a mode. */
void screen_score_list_open(uint8_t selected);
void screen_score_view_open(hs_mode_t mode);

/** Start a game with the saved settings, or restore the saved one (frozen until Fire);
 *  false (nothing changed) if the save cannot be restored. */
void screen_game_new(void);
bool screen_game_continue(void);

/** Show "Silent" in the status bar while the stored sound setting is off. */
void screen_sync_silent(void);

#endif
