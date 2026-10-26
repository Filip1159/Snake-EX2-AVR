/* Q3 visual diff: render the derived 128x160 static frame (maze 0 + HUD) with
 * the real drawing code and a stub LCD.  tools/test_visual.py compares the PPM
 * with the independent Python asset preview and the documented layout mockup. */
#include "assets/maze.h"
#include "render/field.h"
#include "render/hud.h"
#include "drivers/st7735.h"
#include <stdio.h>

static uint16_t fb[160][128];
static int cx, cy, cw, px;

extern "C" {
void st7735_fill_screen(uint16_t c) {
    for (auto &r : fb) for (auto &p : r) p = c;
}
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (x + i < 128 && y + j < 160) fb[y + j][x + i] = c;
}
void st7735_blit(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint16_t *b) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            if (x + i < 128 && y + j < 160) fb[y + j][x + i] = b[j * w + i];
}
void st7735_stream_begin(uint8_t x, uint8_t y, uint8_t w, uint8_t) {
    cx = x; cy = y; cw = w; px = 0;
}
void st7735_stream_pixels(uint16_t c, uint8_t n) {
    while (n--) {
        fb[cy + px / cw][cx + px % cw] = c;
        px++;
    }
}
void st7735_stream_end(void) {}
}

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "/tmp/opencode/static_maze0.ppm";
    uint8_t map[MAZE_CELLS];
    if (maze_load(MAZE_BOX, map) != 0) return 1;
    field_draw_map(map);
    hud_draw_bar();

    FILE *f = fopen(path, "wb");
    if (!f) return 2;
    fprintf(f, "P6\n128 160\n255\n");
    for (auto &r : fb) {
        for (auto p : r) {
            uint8_t c[3] = {
                (uint8_t)((p >> 11) << 3),
                (uint8_t)(((p >> 5) & 63) << 2),
                (uint8_t)((p & 31) << 3)
            };
            fwrite(c, 1, 3, f);
        }
    }
    fclose(f);
    return 0;
}
