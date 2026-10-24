/**
 * menu.cpp — Title-screen main menu (BACKLOG U1).
 */
#include "ui/menu.h"
#include "render/text.h"
#include "drivers/st7735.h"

#define MENU_W          128
#define MENU_H          160
#define MENU_HEADER_H   20
#define MENU_FIRST_Y    26
#define MENU_ROW_H      27
#define MENU_ROW_GAP    2
#define MENU_ROW_X      4
#define MENU_ROW_W      120
#define MENU_FOOTER_Y   140
#define MENU_FOOTER_H   20

#define MENU_BG         0xFFFF
#define MENU_INK        0x0000
#define MENU_HEADER_BG  0x0000
#define MENU_HEADER_INK 0xFFFF
#define MENU_HL_BG      0x001F
#define MENU_HL_INK     0xFFFF
#define MENU_RULE       0xC618
#define MENU_FOOTER_BG  0x0000
#define MENU_FOOTER_INK 0xFFFF

static const char *const MENU_LABELS[MENU_ITEM_COUNT] = {
    "NEW GAME",
    "HIGH SCORES",
    "SETTINGS",
    "INSTRUCTIONS",
};

static uint8_t s_selected;

static uint8_t centered_x(const char *txt, uint8_t scale) {
    uint8_t w = text_width(txt, scale);
    return (w >= MENU_W) ? 0 : (uint8_t)((MENU_W - w) / 2);
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

static void draw_selected_arrow(uint8_t y) {
    /* Small right-pointing triangle in the selected row. */
    st7735_fill_rect(MENU_ROW_X + 4, y + 11, 1, 5, MENU_HL_INK);
    st7735_fill_rect(MENU_ROW_X + 5, y + 12, 1, 3, MENU_HL_INK);
    st7735_fill_rect(MENU_ROW_X + 6, y + 13, 1, 1, MENU_HL_INK);
}

static void draw_row(uint8_t index) {
    uint8_t y = (uint8_t)(MENU_FIRST_Y + index * (MENU_ROW_H + MENU_ROW_GAP));
    bool selected = (index == s_selected);
    uint16_t bg = selected ? MENU_HL_BG : MENU_BG;
    uint16_t ink = selected ? MENU_HL_INK : MENU_INK;

    st7735_fill_rect(MENU_ROW_X, y, MENU_ROW_W, MENU_ROW_H, bg);
    if (selected) draw_selected_arrow(y);
    text_draw(MENU_ROW_X + 8, (uint8_t)(y + 10), MENU_LABELS[index], 1, ink);
}

void menu_draw_item(menu_item_t item) {
    if (item < 0 || item >= MENU_ITEM_COUNT) return;
    draw_row((uint8_t)item);
}

void menu_draw(void) {
    st7735_fill_screen(MENU_BG);

    /* Header */
    st7735_fill_rect(0, 0, MENU_W, MENU_HEADER_H, MENU_HEADER_BG);
    {
        const char *title = "MAIN MENU";
        text_draw(centered_x(title, 2), 3, title, 2, MENU_HEADER_INK);
    }

    /* Four full-width menu rows, with a rule in each inter-row gap. */
    for (uint8_t i = 0; i < MENU_ITEM_COUNT; i++) {
        uint8_t y = (uint8_t)(MENU_FIRST_Y + i * (MENU_ROW_H + MENU_ROW_GAP));
        draw_row(i);
        if (i + 1 < MENU_ITEM_COUNT) {
            st7735_fill_rect(MENU_ROW_X, (uint8_t)(y + MENU_ROW_H),
                             MENU_ROW_W, 1, MENU_RULE);
        }
    }

    /* Soft-key bar */
    st7735_fill_rect(0, MENU_FOOTER_Y, MENU_W, MENU_FOOTER_H, MENU_FOOTER_BG);
    text_draw(5, 146, "FIRE OK", 1, MENU_FOOTER_INK);
    {
        const char *back = "PAUSE BACK";
        uint8_t w = text_width(back, 1);
        text_draw((uint8_t)(MENU_W - w - 5), 146, back, 1, MENU_FOOTER_INK);
    }
}
