/**
 * menu.cpp — Series 40 list-style main menu.
 *
 * The original MIDlet uses the phone's native List UI rather than drawing a
 * menu on the game canvas.  This renderer reconstructs the 128×160 S40 list:
 * a small status bar, the list title, four full-width rows with dotted
 * leaders, and Select/Exit soft keys.  The selected row is a light-blue band
 * with dark text, matching the reference screenshot.
 */
#include "ui/menu.h"
#include "render/text.h"
#include "drivers/st7735.h"

#define MENU_W          128

#define MENU_STATUS_H   14
#define MENU_TITLE_Y    14
#define MENU_TITLE_H    16
#define MENU_FIRST_Y    30
#define MENU_ROW_H      20
#define MENU_FOOTER_Y   140
#define MENU_FOOTER_H   20

#define MENU_BG         0xFFFF
#define MENU_INK        0x0000
#define MENU_SOFT_INK   0x0010
#define MENU_DOT        0x8410
#define MENU_HL_BG      0x6CFD
#define MENU_HL_INK     0x0000
#define MENU_RULE       0x6CFD
#define MENU_STATUS_INK 0x0010
#define MENU_BATT_FILL  0x07E0

/* The port has no RTC; the status bar is decorative S40 chrome. */
#define MENU_STATUS_TIME "22:22"
#define MENU_TITLE_TEXT  "Snake EX2"
#define MENU_SOFT_LEFT   "Select"
#define MENU_SOFT_RIGHT  "Exit"

static const char *const MENU_LABELS[MENU_ITEM_COUNT] = {
    "New game",
    "High scores",
    "Settings",
    "Instructions",
};

static uint8_t s_selected;

static void dotted_leader(uint8_t x0, uint8_t x1, uint8_t y, uint16_t color) {
    if (x0 >= x1) return;
    for (uint8_t x = x0; x < x1; x = (uint8_t)(x + 4)) {
        st7735_fill_rect(x, y, 1, 1, color);
    }
}

static void draw_status_bar(void) {
    st7735_fill_rect(0, 0, MENU_W, MENU_STATUS_H, MENU_BG);

    /* Signal-strength bars at the left. */
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t h = (uint8_t)(2 + i * 2);
        st7735_fill_rect((uint8_t)(3 + i * 3), (uint8_t)(11 - h), 2, h,
                         MENU_STATUS_INK);
    }
    dotted_leader(18, 38, 7, MENU_DOT);

    /* Battery outline, fill and terminal nub. */
    st7735_fill_rect(42, 3, 11, 7, MENU_STATUS_INK);
    st7735_fill_rect(43, 4, 9, 5, MENU_BG);
    st7735_fill_rect(44, 5, 6, 3, MENU_BATT_FILL);
    st7735_fill_rect(53, 5, 2, 3, MENU_STATUS_INK);
    dotted_leader(57, 94, 7, MENU_DOT);

    {
        uint8_t w = text_width(MENU_STATUS_TIME, 1);
        text_draw((uint8_t)(MENU_W - w - 2), 3, MENU_STATUS_TIME, 1,
                  MENU_STATUS_INK);
    }
}

static void draw_title_bar(void) {
    st7735_fill_rect(0, MENU_TITLE_Y, MENU_W, MENU_TITLE_H, MENU_BG);
    text_draw(4, 18, MENU_TITLE_TEXT, 1, MENU_INK);

    {
        uint8_t end = (uint8_t)(4 + text_width(MENU_TITLE_TEXT, 1) + 3);
        dotted_leader(end, 118, 21, MENU_DOT);
    }
    text_draw(121, 18, "1", 1, MENU_INK);
}

static void draw_row(uint8_t index) {
    uint8_t y = (uint8_t)(MENU_FIRST_Y + index * MENU_ROW_H);
    bool selected = (index == s_selected);
    uint16_t bg = selected ? MENU_HL_BG : MENU_BG;
    uint16_t ink = selected ? MENU_HL_INK : MENU_INK;
    const char *label = MENU_LABELS[index];

    st7735_fill_rect(0, y, MENU_W, MENU_ROW_H, bg);
    text_draw(4, (uint8_t)(y + 7), label, 1, ink);

    if (!selected) {
        uint8_t end = (uint8_t)(4 + text_width(label, 1) + 4);
        dotted_leader(end, 124, (uint8_t)(y + 10), MENU_DOT);
    }
}

static void draw_softkeys(void) {
    st7735_fill_rect(0, MENU_FOOTER_Y, MENU_W, MENU_FOOTER_H, MENU_BG);
    st7735_fill_rect(0, MENU_FOOTER_Y, MENU_W, 1, MENU_RULE);

    text_draw(4, 148, MENU_SOFT_LEFT, 1, MENU_SOFT_INK);
    {
        uint8_t lw = text_width(MENU_SOFT_LEFT, 1);
        uint8_t rw = text_width(MENU_SOFT_RIGHT, 1);
        uint8_t rx = (uint8_t)(MENU_W - rw - 4);
        text_draw(rx, 148, MENU_SOFT_RIGHT, 1, MENU_SOFT_INK);
        dotted_leader((uint8_t)(lw + 9), (uint8_t)(rx - 5), 151, MENU_DOT);
    }
}

void menu_init(void) {
    s_selected = MENU_ITEM_NEW_GAME;
}

void menu_move(int8_t delta) {
    int8_t next = (int8_t)s_selected + delta;
    while (next < 0) next += MENU_ITEM_COUNT;
    while (next >= MENU_ITEM_COUNT) next -= MENU_ITEM_COUNT;
    s_selected = (uint8_t)next;
}

menu_item_t menu_selected(void) {
    return (menu_item_t)s_selected;
}

const char *menu_item_label(menu_item_t item) {
    if (item < 0 || item >= MENU_ITEM_COUNT) return MENU_LABELS[0];
    return MENU_LABELS[item];
}

void menu_draw_item(menu_item_t item) {
    if (item < 0 || item >= MENU_ITEM_COUNT) return;
    draw_row((uint8_t)item);
}

void menu_draw(void) {
    st7735_fill_screen(MENU_BG);

    draw_status_bar();
    draw_title_bar();

    for (uint8_t i = 0; i < MENU_ITEM_COUNT; i++) {
        draw_row(i);
    }

    draw_softkeys();
}
