/* Verifies generated flash assets against the independently converted build/assets files. */
#include "assets/maze.h"
#include "render/field.h"
#include "drivers/st7735.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <vector>
#include <cstdlib>

static uint16_t fb[160][128];
extern "C" {
void st7735_fill_screen(uint16_t c) { for (auto &r : fb) for (auto &p : r) p = c; }
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c) {
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) fb[y + j][x + i] = c;
}
void st7735_blit(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint16_t *b) {
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) fb[y + j][x + i] = b[j * w + i];
}
}

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static std::vector<uint8_t> slurp(const char *p) {
    FILE *f = fopen(p, "rb");
    if (!f) { printf("cannot open %s (run tools/extract_snake_ex2.py + convert_assets.py)\n", p); exit(2); }
    std::vector<uint8_t> v; int c; while ((c = fgetc(f)) != EOF) v.push_back((uint8_t)c); fclose(f); return v;
}

int main() {
    auto mz = slurp("build/assets/mazes_21x23.bin");
    auto strip = slurp("build/assets/rgb565/wall_strip.rgb565");   /* 120x6, big-endian */
    CHECK(mz.size() == 6 * 483 && strip.size() == 120 * 6 * 2);

    uint8_t map[MAZE_CELLS];
    CHECK(maze_load(MAZE_COUNT, map) == -1);
    for (int m = 0; m < MAZE_COUNT; m++) {
        memset(map, 0xEE, sizeof map);
        CHECK(maze_load(m, map) == 0);
        CHECK(memcmp(map, &mz[m * 483], 483) == 0);

        field_draw_map(map);
        int bad = 0;
        for (int r = 0; r < 23; r++) for (int c = 0; c < 21; c++) {
            int t = map[r * 21 + c];
            for (int y = 0; y < 6; y++) for (int x = 0; x < 6; x++) {
                uint16_t exp = COLOR_FIELD_BG;
                if (t) { int o = (y * 120 + (t - 1) * 6 + x) * 2; exp = (strip[o] << 8) | strip[o + 1]; }
                if (fb[1 + r * 6 + y][1 + c * 6 + x] != exp) bad++;
            }
        }
        CHECK(bad == 0);
    }
    /* borders stay black */
    int nonblack = 0;
    for (int x = 0; x < 128; x++) nonblack += fb[0][x] != 0 || fb[139][x] != 0 || fb[159][x] != 0;
    for (int y = 0; y < 160; y++) nonblack += fb[y][0] != 0 || fb[y][127] != 0;
    CHECK(nonblack == 0);

    printf(fails ? "maze/field: %d FAILED\n" : "maze/field: all passed\n", fails);
    return fails != 0;
}
