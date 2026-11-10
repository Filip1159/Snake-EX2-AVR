/**
 * mock_display.h — Host mock of the ST7735 driver for the UI tests: a 128x160 RGB565
 * framebuffer plus bookkeeping, and the small helpers the tests share.
 *
 *   fb[y][x]       what is on screen
 *   stamp[y][x]    tick of the last write (ticks advance per stream window / per filled pixel),
 *                  to check the ORDER pixels are painted in
 *   writes[y][x]   number of writes since reset_writes(), to check what is NOT repainted
 *
 * Include in a single translation unit per test binary.
 */
#ifndef TEST_MOCK_DISPLAY_H
#define TEST_MOCK_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "drivers/st7735.h"

#define UNUSED_VAR __attribute__((unused))

static uint16_t fb[160][128];
static int      stamp[160][128] UNUSED_VAR;
static int      writes[160][128] UNUSED_VAR;
static int      op_counter UNUSED_VAR;
static int      fill_screen_calls UNUSED_VAR;
static int      fails UNUSED_VAR;

static inline void mock_put(int x, int y, uint16_t c, int tick) {
    fb[y][x] = c;
    stamp[y][x] = tick;
    writes[y][x]++;
}

extern "C" {
void st7735_fill_screen(uint16_t c) {
    fill_screen_calls++;
    for (int y = 0; y < 160; y++)
        for (int x = 0; x < 128; x++) mock_put(x, y, c, ++op_counter);
}
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (x + i < 128 && y + j < 160) mock_put(x + i, y + j, c, ++op_counter);
}
static int s_cx, s_cy, s_cw, s_px;
void st7735_stream_begin(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    (void)h; ++op_counter; s_cx = x; s_cy = y; s_cw = w; s_px = 0;
}
void st7735_stream_pixels(uint16_t c, uint8_t n) {
    while (n--) { mock_put(s_cx + s_px % s_cw, s_cy + s_px / s_cw, c, op_counter); s_px++; }
}
void st7735_stream_end(void) {}
void st7735_backlight(bool on) { (void)on; }
}

#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static inline void reset_writes(void) { memset(writes, 0, sizeof writes); }

/** Pixels of the box written since reset_writes(). */
static inline int written(int x, int y, int w, int h) {
    int n = 0;
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) n += writes[y + j][x + i] != 0;
    return n;
}

static inline int total_written(void) { return written(0, 0, 128, 160); }

static inline int non_bg(int x, int y, int w, int h, uint16_t bg) {
    int n = 0;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (fb[y + j][x + i] != bg) n++;
    return n;
}

static inline int count_color(int x, int y, int w, int h, uint16_t color) {
    int n = 0;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (fb[y + j][x + i] == color) n++;
    return n;
}

/** Pixels in the box that are dark navy ink (RGB565 red < 6, green < 24). */
static inline int dark_count(int x, int y, int w, int h) {
    int n = 0;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            uint16_t c = fb[y + j][x + i];
            if ((c >> 11) < 6 && ((c >> 5) & 63) < 24) n++;
        }
    return n;
}

#endif
