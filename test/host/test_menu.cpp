/* Host test: title/menu state, labels, layout, text metrics, and partial repaint. */
#include "ui/menu.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include <stdio.h>
#include <string.h>

static uint16_t fb[160][128];
static uint16_t before[160][128];
static uint16_t incremental[160][128];
static int fill_screen_calls;

extern "C" {
void st7735_fill_screen(uint16_t c) {
    fill_screen_calls++;
    for (int y = 0; y < 160; y++)
        for (int x = 0; x < 128; x++) fb[y][x] = c;
}
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (x + i < 128 && y + j < 160) fb[y + j][x + i] = c;
}
}

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static int non_bg(int x, int y, int w, int h, uint16_t bg) {
    int n = 0;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (fb[y + j][x + i] != bg) n++;
    return n;
}

static int row_top(int item) {
    return 26 + item * 29;
}

static int changed_outside_rows(const uint16_t *a, const uint16_t *b,
                                int row_a, int row_b) {
    int bad = 0;
    for (int y = 0; y < 160; y++) {
        for (int x = 0; x < 128; x++) {
            if (a[y * 128 + x] == b[y * 128 + x]) continue;
            bool in_a = y >= row_top(row_a) && y < row_top(row_a) + 27;
            bool in_b = y >= row_top(row_b) && y < row_top(row_b) + 27;
            if (!in_a && !in_b) bad++;
        }
    }
    return bad;
}

int main() {
    CHECK(text_width("", 1) == 0);
    CHECK(text_width("A", 1) == 5);
    CHECK(text_width("ABC", 1) == 17);
    CHECK(text_width("ABC", 2) == 34);

    menu_init();
    CHECK(menu_selected() == MENU_ITEM_NEW_GAME);
    CHECK(strcmp(menu_item_label(MENU_ITEM_NEW_GAME), "NEW GAME") == 0);
    CHECK(strcmp(menu_item_label(MENU_ITEM_HIGH_SCORES), "HIGH SCORES") == 0);
    CHECK(strcmp(menu_item_label(MENU_ITEM_SETTINGS), "SETTINGS") == 0);
    CHECK(strcmp(menu_item_label(MENU_ITEM_INSTRUCTIONS), "INSTRUCTIONS") == 0);

    menu_move(-1);
    CHECK(menu_selected() == MENU_ITEM_INSTRUCTIONS);
    menu_move(1);
    CHECK(menu_selected() == MENU_ITEM_NEW_GAME);
    menu_move(1);
    CHECK(menu_selected() == MENU_ITEM_HIGH_SCORES);
    menu_move(-1);
    CHECK(menu_selected() == MENU_ITEM_NEW_GAME);

    /* Full 128×160 layout: header at the top, footer at the bottom, all four
     * rows visible, and the selected row uses the highlight colour. */
    menu_init();
    menu_draw();
    CHECK(fb[0][0] == 0x0000);
    CHECK(fb[159][0] == 0x0000);
    CHECK(fb[20][0] == 0xFFFF);
    CHECK(fb[26][4] == 0x001F);          /* selected row, first item */
    CHECK(fb[55][4] == 0xFFFF);          /* unselected row, second item */
    CHECK(non_bg(12, 26 + 10, 100, 7, 0x001F) > 20);
    CHECK(non_bg(12, 55 + 10, 100, 7, 0xFFFF) > 20);
    CHECK(non_bg(12, 84 + 10, 100, 7, 0xFFFF) > 20);
    CHECK(non_bg(12, 113 + 10, 100, 7, 0xFFFF) > 20);
    CHECK(non_bg(0, 140, 128, 20, 0x0000) > 20);

    /* Incremental navigation: move from New game to High scores by repainting
     * only the two affected rows. The result must match a full redraw, and the
     * mock must not see any fill_screen() call. */
    memcpy(before, fb, sizeof fb);
    fill_screen_calls = 0;
    {
        menu_item_t previous = menu_selected();
        menu_move(1);
        menu_draw_item(previous);
        menu_draw_item(menu_selected());
    }
    CHECK(fill_screen_calls == 0);
    CHECK(fb[26][4] == 0xFFFF);
    CHECK(fb[55][4] == 0x001F);
    memcpy(incremental, fb, sizeof fb);
    menu_draw();
    CHECK(memcmp(incremental, fb, sizeof fb) == 0);
    CHECK(changed_outside_rows(&before[0][0], &incremental[0][0],
                               MENU_ITEM_NEW_GAME, MENU_ITEM_HIGH_SCORES) == 0);

    /* Wrapping from the first row to the last row must also repaint only those
     * two rows, and must not overwrite the footer with a row rule. */
    menu_init();
    menu_draw();
    memcpy(before, fb, sizeof fb);
    fill_screen_calls = 0;
    {
        menu_item_t previous = menu_selected();
        menu_move(-1);
        menu_draw_item(previous);
        menu_draw_item(menu_selected());
    }
    CHECK(fill_screen_calls == 0);
    CHECK(menu_selected() == MENU_ITEM_INSTRUCTIONS);
    CHECK(fb[26][4] == 0xFFFF);
    CHECK(fb[113][4] == 0x001F);
    CHECK(fb[140][0] == 0x0000);
    memcpy(incremental, fb, sizeof fb);
    menu_draw();
    CHECK(memcmp(incremental, fb, sizeof fb) == 0);
    CHECK(changed_outside_rows(&before[0][0], &incremental[0][0],
                               MENU_ITEM_NEW_GAME, MENU_ITEM_INSTRUCTIONS) == 0);

    printf(fails ? "menu/text: %d FAILED\n" : "menu/text: all passed\n", fails);
    return fails != 0;
}
