/* Renders maze 0 + snake demo to /tmp/opencode/demo_<open><fat>.ppm for visual inspection. */
#include "assets/maze.h"
#include "render/field.h"
#include "render/snake_demo.h"
#include "drivers/st7735.h"
#include <stdio.h>
#include <string.h>
static uint16_t fb[160][128];
static int cx, cy, cw, px;
extern "C" {
void st7735_fill_screen(uint16_t c) { for (auto &r : fb) for (auto &p : r) p = c; }
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) fb[y + j][x + i] = c; }
void st7735_blit(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint16_t *b) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) fb[y + j][x + i] = b[j * w + i]; }
void st7735_stream_begin(uint8_t x, uint8_t y, uint8_t w, uint8_t) { cx = x; cy = y; cw = w; px = 0; }
void st7735_stream_pixels(uint16_t c, uint8_t n) { while (n--) { fb[cy + px / cw][cx + px % cw] = c; px++; } }
void st7735_stream_end(void) {}
}
int main() {
    for (int o = 0; o < 2; o++) for (int f = 0; f < 2; f++) {
        uint8_t map[MAZE_CELLS]; maze_load(0, map); field_draw_map(map); snake_demo_draw(o, f);
        char name[64]; snprintf(name, sizeof name, "/tmp/opencode/demo_%d%d.ppm", o, f);
        FILE *fp = fopen(name, "wb"); fprintf(fp, "P6\n128 160\n255\n");
        for (auto &r : fb) for (auto p : r) { uint8_t c[3] = {(uint8_t)((p >> 11) << 3), (uint8_t)(((p >> 5) & 63) << 2), (uint8_t)((p & 31) << 3)}; fwrite(c, 1, 3, fp); }
        fclose(fp);
    }
}
