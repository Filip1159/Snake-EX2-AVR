/**
 * menu.cpp — Series 40 list-style menus (main menu and the high-score mode list).
 *
 * The original MIDlet uses the phone's native List UI rather than drawing a
 * menu on the game canvas.  This renderer reconstructs the 128×160 S40 list:
 * a small status bar, the list title, full-width rows with dotted leaders, a
 * scrollbar and Select/Exit(Back) soft keys over the reconstructed S40 wallpaper
 * (IMG_WALLPAPER, see chrome.cpp).  The selected row is a blue-to-navy gradient
 * band with white text.
 */
#include "ui/menu.h"
#include "ui/chrome.h"
#include "render/text.h"
#include "drivers/st7735.h"

#define MENU_FIRST_Y    CHROME_LIST_Y
#define MENU_ROW_H      CHROME_ROW_H

#define MENU_LEADER_END 118

#define MENU_SOFT_LEFT  "Select"

static const char *const MAIN_LABELS[MENU_ITEM_COUNT] = {
    "New game",
    "High scores",
    "Settings",
    "Instructions",
};
static const char *const CONTINUE_LABELS[MENU_ITEM_COUNT + 1] = {
    "Continue",
    "New game",
    "High scores",
    "Settings",
    "Instructions",
};
static const char *const SCORE_LABELS[MENU_SCORES_COUNT] = {
    "Classic",
    "Extreme",
};

static const char *const *s_labels = MAIN_LABELS;   /* the open list */
static uint8_t s_count = MENU_ITEM_COUNT;
static const char *s_title = "Snake EX2";
static const char *s_right = "Exit";
static uint8_t s_selected;
static bool s_continue;          /* main menu has the "Continue" row */
static bool s_main_open = true;  /* the open list is the main menu */

static void draw_title_bar(void) {
    /* Index of the highlighted item, 1-based, right-aligned over the scrollbar (the 5 px
     * digit sits exactly above the thumb body). */
    char idx[2] = { (char)('1' + s_selected), '\0' };
    chrome_draw_title(s_title, idx);
}

static void draw_row(uint8_t index) {
    uint8_t y = (uint8_t)(MENU_FIRST_Y + index * MENU_ROW_H);
    bool selected = (index == s_selected);
    uint16_t ink = selected ? CHROME_HL_INK : CHROME_INK;
    const char *label = s_labels[index];

    if (selected) chrome_draw_band(y);
    else          chrome_restore(0, CHROME_SCROLL_X, y, MENU_ROW_H);   /* the bar is never touched */
    text_draw_big(4, (uint8_t)(y + 3), label, ink);

    if (!selected) {
        uint8_t end = (uint8_t)(4 + text_width_big(label) + 4);
        chrome_dotted_leader(end, MENU_LEADER_END, (uint8_t)(y + 14), CHROME_DOT);   /* on the baseline */
    }
}

void menu_open(menu_list_t list) {
    if (list == MENU_LIST_SCORES) {
        s_main_open = false;
        s_labels = SCORE_LABELS;
        s_count = MENU_SCORES_COUNT;
        s_title = "High scores";
        s_right = "Back";
    } else {
        s_main_open = true;
        s_labels = s_continue ? CONTINUE_LABELS : MAIN_LABELS;
        s_count = s_continue ? MENU_ITEM_COUNT + 1 : MENU_ITEM_COUNT;
        s_title = "Snake EX2";
        s_right = "Exit";
    }
    s_selected = 0;
}

void menu_set_continue(bool available) {
    s_continue = available;
}

menu_item_t menu_item_at(uint8_t index) {
    if (!s_continue || !s_main_open) return (menu_item_t)index;
    return index == 0 ? MENU_ITEM_CONTINUE : (menu_item_t)(index - 1);
}

uint8_t menu_index_of(menu_item_t item) {
    if (!s_continue || !s_main_open) return (uint8_t)item;
    return item == MENU_ITEM_CONTINUE ? 0 : (uint8_t)(item + 1);
}

void menu_init(void) {
    menu_open(MENU_LIST_MAIN);
}

void menu_move(int8_t delta) {
    int8_t next = (int8_t)s_selected + delta;
    while (next < 0) next += s_count;
    while (next >= s_count) next -= s_count;
    s_selected = (uint8_t)next;
}

menu_item_t menu_selected(void) {
    return (menu_item_t)s_selected;
}

uint8_t menu_selected_index(void) {
    return s_selected;
}

const char *menu_item_label(menu_item_t item) {
    if (item == MENU_ITEM_CONTINUE) return CONTINUE_LABELS[0];
    if (item < 0 || item >= MENU_ITEM_COUNT) return MAIN_LABELS[0];
    return MAIN_LABELS[item];
}

void menu_draw_item(menu_item_t item) {
    if (item < 0 || item >= s_count) return;
    draw_row((uint8_t)item);
    if (item == s_selected) {
        draw_title_bar();                                   /* shows the item index */
        chrome_scrollbar_move(s_count, s_selected);
    }
}

void menu_draw(void) {
    chrome_use_wallpaper();
    chrome_draw_wallpaper();

    chrome_draw_status_bar();
    draw_title_bar();

    for (uint8_t i = 0; i < s_count; i++) {
        draw_row(i);
    }

    chrome_scrollbar_draw(s_count, s_selected);
    chrome_draw_softkeys(MENU_SOFT_LEFT, s_right);
}
