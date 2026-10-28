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

#define MENU_FIRST_Y    30
#define MENU_ROW_H      20

#define MENU_HL_INK     0xFFFF
/* The S40 selection band is a vertical gradient, light blue (14,101,172) at the top to
 * navy (1,53,126) at the bottom, sampled from the reference photos. */
#define MENU_HL_TOP     0x0B35
#define MENU_HL_BOTTOM  0x01AF

/* Scrollbar (as on the S40 screenshots): a thin 3 px track from the top of the list down
 * to just above the right soft key, centred under a 6 px thumb (5 px body + 1 px shadow
 * on its right) that is brighter than the track: white to peach with a brown edge.  The
 * thumb is 1/count of the track and travels over it with the selection. The selection
 * band stops before the bar. */
#define MENU_BAND_W     120
#define MENU_SCROLL_X   121
#define MENU_SCROLL_W   6
#define MENU_SCROLL_Y   MENU_FIRST_Y
#define MENU_SCROLL_H   116
#define MENU_TRACK_X    (MENU_SCROLL_X + 1)      /* 3 px track, centred on the 5 px thumb body */
#define MENU_TRACK_W    3
#define MENU_THUMB_STRIPS 3                      /* thumb moves in this many steps */
#define MENU_LEADER_END 118

#define MENU_SOFT_LEFT  "Select"

static const uint16_t SCROLL_TRACK[MENU_TRACK_W] = { 0x21B2, 0x7C7A, 0x94FB };
static const uint16_t SCROLL_THUMB[MENU_SCROLL_W] = {
    0xFFFF, 0xFF5A, 0xF6B7, 0xDD0F, 0x9225, 0x118F,   /* last column = shadow */
};

static const char *const MAIN_LABELS[MENU_ITEM_COUNT] = {
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
static uint8_t s_thumb;      /* item the on-screen thumb currently shows */

static uint8_t thumb_h(void) {
    return (uint8_t)(MENU_SCROLL_H / s_count);
}

static uint16_t lerp565(uint16_t a, uint16_t b, uint8_t k, uint8_t n) {
    uint16_t out = 0;
    static const uint8_t shift[3] = {11, 5, 0};
    static const uint8_t bits[3]  = {5, 6, 5};
    for (uint8_t c = 0; c < 3; c++) {
        uint8_t mask = (uint8_t)((1u << bits[c]) - 1);
        int16_t ca = (a >> shift[c]) & mask, cb = (b >> shift[c]) & mask;
        out |= (uint16_t)((ca + (cb - ca) * k / n) << shift[c]);
    }
    return out;
}

static void draw_band(uint8_t y) {
    for (uint8_t k = 0; k < MENU_ROW_H; k++)
        st7735_fill_rect(0, (uint8_t)(y + k), MENU_BAND_W, 1,
                         lerp565(MENU_HL_TOP, MENU_HL_BOTTOM, k, MENU_ROW_H - 1));
}

static void draw_title_bar(void) {
    /* Index of the highlighted item, 1-based, right-aligned over the scrollbar (the 5 px
     * digit sits exactly above the thumb body). */
    char idx[2] = { (char)('1' + s_selected), '\0' };
    chrome_draw_title(s_title, idx);
}

static uint8_t thumb_top(uint8_t item) {
    return (uint8_t)(MENU_SCROLL_Y + item * (MENU_SCROLL_H - thumb_h()) / (s_count - 1));
}

/* Thumb pixels for screen rows [y, y+h): one window, row-major. */
static void draw_thumb_part(uint8_t y, uint8_t h) {
    st7735_stream_begin(MENU_SCROLL_X, y, MENU_SCROLL_W, h);
    for (uint8_t r = 0; r < h; r++)
        for (uint8_t c = 0; c < MENU_SCROLL_W; c++) st7735_stream_pixels(SCROLL_THUMB[c], 1);
    st7735_stream_end();
}

/* Put the bar's background (wallpaper + track) back on rows [y, y+h). */
static void clear_thumb_part(uint8_t y, uint8_t h) {
    chrome_restore(MENU_SCROLL_X, MENU_SCROLL_W, y, h);
    for (uint8_t c = 0; c < MENU_TRACK_W; c++)
        st7735_fill_rect((uint8_t)(MENU_TRACK_X + c), y, 1, h, SCROLL_TRACK[c]);
}

/* Full bar over a freshly drawn wallpaper: track, then the thumb of the selected item. */
static void draw_scrollbar(void) {
    for (uint8_t c = 0; c < MENU_TRACK_W; c++)
        st7735_fill_rect((uint8_t)(MENU_TRACK_X + c), MENU_SCROLL_Y, 1, MENU_SCROLL_H, SCROLL_TRACK[c]);
    draw_thumb_part(thumb_top(s_selected), thumb_h());
    s_thumb = s_selected;
}

/* Run f over rows [y, y+h) in MENU_THUMB_STRIPS pieces, top to bottom or bottom to top. */
static void for_strips(uint8_t y, uint8_t h, bool top_first, void (*f)(uint8_t, uint8_t)) {
    uint8_t step = (uint8_t)((h + MENU_THUMB_STRIPS - 1) / MENU_THUMB_STRIPS);
    for (uint8_t i = 0; i < MENU_THUMB_STRIPS; i++) {
        uint8_t off = (uint8_t)(i * step);
        if (off >= h) break;
        uint8_t n = (uint8_t)(h - off < step ? h - off : step);
        f(top_first ? (uint8_t)(y + off) : (uint8_t)(y + h - off - n), n);
    }
}

/* Move the thumb without ever leaving the bar empty: the SPI link is slow enough to see
 * the order of the pixel updates.  First paint the new thumb starting at the edge that
 * touches the old one (the bar visibly grows towards the new position), then clear the
 * old thumb starting at the edge farthest from the new one (it shrinks away from it). */
static void move_thumb(uint8_t from, uint8_t to) {
    if (from == to) return;
    uint8_t oy = thumb_top(from), ny = thumb_top(to);
    bool down = ny > oy;
    uint8_t th = thumb_h();

    for_strips(ny, th, down, draw_thumb_part);

    /* old rows not covered by the new thumb (the steps are >= one thumb, so all of them) */
    uint8_t a = oy, e = (uint8_t)(oy + th);
    if (down  && ny < e) e = ny;
    if (!down && ny + th > a) a = (uint8_t)(ny + th);
    if (a < e) for_strips(a, (uint8_t)(e - a), down, clear_thumb_part);
    s_thumb = to;
}

static void draw_row(uint8_t index) {
    uint8_t y = (uint8_t)(MENU_FIRST_Y + index * MENU_ROW_H);
    bool selected = (index == s_selected);
    uint16_t ink = selected ? MENU_HL_INK : CHROME_INK;
    const char *label = s_labels[index];

    if (selected) draw_band(y);
    else          chrome_restore(0, MENU_SCROLL_X, y, MENU_ROW_H);   /* the bar is never touched */
    text_draw_big(4, (uint8_t)(y + 4), label, ink);

    if (!selected) {
        uint8_t end = (uint8_t)(4 + text_width_big(label) + 4);
        chrome_dotted_leader(end, MENU_LEADER_END, (uint8_t)(y + 13), CHROME_DOT);   /* on the baseline */
    }
}

static void draw_softkeys(void) {
    /* "Select" is the main action: bold and centred. */
    uint8_t lw = text_width_big(MENU_SOFT_LEFT);
    uint8_t lx = (uint8_t)((CHROME_W - lw) / 2);
    text_draw_big(lx, 144, MENU_SOFT_LEFT, CHROME_SOFT_INK);
    uint8_t rx = chrome_draw_softkey_right(s_right);
    chrome_dotted_leader((uint8_t)(lx + lw + 5), (uint8_t)(rx - 5), 153, CHROME_DOT);
}

void menu_open(menu_list_t list) {
    if (list == MENU_LIST_SCORES) {
        s_labels = SCORE_LABELS;
        s_count = MENU_SCORES_COUNT;
        s_title = "High scores";
        s_right = "Back";
    } else {
        s_labels = MAIN_LABELS;
        s_count = MENU_ITEM_COUNT;
        s_title = "Snake EX2";
        s_right = "Exit";
    }
    s_selected = 0;
    s_thumb = 0;
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
    if (item < 0 || item >= MENU_ITEM_COUNT) return MAIN_LABELS[0];
    return MAIN_LABELS[item];
}

void menu_draw_item(menu_item_t item) {
    if (item < 0 || item >= s_count) return;
    draw_row((uint8_t)item);
    if (item == s_selected) {
        draw_title_bar();                                   /* shows the item index */
        move_thumb(s_thumb, s_selected);
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

    draw_scrollbar();
    draw_softkeys();
}
