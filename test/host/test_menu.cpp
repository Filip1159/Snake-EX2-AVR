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
            bool bar = x >= 121 && x < 127 && y >= 30 && y < 146;   /* scrollbar thumb */
            if (!in_a && !in_b && !title && !bar) bad++;
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
    img_draw(0, 0, &IMG_WALLPAPER);
    memcpy(wall, fb, sizeof fb);
    CHECK(wall[0][0] != wall[159][127]);                 /* a real picture, not a flat fill */
    menu_init();
    menu_draw();
    CHECK(fb[0][0] == wall[0][0]);           /* status bar background */
    CHECK(fb[14][0] == wall[14][0]);          /* title-row background */
    CHECK(fb[30][0] == 0x0B35);          /* selected row starts at y=30 */
    CHECK(fb[49][0] == 0x01AF);          /* selected row is 20 px high */
    CHECK(fb[50][0] == wall[50][0]);          /* second row, unselected */
    CHECK(fb[70][0] == wall[70][0]);          /* third row */
    CHECK(fb[90][0] == wall[90][0]);          /* fourth row */
    CHECK(fb[141][0] == wall[141][0]);         /* soft-key area */
    CHECK(fb[159][0] == wall[159][0]);
    CHECK(fb[30][119] == 0x0B35);        /* selected band ends at the scrollbar */
    CHECK(fb[49][119] == 0x01AF);

    CHECK(non_wall(0, 0, 128, 14) > 20);       /* signal/battery/time */
    CHECK(non_wall(0, 14, 128, 16) > 20);      /* title and leaders */
    CHECK(count_color(4, 30 + 4, 110, 10, 0xFFFF) > 20 /* white text */); /* New game */
    CHECK(non_wall(4, 50 + 4, 110, 10) > 20);   /* High scores */
    CHECK(non_wall(4, 70 + 4, 110, 10) > 20);   /* Settings */
    CHECK(non_wall(4, 90 + 4, 110, 10) > 20);   /* Instructions */
    CHECK(non_wall(0, 140, 128, 20) > 20);     /* soft keys + rule */
    CHECK(non_wall(46, 144, 36, 12) > 60);     /* bold, centred "Select" */
    CHECK(non_wall(0, 142, 40, 14) == 0);      /* nothing left of it */
    CHECK(text_width_big("Select") == 36 && text_width_big("Instructions") == 77);

    /* The selected row has no dotted leader; the other rows do. */
    CHECK(count_color(80, 43, 44, 1, 0x8410) == 0);
    CHECK(non_wall(80, 63, 44, 1) > 5);
    CHECK(non_wall(95, 103, 29, 1) > 5);

    /* Scrollbar: a 3 px track (x=122..124) from the list top (y=30) to just above "Exit"
     * (y=145), centred under the 6 px thumb (x=121..126: 5 px body + shadow). The thumb is
     * brighter than the track, 1/4 of its height (29 px), with a brown edge and a navy
     * shadow on its right, and sits at the top for the first item. */
    CHECK(fb[30][121] == 0xFFFF && fb[58][121] == 0xFFFF);          /* thumb, left (bright) edge */
    CHECK(fb[40][125] == 0x9225 && fb[40][126] == 0x118F);          /* thumb edge + shadow on the right */
    CHECK(fb[59][122] == 0x21B2 && fb[145][122] == 0x21B2);         /* track below it, dark left edge */
    CHECK(fb[100][123] == 0x7C7A && fb[100][124] == 0x94FB);
    CHECK(fb[100][121] == wall[100][121] && fb[100][125] == wall[100][125]);   /* only 3 px wide */
    CHECK(fb[100][126] == wall[100][126] && fb[100][127] == wall[100][127]);
    CHECK(fb[29][123] == wall[29][123] && fb[146][123] == wall[146][123]);     /* nothing beyond the track */
    CHECK(count_color(120, 14, 8, 16, 0x0000) > 5);                 /* index digit */
    {   /* the digit is as wide as the thumb body (5 px) and starts at its left edge */
        int left = 128, right = 0;
        for (int y = 14; y < 30; y++) for (int x = 119; x < 128; x++)
            if (fb[y][x] == 0x0000) { if (x < left) left = x; if (x > right) right = x; }
        CHECK(left >= 121 && right <= 125);
    }
    {   /* thumb height is 1/4 of the track at every position */
        for (int item = 0; item < 4; item++) {
            menu_init();
            menu_move((int8_t)item);
            menu_draw();
            int bright = 0;
            for (int y = 30; y < 146; y++) if (fb[y][121] == 0xFFFF) bright++;
            CHECK(bright == 29);
            CHECK(fb[30 + item * 29][121] == 0xFFFF);
        }
    }
    {   /* moving the thumb: the new thumb is painted first, starting at the edge that touches
         * the old one, then the old one is cleared starting at the edge farthest from the
         * new one. Check on the bright column x=121, both directions and a wrap. */
        const int top[4] = {30, 59, 88, 117};
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
            for (int y = nt; y < nt + 29; y++) {
                CHECK(fb[y][121] == 0xFFFF);
                if (stamp[y][121] > max_new) max_new = stamp[y][121];
                /* new thumb: from the old one outwards */
                int next = down ? y + 1 : y - 1;
                if (next >= nt && next < nt + 29)
                    CHECK(stamp[next][121] >= stamp[y][121]);
            }
            for (int y = ot; y < ot + 29; y++) {
                CHECK(fb[y][121] == wall[y][121]);
                if (stamp[y][121] < min_old) min_old = stamp[y][121];
                /* old thumb: cleared from the far edge towards the new thumb */
                int next = down ? y + 1 : y - 1;
                if (next >= ot && next < ot + 29)
                    CHECK(stamp[next][121] >= stamp[y][121]);
            }
            CHECK(max_new < min_old);                 /* clearing starts after the new thumb is complete */
            CHECK(fb[ot][122] == 0x21B2 && fb[ot + 28][122] == 0x21B2);   /* old rows show the track again */
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
    CHECK(fb[50][0] == 0x0B35);
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
    CHECK(fb[90][0] == 0x0B35);
    CHECK(fb[0][0] == wall[0][0]);
    CHECK(fb[140][0] == wall[140][0]);  /* soft-key area unchanged */
    CHECK(fb[141][0] == wall[141][0]);
    memcpy(incremental, fb, sizeof fb);
    menu_draw();
    CHECK(memcmp(incremental, fb, sizeof fb) == 0);
    CHECK(changed_outside_rows(&before[0][0], &incremental[0][0],
                               MENU_ITEM_NEW_GAME, MENU_ITEM_INSTRUCTIONS) == 0);

    printf(fails ? "menu/text: %d FAILED\n" : "menu/text: all passed\n", fails);
    return fails != 0;
}
