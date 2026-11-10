/**
 * list.h — Series 40 list widget shared by the main menu, the Classic/Extreme list and the
 * settings screen: title row with the selection index, one 20 px row per entry (bold label,
 * optional right-aligned value, blue band on the selected one), scrollbar and soft keys.
 *
 * The widget keeps no selection of its own: the owner passes it in, so the owner stays the
 * single place that moves it.  Every call repaints the least it can (see chrome.h).
 */
#ifndef UI_LIST_H
#define UI_LIST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIST_MAX_ROWS   5
#define LIST_VALUE_END  116       /* values are right-aligned to this x */

typedef struct {
    const char *title;
    const char *const *labels;                          /* `count` bold row labels */
    uint8_t count;                                      /* <= LIST_MAX_ROWS */
    const char *(*value)(uint8_t row, char *buf);       /* row value text (buf: 9 bytes), or NULL */
    const char *left_key;                               /* centre soft key (may be NULL) */
    const char *right_key;
} list_t;

/** Show the list with row `selected` highlighted. Coming from another wallpaper screen only
 *  what differs is painted; otherwise the whole screen (wallpaper and status bar included). */
void list_show(const list_t *list, uint8_t selected);

/** Same, after something else (title screen, game, flat view) used the display. */
void list_draw(const list_t *list, uint8_t selected);

/** Repaint one row after the selection moved (the one that lost / gained the highlight).
 *  The row `selected` also updates the title index and the scrollbar thumb. */
void list_draw_row(const list_t *list, uint8_t row, uint8_t selected);

/** Only the value text of the selected row changed. */
void list_draw_value(const list_t *list, uint8_t row, uint8_t selected);

#ifdef __cplusplus
}
#endif

#endif
