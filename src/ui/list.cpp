/**
 * list.cpp — Series 40 list widget (see list.h).
 */
#include "ui/list.h"
#include "ui/chrome.h"
#include "render/text.h"

#define LIST_LABEL_X  4
#define LIST_TEXT_DY  3           /* label frame offset inside a row */

/* What is on screen, so a following screen erases exactly that (the owner may already have
 * changed the list it passed in). */
static struct {
    const char *const *labels;
    uint8_t count;
    uint8_t selected;
    uint8_t value_x[LIST_MAX_ROWS];     /* left edge of each drawn value, 0 = none */
} s_on_screen;

static uint8_t row_y(uint8_t row) {
    return (uint8_t)(CHROME_LIST_Y + row * CHROME_ROW_H);
}

static void erase_content(void) {
    for (uint8_t i = 0; i < s_on_screen.count; i++) {
        uint8_t y = row_y(i);
        if (i == s_on_screen.selected) {
            chrome_restore(0, CHROME_BAND_W, y, CHROME_ROW_H);
            continue;
        }
        chrome_restore(LIST_LABEL_X, (uint8_t)(text_width_big(s_on_screen.labels[i]) + 1),
                       (uint8_t)(y + LIST_TEXT_DY), CHROME_BIG_H);
        if (s_on_screen.value_x[i])
            chrome_restore(s_on_screen.value_x[i], (uint8_t)(LIST_VALUE_END + 1 - s_on_screen.value_x[i]),
                           (uint8_t)(y + LIST_TEXT_DY), CHROME_BIG_H);
    }
}

static void draw_title(const list_t *list, uint8_t selected) {
    char idx[2] = { (char)('1' + selected), '\0' };      /* 1-based index over the scrollbar */
    chrome_draw_title(list->title, idx);
}

/* `repaint`: the row may hold the band or an older value, so put the wallpaper back first. */
static void draw_row(const list_t *list, uint8_t row, uint8_t selected, bool repaint) {
    uint8_t y = row_y(row);
    uint16_t ink = row == selected ? CHROME_HL_INK : CHROME_INK;

    if (row == selected) chrome_draw_band(y);
    else if (repaint)    chrome_restore(0, CHROME_BAND_W, y, CHROME_ROW_H);
    text_draw_big(LIST_LABEL_X, (uint8_t)(y + LIST_TEXT_DY), list->labels[row], ink);

    if (list->value) {
        char buf[9];
        const char *value = list->value(row, buf);
        uint8_t vx = (uint8_t)(LIST_VALUE_END - text_width_big(value));
        text_draw_big(vx, (uint8_t)(y + LIST_TEXT_DY), value, ink);
        s_on_screen.value_x[row] = vx;
    }
}

void list_show(const list_t *list, uint8_t selected) {
    chrome_begin(erase_content);
    draw_title(list, selected);
    for (uint8_t i = 0; i < list->count; i++) {
        s_on_screen.value_x[i] = 0;
        draw_row(list, i, selected, false);
    }
    s_on_screen.labels = list->labels;
    s_on_screen.count = list->count;
    s_on_screen.selected = selected;
    chrome_scrollbar_draw(list->count, selected);
    chrome_draw_softkeys(list->left_key, list->right_key);
}

void list_draw(const list_t *list, uint8_t selected) {
    chrome_invalidate();
    list_show(list, selected);
}

void list_draw_row(const list_t *list, uint8_t row, uint8_t selected) {
    if (row >= list->count) return;
    draw_row(list, row, selected, true);
    if (row == selected) {
        s_on_screen.selected = selected;
        draw_title(list, selected);                      /* shows the item index */
        chrome_scrollbar_move(list->count, selected);
    }
}

void list_draw_value(const list_t *list, uint8_t row, uint8_t selected) {
    if (row >= list->count || !list->value) return;
    if (row != selected) { draw_row(list, row, selected, true); return; }

    /* Only the value changes: repaint the band behind the wider of the old and new text
     * and draw the new text. The label and the rest of the row stay untouched. */
    char buf[9];
    const char *value = list->value(row, buf);
    uint8_t vx = (uint8_t)(LIST_VALUE_END - text_width_big(value));
    uint8_t x0 = vx < s_on_screen.value_x[row] ? vx : s_on_screen.value_x[row];
    uint8_t y = row_y(row);
    chrome_draw_band_part(y, x0, (uint8_t)(LIST_VALUE_END - x0));
    text_draw_big(vx, (uint8_t)(y + LIST_TEXT_DY), value, CHROME_HL_INK);
    s_on_screen.value_x[row] = vx;
}
