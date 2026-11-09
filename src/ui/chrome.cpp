/**
 * chrome.cpp — Shared Series 40 screen chrome (see chrome.h).
 */
#include "ui/chrome.h"
#include "render/text.h"
#include "render/image.h"
#include "render/sprite.h"
#include "assets/wallpaper.h"
#include "assets/statusbar.h"
#include "drivers/st7735.h"
#include "config.h"
#include <string.h>

#define CHROME_RIGHT_EDGE 123          /* right-aligned title text ends here */

static bool     s_flat;
static uint16_t s_flat_color;
static bool     s_ready;                  /* wallpaper + status bar are on screen */
static void   (*s_eraser)(void);          /* erases the open screen's own content */

/* What the shared chrome currently shows, so a new screen only repaints what differs. */
typedef struct { const char *s; uint8_t x, w; } label_slot_t;
static label_slot_t s_title, s_key_left, s_key_right;
static struct { char c; uint8_t x, w; } s_index;
static bool    s_bar_on;
static uint8_t s_bar_count;

void chrome_use_wallpaper(void) { s_flat = false; }
void chrome_use_flat(uint16_t color) { s_flat = true; s_flat_color = color; s_ready = false; }
void chrome_invalidate(void) { s_ready = false; }

void chrome_draw_wallpaper(void) {
    if (s_flat) st7735_fill_rect(0, 0, CHROME_W, 160, s_flat_color);
    else        img_draw(0, 0, &IMG_WALLPAPER);
}

void chrome_restore(uint8_t x, uint8_t w, uint8_t y, uint8_t h) {
    if (s_flat) st7735_fill_rect(x, y, w, h, s_flat_color);
    else        img_draw_rect(0, 0, &IMG_WALLPAPER, x, y, w, h);
}

static bool    s_silent;
static uint8_t s_battery = 100;            /* percent shown by the battery icon */

#define SILENT_W 36
#define SILENT_H 12

/* The battery is drawn in code (the sprite's copy is clipped out) so it can show the level:
 * columns [BATT_X0, BATT_X1) of the bar, a 4-segment gauge, red below BATTERY_LOW_PERCENT. */
#define BATT_X0          31
#define BATT_X1          47
#define BATT_Y           1
#define BATT_H           12
#define BATT_SEGMENTS    4
#define BATT_OUTLINE     0xBE3D
#define BATT_FILL        0x3C8C
#define BATT_LOW         0xD16A            /* the red of the muted-note icon */

static void draw_battery(void) {
    bool low = s_battery < BATTERY_LOW_PERCENT;
    uint16_t edge = low ? BATT_LOW : BATT_OUTLINE, fill = low ? BATT_LOW : BATT_FILL;
    st7735_fill_rect(33, 3, 11, 1, edge);                /* body outline, tip at the right */
    st7735_fill_rect(33, 11, 11, 1, edge);
    st7735_fill_rect(33, 3, 1, 9, edge);
    st7735_fill_rect(43, 3, 1, 9, edge);
    st7735_fill_rect(44, 5, 2, 5, edge);
    uint8_t lit = (uint8_t)((s_battery * BATT_SEGMENTS + 99) / 100);   /* 1..100 % lights >= 1 */
    for (uint8_t i = 0; i < lit && i < BATT_SEGMENTS; i++)
        st7735_fill_rect((uint8_t)(35 + 2 * i), 5, 2, 5, fill);
}

void chrome_draw_status_bar(void) {
    sprite_set_clip(0, 0, BATT_X0, 160);
    sprite_draw_p(STATUSBAR_X, STATUSBAR_Y, &SPR_STATUSBAR);
    sprite_set_clip(BATT_X1, 0, CHROME_W, 160);
    sprite_draw_p(STATUSBAR_X, STATUSBAR_Y, &SPR_STATUSBAR);
    sprite_reset_clip();
    draw_battery();
    if (s_silent) sprite_draw_p(SILENT_X, SILENT_Y, &SPR_SILENT);
}

void chrome_set_silent(bool silent) {
    if (silent == s_silent) return;
    s_silent = silent;
    if (!s_ready) return;
    /* only the label's area of the bar changes */
    chrome_restore(SILENT_X, SILENT_W, SILENT_Y, SILENT_H);
    if (s_silent) sprite_draw_p(SILENT_X, SILENT_Y, &SPR_SILENT);
}

void chrome_set_battery(uint8_t percent) {
    if (percent > 100) percent = 100;
    if (percent == s_battery) return;
    bool recolor = (percent < BATTERY_LOW_PERCENT) != (s_battery < BATTERY_LOW_PERCENT);
    uint8_t old_lit = (uint8_t)((s_battery * BATT_SEGMENTS + 99) / 100);
    s_battery = percent;
    if (!s_ready) return;
    if (!recolor && old_lit == (uint8_t)((percent * BATT_SEGMENTS + 99) / 100)) return;
    chrome_restore(BATT_X0, BATT_X1 - BATT_X0, BATT_Y, BATT_H);
    draw_battery();
}

void chrome_begin(void (*erase)(void)) {
    if (!s_ready || s_flat) {
        s_flat = false;
        chrome_draw_wallpaper();
        chrome_draw_status_bar();
        s_title.s = s_key_left.s = s_key_right.s = 0;
        s_index.c = 0;
        s_bar_on = false;
        s_ready = true;
    } else if (s_eraser) {
        s_eraser();                       /* the previous screen's labels, nothing else */
    }
    s_eraser = erase;
}

static bool same_text(const char *a, const char *b) {
    return a == b || (a && b && strcmp(a, b) == 0);
}

void chrome_draw_title(const char *title, const char *right) {
    if (!same_text(s_title.s, title)) {
        if (s_title.s) chrome_restore(s_title.x, s_title.w, CHROME_TITLE_Y, CHROME_TEXT_H);
        s_title.s = title;
        if (title) {
            s_title.x = 4;
            s_title.w = (uint8_t)(text_width(title, 1) + 1);
            text_draw(s_title.x, CHROME_TITLE_Y, title, 1, CHROME_INK);
        }
    }
    char c = right ? right[0] : 0;           /* the list index: one character */
    if (c != s_index.c) {
        if (s_index.c) chrome_restore(s_index.x, s_index.w, CHROME_TITLE_Y, CHROME_TEXT_H);
        s_index.c = c;
        if (c) {
            s_index.w = (uint8_t)(text_width(right, 1) + 1);
            s_index.x = (uint8_t)(CHROME_RIGHT_EDGE + 1 - text_width(right, 1));
            text_draw(s_index.x, CHROME_TITLE_Y, right, 1, CHROME_INK);
        }
    }
}

/* The S40 selection band is a vertical gradient, (4,123,222) at the top to (0,94,199) at
 * the bottom, sampled from Nokia-GUI.jpg; its right corners are rounded by 2 px. */
#define CHROME_HL_TOP     0x03DB
#define CHROME_HL_BOTTOM  0x02F8

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

/* Pixels cut off the right end of row k of a 2 px rounded corner (top and bottom rows). */
static uint8_t corner_cut(uint8_t k, uint8_t h) {
    uint8_t e = k < h - 1 - k ? k : (uint8_t)(h - 1 - k);
    return e == 0 ? 2 : e == 1 ? 1 : 0;
}

void chrome_draw_band_part(uint8_t y, uint8_t x, uint8_t w) {
    bool at_end = x + w == CHROME_BAND_W;
    for (uint8_t k = 0; k < CHROME_ROW_H; k++) {
        uint8_t cut = at_end ? corner_cut(k, CHROME_ROW_H) : 0;
        st7735_fill_rect(x, (uint8_t)(y + k), (uint8_t)(w - cut), 1,
                         lerp565(CHROME_HL_TOP, CHROME_HL_BOTTOM, k, CHROME_ROW_H - 1));
        if (cut) chrome_restore((uint8_t)(x + w - cut), cut, (uint8_t)(y + k), 1);
    }
}

void chrome_draw_band(uint8_t y) {
    chrome_draw_band_part(y, 0, CHROME_BAND_W);
}

void chrome_draw_softkeys(const char *left, const char *right) {
    if (!same_text(s_key_left.s, left)) {
        if (s_key_left.s) chrome_restore(s_key_left.x, s_key_left.w, CHROME_SOFTKEY_BIG_Y, CHROME_BIG_H);
        s_key_left.s = left;
        if (left) {
            uint8_t lw = text_width_big(left);
            s_key_left.x = (uint8_t)((CHROME_W - lw) / 2);
            s_key_left.w = (uint8_t)(lw + 1);
            text_draw_big(s_key_left.x, CHROME_SOFTKEY_BIG_Y, left, CHROME_SOFT_INK);
        }
    }
    if (!same_text(s_key_right.s, right)) {
        if (s_key_right.s) chrome_restore(s_key_right.x, s_key_right.w, CHROME_SOFTKEY_Y, CHROME_TEXT_H);
        s_key_right.s = right;
        if (right) {
            uint8_t rw = text_width(right, 1);
            s_key_right.x = (uint8_t)(CHROME_W - rw - 4);
            s_key_right.w = (uint8_t)(rw + 1);
            text_draw(s_key_right.x, CHROME_SOFTKEY_Y, right, 1, CHROME_SOFT_INK);
        }
    }
}

/* ── Scrollbar ────────────────────────────────────────────────────────────── */

/* As on the S40 screenshots: a thin 3 px track from the top of the list down to just above
 * the right soft key, centred under a 4 px thumb with 1 px rounded corners
 * that is brighter than the track: cream to peach with a brown edge.  The thumb is
 * 1/count of the track and travels over it with the selection.  The selection band stops
 * before the bar. */
#define SCROLL_W        6
#define SCROLL_H        111                        /* ends 5 px above the soft key row */
#define TRACK_X         (CHROME_SCROLL_X + 1)      /* 3 px track, centred on the 5 px thumb body */
#define TRACK_W         3
#define THUMB_C0        1                          /* thumb = columns 1..4 of SCROLL_THUMB */
#define THUMB_W         4
#define THUMB_STRIPS    3                          /* thumb moves in this many steps */

static const uint16_t SCROLL_TRACK[TRACK_W] = { 0x21B2, 0x7C7A, 0x94FB };
static const uint16_t SCROLL_THUMB[SCROLL_W] = {
    0xFFFF, 0xFF5A, 0xF6B7, 0xDD0F, 0x9225, 0x118F,   /* last column = shadow */
};

static uint8_t s_thumb;      /* item the on-screen thumb currently shows */

static uint8_t thumb_h(uint8_t count) {
    return (uint8_t)(SCROLL_H / count);
}

static uint8_t thumb_top(uint8_t count, uint8_t item) {
    return (uint8_t)(CHROME_LIST_Y + item * (SCROLL_H - thumb_h(count)) / (count - 1));
}

/* Thumb pixels for screen rows [y, y+h): one window, row-major. */
static uint8_t s_thumb_y, s_thumb_h;     /* thumb being drawn, for its rounded corners */
static bool    s_thumb_down = true;      /* rows are painted top to bottom (else bottom to top) */

static void draw_thumb_part(uint8_t y, uint8_t h) {
    for (uint8_t i = 0; i < h; i++) {
        uint8_t r = s_thumb_down ? i : (uint8_t)(h - 1 - i);
        uint8_t rel = (uint8_t)(y + r - s_thumb_y);
        uint8_t cut = (rel == 0 || rel == s_thumb_h - 1) ? 1 : 0;   /* 1 px rounded corners */
        uint8_t row = (uint8_t)(y + r);
        st7735_stream_begin((uint8_t)(CHROME_SCROLL_X + THUMB_C0 + cut), row, (uint8_t)(THUMB_W - 2 * cut), 1);
        for (uint8_t c = (uint8_t)(THUMB_C0 + cut); c < THUMB_C0 + THUMB_W - cut; c++)
            st7735_stream_pixels(SCROLL_THUMB[c], 1);
        st7735_stream_end();
        if (cut) {   /* corner pixels show the track (left, over it) and the background (right) */
            chrome_restore((uint8_t)(CHROME_SCROLL_X + THUMB_C0), 1, row, 1);
            st7735_fill_rect((uint8_t)(CHROME_SCROLL_X + THUMB_C0), row, 1, 1, SCROLL_TRACK[0]);
            chrome_restore((uint8_t)(CHROME_SCROLL_X + THUMB_C0 + THUMB_W - 1), 1, row, 1);
        }
    }
}

/* Put the bar's background (background + track) back on rows [y, y+h). */
static void clear_thumb_part(uint8_t y, uint8_t h) {
    chrome_restore(CHROME_SCROLL_X, SCROLL_W, y, h);
    for (uint8_t c = 0; c < TRACK_W; c++)
        st7735_fill_rect((uint8_t)(TRACK_X + c), y, 1, h, SCROLL_TRACK[c]);
}

void chrome_scrollbar_draw(uint8_t count, uint8_t selected) {
    if (s_bar_on) { chrome_scrollbar_move(count, selected); return; }
    for (uint8_t c = 0; c < TRACK_W; c++)
        st7735_fill_rect((uint8_t)(TRACK_X + c), CHROME_LIST_Y, 1, SCROLL_H, SCROLL_TRACK[c]);
    s_thumb_y = thumb_top(count, selected); s_thumb_h = thumb_h(count); s_thumb_down = true;
    draw_thumb_part(s_thumb_y, s_thumb_h);
    s_thumb = selected;
    s_bar_count = count;
    s_bar_on = true;
}

void chrome_scrollbar_hide(void) {
    if (!s_bar_on) return;
    chrome_restore(CHROME_SCROLL_X, SCROLL_W, CHROME_LIST_Y, SCROLL_H);
    s_bar_on = false;
}

/* Run f over rows [y, y+h) in THUMB_STRIPS pieces, top to bottom or bottom to top. */
static void for_strips(uint8_t y, uint8_t h, bool top_first, void (*f)(uint8_t, uint8_t)) {
    uint8_t step = (uint8_t)((h + THUMB_STRIPS - 1) / THUMB_STRIPS);
    for (uint8_t i = 0; i < THUMB_STRIPS; i++) {
        uint8_t off = (uint8_t)(i * step);
        if (off >= h) break;
        uint8_t n = (uint8_t)(h - off < step ? h - off : step);
        f(top_first ? (uint8_t)(y + off) : (uint8_t)(y + h - off - n), n);
    }
}

/* Move the thumb (of a list of `count` rows, possibly not the one it was drawn for) without
 * ever leaving the bar empty: the SPI link is slow enough to see the order of the pixel
 * updates.  First paint the new thumb starting at the edge that touches the old one (the
 * bar visibly grows towards the new position), then clear the old thumb's rows that the
 * new one does not cover, starting at the edge farthest from the new one.  The track and
 * the rest of the bar are never repainted. */
void chrome_scrollbar_move(uint8_t count, uint8_t to) {
    if (!s_bar_on) { chrome_scrollbar_draw(count, to); return; }
    if (s_bar_count == count && s_thumb == to) return;
    uint8_t oy = thumb_top(s_bar_count, s_thumb), oe = (uint8_t)(oy + thumb_h(s_bar_count));
    uint8_t ny = thumb_top(count, to), th = thumb_h(count), ne = (uint8_t)(ny + th);
    bool down = ny > oy || (ny == oy && ne > oe);

    s_thumb_y = ny; s_thumb_h = th; s_thumb_down = down;
    for_strips(ny, th, down, draw_thumb_part);

    if (oy < ny) {                                    /* old rows above the new thumb */
        uint8_t e = oe < ny ? oe : ny;
        for_strips(oy, (uint8_t)(e - oy), down, clear_thumb_part);
    }
    if (ne < oe) {                                    /* old rows below it */
        uint8_t a = oy > ne ? oy : ne;
        for_strips(a, (uint8_t)(oe - a), down, clear_thumb_part);
    }
    s_thumb = to;
    s_bar_count = count;
}
