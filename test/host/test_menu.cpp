/* Host test: title/menu state, labels, layout, text metrics, and partial repaint. */
#include "ui/menu.h"
#include "render/text.h"
#include "render/image.h"
#include "assets/wallpaper.h"
#include "drivers/st7735.h"
#include <stdio.h>
#include <string.h>

static uint16_t fb[160][128];
static uint16_t wall[160][128];   /* reference wallpaper */
static uint16_t before[160][128];
static uint16_t incremental[160][128];
static int fill_screen_calls;
static int op_counter;                 /* one tick per drawing call */
static int stamp[160][128];            /* tick of the last write to each pixel */

extern "C" {
void st7735_fill_screen(uint16_t c) {
    fill_screen_calls++;
    for (int y = 0; y < 160; y++)
        for (int x = 0; x < 128; x++) fb[y][x] = c;
}
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (x + i < 128 && y + j < 160) { fb[y + j][x + i] = c; stamp[y + j][x + i] = ++op_counter; }
}
static int s_cx, s_cy, s_cw, s_px;
void st7735_stream_begin(uint8_t x, uint8_t y, uint8_t w, uint8_t h) { (void)h; ++op_counter; s_cx = x; s_cy = y; s_cw = w; s_px = 0; }
void st7735_stream_pixels(uint16_t c, uint8_t n) {
    while (n--) { fb[s_cy + s_px / s_cw][s_cx + s_px % s_cw] = c; stamp[s_cy + s_px / s_cw][s_cx + s_px % s_cw] = op_counter; s_px++; }
}
void st7735_stream_end(void) {}

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

/* pixels in a region that differ from the wallpaper */
static int non_wall(int x, int y, int w, int h) {
    int n = 0;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (fb[y + j][x + i] != wall[y + j][x + i]) n++;
    return n;
}

static int count_color(int x, int y, int w, int h, uint16_t c) {
    int n = 0;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (fb[y + j][x + i] == c) n++;
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
            bool title = y >= 14 && y < 30;       /* item index digit */
            bool bar = x >= 122 && x < 128 && y >= 30 && y < 141;   /* scrollbar thumb */
            if (!in_a && !in_b && !title && !bar) bad++;
        }
    }
    return bad;
}

static void continue_row_tests(void) {
    /* With a saved game the main menu gets a "Continue" row on top. */
    menu_set_continue(true);
    menu_init();
    CHECK(menu_selected_index() == 0 && menu_item_at(0) == MENU_ITEM_CONTINUE);
    CHECK(menu_item_at(1) == MENU_ITEM_NEW_GAME && menu_item_at(4) == MENU_ITEM_INSTRUCTIONS);
    CHECK(menu_index_of(MENU_ITEM_HIGH_SCORES) == 2 && menu_index_of(MENU_ITEM_CONTINUE) == 0);
    CHECK(strcmp(menu_item_label(MENU_ITEM_CONTINUE), "Continue") == 0);
    menu_draw();
    CHECK(fb[30][0] == 0x03DB);                                 /* first row selected */
    CHECK(non_wall(4, 30 + 4, 60, 10) > 20);                    /* "Continue" */
    CHECK(non_wall(4, 110 + 4, 100, 10) > 20);                  /* 5th row: Instructions */
    menu_move(-1);
    CHECK(menu_selected_index() == 4);                          /* wraps over five rows */
    /* The score list is unaffected by the extra row. */
    menu_open(MENU_LIST_SCORES);
    CHECK(menu_item_at(1) == 1 && menu_index_of((menu_item_t)1) == 1);
    menu_set_continue(false);
    menu_init();
    CHECK(menu_item_at(0) == MENU_ITEM_NEW_GAME && menu_index_of(MENU_ITEM_SETTINGS) == 2);
}

int main() {
    CHECK(text_width("", 1) == 0);
    CHECK(text_width("A", 1) == 5);
    CHECK(text_width("ABC", 1) == 17);
    CHECK(text_width("ABC", 2) == 34);
    CHECK(text_width("a", 1) == 4);            /* proportional */
    CHECK(text_width("i", 1) == 1 && text_width("m", 1) == 5);
    CHECK(text_width("Snake EX2", 1) == 47);

    /* Lowercase glyphs are distinct from uppercase, not folded. */
    memset(fb, 0xFF, sizeof fb);
    text_draw(0, 0, "a", 1, 0x0000);
    int lower_pixels = non_bg(0, 0, 5, 13, 0xFFFF);
    memset(fb, 0xFF, sizeof fb);
    text_draw(0, 0, "A", 1, 0x0000);
    int upper_pixels = non_bg(0, 0, 5, 13, 0xFFFF);
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
    img_draw(0, 0, &IMG_WALLPAPER);
    memcpy(wall, fb, sizeof fb);
    CHECK(wall[0][0] != wall[159][127]);                 /* a real picture, not a flat fill */
    menu_init();
    menu_draw();
    CHECK(fb[0][0] == wall[0][0]);           /* status bar background */
    CHECK(fb[14][0] == wall[14][0]);          /* title-row background */
    CHECK(fb[30][0] == 0x03DB);          /* selected row starts at y=30 */
    CHECK(fb[49][0] == 0x02F8);          /* selected row is 20 px high */
    CHECK(fb[50][0] == wall[50][0]);          /* second row, unselected */
    CHECK(fb[70][0] == wall[70][0]);          /* third row */
    CHECK(fb[90][0] == wall[90][0]);          /* fourth row */
    CHECK(fb[141][0] == wall[141][0]);         /* soft-key area */
    CHECK(fb[159][0] == wall[159][0]);
    CHECK(fb[40][119] == fb[40][0] && fb[40][120] == wall[40][120]);   /* band ends at x=119 */
    /* right corners rounded by 2 px: two pixels off the outer rows, one off the next */
    CHECK(fb[30][117] == 0x03DB && fb[30][118] == wall[30][118] && fb[30][119] == wall[30][119]);
    CHECK(fb[31][118] != wall[31][118] && fb[31][119] == wall[31][119]);
    CHECK(fb[49][118] == wall[49][118] && fb[48][119] == wall[48][119] && fb[48][118] != wall[48][118]);
    CHECK(fb[30][0] != wall[30][0] && fb[49][0] != wall[49][0]);       /* left corners square */

    CHECK(non_wall(0, 0, 128, 14) > 20);       /* signal/battery/time */
    CHECK(non_wall(0, 14, 128, 16) > 20);      /* title */
    CHECK(count_color(4, 30 + 4, 110, 10, 0xFFFF) > 20 /* white text */); /* New game */
    CHECK(non_wall(4, 50 + 4, 110, 10) > 20);   /* High scores */
    CHECK(non_wall(4, 70 + 4, 110, 10) > 20);   /* Settings */
    CHECK(non_wall(4, 90 + 4, 110, 10) > 20);   /* Instructions */
    CHECK(non_wall(0, 140, 128, 20) > 20);     /* soft keys + rule */
    CHECK(non_wall(46, 144, 36, 12) > 60);     /* bold, centred "Select" */
    CHECK(non_wall(0, 142, 40, 14) == 0);      /* nothing left of it */
    CHECK(text_width_big("Select") == 38 && text_width_big("Instructions") == 76);

    /* No dotted leaders: the row baselines right of the text stay wallpaper. */
    CHECK(non_wall(80, 64, 38, 1) == 0);
    CHECK(non_wall(95, 104, 24, 1) == 0);

    /* Scrollbar: a 3 px track (x=123..125) from the list top (y=30) to 5 px above "Exit"
     * (y=140) under a 4 px thumb (x=123..126), brighter than the track, 1/4 of its height
     * (27 px), with a brown right edge; it sits at the top for the first item. All four
     * corners are rounded by 1 px. */
    CHECK(fb[31][123] == 0xFF5A && fb[55][123] == 0xFF5A);          /* thumb, left edge */
    CHECK(fb[40][126] == 0x9225 && fb[40][127] == wall[40][127]);   /* right edge, no shadow */
    CHECK(fb[40][122] == wall[40][122]);
    CHECK(fb[30][123] == 0x21B2 && fb[30][126] == wall[30][126] && fb[30][124] == 0xF6B7);   /* top corners */
    CHECK(fb[56][123] == 0x21B2 && fb[56][126] == wall[56][126] && fb[56][125] == 0xDD0F);   /* bottom corners */
    CHECK(fb[57][123] == 0x21B2 && fb[140][123] == 0x21B2);         /* track below it, dark left edge */
    CHECK(fb[100][124] == 0x7C7A && fb[100][125] == 0x94FB);
    CHECK(fb[100][122] == wall[100][122] && fb[100][126] == wall[100][126]);   /* only 3 px wide */
    CHECK(fb[100][127] == wall[100][127]);
    CHECK(fb[29][124] == wall[29][124] && fb[141][124] == wall[141][124]);     /* nothing beyond the track */
    CHECK(count_color(118, 14, 8, 16, 0x0000) > 5);                 /* index digit */
    {   /* the digit is right-aligned at x=123 */
        int left = 128, right = 0;
        for (int y = 14; y < 30; y++) for (int x = 116; x < 128; x++)
            if (fb[y][x] == 0x0000) { if (x < left) left = x; if (x > right) right = x; }
        CHECK(left >= 118 && right <= 123);
    }
    {   /* thumb height is 1/4 of the track at every position */
        for (int item = 0; item < 4; item++) {
            menu_init();
            menu_move((int8_t)item);
            menu_draw();
            int bright = 0;
            for (int y = 30; y < 141; y++) if (fb[y][124] == 0xF6B7) bright++;
            CHECK(bright == 27);
            CHECK(fb[30 + item * 28][124] == 0xF6B7);
        }
    }
    {   /* moving the thumb: the new thumb is painted first, starting at the edge that touches
         * the old one, then the old one is cleared starting at the edge farthest from the
         * new one. Check on the bright column x=121, both directions and a wrap. */
        const int top[4] = {30, 58, 86, 114};
        for (int dir = 0; dir < 3; dir++) {
            int from = dir == 0 ? 0 : dir == 1 ? 2 : 0;
            int to   = dir == 0 ? 1 : dir == 1 ? 1 : 3;
            menu_init(); menu_move((int8_t)from); menu_draw();
            menu_move((int8_t)(to - from));
            memset(stamp, 0, sizeof stamp);
            op_counter = 0;
            menu_draw_item((menu_item_t)from);
            menu_draw_item((menu_item_t)to);
            bool down = to > from;
            int nt = top[to], ot = top[from];
            int max_new = 0, min_old = 1 << 30;
            for (int y = nt; y < nt + 27; y++) {
                CHECK(fb[y][124] == 0xF6B7);
                if (stamp[y][124] > max_new) max_new = stamp[y][124];
                /* new thumb: from the old one outwards */
                int next = down ? y + 1 : y - 1;
                if (next >= nt && next < nt + 27)
                    CHECK(stamp[next][124] >= stamp[y][124]);
            }
            for (int y = ot; y < ot + 27; y++) {
                CHECK(fb[y][124] == 0x7C7A);
                if (stamp[y][122] < min_old) min_old = stamp[y][122];
                /* old thumb: cleared from the far edge towards the new thumb */
                int next = down ? y + 1 : y - 1;
                if (next >= ot && next < ot + 27)
                    CHECK(stamp[next][122] >= stamp[y][122]);
            }
            CHECK(max_new < min_old);                 /* clearing starts after the new thumb is complete */
            CHECK(fb[ot][124] == 0x7C7A && fb[ot + 26][124] == 0x7C7A);   /* old rows show the track again */
        }
        menu_init();
        menu_draw();
    }

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
    CHECK(fb[30][0] == wall[30][0]);
    CHECK(fb[50][0] == 0x03DB);
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
    CHECK(fb[30][0] == wall[30][0]);
    CHECK(fb[90][0] == 0x03DB);
    CHECK(fb[0][0] == wall[0][0]);
    CHECK(fb[140][0] == wall[140][0]);  /* soft-key area unchanged */
    CHECK(fb[141][0] == wall[141][0]);
    memcpy(incremental, fb, sizeof fb);
    menu_draw();
    CHECK(memcmp(incremental, fb, sizeof fb) == 0);
    CHECK(changed_outside_rows(&before[0][0], &incremental[0][0],
                               MENU_ITEM_NEW_GAME, MENU_ITEM_INSTRUCTIONS) == 0);

    /* Classic/Extreme list: two rows, "Back" soft key, thumb is half of the track */
    menu_open(MENU_LIST_SCORES);
    CHECK(menu_selected_index() == 0);
    menu_draw();
    CHECK(fb[30][0] == 0x03DB && fb[49][0] == 0x02F8);          /* selected band */
    CHECK(fb[50][0] == wall[50][0] && fb[90][0] == wall[90][0]); /* second row unselected, rest empty */
    CHECK(count_color(4, 54, 110, 10, 0x0000) > 20);              /* "Extreme" */
    CHECK(count_color(100, 148, 24, 7, 0x0010) > 10);             /* "Back", not "Exit" */
    {
        int bright = 0;
        for (int y = 30; y < 141; y++) if (fb[y][124] == 0xF6B7) bright++;
        CHECK(bright == 55);
    }
    menu_move(1);
    menu_draw_item(MENU_ITEM_NEW_GAME);
    menu_draw_item(MENU_ITEM_HIGH_SCORES);
    CHECK(menu_selected_index() == 1 && fb[50][0] == 0x03DB && fb[30][0] == wall[30][0]);
    CHECK(fb[86][124] == 0xF6B7 && fb[140][124] == 0xF6B7 && fb[85][124] == 0x7C7A && fb[30][122] == wall[30][122]);
    menu_move(1);                                                  /* wraps to Classic */
    CHECK(menu_selected_index() == 0);
    menu_init();                                                   /* back to the main list */
    menu_draw();
    CHECK(fb[30][0] == 0x03DB && count_color(100, 148, 24, 7, 0x0010) > 10);

    continue_row_tests();

    printf(fails ? "menu/text: %d FAILED\n" : "menu/text: all passed\n", fails);
    return fails != 0;
}
