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

#define CHROME_RIGHT_EDGE 126          /* right-aligned title text ends here */

static bool     s_flat;
static uint16_t s_flat_color;

void chrome_use_wallpaper(void) { s_flat = false; }
void chrome_use_flat(uint16_t color) { s_flat = true; s_flat_color = color; }

void chrome_draw_wallpaper(void) {
    if (s_flat) st7735_fill_rect(0, 0, CHROME_W, 160, s_flat_color);
    else        img_draw(0, 0, &IMG_WALLPAPER);
}

void chrome_restore(uint8_t x, uint8_t w, uint8_t y, uint8_t h) {
    if (s_flat) st7735_fill_rect(x, y, w, h, s_flat_color);
    else        img_draw_rect(0, 0, &IMG_WALLPAPER, x, y, w, h);
}

void chrome_dotted_leader(uint8_t x0, uint8_t x1, uint8_t y, uint16_t color) {
    if (x0 >= x1) return;
    for (uint8_t x = x0; x < x1; x = (uint8_t)(x + 4)) {
        st7735_fill_rect(x, y, 1, 1, color);
    }
}

static bool s_silent;

void chrome_set_silent(bool silent) { s_silent = silent; }

void chrome_draw_status_bar(void) {
    sprite_draw_p(STATUSBAR_X, STATUSBAR_Y, &SPR_STATUSBAR);
    if (s_silent) sprite_draw_p(SILENT_X, SILENT_Y, &SPR_SILENT);
}

void chrome_draw_title(const char *title, const char *right) {
    chrome_restore(0, CHROME_W, CHROME_TITLE_Y, CHROME_TITLE_H);
    text_draw(4, CHROME_TITLE_Y, title, 1, CHROME_INK);

    uint8_t leader_end = 118;
    if (right) {
        uint8_t rx = (uint8_t)(CHROME_RIGHT_EDGE + 1 - text_width(right, 1));
        text_draw(rx, CHROME_TITLE_Y, right, 1, CHROME_INK);
        leader_end = (uint8_t)(rx - 3);
    }
    chrome_dotted_leader((uint8_t)(4 + text_width(title, 1) + 3), leader_end, 21, CHROME_DOT);
}

uint8_t chrome_draw_softkey_right(const char *label) {
    uint8_t rx = (uint8_t)(CHROME_W - text_width(label, 1) - 4);
    text_draw(rx, CHROME_SOFTKEY_Y, label, 1, CHROME_SOFT_INK);
    return rx;
}

/* The S40 selection band is a vertical gradient, light blue (14,101,172) at the top to
 * navy (1,53,126) at the bottom, sampled from the reference photos. */
#define CHROME_HL_TOP     0x0B35
#define CHROME_HL_BOTTOM  0x01AF

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

void chrome_draw_band_part(uint8_t y, uint8_t x, uint8_t w) {
    for (uint8_t k = 0; k < CHROME_ROW_H; k++)
        st7735_fill_rect(x, (uint8_t)(y + k), w, 1,
                         lerp565(CHROME_HL_TOP, CHROME_HL_BOTTOM, k, CHROME_ROW_H - 1));
}

void chrome_draw_band(uint8_t y) {
    chrome_draw_band_part(y, 0, CHROME_BAND_W);
}

void chrome_draw_softkeys(const char *left, const char *right) {
    uint8_t lw = text_width_big(left);
    uint8_t lx = (uint8_t)((CHROME_W - lw) / 2);
    text_draw_big(lx, CHROME_SOFTKEY_BIG_Y, left, CHROME_SOFT_INK);
    uint8_t rx = chrome_draw_softkey_right(right);
    chrome_dotted_leader((uint8_t)(lx + lw + 5), (uint8_t)(rx - 5), CHROME_SOFTKEY_BIG_Y + 11, CHROME_DOT);
}

/* ── Scrollbar ────────────────────────────────────────────────────────────── */

/* As on the S40 screenshots: a thin 3 px track from the top of the list down to just above
 * the right soft key, centred under a 6 px thumb (5 px body + 1 px shadow on its right)
 * that is brighter than the track: white to peach with a brown edge.  The thumb is
 * 1/count of the track and travels over it with the selection.  The selection band stops
 * before the bar. */
#define SCROLL_W        6
#define SCROLL_H        116
#define TRACK_X         (CHROME_SCROLL_X + 1)      /* 3 px track, centred on the 5 px thumb body */
#define TRACK_W         3
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
static void draw_thumb_part(uint8_t y, uint8_t h) {
    st7735_stream_begin(CHROME_SCROLL_X, y, SCROLL_W, h);
    for (uint8_t r = 0; r < h; r++)
        for (uint8_t c = 0; c < SCROLL_W; c++) st7735_stream_pixels(SCROLL_THUMB[c], 1);
    st7735_stream_end();
}

/* Put the bar's background (background + track) back on rows [y, y+h). */
static void clear_thumb_part(uint8_t y, uint8_t h) {
    chrome_restore(CHROME_SCROLL_X, SCROLL_W, y, h);
    for (uint8_t c = 0; c < TRACK_W; c++)
        st7735_fill_rect((uint8_t)(TRACK_X + c), y, 1, h, SCROLL_TRACK[c]);
}

void chrome_scrollbar_draw(uint8_t count, uint8_t selected) {
    for (uint8_t c = 0; c < TRACK_W; c++)
        st7735_fill_rect((uint8_t)(TRACK_X + c), CHROME_LIST_Y, 1, SCROLL_H, SCROLL_TRACK[c]);
    draw_thumb_part(thumb_top(count, selected), thumb_h(count));
    s_thumb = selected;
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

/* Move the thumb without ever leaving the bar empty: the SPI link is slow enough to see
 * the order of the pixel updates.  First paint the new thumb starting at the edge that
 * touches the old one (the bar visibly grows towards the new position), then clear the
 * old thumb starting at the edge farthest from the new one (it shrinks away from it). */
void chrome_scrollbar_move(uint8_t count, uint8_t to) {
    uint8_t from = s_thumb;
    if (from == to) return;
    uint8_t oy = thumb_top(count, from), ny = thumb_top(count, to);
    bool down = ny > oy;
    uint8_t th = thumb_h(count);

    for_strips(ny, th, down, draw_thumb_part);

    /* old rows not covered by the new thumb (the steps are >= one thumb, so all of them) */
    uint8_t a = oy, e = (uint8_t)(oy + th);
    if (down  && ny < e) e = ny;
    if (!down && ny + th > a) a = (uint8_t)(ny + th);
    if (a < e) for_strips(a, (uint8_t)(e - a), down, clear_thumb_part);
    s_thumb = to;
}
