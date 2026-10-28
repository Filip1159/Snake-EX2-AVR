/**
 * credits.cpp — Credits screen (see credits.h).
 */
#include "ui/credits.h"
#include "ui/chrome.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include "drivers/stack_measure.h"
#include "config.h"

#define CREDITS_LINE_H  10
#define CREDITS_GAP     6            /* extra space between the author and the link */
#define SCREEN_H        160

/* The link is split at the slashes so each line fits the 128 px width. */
static const char *const AUTHOR[] = { "Created by", "Filip Wisniewski." };
static const char *const LINK[]   = { "https://github.com/", "Filip1159/", "Snake-EX2-AVR" };

#define COUNT(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))

static uint8_t draw_lines(const char *const *lines, uint8_t n, uint8_t y) {
    for (uint8_t i = 0; i < n; i++) {
        uint8_t w = text_width(lines[i], 1);
        text_draw((uint8_t)((CHROME_W - w) / 2), y, lines[i], 1, CHROME_INK);
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
    text_draw(4 + 11 * 6, y, num, 1, 0xFFFF);
}
#endif

void credits_draw(void) {
    chrome_use_wallpaper();
    chrome_draw_wallpaper();
    chrome_draw_status_bar();
    chrome_draw_title("Snake EX2", 0);

    uint8_t lines = (uint8_t)(COUNT(AUTHOR) + COUNT(LINK));
    uint8_t h = (uint8_t)(lines * CREDITS_LINE_H - (CREDITS_LINE_H - 7) + CREDITS_GAP);
    uint8_t y = (uint8_t)((SCREEN_H - h) / 2);
    y = draw_lines(AUTHOR, COUNT(AUTHOR), y);
    draw_lines(LINK, COUNT(LINK), (uint8_t)(y + CREDITS_GAP));

    chrome_draw_softkey_right("Back");
#if STACK_MEASURE
    draw_stack_debug(130);
#endif
}
