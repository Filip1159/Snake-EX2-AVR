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
    return 30 + item * 20;
}

static int changed_outside_rows(const uint16_t *a, const uint16_t *b,
                                int row_a, int row_b) {
    int bad = 0;
    for (int y = 0; y < 160; y++) {
        for (int x = 0; x < 128; x++) {
            if (a[y * 128 + x] == b[y * 128 + x]) continue;
            bool in_a = y >= row_top(row_a) && y < row_top(row_a) + 20;
            bool in_b = y >= row_top(row_b) && y < row_top(row_b) + 20;
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
    CHECK(text_width("a", 1) == 5);
    CHECK(text_width("Snake EX2", 1) == 53);

    /* Lowercase glyphs are distinct from uppercase, not folded. */
    memset(fb, 0xFF, sizeof fb);
    text_draw(0, 0, "a", 1, 0x0000);
    int lower_pixels = non_bg(0, 0, 5, 7, 0xFFFF);
    memset(fb, 0xFF, sizeof fb);
    text_draw(0, 0, "A", 1, 0x0000);
    int upper_pixels = non_bg(0, 0, 5, 7, 0xFFFF);
    CHECK(lower_pixels > 0 && upper_pixels > 0 && lower_pixels != upper_pixels);

    menu_init();
    CHECK(menu_selected() == MENU_ITEM_NEW_GAME);
    CHECK(strcmp(menu_item_label(MENU_ITEM_NEW_GAME), "New game") == 0);
    CHECK(strcmp(menu_item_label(MENU_ITEM_HIGH_SCORES), "High scores") == 0);
    CHECK(strcmp(menu_item_label(MENU_ITEM_SETTINGS), "Settings") == 0);
    CHECK(strcmp(menu_item_label(MENU_ITEM_INSTRUCTIONS), "Instructions") == 0);

    menu_move(-1);
    CHECK(menu_selected() == MENU_ITEM_INSTRUCTIONS);
    menu_move(1);
    CHECK(menu_selected() == MENU_ITEM_NEW_GAME);
    menu_move(1);
    CHECK(menu_selected() == MENU_ITEM_HIGH_SCORES);
    menu_move(-1);
    CHECK(menu_selected() == MENU_ITEM_NEW_GAME);

    /* Full 128×160 layout: status bar, title row, four 20 px list rows and
     * Select/Exit soft keys. The selected row is the light-blue S40 band. */
    menu_init();
    menu_draw();
    CHECK(fb[0][0] == 0xFFFF);           /* status bar background */
    CHECK(fb[14][0] == 0xFFFF);          /* title-row background */
    CHECK(fb[30][0] == 0x6CFD);          /* selected row starts at y=30 */
    CHECK(fb[49][0] == 0x6CFD);          /* selected row is 20 px high */
    CHECK(fb[50][0] == 0xFFFF);          /* second row, unselected */
    CHECK(fb[70][0] == 0xFFFF);          /* third row */
    CHECK(fb[90][0] == 0xFFFF);          /* fourth row */
    CHECK(fb[141][0] == 0xFFFF);         /* soft-key area */
    CHECK(fb[159][0] == 0xFFFF);
    CHECK(fb[30][127] == 0x6CFD);        /* selected band is full width */
    CHECK(fb[49][127] == 0x6CFD);

    CHECK(non_bg(0, 0, 128, 14, 0xFFFF) > 20);       /* signal/battery/time */
    CHECK(non_bg(0, 14, 128, 16, 0xFFFF) > 20);      /* title and leaders */
    CHECK(non_bg(4, 30 + 4, 110, 10, 0x6CFD) > 20); /* New game */
    CHECK(non_bg(4, 50 + 4, 110, 10, 0xFFFF) > 20);   /* High scores */
    CHECK(non_bg(4, 70 + 4, 110, 10, 0xFFFF) > 20);   /* Settings */
    CHECK(non_bg(4, 90 + 4, 110, 10, 0xFFFF) > 20);   /* Instructions */
    CHECK(non_bg(0, 140, 128, 20, 0xFFFF) > 20);     /* soft keys + rule */
    CHECK(non_bg(46, 144, 36, 12, 0xFFFF) > 60);     /* bold, centred "Select" */
    CHECK(non_bg(0, 142, 40, 14, 0xFFFF) == 0);      /* nothing left of it */
    CHECK(text_width_big("Select") == 36 && text_width_big("Instructions") == 77);

    /* The selected row has no dotted leader; the other rows do. */
    CHECK(non_bg(80, 43, 44, 1, 0x6CFD) == 0);
    CHECK(non_bg(80, 63, 44, 1, 0xFFFF) > 5);
    CHECK(non_bg(95, 103, 29, 1, 0xFFFF) > 5);

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
    CHECK(fb[30][0] == 0xFFFF);
    CHECK(fb[50][0] == 0x6CFD);
    memcpy(incremental, fb, sizeof fb);
    menu_draw();
    CHECK(memcmp(incremental, fb, sizeof fb) == 0);
    CHECK(changed_outside_rows(&before[0][0], &incremental[0][0],
                               MENU_ITEM_NEW_GAME, MENU_ITEM_HIGH_SCORES) == 0);

    /* Wrapping from the first row to the last row must also repaint only those
     * two rows, and must not overwrite the soft-key rule or status bar. */
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
    CHECK(fb[30][0] == 0xFFFF);
    CHECK(fb[90][0] == 0x6CFD);
    CHECK(fb[0][0] == 0xFFFF);
    CHECK(fb[140][0] == 0x6CFD);       /* soft-key rule unchanged */
    CHECK(fb[141][0] == 0xFFFF);
    memcpy(incremental, fb, sizeof fb);
    menu_draw();
    CHECK(memcmp(incremental, fb, sizeof fb) == 0);
    CHECK(changed_outside_rows(&before[0][0], &incremental[0][0],
                               MENU_ITEM_NEW_GAME, MENU_ITEM_INSTRUCTIONS) == 0);

    printf(fails ? "menu/text: %d FAILED\n" : "menu/text: all passed\n", fails);
    return fails != 0;
}
