/**
 * menu.cpp — Series 40 list-style main menu.
 *
 * The original MIDlet uses the phone's native List UI rather than drawing a
 * menu on the game canvas.  This renderer reconstructs the 128×160 S40 list:
 * a small status bar, the list title, four full-width rows with dotted
 * leaders, and Select/Exit soft keys over the reconstructed S40 wallpaper
 * (IMG_WALLPAPER).  The selected row is a blue-to-navy gradient band with white text.
 */
#include "ui/menu.h"
#include "render/text.h"
#include "render/image.h"
#include "assets/wallpaper.h"
#include "drivers/st7735.h"

#define MENU_W          128

#define MENU_STATUS_H   14
#define MENU_TITLE_Y    14
#define MENU_TITLE_H    16
#define MENU_FIRST_Y    30
#define MENU_ROW_H      20
#define MENU_FOOTER_Y   140
#define MENU_FOOTER_H   20

#define MENU_INK        0x0000
#define MENU_SOFT_INK   0x0010
#define MENU_DOT        0x8410
#define MENU_HL_INK     0xFFFF
/* The S40 selection band is a vertical gradient, light blue (14,101,172) at the top to
 * navy (1,53,126) at the bottom, sampled from the reference photos. */
#define MENU_HL_TOP     0x0B35
#define MENU_HL_BOTTOM  0x01AF
#define MENU_STATUS_INK 0x0010

/* Scrollbar (as on the S40 screenshots): a thin 3 px track from the top of the list down
 * to just above the "Exit" soft key, centred under a 6 px thumb (5 px body + 1 px shadow
 * on its right) that is brighter than the track: white to peach with a brown edge.  The
 * thumb is 1/MENU_ITEM_COUNT of the track and travels over it with the selection. The
 * selection band stops before the bar. */
#define MENU_BAND_W     120
#define MENU_SCROLL_X   121
#define MENU_SCROLL_W   6
#define MENU_SCROLL_Y   MENU_FIRST_Y
#define MENU_SCROLL_H   116
#define MENU_THUMB_H    (MENU_SCROLL_H / MENU_ITEM_COUNT)
#define MENU_TRACK_X    (MENU_SCROLL_X + 1)      /* 3 px track, centred on the 5 px thumb body */
#define MENU_TRACK_W    3
#define MENU_THUMB_STRIPS 3                      /* thumb moves in this many steps */
#define MENU_LEADER_END 118
#define MENU_BATT_FILL  0x07E0

/* The port has no RTC; the status bar is decorative S40 chrome. */
#define MENU_STATUS_TIME "22:22"
#define MENU_TITLE_TEXT  "Snake EX2"
#define MENU_SOFT_LEFT   "Select"
#define MENU_SOFT_RIGHT  "Exit"

static const uint16_t SCROLL_TRACK[MENU_TRACK_W] = { 0x21B2, 0x7C7A, 0x94FB };
static const uint16_t SCROLL_THUMB[MENU_SCROLL_W] = {
    0xFFFF, 0xFF5A, 0xF6B7, 0xDD0F, 0x9225, 0x118F,   /* last column = shadow */
};

static uint8_t s_selected;
static uint8_t s_thumb;      /* item the on-screen thumb currently shows */

static const char *const MENU_LABELS[MENU_ITEM_COUNT] = {
    "New game",
    "High scores",
    "Settings",
    "Instructions",
};


/* Restore wallpaper rows [y, y+h), x in [x0, x0+w). */
static void draw_wallpaper(uint8_t x0, uint8_t w, uint8_t y, uint8_t h) {
    img_draw_rect(0, 0, &IMG_WALLPAPER, x0, y, w, h);
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

static void dotted_leader(uint8_t x0, uint8_t x1, uint8_t y, uint16_t color) {
    if (x0 >= x1) return;
    for (uint8_t x = x0; x < x1; x = (uint8_t)(x + 4)) {
        st7735_fill_rect(x, y, 1, 1, color);
    }
}

static void draw_status_bar(void) {
    /* Signal-strength bars at the left. */
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t h = (uint8_t)(2 + i * 2);
        st7735_fill_rect((uint8_t)(3 + i * 3), (uint8_t)(11 - h), 2, h,
                         MENU_STATUS_INK);
    }
    dotted_leader(18, 38, 7, MENU_DOT);

    /* Battery outline, fill and terminal nub. */
    st7735_fill_rect(42, 3, 11, 7, MENU_STATUS_INK);
    st7735_fill_rect(43, 4, 9, 5, 0xFFFF);
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
    draw_wallpaper(0, MENU_W, MENU_TITLE_Y, MENU_TITLE_H);
    text_draw(4, 18, MENU_TITLE_TEXT, 1, MENU_INK);

    {
        uint8_t end = (uint8_t)(4 + text_width(MENU_TITLE_TEXT, 1) + 3);
        dotted_leader(end, 118, 21, MENU_DOT);
    }
    /* Index of the highlighted item, 1-based, above the scrollbar. */
    char idx[2] = { (char)('1' + s_selected), '\0' };
    text_draw(MENU_SCROLL_X, 18, idx, 1, MENU_INK);   /* 5 px wide: centred over the thumb */
}

static uint8_t thumb_top(uint8_t item) {
    return (uint8_t)(MENU_SCROLL_Y + item * (MENU_SCROLL_H - MENU_THUMB_H) / (MENU_ITEM_COUNT - 1));
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
    draw_wallpaper(MENU_SCROLL_X, MENU_SCROLL_W, y, h);
    for (uint8_t c = 0; c < MENU_TRACK_W; c++)
        st7735_fill_rect((uint8_t)(MENU_TRACK_X + c), y, 1, h, SCROLL_TRACK[c]);
}

/* Full bar over a freshly drawn wallpaper: track, then the thumb of the selected item. */
static void draw_scrollbar(void) {
    for (uint8_t c = 0; c < MENU_TRACK_W; c++)
        st7735_fill_rect((uint8_t)(MENU_TRACK_X + c), MENU_SCROLL_Y, 1, MENU_SCROLL_H, SCROLL_TRACK[c]);
    draw_thumb_part(thumb_top(s_selected), MENU_THUMB_H);
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

    for_strips(ny, MENU_THUMB_H, down, draw_thumb_part);

    /* old rows not covered by the new thumb (the steps are >= one thumb, so all of them) */
    uint8_t a = oy, e = (uint8_t)(oy + MENU_THUMB_H);
    if (down  && ny < e) e = ny;
    if (!down && ny + MENU_THUMB_H > a) a = (uint8_t)(ny + MENU_THUMB_H);
    if (a < e) for_strips(a, (uint8_t)(e - a), down, clear_thumb_part);
    s_thumb = to;
}

static void draw_row(uint8_t index) {
    uint8_t y = (uint8_t)(MENU_FIRST_Y + index * MENU_ROW_H);
    bool selected = (index == s_selected);
    uint16_t ink = selected ? MENU_HL_INK : MENU_INK;
    const char *label = MENU_LABELS[index];

    if (selected) draw_band(y);
    else          draw_wallpaper(0, MENU_SCROLL_X, y, MENU_ROW_H);   /* the bar is never touched */
    text_draw_big(4, (uint8_t)(y + 4), label, ink);

    if (!selected) {
        uint8_t end = (uint8_t)(4 + text_width_big(label) + 4);
        dotted_leader(end, MENU_LEADER_END, (uint8_t)(y + 13), MENU_DOT);   /* on the baseline */
    }

}

static void draw_softkeys(void) {
    /* "Select" is the main action: bold and centred. */
    uint8_t lw = text_width_big(MENU_SOFT_LEFT);
    uint8_t lx = (uint8_t)((MENU_W - lw) / 2);
    text_draw_big(lx, 144, MENU_SOFT_LEFT, MENU_SOFT_INK);
    {
        uint8_t rw = text_width(MENU_SOFT_RIGHT, 1);
        uint8_t rx = (uint8_t)(MENU_W - rw - 4);
        text_draw(rx, 148, MENU_SOFT_RIGHT, 1, MENU_SOFT_INK);
        dotted_leader((uint8_t)(lx + lw + 5), (uint8_t)(rx - 5), 153, MENU_DOT);
    }
}

void menu_init(void) {
    s_selected = MENU_ITEM_NEW_GAME;
    s_thumb = s_selected;
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
    if (item == s_selected) {
        draw_title_bar();                                   /* shows the item index */
        move_thumb(s_thumb, s_selected);
    }
}

void menu_draw(void) {
    img_draw(0, 0, &IMG_WALLPAPER);

    draw_status_bar();
    draw_title_bar();

    for (uint8_t i = 0; i < MENU_ITEM_COUNT; i++) {
        draw_row(i);
    }

    draw_scrollbar();
    draw_softkeys();
}
