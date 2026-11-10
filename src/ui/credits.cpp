/**
 * credits.cpp — Credits screen (see credits.h).
 */
#include "ui/credits.h"
#include "ui/chrome.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include "drivers/stack_measure.h"
#include "config.h"

#define CREDITS_LINE_H  14           /* small font frame is 13 rows */
#define CREDITS_INK_H   10           /* ink rows of a line without descenders (frame rows 0..9) */
#define CREDITS_GAP     6            /* extra space between the author and the link */
#define SCREEN_H        160

/* The link is split at the slashes so each line fits the 128 px width. */
static const char *const AUTHOR[] = { "Created by", "Filip Wisniewski." };
static const char *const LINK[]   = { "https://github.com/", "Filip1159/", "Snake-EX2-AVR" };

#define COUNT(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))

/* Draw the lines, or (erase) put the wallpaper back behind each one. */
static uint8_t draw_lines(const char *const *lines, uint8_t n, uint8_t y, bool erase) {
    for (uint8_t i = 0; i < n; i++) {
        uint8_t w = text_width(lines[i], 1);
        uint8_t x = (uint8_t)((CHROME_W - w) / 2);
        if (erase) chrome_restore(x, (uint8_t)(w + 1), y, CHROME_TEXT_H);
        else       text_draw(x, y, lines[i], 1, CHROME_INK);
        y = (uint8_t)(y + CREDITS_LINE_H);
    }
    return y;
}

#if STACK_MEASURE
/* Q5 debug build only: show the free SRAM below the deepest stack use. */
static void draw_stack_debug(uint8_t y) {
    uint16_t free_b = stack_measure_update();
    char num[6];
    uint8_t n = 0;
    do {
        num[n++] = (char)('0' + (free_b % 10));
        free_b /= 10;
    } while (free_b && n < 5);
    for (uint8_t i = 0; i < n / 2; i++) {
        char t = num[i];
        num[i] = num[n - 1 - i];
        num[n - 1 - i] = t;
    }
    num[n] = '\0';

    st7735_fill_rect(0, (uint8_t)(y - 2), 128, 12, 0x0000);
    text_draw(4, y, "STACK FREE:", 1, 0xFFFF);
    text_draw((uint8_t)(4 + text_width("STACK FREE:", 1) + 4), y, num, 1, 0xFFFF);
}
#endif

static uint8_t text_top(void) {
    uint8_t lines = (uint8_t)(COUNT(AUTHOR) + COUNT(LINK));
    uint8_t h = (uint8_t)((lines - 1) * CREDITS_LINE_H + CREDITS_INK_H + CREDITS_GAP);
    return (uint8_t)((SCREEN_H - h) / 2);
}

static void lines_pass(bool erase) {
    uint8_t y = draw_lines(AUTHOR, COUNT(AUTHOR), text_top(), erase);
    draw_lines(LINK, COUNT(LINK), (uint8_t)(y + CREDITS_GAP), erase);
}

static void erase_content(void) { lines_pass(true); }

void credits_show(void) {
    chrome_begin(erase_content);       /* wallpaper + status bar only if not already there */
    chrome_draw_title("Snake EX2", 0);
    chrome_scrollbar_hide();
    lines_pass(false);
    chrome_draw_softkeys(0, "Back");
#if STACK_MEASURE
    draw_stack_debug(130);
#endif
}

void credits_draw(void) {
    chrome_invalidate();
    credits_show();
}
