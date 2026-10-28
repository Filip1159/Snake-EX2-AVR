/**
 * chrome.h — Shared Series 40 screen chrome: wallpaper, status bar, title row, soft key.
 *
 * Used by the main menu, its sub-lists and the high-score view so they all look alike.
 */
#ifndef UI_CHROME_H
#define UI_CHROME_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHROME_W          128
#define CHROME_TITLE_Y    14          /* top of the small font frame (baseline at +9) */
#define CHROME_SOFTKEY_Y  147         /* small right soft key: baseline at 156 */
#define CHROME_SOFTKEY_BIG_Y 144      /* bold centre soft key: baseline at 155 */
#define CHROME_TITLE_H    16
#define CHROME_INK        0x0000
#define CHROME_SOFT_INK   0x0010
#define CHROME_DOT        0x8410
#define CHROME_ROW_H      20
#define CHROME_LIST_Y     30          /* top of the first list row and of the scrollbar */
#define CHROME_SCROLL_X   121
#define CHROME_BAND_W     120
#define CHROME_HL_INK     0xFFFF

/** Background used by the chrome: the S40 wallpaper (default; menus) or a flat colour
 *  (e.g. the game board colour for the high-score view). Set before drawing a screen. */
void chrome_use_wallpaper(void);
void chrome_use_flat(uint16_t color);

/** Draw the full-screen background. */
void chrome_draw_wallpaper(void);

/** Restore background pixels of the rectangle (x,y,w,h). */
void chrome_restore(uint8_t x, uint8_t w, uint8_t y, uint8_t h);

/** Dotted leader line (1 px dots every 4 px) from x0 to x1 at row y. */
void chrome_dotted_leader(uint8_t x0, uint8_t x1, uint8_t y, uint16_t color);

/** Status bar copied from the S40 screenshot: antenna, signal, battery and a clock
 *  (decorative: the port has no RTC); with chrome_set_silent(true) also the red muted-note
 *  icon and "Silent". Transparent, so it works on any background. */
void chrome_set_silent(bool silent);
void chrome_draw_status_bar(void);

/** Title row: `title` at the left, a dotted leader, and optional `right` text right-aligned
 *  at x=126 (the menu's item index, the high-score mode). Repaints the row's wallpaper. */
void chrome_draw_title(const char *title, const char *right);

/** Selection band (blue-to-navy gradient, CHROME_BAND_W x CHROME_ROW_H) with its top at y. */
void chrome_draw_band(uint8_t y);

/** Same band, only columns [x, x+w) (to repaint part of a selected row). */
void chrome_draw_band_part(uint8_t y, uint8_t x, uint8_t w);

/** Scrollbar for a list of `count` rows: draw it whole (track + thumb at `selected`), or
 *  animate the thumb from where it is to `to`. */
void chrome_scrollbar_draw(uint8_t count, uint8_t selected);
void chrome_scrollbar_move(uint8_t count, uint8_t to);

/** Soft keys: `left` bold and centred (the main action), `right` bottom-right, a dotted
 *  leader between them. */
void chrome_draw_softkeys(const char *left, const char *right);

/** Right soft key label in the bottom-right corner. Returns its left x. */
uint8_t chrome_draw_softkey_right(const char *label);

#ifdef __cplusplus
}
#endif

#endif
