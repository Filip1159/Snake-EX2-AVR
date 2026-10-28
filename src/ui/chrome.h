/**
 * chrome.h — Shared Series 40 screen chrome: wallpaper, status bar, title row, soft key.
 *
 * Used by the main menu, its sub-lists and the high-score view so they all look alike.
 */
#ifndef UI_CHROME_H
#define UI_CHROME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHROME_W          128
#define CHROME_TITLE_Y    14
#define CHROME_TITLE_H    16
#define CHROME_INK        0x0000
#define CHROME_SOFT_INK   0x0010
#define CHROME_DOT        0x8410

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

/** Signal bars, battery and clock (decorative: the port has no RTC). */
void chrome_draw_status_bar(void);

/** Title row: `title` at the left, a dotted leader, and optional `right` text right-aligned
 *  at x=126 (the menu's item index, the high-score mode). Repaints the row's wallpaper. */
void chrome_draw_title(const char *title, const char *right);

/** Right soft key label in the bottom-right corner. Returns its left x. */
uint8_t chrome_draw_softkey_right(const char *label);

#ifdef __cplusplus
}
#endif

#endif
