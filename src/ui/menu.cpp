/**
 * menu.cpp — Series 40 list-style menus (main menu and the high-score mode list).
 *
 * The original MIDlet uses the phone's native List UI rather than drawing a
 * menu on the game canvas.  This renderer reconstructs the 128×160 S40 list:
 * a small status bar, the list title, full-width rows, a
 * scrollbar and Select/Exit(Back) soft keys over the reconstructed S40 wallpaper
 * (IMG_WALLPAPER, see chrome.cpp).  The selected row is a blue-to-navy gradient
 * band with white text.
 */
#include "ui/menu.h"
#include "ui/list.h"

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

static uint8_t s_selected;
static bool s_continue;          /* main menu has the "Continue" row */
static bool s_main_open = true;  /* the open list is the main menu */

static list_t s_list = { "Snake EX2", MAIN_LABELS, MENU_ITEM_COUNT, 0, MENU_SOFT_LEFT, "Exit" };

void menu_open(menu_list_t list) {
    if (list == MENU_LIST_SCORES) {
        s_main_open = false;
        s_list.labels = SCORE_LABELS;
        s_list.count = MENU_SCORES_COUNT;
        s_list.title = "High scores";
        s_list.right_key = "Back";
    } else {
        s_main_open = true;
        s_list.labels = s_continue ? CONTINUE_LABELS : MAIN_LABELS;
        s_list.count = s_continue ? MENU_ITEM_COUNT + 1 : MENU_ITEM_COUNT;
        s_list.title = "Snake EX2";
        s_list.right_key = "Exit";
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
    while (next < 0) next += s_list.count;
    while (next >= s_list.count) next -= s_list.count;
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

void menu_draw_item(menu_item_t item) { list_draw_row(&s_list, (uint8_t)item, s_selected); }

void menu_show(void) { list_show(&s_list, s_selected); }

void menu_draw(void) { list_draw(&s_list, s_selected); }
